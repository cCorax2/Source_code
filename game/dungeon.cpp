#include "stdafx.h"
#include "dungeon.h"
#include "char.h"
#include "char_manager.h"
#include "party.h"
#include "affect.h"
#include "packet.h"
#include "desc.h"
#include "config.h"
#include "regen.h"
#include "start_position.h"
#include "item.h"
#include "item_manager.h"
#include "utils.h"
#include "questmanager.h"

CDungeon::CDungeon(IdType id, long lOriginalMapIndex, long lMapIndex)
	: m_id(id),
	m_lOrigMapIndex(lOriginalMapIndex),
	m_lMapIndex(lMapIndex),
	m_map_Area(SECTREE_MANAGER::instance().GetDungeonArea(lOriginalMapIndex))
{
	Initialize();
	//sys_log(0,"DUNGEON create orig %d real %d", lOriginalMapIndex, lMapIndex);
}

CDungeon::~CDungeon()
{
	if (m_pParty != NULL)
	{
		m_pParty->SetDungeon_for_Only_party (NULL);
	}
	//sys_log(0,"DUNGEON destroy orig %d real %d", m_lOrigMapIndex, m_lMapIndex	);
	ClearRegen();
	event_cancel(&deadEvent);
	// <Factor>
	event_cancel(&exit_all_event_);
	event_cancel(&jump_to_event_);
}

void CDungeon::Initialize()
{
	deadEvent = NULL;
	// <Factor>
	exit_all_event_ = NULL;
	jump_to_event_ = NULL;
	regen_id_ = 0;

	m_iMobKill = 0;
	m_iStoneKill = 0;
	m_bUsePotion = false;
	m_bUseRevive = false;

	m_iMonsterCount = 0;

	m_bExitAllAtEliminate = false;
	m_bWarpAtEliminate = false;

	m_iWarpDelay = 0;
	m_lWarpMapIndex = 0;
	m_lWarpX = 0;
	m_lWarpY = 0;

	m_stRegenFile = "";

	m_pParty = NULL;
}

void CDungeon::SetFlag(std::string name, int value)
{
	itertype(m_map_Flag) it =  m_map_Flag.find(name);
	if (it != m_map_Flag.end())
		it->second = value;
	else
		m_map_Flag.insert(make_pair(name, value));
}

int CDungeon::GetFlag(std::string name)
{
	itertype(m_map_Flag) it =  m_map_Flag.find(name);
	if (it != m_map_Flag.end())
		return it->second;
	else
		return 0;
}

struct FSendDestPosition
{
	FSendDestPosition(long x, long y)
	{
		p1.bHeader = HEADER_GC_DUNGEON;
		p1.subheader = DUNGEON_SUBHEADER_GC_DESTINATION_POSITION;
		p2.x = x;
		p2.y = y;
		p1.size = sizeof(p1)+sizeof(p2);
	}

	void operator()(LPCHARACTER ch)
	{
		ch->GetDesc()->BufferedPacket(&p1, sizeof(TPacketGCDungeon));
		ch->GetDesc()->Packet(&p2, sizeof(TPacketGCDungeonDestPosition));
	}

	TPacketGCDungeon p1;
	TPacketGCDungeonDestPosition p2;
};

void CDungeon::SendDestPositionToParty(LPPARTY pParty, long x, long y)
{
	if (m_map_pkParty.find(pParty) == m_map_pkParty.end())
	{
		sys_err("PARTY %u not in DUNGEON %d", pParty->GetLeaderPID(), m_lMapIndex);
		return;
	}

	FSendDestPosition f(x, y);
	pParty->ForEachNearMember(f);
}

struct FWarpToDungeon
{
	FWarpToDungeon(long lMapIndex, LPDUNGEON d)
		: m_lMapIndex(lMapIndex), m_pkDungeon(d)
		{
			LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
			m_x = pkSectreeMap->m_setting.posSpawn.x;
			m_y = pkSectreeMap->m_setting.posSpawn.y; 
		}

	void operator () (LPCHARACTER ch)
	{
		ch->SaveExitLocation();
		ch->WarpSet(m_x, m_y, m_lMapIndex);
		//m_pkDungeon->IncPartyMember(ch->GetParty());
	}

	long m_lMapIndex;
	long m_x;
	long m_y;
	LPDUNGEON m_pkDungeon;
};

