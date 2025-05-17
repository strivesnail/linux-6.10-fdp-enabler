#include <linux/gfp_types.h>
#include "linux/xarray.h"
#include <linux/fdp.h>
#include <linux/printk.h>

atomic64_t fdp_logic_clock = ATOMIC64_INIT(0);

DEFINE_XARRAY(fdp_xa);

struct fdp_ops __rcu *fdp_rcu_ops = NULL;

EXPORT_SYMBOL(fdp_rcu_ops);

void fdp_add_info(uint64_t owner_id, uint64_t lifetime, enum rw_hint hint)
{
	struct fdp_ops *ops;
	pr_debug(
		"fdp_add_info: called by %llu with hint: %d with lifetime: %llu",
		owner_id, hint, lifetime);
	rcu_read_lock();
	ops = rcu_dereference(fdp_rcu_ops);
	if (ops) {
		ops->add_info(owner_id, lifetime, hint);
	}
	rcu_read_unlock();
}

enum rw_hint fdp_get_placehandler(uint64_t owner_id, enum rw_hint hint)
{
	enum rw_hint ret = hint;
	struct fdp_ops *ops;
	pr_debug("fdp_get_placehandler: called by %llu with hint: %d", owner_id,
		 hint);
	rcu_read_lock();
	ops = rcu_dereference(fdp_rcu_ops);
	if (ops) {
		ret = ops->get_placehandler(owner_id, hint);
	}
	rcu_read_unlock();
	return ret;
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
