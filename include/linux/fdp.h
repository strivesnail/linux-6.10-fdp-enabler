#ifndef _FS_EXT4_FDP_H_
#define _FS_EXT4_FDP_H_

#include <linux/rw_hint.h>
#include <linux/xarray.h>

struct fdp_ops {
	void (*add_info)(uint64_t owner_id, uint64_t lifetime,
			 enum rw_hint hint);
	enum rw_hint (*get_placehandler)(uint64_t owner_id, enum rw_hint hint);
};

extern struct fdp_ops __rcu *fdp_rcu_ops;

/// Global logic clock: monotonically increasing, indicating the current clock tiks
/// everytime we fall a fcntl(.., SET_RW_HINT, ..) on a new file, it will increment this logic clock by 1
extern atomic64_t fdp_logic_clock;

/// xarray to map from file descriptor id -> logic clock at creation time
extern struct xarray fdp_xa;

// Add the lifetime information of a file to the FDP context.
// Called then file is deleted.
//
// owner_id:    the unique ID
// lifetime:    How long the process has lived in micro-seconds.
//              Equal to deletion time - creation time.
// hint:        pass in user side hints, see linux/rw_hint.h
void fdp_add_info(uint64_t owner_id, uint64_t lifetime, enum rw_hint hint);

// Make decisions when we need to flush block to SSD.
// Called when a inode rw hint is passed to block rw hint.
//
// owner_id:    the unique ID
// hint:        pass in user side hints, see linux/rw_hint.h
// return:      block rw_hint
enum rw_hint fdp_get_placehandler(uint64_t owner_id, enum rw_hint hint);

/// Record current owner_id and its corresponding logic clock and increment it
///
/// file_id: the unique ID per file (inode->i_ino)
void fdp_record_logic_clock(uint64_t file_id);

/// Get the duration for owner_id since recording, and delete this entrance
///
/// file_id: the unique ID per file (inode->i_ino)
/// ret:      how many logic tiks has passed since record
uint64_t fdp_get_and_delete_logic_clock(uint64_t file_id);

#endif