void CDungeon::Join(LPCHARACTER ch)
{
	if (SECTREE_MANAGER::instance().GetMap(m_lMapIndex) == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}
	FWarpToDungeon(m_lMapIndex, this) (ch);
}

void CDungeon::JoinParty(LPPARTY pParty)
{
	pParty->SetDungeon(this);
	m_map_pkParty.insert(std::make_pair(pParty,0));

	if (SECTREE_MANAGER::instance().GetMap(m_lMapIndex) == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}
	FWarpToDungeon f(m_lMapIndex, this);
	pParty->ForEachOnlineMember(f);
	//sys_log(0, "DUNGEON-PARTY join %p %p", this, pParty);
}

void CDungeon::QuitParty(LPPARTY pParty)
{
	pParty->SetDungeon(NULL);
	//sys_log(0, "DUNGEON-PARTY quit %p %p", this, pParty);
	TPartyMap::iterator it = m_map_pkParty.find(pParty);

	if (it != m_map_pkParty.end())
		m_map_pkParty.erase(it);
}

EVENTINFO(dungeon_id_info)
{
	CDungeon::IdType dungeon_id;

	dungeon_id_info() 
	: dungeon_id(0)
	{
	}
};

EVENTFUNC(dungeon_dead_event)
{
	dungeon_id_info* info = dynamic_cast<dungeon_id_info*>( event->info );
	
	if ( info == NULL )
	{
		sys_err( "dungeon_dead_event> <Factor> Null pointer" );
		return 0;
	}

	LPDUNGEON pDungeon = CDungeonManager::instance().Find(info->dungeon_id);
	if (pDungeon == NULL) {
		return 0;
	}

	pDungeon->deadEvent = NULL;

	CDungeonManager::instance().Destroy(info->dungeon_id);
	return 0;
}

void CDungeon::IncMember(LPCHARACTER ch)
{
	if (m_set_pkCharacter.find(ch) == m_set_pkCharacter.end())
		m_set_pkCharacter.insert(ch);

	event_cancel(&deadEvent);
}

void CDungeon::DecMember(LPCHARACTER ch)
{
	itertype(m_set_pkCharacter) it = m_set_pkCharacter.find(ch);

	if (it == m_set_pkCharacter.end()) {
		return;
	}

	m_set_pkCharacter.erase(it);

	if (m_set_pkCharacter.empty())
	{
		dungeon_id_info* info = AllocEventInfo<dungeon_id_info>();
		info->dungeon_id = m_id;

		event_cancel(&deadEvent);
		deadEvent = event_create(dungeon_dead_event, info, PASSES_PER_SEC(10));
	}
}

void CDungeon::IncPartyMember(LPPARTY pParty, LPCHARACTER ch)
{
	//sys_log(0, "DUNGEON-PARTY inc %p %p", this, pParty);
	TPartyMap::iterator it = m_map_pkParty.find(pParty);

	if (it != m_map_pkParty.end())
		it->second++;
	else
		m_map_pkParty.insert(std::make_pair(pParty,1));

	IncMember(ch);
}

void CDungeon::DecPartyMember(LPPARTY pParty, LPCHARACTER ch)
{
	//sys_log(0, "DUNGEON-PARTY dec %p %p", this, pParty);
	TPartyMap::iterator it = m_map_pkParty.find(pParty);

	if (it == m_map_pkParty.end())
		sys_err("cannot find party");
	else
	{
		it->second--;

		if (it->second == 0)
			QuitParty(pParty);
	}

	DecMember(ch);
}

struct FWarpToPosition
{
	long lMapIndex;
	long x;
	long y;
	FWarpToPosition(long lMapIndex, long x, long y)
		: lMapIndex(lMapIndex), x(x), y(y)
		{}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER)) {
			return;
		}
		LPCHARACTER ch = (LPCHARACTER)ent;
		if (!ch->IsPC()) {
			return;
		}
		if (ch->GetMapIndex() == lMapIndex)
		{
			ch->Show(lMapIndex, x, y, 0);
			ch->Stop();
		}
		else
		{
			ch->WarpSet(x,y,lMapIndex);
		}
	}
};

struct FWarpToPositionForce
{
	long lMapIndex;
	long x;
	long y;
	FWarpToPositionForce(long lMapIndex, long x, long y)
		: lMapIndex(lMapIndex), x(x), y(y)
		{}

	void operator()(LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER)) {
			return;
		}
		LPCHARACTER ch = (LPCHARACTER)ent;
		if (!ch->IsPC()) {
			return;
		}
		ch->WarpSet(x,y,lMapIndex);
	}
};

