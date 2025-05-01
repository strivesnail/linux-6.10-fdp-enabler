#ifndef _FS_EXT4_FDP_H_
#define _FS_EXT4_FDP_H_

#include <linux/rw_hint.h>

// Add the lifetime information of a file to the FDP context. 
// Called then file is deleted.
// 
// owner_id:    the process ID
// lifetime:    How long the process has lived in micro-seconds. 
//              Equal to deletion time - creation time.
// hint:        pass in user side hints, see linux/rw_hint.h
void fdp_add_info(uint64_t owner_id, uint64_t lifetime, enum rw_hint hint);

// Make decisions when we need to flush block to SSD. 
// Called when a inode rw hint is passed to block rw hint.
// 
// owner_id:    the process ID
// hint:        pass in user side hints, see linux/rw_hint.h
// return:      block rw_hint
enum rw_hint fdp_get_placehandler(uint64_t owner_id, enum rw_hint hint);
#endif

