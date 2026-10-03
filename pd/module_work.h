#pragma once

#include "work_pool.h"
#include <set>

template<typename T, typename F>
void module_work(work_pool* pool, const std::vector<T>& items, F action)
{
	if (pool)
		pool->parallel_for(items, action);
	else
		for (const auto& item : items) action(item);
}

inline void exclude_module_heaps(std::set<unsigned __int64>& heaps, unsigned __int64 base, unsigned __int64 size)
{
	if (size > _UI64_MAX - base)
		return;
	heaps.erase(heaps.lower_bound(base), heaps.lower_bound(base + size));
}
