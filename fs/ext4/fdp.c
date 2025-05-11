#include <linux/gfp_types.h>
#include "linux/xarray.h"
#include <linux/fdp.h>
#include <linux/printk.h>

atomic64_t fdp_logic_clock = ATOMIC64_INIT(0);

DEFINE_XARRAY(fdp_xa);

void fdp_add_info(uint64_t owner_id, uint64_t lifetime, enum rw_hint hint)
{
	printk(KERN_INFO
	       "fdp_add_info: called by %llu with hint: %d with lifetime: %llu",
	       owner_id, hint, lifetime);
}

enum rw_hint fdp_get_placehandler(uint64_t owner_id, enum rw_hint hint)
{
	printk(KERN_INFO "fdp_get_placehandler: called by %llu with hint: %d",
	       owner_id, hint);

	return hint;
}

void fdp_record_logic_clock(uint64_t file_id)
{
	u64 tik = atomic64_inc_return(&fdp_logic_clock);

	xa_store(&fdp_xa, file_id, xa_mk_value(tik), GFP_KERNEL);
}

uint64_t fdp_get_and_delete_logic_clock(uint64_t file_id)
{
	void *entry = xa_erase(&fdp_xa, file_id);
	if (!entry) {
		return 0;
	}

	u64 delta = atomic64_read(&fdp_logic_clock) - xa_to_value(entry);

	return delta;
}
