#ifndef _DEBUG_PTR_H_
#define _DEBUG_PTR_H_

// Intended to be include only in "debug_allocator.h"
#ifdef DEBUG_ALLOC

#include <algorithm>

// A minimal raw pointer wrapper to support heap pointer tracking along with
// DebugAllocator.
template<typename T>
class DebugPtr {
public:
	DebugPtr() : p_(NULL), age_(0) {}
	DebugPtr(T* p) : p_(p) {
		age_ = DebugAllocator::RetrieveAge(p_);
	}
	template<typename U>
	DebugPtr(U* p) : p_(static_cast<T*>(p)) {
		age_ = DebugAllocator::RetrieveAge(p_);
	}
	DebugPtr(T* p, size_t age) : p_(p), age_(age) {}
	DebugPtr(const DebugPtr& o) : p_(o.p_), age_(o.age_) {}
	template<typename U>
	DebugPtr(const DebugPtr<U>& o) : p_(static_cast<T*>(o.Get())), age_(o.GetAge()) {}
	~DebugPtr() {}

	T& operator*() const {
		return *(GetVerified());
	}
	T* operator->() const {
		return GetVerified();
	}

	// Gets the underlying heap pointer.
	T* Get() const { return p_; }
	// Gets the age of the underlying heap pointer.
	size_t GetAge() const { return age_; }

	// Assignment operator for DebugPtr<T>
	DebugPtr& operator=(const DebugPtr& rhs) {
		DebugPtr ptr(rhs);
		return Swap(ptr);
	}
	// Assignment operator for DebugPtr<U>
	template<typename U>
	DebugPtr& operator=(const DebugPtr<U>& rhs) {
		DebugPtr ptr(rhs);
		return Swap(ptr);
	}

	// Conversion operator
	operator T*() const {
		return GetVerified();
	}

	bool operator!() const {
		return (Get() == NULL);
	}

private:
	T* GetVerified() const {
		T* p = p_;
		if (p != NULL) {
			// Quick test first
			size_t age = *(reinterpret_cast<size_t*>(p) - 1);
			if (age != age_) {
				if (DebugAllocator::Verify(p, age_) != NULL) {
					// Age header corrupted
					DebugAllocator::LogBoundaryCorruption(p, age);
				}
				// Make it crash immediately
				int* no_way = NULL;
				*no_way = 0;
				//p = NULL;
			}
		}
		return p;
	}

	DebugPtr& Swap(DebugPtr& o) {
		std::swap(p_, o.p_);
		std::swap(age_, o.age_);
		return *this;
	}

	T* p_;
	size_t age_;
};

template<typename T, typename U>
inline bool operator==(const DebugPtr<T>& a, const DebugPtr<U>& b) {
	return a.Get() == b.Get();
}
template<typename T, typename U>
inline bool operator!=(const DebugPtr<T>& a, const DebugPtr<U>& b) {
	return a.Get() != b.Get();
}

template<typename T, typename U>
inline bool operator==(const DebugPtr<T>& a, U* b) {
	return a.Get() == b;
}
template<typename T, typename U>
inline bool operator!=(const DebugPtr<T>& a, U* b) {
	return a.Get() != b;
}

template<typename T, typename U>
inline bool operator==(T* a, const DebugPtr<U>& b) {
	return a == b.Get();
}
template<typename T, typename U>
inline bool operator!=(T* a, const DebugPtr<U>& b) {
	return a != b.Get();
}

template<typename T, typename U>
inline DebugPtr<T> static_pointer_cast(const DebugPtr<U>& o) {
	return DebugPtr<T>(static_cast<T*>(o.Get()), o.GetAge());
}
template<typename T, typename U>
inline DebugPtr<T> const_pointer_cast(const DebugPtr<U>& o) {
	return DebugPtr<T>(const_cast<T*>(o.Get()), o.GetAge());
}
template<typename T, typename U>
inline DebugPtr<T> dynamic_pointer_cast(const DebugPtr<U>& o) {
	return DebugPtr<T>(dynamic_cast<T*>(o.Get()), o.GetAge());
}

template<typename T>
T* get_pointer(DebugPtr<T>& ptr) {
	return ptr.Get();
}

namespace std {
template<typename T>
struct less<DebugPtr<T> > {
	bool operator()(const DebugPtr<T>& lhs, const DebugPtr<T>& rhs) const {
		return (reinterpret_cast<size_t>(lhs.Get()) < reinterpret_cast<size_t>(rhs.Get()));
	}
};
template<typename T>
struct equal_to<DebugPtr<T> > {
	bool operator()(const DebugPtr<T>& lhs, const DebugPtr<T>& rhs) const {
		return (lhs.Get() == rhs.Get());
	}
};
template<typename T>
bool operator<(const DebugPtr<T>& lhs, const DebugPtr<T>& rhs) {
	return (reinterpret_cast<size_t>(lhs.Get()) < reinterpret_cast<size_t>(rhs.Get()));
}
} // namespace std

#ifdef __GNUC__
namespace std { namespace tr1 {
#else
namespace boost {
#endif
template<typename T>
struct hash<DebugPtr<T> > {
	size_t operator()(const DebugPtr<T>& v) const {
		return reinterpret_cast<size_t>(v.Get());
	}
};
#ifdef __GNUC__
}} // namespace std::tr1
#else
} // namespace boost
#endif


#endif // DEBUG_ALLOC
#endif // _DEBUG_PTR_H_
