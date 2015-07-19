// Basic features
// Enable or disable memory pooling for specific object types
//#define M2_USE_POOL
// Enable or disable heap allocation debugging
//#define DEBUG_ALLOC

#include "debug_allocator.h"

#include "../../libthecore/include/stdafx.h"

#include "../../common/singleton.h"
#include "../../common/utils.h"
#include "../../common/service.h"

#include <algorithm>
#include <math.h>
#include <list>
#include <map>
#include <set>
#include <queue>
#include <string>
#include <vector>

#ifdef __GNUC__
#include <float.h>
#include <tr1/unordered_map>
#include <tr1/unordered_set>
#define TR1_NS std::tr1
#else
#include <boost/unordered_map.hpp>
#include <boost/unordered_set.hpp>
#define TR1_NS boost
#define isdigit iswdigit
#define isspace iswspace
#endif

#include "typedef.h"
#include "locale.hpp"
#include "event.h"

#define PASSES_PER_SEC(sec) ((sec) * passes_per_sec)

#ifndef M_PI
#define M_PI    3.14159265358979323846 /* pi */
#endif
#ifndef M_PI_2
#define M_PI_2  1.57079632679489661923 /* pi/2 */
#endif

#define IN
#define OUT