void CDungeon::JumpAll(long lFromMapIndex, int x, int y)
{
	x *= 100;
	y *= 100;

	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(lFromMapIndex);

	if (!pMap)
	{
		sys_err("cannot find map by index %d", lFromMapIndex);
		return;
	}

	FWarpToPosition f(m_lMapIndex, x, y);

	// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
	pMap->for_each(f);
}

void CDungeon::WarpAll(long lFromMapIndex, int x, int y)
{
	x *= 100;
	y *= 100;

	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(lFromMapIndex);

	if (!pMap)
	{
		sys_err("cannot find map by index %d", lFromMapIndex);
		return;
	}

	FWarpToPositionForce f(m_lMapIndex, x, y);

	// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
	pMap->for_each(f);
}

void CDungeon::JumpParty(LPPARTY pParty, long lFromMapIndex, int x, int y)
{
	x *= 100;
	y *= 100;

	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(lFromMapIndex);

	if (!pMap)
	{
		sys_err("cannot find map by index %d", lFromMapIndex);
		return;
	}

	if (pParty->GetDungeon_for_Only_party() == NULL)
	{
		if (m_pParty == NULL)
		{
			m_pParty = pParty;
		}
		else if (m_pParty != pParty)
		{
			sys_err ("Dungeon already has party. Another party cannot jump in dungeon : index %d", GetMapIndex());
			return;
		}
		pParty->SetDungeon_for_Only_party (this);
	}

	FWarpToPosition f(m_lMapIndex, x, y);

	pParty->ForEachOnMapMember(f, lFromMapIndex);
}

void CDungeon::SetPartyNull()
{
	m_pParty = NULL;
}


void CDungeonManager::Destroy(CDungeon::IdType dungeon_id)
{
	sys_log(0, "DUNGEON destroy : map index %u", dungeon_id);
	LPDUNGEON pDungeon = Find(dungeon_id);
	if (pDungeon == NULL) {
		return;
	}
	m_map_pkDungeon.erase(dungeon_id);

	long lMapIndex = pDungeon->m_lMapIndex;
	m_map_pkMapDungeon.erase(lMapIndex);

	DWORD server_timer_arg = lMapIndex;
	quest::CQuestManager::instance().CancelServerTimers(server_timer_arg);

	SECTREE_MANAGER::instance().DestroyPrivateMap(lMapIndex);
	M2_DELETE(pDungeon);
}

LPDUNGEON CDungeonManager::Find(CDungeon::IdType dungeon_id)
{
	itertype(m_map_pkDungeon) it = m_map_pkDungeon.find(dungeon_id);
	if (it != m_map_pkDungeon.end())
		return it->second;
	return NULL;
}

LPDUNGEON CDungeonManager::FindByMapIndex(long lMapIndex)
{
	itertype(m_map_pkMapDungeon) it = m_map_pkMapDungeon.find(lMapIndex);
	if (it != m_map_pkMapDungeon.end()) {
		return it->second;
	}
	return NULL;
}

LPDUNGEON CDungeonManager::Create(long lOriginalMapIndex)
{
	DWORD lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(lOriginalMapIndex);

	if (!lMapIndex) 
	{
		sys_log( 0, "Fail to Create Dungeon : OrginalMapindex %d NewMapindex %d", lOriginalMapIndex, lMapIndex );
		return NULL;
	}

	// <Factor> TODO: Change id assignment, or drop it
	CDungeon::IdType id = next_id_++;
	while (Find(id) != NULL) {
		id = next_id_++;
	}

	LPDUNGEON pDungeon = M2_NEW CDungeon(id, lOriginalMapIndex, lMapIndex);
	if (!pDungeon)
	{
		sys_err("M2_NEW CDungeon failed");
		return NULL;
	}
	m_map_pkDungeon.insert(std::make_pair(id, pDungeon));
	m_map_pkMapDungeon.insert(std::make_pair(lMapIndex, pDungeon));

	return pDungeon;
}

CDungeonManager::CDungeonManager()
	: next_id_(0)
{
}

CDungeonManager::~CDungeonManager()
{
}

void CDungeon::UniqueSetMaxHP(const std::string& key, int iMaxHP)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		sys_err("Unknown Key : %s", key.c_str());
		return;
	}
	it->second->SetMaxHP(iMaxHP);
}

