
#include "stdafx.h"

#include "DragonLair.h"

#include "entity.h"
#include "sectree_manager.h"
#include "char.h"
#include "guild.h"
#include "locale_service.h"
#include "regen.h"
#include "log.h"
#include "utils.h"

extern int passes_per_sec;

struct FWarpToDragronLairWithGuildMembers
{
	DWORD dwGuildID;
	long mapIndex;
	long x, y;

	FWarpToDragronLairWithGuildMembers( DWORD guildID, long map, long X, long Y )
		: dwGuildID(guildID), mapIndex(map), x(X), y(Y)
	{
	}

	void operator()(LPENTITY ent)
	{
		if (NULL != ent && true == ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (true == pChar->IsPC())
			{
				if (NULL != pChar->GetGuild())
				{
					if (dwGuildID == pChar->GetGuild()->GetID())
					{
						pChar->WarpSet(x, y, mapIndex);
					}
				}
			}
		}
	}
};

struct FWarpToVillage
{
	void operator() (LPENTITY ent)
	{
		if (NULL != ent)
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (NULL != pChar)
			{
				if (true == pChar->IsPC())
				{
					pChar->GoHome();
				}
			}
		}
	}
};

EVENTINFO(tag_DragonLair_Collapse_EventInfo)
{
	int step;
	CDragonLair* pLair;
	long InstanceMapIndex;

	tag_DragonLair_Collapse_EventInfo()
	: step( 0 )
	, pLair( 0 )
	, InstanceMapIndex( 0 )
	{
	}
};

EVENTFUNC( DragonLair_Collapse_Event )
{
	tag_DragonLair_Collapse_EventInfo* pInfo = dynamic_cast<tag_DragonLair_Collapse_EventInfo*>(event->info);

	if ( pInfo == NULL )
	{
		sys_err( "DragonLair_Collapse_Event> <Factor> Null pointer" );
		return 0;
	}

	if (0 == pInfo->step)
	{
		char buf[512];
		snprintf(buf, 512, LC_TEXT("용가리가 %d 초만에 죽어써효ㅠㅠ"), pInfo->pLair->GetEstimatedTime());
		SendNoticeMap(buf, pInfo->InstanceMapIndex, true);

		pInfo->step++;

		return PASSES_PER_SEC( 30 );
	}
	else if (1 == pInfo->step)
	{
		LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap( pInfo->InstanceMapIndex );

		if (NULL != pMap)
		{
			FWarpToVillage f;
			pMap->for_each( f );
		}

		pInfo->step++;

		return PASSES_PER_SEC( 30 );
	}
	else
	{
		SECTREE_MANAGER::instance().DestroyPrivateMap( pInfo->InstanceMapIndex );
		M2_DELETE(pInfo->pLair);
	}

	return 0;
}









CDragonLair::CDragonLair(DWORD guildID, long BaseMapID, long PrivateMapID)
	: GuildID_(guildID), BaseMapIndex_(BaseMapID), PrivateMapIndex_(PrivateMapID)
{
	StartTime_ = get_global_time();
}

CDragonLair::~CDragonLair()
{
}

DWORD CDragonLair::GetEstimatedTime() const
{
	return get_global_time() - StartTime_;
}

void CDragonLair::OnDragonDead(LPCHARACTER pDragon)
{
	sys_log(0, "DragonLair: 도라곤이 죽어써효");

	LogManager::instance().DragonSlayLog(  GuildID_, pDragon->GetMobTable().dwVnum, StartTime_, get_global_time() );
}












CDragonLairManager::CDragonLairManager()
{
}

CDragonLairManager::~CDragonLairManager()
{
}

bool CDragonLairManager::Start(long MapIndexFrom, long BaseMapIndex, DWORD GuildID)
{
	long instanceMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(BaseMapIndex);
	if (instanceMapIndex == 0) {
		sys_err("CDragonLairManager::Start() : no private map index available");
		return false;
	}

	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(MapIndexFrom);

	if (NULL != pMap)
	{
		LPSECTREE_MAP pTargetMap = SECTREE_MANAGER::instance().GetMap(BaseMapIndex);

		if (NULL == pTargetMap)
		{
			return false;
		}

		const TMapRegion* pRegionInfo = SECTREE_MANAGER::instance().GetMapRegion( pTargetMap->m_setting.iIndex );

		if (NULL != pRegionInfo)
		{
			FWarpToDragronLairWithGuildMembers f(GuildID, instanceMapIndex, 844000, 1066900);

			pMap->for_each( f );

			LairMap_.insert( std::make_pair(GuildID, M2_NEW CDragonLair(GuildID, BaseMapIndex, instanceMapIndex)) );

			std::string strMapBasePath( LocaleService_GetMapPath() );

			strMapBasePath += "/" + pRegionInfo->strMapName + "/instance_regen.txt";

			sys_log(0, "%s", strMapBasePath.c_str());

			regen_do(strMapBasePath.c_str(), instanceMapIndex, pTargetMap->m_setting.iBaseX, pTargetMap->m_setting.iBaseY, NULL, true);

			return true;
		}
	}

	return false;
}

void CDragonLairManager::OnDragonDead(LPCHARACTER pDragon, DWORD KillerGuildID)
{
	if (NULL == pDragon)
		return;

	if (false == pDragon->IsMonster())
		return;

	boost::unordered_map<DWORD, CDragonLair*>::iterator iter = LairMap_.find( KillerGuildID );

	if (LairMap_.end() == iter)
	{
		return;
	}

	LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap( pDragon->GetMapIndex() );

	if (NULL == iter->second || NULL == pMap)
	{
		LairMap_.erase( iter );
		return;
	}

	iter->second->OnDragonDead( pDragon );

	// 애들 다 집으로 보내고 맵 없애기

	tag_DragonLair_Collapse_EventInfo* info;
	info = AllocEventInfo<tag_DragonLair_Collapse_EventInfo>();

	info->step = 0;
	info->pLair = iter->second;
	info->InstanceMapIndex = pDragon->GetMapIndex();

	event_create(DragonLair_Collapse_Event, info, PASSES_PER_SEC(10));

	LairMap_.erase( iter );
}

