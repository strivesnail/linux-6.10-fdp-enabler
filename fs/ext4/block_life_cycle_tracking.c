#include <linux/fs.h>
#include <linux/fs_struct.h>
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/file.h>
#include <linux/path.h>
#include "ext4.h"
#include "ext4_jbd2.h"

atomic64_t global_logical_clock = ATOMIC64_INIT(0);

static u64 ext4_calculate_dir_blocks(struct inode *dir)
{
    u64 dir_blocks;
    
    if (!dir || !S_ISDIR(dir->i_mode))
        return 0;

    /* Get blocks directly associated with this directory inode */
    dir_blocks = dir->i_blocks >> (dir->i_sb->s_blocksize_bits - 9);
    
    pr_info("EXT4-Track: Directory inode %lu has %llu blocks of its own\n", 
            dir->i_ino, dir_blocks);
    
    return dir_blocks;
}

// static unsigned long long ext4_calculate_dir_total_blocks(struct inode *dir)
// {
//     unsigned long long total_blocks = 0;
//     struct dentry *parent_dentry;
//     struct dentry *child_dentry;
//     struct inode *child_inode;
    
//     /* First add the directory's own blocks */
//     total_blocks = dir->i_blocks >> (dir->i_sb->s_blocksize_bits - 9);
    
//     /* Find a dentry for this directory inode */
//     parent_dentry = d_find_alias(dir);
//     if (!parent_dentry) {
//         pr_info("EXT4-Track: No dentry found for directory inode %lu\n", dir->i_ino);
//         return total_blocks;
//     }
    
//     /* Scan the directory's children */
//     down_read(&dir->i_rwsem); /* Lock directory during traversal */
    
//     spin_lock(&parent_dentry->d_lock);
//     list_for_each_entry(child_dentry, &parent_dentry->d_subdirs, d_child) {
//         spin_unlock(&parent_dentry->d_lock);
        
//         child_inode = d_inode(child_dentry);
//         if (!child_inode)
//             goto next_child; /* Skip negative dentries */
        
//         /* Skip "." and ".." entries */
//         if (child_dentry->d_name.len <= 2 && 
//             (child_dentry->d_name.name[0] == '.' && 
//              (child_dentry->d_name.len == 1 || child_dentry->d_name.name[1] == '.')))
//             goto next_child;
            
//         /* Add this child's blocks to the total */
//         if (S_ISDIR(child_inode->i_mode)) {
//             /* Recursively calculate for subdirectories */
//             total_blocks += ext4_calculate_dir_total_blocks(child_inode);
//         } else {
//             /* Just add the regular file's blocks */
//             total_blocks += child_inode->i_blocks >> (child_inode->i_sb->s_blocksize_bits - 9);
//         }
        
// next_child:
//         spin_lock(&parent_dentry->d_lock);
//     }
//     spin_unlock(&parent_dentry->d_lock);
    
//     up_read(&dir->i_rwsem);
//     dput(parent_dentry); /* Release dentry reference */
    
//     pr_info("EXT4-Track: Directory inode %lu has %llu total blocks (including children)\n", 
//             dir->i_ino, total_blocks);
    
//     return total_blocks;
// }

/* Enable tracking for a single inode */
int ext4_enable_tracking_single(struct inode *inode)
{
    struct block_lifecycle_stats *stats;
    u64 existing_blocks;
    
    /* Check if already tracked */
    if (EXT4_I(inode)->i_blk_lc_stats) {
        pr_info("EXT4-Track: Inode %lu already tracked births %llu deaths %llu \n", 
            inode->i_ino, 
            atomic64_read(&EXT4_I(inode)->i_blk_lc_stats->births), 
            atomic64_read(&EXT4_I(inode)->i_blk_lc_stats->deaths));
        return 0;
    }

    /* Use GFP_NOFS to avoid potential deadlocks during filesystem operations */
    stats = kmalloc(sizeof(struct block_lifecycle_stats), GFP_NOFS);
    if (!stats) {
        pr_err("EXT4-Track: Failed to allocate stats for inode %lu\n",
               inode->i_ino);
        return -ENOMEM;
    }

    // existing_blocks = inode->i_blocks >> (inode->i_sb->s_blocksize_bits - 9);

    if (S_ISDIR(inode->i_mode)) {
        existing_blocks = ext4_calculate_dir_blocks(inode);
        // existing_blocks = ext4_calculate_dir_total_blocks(inode);
        pr_info("EXT4-Track: Initializing directory inode %lu with %llu total blocks\n",
                inode->i_ino, existing_blocks);
    } else {
        existing_blocks = inode->i_blocks >> (inode->i_sb->s_blocksize_bits - 9);
        pr_info("EXT4-Track: Initializing file inode %lu with %llu blocks\n",
                inode->i_ino, existing_blocks);
    }    

    /* Initialize atomic counters */
    atomic64_set(&stats->births, existing_blocks);
    atomic64_set(&stats->deaths, 0);
    atomic64_set(&stats->logical_clock, 0);
    atomic64_set(&stats->previous_death, 0);
    atomic64_set(&stats->first_timestamp, atomic64_read(&global_logical_clock));
    atomic64_set(&stats->initial_alive, existing_blocks);
    atomic_set(&stats->tracking_started, 1);

    /* Assign stats to inode, use a memory barrier to ensure proper assignment */
    smp_mb();
    EXT4_I(inode)->i_blk_lc_stats = stats;
    EXT4_I(inode)->i_enable_track = 1;

    /* Mark inode as dirty */
    __mark_inode_dirty(inode, I_DIRTY_SYNC | I_DIRTY_DATASYNC);

    return 0;
}

/* Disable tracking for a single inode */
int ext4_disable_tracking_single(struct inode *inode)
{
    if (!inode)
        return -EINVAL;

    pr_info("EXT4-Track: Disabling tracking for inode %lu (mode: %o)\n",
            inode->i_ino, inode->i_mode);

    /* Check if already not tracked */
    if (!EXT4_I(inode)->i_blk_lc_stats)
        return 0;

    /* Free stats and clear pointer */
    kfree(EXT4_I(inode)->i_blk_lc_stats);
    EXT4_I(inode)->i_blk_lc_stats = NULL;
    EXT4_I(inode)->i_enable_track = 0;

    /* Mark inode as dirty */
    __mark_inode_dirty(inode, I_DIRTY_SYNC | I_DIRTY_DATASYNC);

    return 0;
}