void CDungeon::UniqueSetHP(const std::string& key, int iHP)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		sys_err("Unknown Key : %s", key.c_str());
		return;
	}
	it->second->SetHP(iHP);
}

void CDungeon::UniqueSetDefGrade(const std::string& key, int iGrade)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		sys_err("Unknown Key : %s", key.c_str());
		return;
	}
	it->second->PointChange(POINT_DEF_GRADE,iGrade - it->second->GetPoint(POINT_DEF_GRADE));
}

void CDungeon::SpawnMoveUnique(const char* key, DWORD vnum, const char* pos_from, const char* pos_to)
{
	TAreaMap::iterator it_to = m_map_Area.find(pos_to);
	if (it_to == m_map_Area.end())
	{
		sys_err("Wrong position string : %s", pos_to);
		return;
	}

	TAreaMap::iterator it_from = m_map_Area.find(pos_from);
	if (it_from == m_map_Area.end())
	{
		sys_err("Wrong position string : %s", pos_from);
		return;
	}

	TAreaInfo & ai = it_from->second;
	TAreaInfo & ai_to = it_to->second;
	int dir = ai.dir;
	if (dir==-1)
		dir = number(0,359);

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}
	for (int i=0;i<100;i++)
	{
		int dx = number(ai.sx, ai.ex);
		int dy = number(ai.sy, ai.ey);
		int tx = number(ai_to.sx, ai_to.ex);
		int ty = number(ai_to.sy, ai_to.ey);

		LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(vnum, m_lMapIndex, pkSectreeMap->m_setting.iBaseX+dx, pkSectreeMap->m_setting.iBaseY+dy, 0, false, dir);

		if (ch)
		{
			m_map_UniqueMob.insert(make_pair(std::string(key), ch));
			ch->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
			ch->SetDungeon(this);

			if (ch->Goto(pkSectreeMap->m_setting.iBaseX+tx, pkSectreeMap->m_setting.iBaseY+ty))
				ch->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
		}
		else
		{
			sys_err("Cannot spawn at %d %d", pkSectreeMap->m_setting.iBaseX+((ai.sx+ai.ex)>>1), pkSectreeMap->m_setting.iBaseY+((ai.sy+ai.ey)>>1));
		}
	}

}

void CDungeon::SpawnUnique(const char* key, DWORD vnum, const char* pos)
{
	TAreaMap::iterator it = m_map_Area.find(pos);
	if (it == m_map_Area.end())
	{
		sys_err("Wrong position string : %s", pos);
		return;
	}

	TAreaInfo & ai = it->second;
	int dir = ai.dir;
	if (dir==-1)
		dir = number(0,359);

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}
	for (int i=0;i<100;i++)
	{
		int dx = number(ai.sx, ai.ex);
		int dy = number(ai.sy, ai.ey);

		LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(vnum, m_lMapIndex, pkSectreeMap->m_setting.iBaseX+dx, pkSectreeMap->m_setting.iBaseY+dy, 0, false, dir);

		if (ch)
		{
			m_map_UniqueMob.insert(make_pair(std::string(key), ch));
			ch->SetDungeon(this);
			ch->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
			break;
		}
		else
		{
			sys_err("Cannot spawn at %d %d", pkSectreeMap->m_setting.iBaseX+((ai.sx+ai.ex)>>1), pkSectreeMap->m_setting.iBaseY+((ai.sy+ai.ey)>>1));
		}
	}
}

void CDungeon::SetUnique(const char* key, DWORD vid)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().Find(vid);
	if (ch)
	{
		m_map_UniqueMob.insert(make_pair(std::string(key), ch));
		ch->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	}
}

void CDungeon::SpawnStoneDoor(const char* key, const char* pos) 
{
	SpawnUnique(key, 13001, pos);
}

void CDungeon::SpawnWoodenDoor(const char* key, const char* pos)
{
	SpawnUnique(key, 13000, pos);
	UniqueSetMaxHP(key, 10000);
	UniqueSetHP(key, 10000);
	UniqueSetDefGrade(key, 300);
}

void CDungeon::PurgeUnique(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		sys_err("Unknown Key or Dead: %s", key.c_str());
		return;
	}
	LPCHARACTER ch = it->second;
	m_map_UniqueMob.erase(it);
	M2_DESTROY_CHARACTER(ch);
}

void CDungeon::KillUnique(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		sys_err("Unknown Key or Dead: %s", key.c_str());
		return;
	}
	LPCHARACTER ch = it->second;
	m_map_UniqueMob.erase(it);
	ch->Dead();
}

