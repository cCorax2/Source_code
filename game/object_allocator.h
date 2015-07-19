#ifndef _OBJECT_ALLOCATOR_H_
#define _OBJECT_ALLOCATOR_H_

#include "debug_allocator.h"

#include <assert.h>
#include <deque>

enum { DEFAULT_FREE_TRIGGER_COUNT = 32 };

typedef std::deque<void*> FreeList; 

template <typename OBJ, size_t FREE_TRIGGER>
class LateAllocator 
{
public:
	LateAllocator() {}

	~LateAllocator()
	{
		while ( m_freeBlockCount > 0 )
		{
			void* p = m_freeBlocks.front();
#ifdef DEBUG_ALLOC
			::free(reinterpret_cast<size_t*>(p) - 1);
#else
			::free( p );
#endif

			m_freeBlocks.pop_front();

			--m_freeBlockCount;
		}
	}

	// Allocates a memory block.
	static void* Alloc(size_t size) 
	{
		void* p = 0;

		if ( m_freeBlockCount > 0 )
		{	
			p = m_freeBlocks.front();

			m_freeBlocks.pop_front();

			--m_freeBlockCount;
		}	
		else
		{
#ifdef DEBUG_ALLOC
			// Reserve size_t age header at the beginning of the block
			// to support quick reference test
			p = ::malloc(size + sizeof(size_t));
			p = reinterpret_cast<size_t*>(p) + 1;
#else
			p = ::malloc(size);
#endif
		}

		return p;
	}

	// Deallocates a memory block.
	static void Free(void* p) 
	{
		if ( p == NULL )
		{
			return;
		}

#ifdef DEBUG_ALLOC
		::memset( p, 0, sizeof(OBJ) );
#endif

		if ( m_freeBlockCount >= FREE_TRIGGER )
		{
#ifdef DEBUG_ALLOC
			// Include size_t age header.
			::free(reinterpret_cast<size_t*>(p) - 1);
#else
			::free(p);
#endif
		}
		else
		{
			++m_freeBlockCount;	

			m_freeBlocks.push_back(p);
		}
	}

	static size_t GetFreeBlockCount() 
	{
		return m_freeBlockCount;
	}

private:

	static size_t 		m_freeBlockCount;
	static FreeList 	m_freeBlocks;
};

template <typename OBJ, size_t FREE_TRIGGER>
size_t LateAllocator<OBJ, FREE_TRIGGER>::m_freeBlockCount = 0;

template <typename OBJ, size_t FREE_TRIGGER>
FreeList LateAllocator<OBJ, FREE_TRIGGER>::m_freeBlocks;

/**
 * @class ObjectAllocator 
 * 
 * NOTE: One must use M2_OBJ_NEW, M2_OBJ_DELETE macros 
 */
template <typename OBJ, size_t FREE_TRIGGER=DEFAULT_FREE_TRIGGER_COUNT> 
class ObjectAllocator 
{
public:
	ObjectAllocator() {}
	virtual ~ObjectAllocator() {}

	static void* operator new( size_t size )
	{
		return m_allocator.Alloc( size );
	}

	static void operator delete( void* p, size_t size )
	{
#ifdef DEBUG_ALLOC
		size_t& age = *(reinterpret_cast<size_t*>(p) - 1);
		age = AllocTag::IncreaseAge(age);
#endif 		
		m_allocator.Free( p );
	}

	static void* operator new( size_t size, const char* f, size_t l )
	{
		void* p = m_allocator.Alloc( size );
#ifdef DEBUG_ALLOC
		if (p != NULL) 
		{
			size_t age = DebugAllocator::MarkAcquired(p, f, l, "new_obj");
			*(reinterpret_cast<size_t*>(p) - 1) = age;
		}
#endif 		
		return p;
	}

	static size_t GetFreeBlockCount() 
	{
		return m_allocator.GetFreeBlockCount();
	}

private:
	static LateAllocator<OBJ, FREE_TRIGGER> m_allocator;	
};

#ifdef DEBUG_ALLOC
#define M2_OBJ_NEW new(__FILE__, __LINE__)
#define M2_OBJ_DELETE(p) \
	delete (get_pointer(p)); \
	DebugAllocator::MarkReleased(p, __FILE__, __LINE__, "delete_obj");
#define M2_OBJ_DELETE_EX(p, f, l) \
	delete (get_pointer(p)); \
	DebugAllocator::MarkReleased(p, f, l, "delete_obj");
#else
#define M2_OBJ_NEW new
#define M2_OBJ_DELETE(p) delete (p)
#endif // DEBUG_ALLOC

#endif // _OBJECT_ALLOCATOR_H_
