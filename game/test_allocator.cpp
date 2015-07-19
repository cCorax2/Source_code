#include "stdafx.h"

#include <gtest/gtest.h>
#define DBG_ALLOC
#include "object_allocator.h"
#include "char.h"
#include "char_manager.h"

struct T : public ObjectAllocator<T>
{
	int v[3];
};

TEST( allocator, object_allocator )
{
	T* p1 = M2_OBJ_NEW T;
	T* p2 = M2_OBJ_NEW T;

	M2_OBJ_DELETE( p1 );
	EXPECT_TRUE( T::GetFreeBlockCount() == 1 );

	M2_OBJ_DELETE( p2 );
	EXPECT_TRUE( T::GetFreeBlockCount() == 2 );

	enum { eTEST_COUNT = 10000 };

	T* p[eTEST_COUNT];

	for ( size_t i=0; i<eTEST_COUNT; ++i )
	{
		p[i] = M2_OBJ_NEW T;
	}

	for ( size_t i=0; i<eTEST_COUNT; ++i )
	{
		M2_OBJ_DELETE( p[i] );
	}

	EXPECT_TRUE( T::GetFreeBlockCount() == 128 );
}

struct T2 : public ObjectAllocator<T2>
{
	int v[128];
};

TEST( allocator, object_allocator_2 )
{
	T* p1 = M2_OBJ_NEW T;
	T2* p2 = M2_OBJ_NEW T2; 

	M2_OBJ_DELETE( p1 );
	M2_OBJ_DELETE( p2 );

	EXPECT_TRUE( T::GetFreeBlockCount() == 128 );
	EXPECT_TRUE( T2::GetFreeBlockCount() == 1 );
}

TEST( allocator, character_alloc )
{
#if 0
	// Allocating CHARACTER makes game crashes on singleon
	// The point of reference cannot be identified for now
	CHARACTER* p1 = new CHARACTER;

	delete p1;

	M2_OBJ_DELETE( p1 );

	EXPECT_TRUE( CHARACTER::GetFreeBlockCount() == 1 );
#endif	
}