DWORD CDungeon::GetUniqueVid(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		sys_err("Unknown Key or Dead: %s", key.c_str());
		return 0;
	}
	LPCHARACTER ch = it->second;
	return ch->GetVID();
}

float CDungeon::GetUniqueHpPerc(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);
	if (it == m_map_UniqueMob.end())
	{
		sys_err("Unknown Key : %s", key.c_str());
		return false;
	}
	return (100.f*it->second->GetHP())/it->second->GetMaxHP();
}

void CDungeon::DeadCharacter(LPCHARACTER ch)
{
	if (!ch->IsPC())
	{
		TUniqueMobMap::iterator it = m_map_UniqueMob.begin();
		while (it!=m_map_UniqueMob.end())
		{
			if (it->second == ch)
			{
				//sys_log(0,"Dead unique %s", it->first.c_str());
				m_map_UniqueMob.erase(it);
				break;
			}
			++it;
		}
	}
}

bool CDungeon::IsUniqueDead(const std::string& key)
{
	TUniqueMobMap::iterator it = m_map_UniqueMob.find(key);

	if (it == m_map_UniqueMob.end())
	{
		sys_err("Unknown Key or Dead : %s", key.c_str());
		return true;
	}

	return it->second->IsDead();
}

void CDungeon::Spawn(DWORD vnum, const char* pos)
{
	//sys_log(0,"DUNGEON Spawn %u %s", vnum, pos);
	TAreaMap::iterator it = m_map_Area.find(pos);

	if (it == m_map_Area.end())
	{
		sys_err("Wrong position string : %s", pos);
		return;
	}

	TAreaInfo & ai = it->second;
	int dir = ai.dir;
	if (dir==-1)
		dir = number(0,359);

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL)
	{
		sys_err("cannot find map by index %d", m_lMapIndex);
		return;
	}
	int dx = number(ai.sx, ai.ex);
	int dy = number(ai.sy, ai.ey);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(vnum, m_lMapIndex, pkSectreeMap->m_setting.iBaseX+dx, pkSectreeMap->m_setting.iBaseY+dy, 0, false, dir);
	if (ch)
		ch->SetDungeon(this);
}

LPCHARACTER CDungeon::SpawnMob(DWORD vnum, int x, int y, int dir)
{
	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return NULL;
	}
	sys_log(0, "CDungeon::SpawnMob %u %d %d", vnum, x,  y);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(vnum, m_lMapIndex, pkSectreeMap->m_setting.iBaseX+x*100, pkSectreeMap->m_setting.iBaseY+y*100, 0, false, dir == 0 ? -1 : (dir - 1) * 45);

	if (ch)
	{
		ch->SetDungeon(this);
		sys_log(0, "CDungeon::SpawnMob name %s", ch->GetName());
	}

	return ch;
}

LPCHARACTER CDungeon::SpawnMob_ac_dir(DWORD vnum, int x, int y, int dir)
{
	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return NULL;
	}
	sys_log(0, "CDungeon::SpawnMob %u %d %d", vnum, x,  y);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(vnum, m_lMapIndex, pkSectreeMap->m_setting.iBaseX+x*100, pkSectreeMap->m_setting.iBaseY+y*100, 0, false, dir);

	if (ch)
	{
		ch->SetDungeon(this);
		sys_log(0, "CDungeon::SpawnMob name %s", ch->GetName());
	}

	return ch;
}

void CDungeon::SpawnNameMob(DWORD vnum, int x, int y, const char* name)
{
	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}

	LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(vnum, m_lMapIndex, pkSectreeMap->m_setting.iBaseX+x, pkSectreeMap->m_setting.iBaseY+y, 0, false, -1);
	if (ch)
	{
		ch->SetName(name);
		ch->SetDungeon(this);
	}
}

void CDungeon::SpawnGotoMob(long lFromX, long lFromY, long lToX, long lToY)
{
	const int MOB_GOTO_VNUM = 20039;

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}

	sys_log(0, "SpawnGotoMob %d %d to %d %d", lFromX, lFromY, lToX, lToY);

	lFromX = pkSectreeMap->m_setting.iBaseX+lFromX*100;
	lFromY = pkSectreeMap->m_setting.iBaseY+lFromY*100;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(MOB_GOTO_VNUM, m_lMapIndex, lFromX, lFromY, 0, false, -1);

	if (ch)
	{
		char buf[30+1];
		snprintf(buf, sizeof(buf), ". %ld %ld", lToX, lToY);

		ch->SetName(buf);
		ch->SetDungeon(this);
	}
}

