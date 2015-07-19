
#include "stdafx.h"

#ifndef DEBUG_ALLOC
#include <boost/pool/object_pool.hpp>
#endif

#include "affect.h"

#ifndef DEBUG_ALLOC
boost::object_pool<CAffect> affect_pool;
#endif

CAffect* CAffect::Acquire()
{
#ifndef DEBUG_ALLOC
	return affect_pool.malloc();
#else
	return M2_NEW CAffect;
#endif
}

void CAffect::Release(CAffect* p)
{
#ifndef DEBUG_ALLOC
	affect_pool.free(p);
#else
	M2_DELETE(p);
#endif
}

