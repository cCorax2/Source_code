#ifndef _DEBUG_ALLOCATOR_H_
#define _DEBUG_ALLOCATOR_H_

#include <cstdlib>
#include <new>

#ifdef DEBUG_ALLOC

// Whether to use DebugPtr for specific objects
#define USE_DEBUG_PTR

// Here we specify the implementation detail to use.
#define ALLOCATOR_DETAIL FifoAllocator
#define ALLOCATOR_DETAIL_HEADER "fifo_allocator.h"

#include ALLOCATOR_DETAIL_HEADER

/* Detail implementation contract
class AllocatorDetail {
public:
	AllocatorDetail();
	~AllocatorDetail();
	void SetUp(); // Initializer
	void TearDown(); // Finalizer
	void* Alloc(size_t size); // Allocates a memory block
	void Free(void* p); // Deallocates a memory block
private:
	NO_COPY(AllocatorDetail);
};
*/

#include "debug_allocator_adapter.h"

typedef DebugAllocatorAdapter<ALLOCATOR_DETAIL> DebugAllocator;

#include "debug_ptr.h"

inline void* operator new(size_t size, const char* file, size_t line) {
#ifndef USE_DEBUG_PTR
	void* p = DebugAllocator::Alloc(size);
#else
	void* p = DebugAllocator::Alloc(size + sizeof(size_t));
	p = reinterpret_cast<size_t*>(p) + 1;
#endif
	if (p != NULL) {
		size_t age = DebugAllocator::MarkAcquired(p, file, line, "new");
#ifdef USE_DEBUG_PTR
		*(reinterpret_cast<size_t*>(p) - 1) = age;
#endif
	}
	return p;
}
inline void* operator new[](size_t size, const char* file, size_t line) {
#ifndef USE_DEBUG_PTR
	void* p = DebugAllocator::Alloc(size);
#else
	void* p = DebugAllocator::Alloc(size + sizeof(size_t));
	p = reinterpret_cast<size_t*>(p) + 1;
#endif
	if (p != NULL) {
		size_t age = DebugAllocator::MarkAcquired(p, file, line, "new[]");
#ifdef USE_DEBUG_PTR
		*(reinterpret_cast<size_t*>(p) - 1) = age;
#endif
	}
	return p;
}
template<typename T>
inline void debug_delete(T* p, const char* file, size_t line) {
	if (p != NULL) {
		DebugAllocator::VerifyDeletion(p, file, line, false);
		p->~T();
#ifndef USE_DEBUG_PTR
		DebugAllocator::Free(p);
#else
		size_t* px = reinterpret_cast<size_t*>(p) - 1;
		*px = AllocTag::IncreaseAge(*px);
		DebugAllocator::Free(px);
#endif
		DebugAllocator::MarkReleased(p, file, line, "delete");
	}
}
#ifdef USE_DEBUG_PTR
template<typename T>
inline void debug_delete(DebugPtr<T> ptr, const char* file, size_t line) {
	T* p = ptr.Get();
	if (p != NULL) {
		DebugAllocator::VerifyDeletion(ptr.Get(), file, line, true, ptr.GetAge());
		p->~T();
		size_t* px = reinterpret_cast<size_t*>(p) - 1;
		*px = AllocTag::IncreaseAge(*px);
		DebugAllocator::Free(px);
		DebugAllocator::MarkReleased(ptr, file, line, "delete");
	}
}
#endif
template<typename T>
inline void debug_delete_array(T* p, const char* file, size_t line) {
	if (p != NULL) {
		DebugAllocator::VerifyDeletion(p, file, line, false);
		// Caution: no destructor call here!
		// It means that no serious destructor-equipped class should be
		// used with new[]/delete[].
#ifndef USE_DEBUG_PTR
		DebugAllocator::Free(p);
#else
		size_t* px = reinterpret_cast<size_t*>(p) - 1;
		*px = AllocTag::IncreaseAge(*px);
		DebugAllocator::Free(px);
#endif
		DebugAllocator::MarkReleased(p, file, line, "delete[]");
	}
}
#ifdef USE_DEBUG_PTR
template<typename T>
inline void debug_delete_array(DebugPtr<T> ptr, const char* file, size_t line) {
	T* p = ptr.Get();
	if (p != NULL) {
		DebugAllocator::VerifyDeletion(ptr.Get(), file, line, true, ptr.GetAge());
		// Caution: no destructor call here!
		// It means that no serious destructor-equipped class should be
		// used with new[]/delete[].
		size_t* px = reinterpret_cast<size_t*>(p) - 1;
		*px = AllocTag::IncreaseAge(*px);
		DebugAllocator::Free(px);
		DebugAllocator::MarkReleased(ptr, file, line, "delete[]");
	}
}
#endif
#define M2_NEW new(__FILE__, __LINE__)
#define M2_DELETE(p) debug_delete(p, __FILE__, __LINE__)
#define M2_DELETE_EX(p, f, l) debug_delete(p, f, l)
#define M2_DELETE_ARRAY(p) debug_delete_array(p, __FILE__, __LINE__)
#define M2_PTR_REF(ptr) (DebugAllocator::Verify(ptr.Get(), ptr.GetAge(), __FILE__, __LINE__))
#define M2_PTR_DEREF(ptr) (*(DebugAllocator::Verify(ptr.Get(), ptr.GetAge(), __FILE__, __LINE__)))

#ifdef _MSC_VER
// To surpress warning C4291
inline void operator delete(void* p, const char* file, size_t line) {
	if (p != NULL) {
		DebugAllocator::VerifyDeletion(p, file, line, false);
#ifndef USE_DEBUG_PTR
		DebugAllocator::Free(p);
#else
		size_t* px = reinterpret_cast<size_t*>(p) - 1;
		*px = AllocTag::IncreaseAge(*px);
		DebugAllocator::Free(px);
#endif
		DebugAllocator::MarkReleased(p, file, line, "delete");
	}
}
#endif

#else

#define M2_NEW new
#define M2_DELETE(p) delete (p)
// No M2_DELETE_EX
#define M2_DELETE_ARRAY(p) delete[] (p)
#define M2_PTR_REF(p) (p)
#define M2_PTR_DEREF(p) (*(p))

#endif // DEBUG_ALLOC

// Default get_pointer() free function template.
template<typename T>
T* get_pointer(T* p) {
	return p;
}

#endif // _DEBUG_ALLOCATOR_H_