LPCHARACTER CDungeon::SpawnGroup(DWORD vnum, long x, long y, float radius, bool bAggressive, int count)
{
	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return NULL;
	}

	int iRadius = (int) radius;

	int sx = pkSectreeMap->m_setting.iBaseX + x - iRadius;
	int sy = pkSectreeMap->m_setting.iBaseY + y - iRadius;
	int ex = sx + iRadius;
	int ey = sy + iRadius;

	LPCHARACTER ch = NULL;

	while (count--)
	{
		LPCHARACTER chLeader = CHARACTER_MANAGER::instance().SpawnGroup(vnum, m_lMapIndex, sx, sy, ex, ey, NULL, bAggressive, this);
		if (chLeader && !ch)
			ch = chLeader;
	}

	return ch;
}

void CDungeon::SpawnRegen(const char* filename, bool bOnce)
{
	if (!filename)
	{
		sys_err("CDungeon::SpawnRegen(filename=NULL, bOnce=%d) - m_lMapIndex[%d]", bOnce, m_lMapIndex); 
		return;
	}

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (!pkSectreeMap)
	{
		sys_err("CDungeon::SpawnRegen(filename=%s, bOnce=%d) - m_lMapIndex[%d]", filename, bOnce, m_lMapIndex); 
		return;
	}
	regen_do(filename, m_lMapIndex, pkSectreeMap->m_setting.iBaseX, pkSectreeMap->m_setting.iBaseY, this, bOnce);
}

void CDungeon::AddRegen(LPREGEN regen)
{
	regen->id = regen_id_++;
	m_regen.push_back(regen);
}

void CDungeon::ClearRegen()
{
	for (itertype(m_regen) it = m_regen.begin(); it != m_regen.end(); ++it)
	{
		LPREGEN regen = *it;

		event_cancel(&regen->event);
		M2_DELETE(regen);
	}
	m_regen.clear();
}

bool CDungeon::IsValidRegen(LPREGEN regen, size_t regen_id) {
	itertype(m_regen) it = std::find(m_regen.begin(), m_regen.end(), regen);
	if (it == m_regen.end()) {
		return false;
	}
	LPREGEN found = *it;
	return (found->id == regen_id);
}

void CDungeon::SpawnMoveGroup(DWORD vnum, const char* pos_from, const char* pos_to, int count)
{
	TAreaMap::iterator it_to = m_map_Area.find(pos_to);

	if (it_to == m_map_Area.end())
	{
		sys_err("Wrong position string : %s", pos_to);
		return;
	}

	TAreaMap::iterator it_from = m_map_Area.find(pos_from);

	if (it_from == m_map_Area.end())
	{
		sys_err("Wrong position string : %s", pos_from);
		return;
	}

	TAreaInfo & ai = it_from->second;
	TAreaInfo & ai_to = it_to->second;
	int dir = ai.dir;

	if (dir == -1)
		dir = number(0,359);

	LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkSectreeMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}

	while (count--)
	{
		int tx = number(ai_to.sx, ai_to.ex)+pkSectreeMap->m_setting.iBaseX;
		int ty = number(ai_to.sy, ai_to.ey)+pkSectreeMap->m_setting.iBaseY;
		CHARACTER_MANAGER::instance().SpawnMoveGroup(vnum, m_lMapIndex, pkSectreeMap->m_setting.iBaseX+ai.sx, pkSectreeMap->m_setting.iBaseY+ai.sy, pkSectreeMap->m_setting.iBaseX+ai.ex, pkSectreeMap->m_setting.iBaseY+ai.ey, tx, ty, NULL, true);
	}
}

namespace
{
	// DUNGEON_KILL_ALL_BUG_FIX
	struct FKillSectree
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (!ch->IsPC() && !ch->IsPet())
					ch->Dead();
			}
		}
	};
	// END_OF_DUNGEON_KILL_ALL_BUG_FIX

	struct FPurgeSectree
	{
		void operator () (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				if (!ch->IsPC() && !ch->IsPet()) {
					M2_DESTROY_CHARACTER(ch);
				}
			}
			else if (ent->IsType(ENTITY_ITEM))
			{
				LPITEM item = (LPITEM) ent;
				M2_DESTROY_ITEM(item);
			}
			else
				sys_err("unknown entity type %d is in dungeon", ent->GetType());
		}
	};
}

