#include <linux/fdp.h>

void fdp_add_info(uint64_t owner_id, uint64_t lifetime, enum rw_hint hint)
{
}

enum rw_hint fdp_get_placehandler(uint64_t owner_id, enum rw_hint hint)
{
	return WRITE_LIFE_NOT_SET;
}
