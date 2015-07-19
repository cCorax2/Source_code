#ifndef __INC_METIN_II_GAME_TYPEDEF_H__
#define __INC_METIN_II_GAME_TYPEDEF_H__

class DESC;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<DESC> LPDESC;
#else
typedef DESC* LPDESC;
#endif

class CLIENT_DESC;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<CLIENT_DESC> LPCLIENT_DESC;
#else
typedef CLIENT_DESC* LPCLIENT_DESC;
#endif

class DESC_P2P;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<DESC_P2P> LPDESC_P2P;
#else
typedef DESC_P2P* LPDESC_P2P;
#endif

class CHARACTER;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<CHARACTER> LPCHARACTER;
#else
typedef CHARACTER* LPCHARACTER;
#endif
typedef std::vector<LPCHARACTER> CHARACTER_VECTOR;
typedef std::list<LPCHARACTER> CHARACTER_LIST;
typedef TR1_NS::unordered_set<LPCHARACTER> CHARACTER_SET;

class CItem;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<CItem> LPITEM;
#else
typedef CItem* LPITEM;
#endif

namespace building {
class CObject;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<CObject> LPOBJECT;
#else
typedef CObject* LPOBJECT;
#endif
}

typedef struct regen* LPREGEN;
typedef struct regen_exception* LPREGEN_EXCEPTION;

class CEntity;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<CEntity> LPENTITY;
#else
typedef CEntity* LPENTITY;
#endif
typedef std::vector<LPENTITY> ENTITY_VECTOR;
typedef TR1_NS::unordered_set<LPENTITY> ENTITY_SET;

class SECTREE;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<SECTREE> LPSECTREE;
#else
typedef SECTREE* LPSECTREE;
#endif
typedef std::list<LPSECTREE> LPSECTREE_LIST;

class SECTREE_MAP;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<SECTREE_MAP> LPSECTREE_MAP;
#else
typedef SECTREE_MAP* LPSECTREE_MAP;
#endif

class CDungeon;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<CDungeon> LPDUNGEON;
#else
typedef CDungeon* LPDUNGEON;
#endif

class CParty;
#ifdef USE_DEBUG_PTR
typedef DebugPtr<CParty> LPPARTY;
#else
typedef CParty* LPPARTY;
#endif

typedef struct pixel_position_s
{
	INT x, y, z;
} PIXEL_POSITION;

enum EEntityTypes
{
	ENTITY_CHARACTER,
	ENTITY_ITEM,
	ENTITY_OBJECT,
};

#ifndef itertype
#define itertype(v) typeof((v).begin())
#endif

#endif /* __INC_METIN_II_GAME_TYPEDEF_H__ */