// DUNGEON_KILL_ALL_BUG_FIX
void CDungeon::KillAll()
{
	LPSECTREE_MAP pkMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}
	FKillSectree f;
	pkMap->for_each(f);
}
// END_OF_DUNGEON_KILL_ALL_BUG_FIX

void CDungeon::Purge()
{
	LPSECTREE_MAP pkMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);
	if (pkMap == NULL) {
		sys_err("CDungeon: SECTREE_MAP not found for #%ld", m_lMapIndex);
		return;
	}
	FPurgeSectree f;
	pkMap->for_each(f);
}

void CDungeon::IncKillCount(LPCHARACTER pkKiller, LPCHARACTER pkVictim)
{
	if (pkVictim->IsStone())
		m_iStoneKill ++;
	else
		m_iMobKill ++;
}

void CDungeon::UsePotion(LPCHARACTER ch)
{
	m_bUsePotion = true;
}

void CDungeon::UseRevive(LPCHARACTER ch)
{
	m_bUseRevive = true;
}

bool CDungeon::IsUsePotion()
{
	return m_bUsePotion;
}

bool CDungeon::IsUseRevive()
{
	return m_bUseRevive;
}

int CDungeon::GetKillMobCount()
{
	return m_iMobKill;
}
int CDungeon::GetKillStoneCount()
{
	return m_iStoneKill;
}

struct FCountMonster
{
	int n;
	FCountMonster() : n(0) {};
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (!ch->IsPC())
				n++;
		}
	}
};

int CDungeon::CountRealMonster()
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(m_lOrigMapIndex);

	if (!pMap)
	{
		sys_err("cannot find map by index %d", m_lOrigMapIndex);
		return 0;
	}

	FCountMonster f;

	// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
	pMap->for_each(f);
	return f.n;
}

struct FExitDungeon
{
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->IsPC())
				ch->ExitToSavedLocation();
		}
	}
};

void CDungeon::ExitAll()
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);

	if (!pMap)
	{
		sys_err("cannot find map by index %d", m_lMapIndex);
		return;
	}

	FExitDungeon f;

	// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
	pMap->for_each(f);
}

// DUNGEON_NOTICE
namespace
{
	struct FNotice
	{
		FNotice(const char * psz) : m_psz(psz)
		{
		}

		void operator() (LPENTITY ent)
		{
			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;
				ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
			}
		}

		const char * m_psz;
	};
}

void CDungeon::Notice(const char* msg)
{
	sys_log(0, "XXX Dungeon Notice %p %s", this, msg);
	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);

	if (!pMap)
	{
		sys_err("cannot find map by index %d", m_lMapIndex);
		return;
	}

	FNotice f(msg);
	pMap->for_each(f);
}
// END_OF_DUNGEON_NOTICE

struct FExitDungeonToStartPosition
{
	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->IsPC())
			{
				PIXEL_POSITION posWarp;

				// 현재 맵 인덱스를 넣는 것이 아니라 시작하는 맵 인덱스를 넣는다.
				if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(g_start_map[ch->GetEmpire()], ch->GetEmpire(), posWarp))
					ch->WarpSet(posWarp.x, posWarp.y);
				else
					ch->ExitToSavedLocation();
			}
		}
	}
};

void CDungeon::ExitAllToStartPosition()
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);

	if (!pMap)
	{
		sys_err("cannot find map by index %d", m_lMapIndex);
		return;
	}

	FExitDungeonToStartPosition f;

	// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
	pMap->for_each(f);
}

EVENTFUNC(dungeon_jump_to_event)
{
	dungeon_id_info * info = dynamic_cast<dungeon_id_info *>(event->info);

	if ( info == NULL )
	{
		sys_err( "dungeon_jump_to_event> <Factor> Null pointer" );
		return 0;
	}

	LPDUNGEON pDungeon = CDungeonManager::instance().Find(info->dungeon_id);
	pDungeon->jump_to_event_ = NULL;

	if (pDungeon)
		pDungeon->JumpToEliminateLocation();
	else
		sys_err("cannot find dungeon with map index %u", info->dungeon_id);

	return 0;
}

EVENTFUNC(dungeon_exit_all_event)
{
	dungeon_id_info * info = dynamic_cast<dungeon_id_info *>(event->info);

	if ( info == NULL )
	{
		sys_err( "dungeon_exit_all_event> <Factor> Null pointer" );
		return 0;
	}

	LPDUNGEON pDungeon = CDungeonManager::instance().Find(info->dungeon_id);
	pDungeon->exit_all_event_ = NULL;

	if (pDungeon)
		pDungeon->ExitAll();

	return 0;
}

