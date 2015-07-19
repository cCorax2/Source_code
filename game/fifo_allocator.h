#ifndef _FIFO_ALLOCATOR_H_
#define _FIFO_ALLOCATOR_H_

#include <deque>

#ifdef __GNUC__
#include <tr1/unordered_map>
#define TR1_NS std::tr1
#else
#include <boost/unordered_map.hpp>
#define TR1_NS boost
#endif

// Allocator implementation detail with simple FIFO grow-only pool.
// It relies on default CRT malloc/free.
class FifoAllocator {
public:
	FifoAllocator() {}
	~FifoAllocator() {}

	void SetUp() {}
	void TearDown() {
		CleanUp(); // deallocate pooled blocks
	}

	void* Alloc(size_t size) {
		void* p = NULL;
		PoolType& pool = pool_map_[size];
		if (pool.size() < kWatermark) {
			p = ::malloc(size);
		} else {
			p = pool.front();
			pool.pop_front();
		}
		if (p != NULL) {
			alloc_map_[p] = size;
		}
		return p;
	}
	void Free(void* p) {
		if (p == NULL) {
			return;
		}
		AllocMapType::iterator it = alloc_map_.find(p);
		if (it == alloc_map_.end()) {
			return;
		}
		size_t size = it->second;
		alloc_map_.erase(it);
		PoolMapType::iterator it2 = pool_map_.find(size);
		if (it2 == pool_map_.end()) {
			return;
		}
		PoolType& pool = it2->second;
		pool.push_back(p);
	}

private:
	void CleanUp() {
		PoolMapType::iterator it = pool_map_.begin(), end = pool_map_.end();
		for ( ; it != end; ++it) {
			PoolType& pool = it->second;
			PoolType::iterator it2 = pool.begin(), end2 = pool.end();
			for ( ; it2 != end2; ++it2) {
				::free(*it2);
			}
			pool.clear();
		}
		pool_map_.clear();
	}

	typedef std::deque<void*> PoolType;
	typedef TR1_NS::unordered_map<size_t, PoolType> PoolMapType;
	typedef TR1_NS::unordered_map<void*, size_t> AllocMapType;

	static const size_t kWatermark = 4; // FIFO enforcement level

	PoolMapType pool_map_;
	AllocMapType alloc_map_;
};

#endif // _FIFO_ALLOCATOR_H_