void CDungeon::CheckEliminated()
{
	if (m_iMonsterCount > 0)
		return;

	if (m_bExitAllAtEliminate)
	{
		sys_log(0, "CheckEliminated: exit");
		m_bExitAllAtEliminate = false;

		if (m_iWarpDelay)
		{
			dungeon_id_info* info = AllocEventInfo<dungeon_id_info>();
			info->dungeon_id = m_id;

			event_cancel(&exit_all_event_);
			exit_all_event_ = event_create(dungeon_exit_all_event, info, PASSES_PER_SEC(m_iWarpDelay));
		}
		else
		{
			ExitAll();
		}
	}
	else if (m_bWarpAtEliminate)
	{
		sys_log(0, "CheckEliminated: warp");
		m_bWarpAtEliminate = false;

		if (m_iWarpDelay)
		{
			dungeon_id_info* info = AllocEventInfo<dungeon_id_info>();
			info->dungeon_id = m_id;

			event_cancel(&jump_to_event_);
			jump_to_event_ = event_create(dungeon_jump_to_event, info, PASSES_PER_SEC(m_iWarpDelay));
		}
		else
		{
			JumpToEliminateLocation();
		}
	}
	else
		sys_log(0, "CheckEliminated: none");
}

void CDungeon::SetExitAllAtEliminate(long time)
{
	sys_log(0, "SetExitAllAtEliminate: time %d", time);
	m_bExitAllAtEliminate = true;
	m_iWarpDelay = time;
}

void CDungeon::SetWarpAtEliminate(long time, long lMapIndex, int x, int y, const char* regen_file)
{
	m_bWarpAtEliminate = true;
	m_iWarpDelay = time;
	m_lWarpMapIndex = lMapIndex;
	m_lWarpX = x;
	m_lWarpY = y;

	if (!regen_file || !*regen_file)
		m_stRegenFile.clear();
	else
		m_stRegenFile = regen_file;

	sys_log(0, "SetWarpAtEliminate: time %d map %d %dx%d regenfile %s", time, lMapIndex, x, y, m_stRegenFile.c_str());
}

void CDungeon::JumpToEliminateLocation()
{
	LPDUNGEON pDungeon = CDungeonManager::instance().FindByMapIndex(m_lWarpMapIndex);

	if (pDungeon)
	{
		pDungeon->JumpAll(m_lMapIndex, m_lWarpX, m_lWarpY);

		if (!m_stRegenFile.empty())
		{
			pDungeon->SpawnRegen(m_stRegenFile.c_str());
			m_stRegenFile.clear();
		}
	}
	else
	{
		// 일반 맵으로 워프
		LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);

		if (!pMap)
		{
			sys_err("no map by index %d", m_lMapIndex);
			return;
		}

		FWarpToPosition f(m_lWarpMapIndex, m_lWarpX * 100, m_lWarpY * 100);

		// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
		pMap->for_each(f);
	}
}

struct FNearPosition
{
	long x;
	long y;
	int dist;
	bool ret;

	FNearPosition(long x, long y, int d) :
		x(x), y(y), dist(d), ret(true)
	{
	}

	void operator()(LPENTITY ent)
	{
		if (ret == false)
			return;

		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;

			if (ch->IsPC())
			{
				if (DISTANCE_APPROX(ch->GetX() - x * 100, ch->GetY() - y * 100) > dist * 100)
					ret = false;
			}
		}
	}
};

bool CDungeon::IsAllPCNearTo(int x, int y, int dist)
{
	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(m_lMapIndex);

	if (!pMap)
	{
		sys_err("cannot find map by index %d", m_lMapIndex);
		return false;
	}

	FNearPosition f(x, y, dist);

	// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
	pMap->for_each(f);

	return f.ret;
}

void CDungeon::CreateItemGroup (std::string& group_name, ItemGroup& item_group)
{
	m_map_ItemGroup.insert (ItemGroupMap::value_type (group_name, item_group));
}

const CDungeon::ItemGroup* CDungeon::GetItemGroup (std::string& group_name)
{
	ItemGroupMap::iterator it = m_map_ItemGroup.find (group_name);
	if (it != m_map_ItemGroup.end())
		return &(it->second);
	else
		return NULL;
}
