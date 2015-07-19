#include "stdafx.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char_manager.h"
#include "sectree_manager.h"
#include "config.h"
#include "char.h"
#include "wedding.h"
#include "regen.h"
#include "locale_service.h"

namespace marriage
{
	using namespace std;

	EVENTINFO(wedding_map_info)
	{
		WeddingMap * pWeddingMap;
		int iStep;

		wedding_map_info()
		: pWeddingMap( 0 )
		, iStep( 0 )
		{
		}
	};

	EVENTFUNC(wedding_end_event)
	{
		wedding_map_info* info = dynamic_cast<wedding_map_info*>( event->info );

		if ( info == NULL )
		{
			sys_err( "wedding_end_event> <Factor> Null pointer" );
			return 0;
		}
		
		WeddingMap* pMap = info->pWeddingMap;

		if (info->iStep == 0)
		{
			++info->iStep;
			pMap->WarpAll(); 
			return PASSES_PER_SEC(15);
		}
		WeddingManager::instance().DestroyWeddingMap(pMap);
		return 0;
	}

	// Map instance
	WeddingMap::WeddingMap(DWORD dwMapIndex, DWORD dwPID1, DWORD dwPID2) :
		m_dwMapIndex(dwMapIndex),
		m_pEndEvent(NULL),
		m_isDark(false),
		m_isSnow(false),
		m_isMusic(false),
		dwPID1(dwPID1),
		dwPID2(dwPID2)
	{
	}

	WeddingMap::~WeddingMap()
	{
		event_cancel(&m_pEndEvent);
	}

	void WeddingMap::SetEnded()
	{
		if (m_pEndEvent)
		{
			sys_err("WeddingMap::SetEnded - ALREADY EndEvent(m_pEndEvent=%x)", get_pointer(m_pEndEvent));
			return;
		}

		wedding_map_info* info = AllocEventInfo<wedding_map_info>();

		info->pWeddingMap = this;

		m_pEndEvent = event_create(wedding_end_event, info, PASSES_PER_SEC(5));

		Notice(LC_TEXT("결혼식이 종료됩니다."));
		Notice(LC_TEXT("자동으로 나가게됩니다."));

		for (itertype(m_set_pkChr) it = m_set_pkChr.begin(); it != m_set_pkChr.end(); ++it)
		{
			LPCHARACTER ch = *it;
			if (ch->GetPlayerID() == dwPID1 || ch->GetPlayerID() == dwPID2)
				continue;

			if (ch->GetLevel() < 10) // 10 레벨이하는 주지않는다.
				continue;

			//ch->AutoGiveItem(27003, 5);
			ch->AutoGiveItem(27002, 5);
		}
	}

	struct FNotice
	{
		FNotice(const char * psz) :
			m_psz(psz)
		{
		}

		void operator() (LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_psz);
		}

		const char * m_psz;
	};

	void WeddingMap::Notice(const char * psz)
	{
		FNotice f(psz);
		for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
	}

	struct FWarpEveryone
	{
		void operator() (LPCHARACTER ch)
		{
			if (ch->IsPC())
			{
				// ExitToSavedLocation은 WarpSet을 부르는데 이 함수에서
				// Sectree가 NULL이 된다. 추 후 SectreeManager로 부터는
				// 이 캐릭터를 찾을 수 없으므로 아래 DestroyAll에서 별도 처리함
				ch->ExitToSavedLocation();
			}
		}
	};

	void WeddingMap::WarpAll()
	{
		FWarpEveryone f;
		for_each(m_set_pkChr.begin(), m_set_pkChr.end(), f);
	}

	struct FDestroyEveryone
	{
		void operator() (LPCHARACTER ch)
		{
			sys_log(0, "WeddingMap::DestroyAll: %s", ch->GetName());

			if (ch->GetDesc())
				DESC_MANAGER::instance().DestroyDesc(ch->GetDesc());
			else
				M2_DESTROY_CHARACTER(ch);
		}
	};

	void WeddingMap::DestroyAll()
	{
		sys_log(0, "WeddingMap::DestroyAll: m_set_pkChr size %zu", m_set_pkChr.size());
		
		FDestroyEveryone f;

		for (charset_t::iterator it = m_set_pkChr.begin(); it != m_set_pkChr.end(); it = m_set_pkChr.begin())
			f(*it);
	}

	void WeddingMap::IncMember(LPCHARACTER ch)
	{
		if (IsMember(ch) == true)
			return;

		//sys_log(0, "WeddingMap: IncMember %s", ch->GetName());
		m_set_pkChr.insert(ch);

		SendLocalEvent(ch);

		if (ch->GetLevel() < 10)
		{
			ch->SetObserverMode(true);
		}
	}

	void WeddingMap::DecMember(LPCHARACTER ch)
	{
		if (IsMember(ch) == false)
			return;

		//sys_log(0, "WeddingMap: DecMember %s", ch->GetName());
		m_set_pkChr.erase(ch);

		if (ch->GetLevel() < 10)
		{
			ch->SetObserverMode(false);
		}
	}

	bool WeddingMap::IsMember(LPCHARACTER ch)
	{
		if (m_set_pkChr.size() <= 0)
			return false;

		return m_set_pkChr.find(ch) != m_set_pkChr.end();
	}

	void WeddingMap::ShoutInMap(BYTE type, const char* msg)
	{
		for (itertype(m_set_pkChr) it = m_set_pkChr.begin(); it != m_set_pkChr.end(); ++it)
		{
			LPCHARACTER ch = *it;
			ch->ChatPacket(CHAT_TYPE_COMMAND, msg);
		}
	}

	void WeddingMap::SetMusic(bool bSet, const char* musicFileName)
	{
		if (m_isMusic != bSet)
		{
			m_isMusic = bSet;
			m_stMusicFileName = musicFileName;

			char szCommand[256];
			if (m_isMusic)
			{
				ShoutInMap(CHAT_TYPE_COMMAND, __BuildCommandPlayMusic(szCommand, sizeof(szCommand), 1, m_stMusicFileName.c_str()));
			}
			else
			{
				ShoutInMap(CHAT_TYPE_COMMAND, __BuildCommandPlayMusic(szCommand, sizeof(szCommand), 0, "default"));
			}
		} 
	}

	void WeddingMap::SetDark(bool bSet)
	{
		if (m_isDark != bSet)
		{
			m_isDark = bSet;

			if (m_isDark)
				ShoutInMap(CHAT_TYPE_COMMAND, "DayMode dark");
			else
				ShoutInMap(CHAT_TYPE_COMMAND, "DayMode light");
		}
	}

	void WeddingMap::SetSnow(bool bSet)
	{
		if (m_isSnow != bSet)
		{
			m_isSnow = bSet;

			if (m_isSnow)
				ShoutInMap(CHAT_TYPE_COMMAND, "xmas_snow 1");
			else
				ShoutInMap(CHAT_TYPE_COMMAND, "xmas_snow 0");
		}
	}

	bool WeddingMap::IsPlayingMusic()
	{
		return m_isMusic;
	}

	void WeddingMap::SendLocalEvent(LPCHARACTER ch)
	{
		char szCommand[256];

		if (m_isDark)
			ch->ChatPacket(CHAT_TYPE_COMMAND, "DayMode dark");
		if (m_isSnow)
			ch->ChatPacket(CHAT_TYPE_COMMAND, "xmas_snow 1");
		if (m_isMusic)
			ch->ChatPacket(CHAT_TYPE_COMMAND, __BuildCommandPlayMusic(szCommand, sizeof(szCommand), 1, m_stMusicFileName.c_str()));	
	}

	const char* WeddingMap::__BuildCommandPlayMusic(char* szCommand, size_t nCmdLen, BYTE bSet, const char* c_szMusicFileName)
	{
		if (nCmdLen < 1)
		{
			szCommand[0] = '\0';
			return "PlayMusic 0 CommandLengthError";
		}

		snprintf(szCommand, nCmdLen, "PlayMusic %d %s", bSet, c_szMusicFileName);
		return szCommand;
	}
	// Manager

	WeddingManager::WeddingManager()
	{
	}

	WeddingManager::~WeddingManager()
	{
	}

	bool WeddingManager::IsWeddingMap(DWORD dwMapIndex)
	{
		return (dwMapIndex == WEDDING_MAP_INDEX || dwMapIndex / 10000 == WEDDING_MAP_INDEX);
	}

	WeddingMap* WeddingManager::Find(DWORD dwMapIndex)
	{
		itertype(m_mapWedding) it = m_mapWedding.find(dwMapIndex);

		if (it == m_mapWedding.end())
			return NULL;

		return it->second;
	}

	DWORD WeddingManager::__CreateWeddingMap(DWORD dwPID1, DWORD dwPID2)
	{
		SECTREE_MANAGER& rkSecTreeMgr = SECTREE_MANAGER::instance();

		DWORD dwMapIndex = rkSecTreeMgr.CreatePrivateMap(WEDDING_MAP_INDEX);

		if (!dwMapIndex)
		{
			sys_err("CreateWeddingMap(pid1=%d, pid2=%d) / CreatePrivateMap(%d) FAILED", dwPID1, dwPID2, WEDDING_MAP_INDEX);
			return 0;
		}

		m_mapWedding.insert(make_pair(dwMapIndex, M2_NEW WeddingMap(dwMapIndex, dwPID1, dwPID2)));


		// LOCALE_SERVICE
		LPSECTREE_MAP pkSectreeMap = rkSecTreeMgr.GetMap(dwMapIndex);
		if (pkSectreeMap == NULL) {
			return 0;
		}
		string st_weddingMapRegenFileName;
		st_weddingMapRegenFileName.reserve(64);
		st_weddingMapRegenFileName  = LocaleService_GetMapPath();
		st_weddingMapRegenFileName += "/metin2_map_wedding_01/npc.txt";

		if (!regen_do(st_weddingMapRegenFileName.c_str(), dwMapIndex, pkSectreeMap->m_setting.iBaseX, pkSectreeMap->m_setting.iBaseY, NULL, true))
		{
			sys_err("CreateWeddingMap(pid1=%d, pid2=%d) / regen_do(fileName=%s, mapIndex=%d, basePos=(%d, %d)) FAILED", dwPID1, dwPID2, st_weddingMapRegenFileName.c_str(), dwMapIndex, pkSectreeMap->m_setting.iBaseX, pkSectreeMap->m_setting.iBaseY);
		}
		else
		{
			sys_log(0, "CreateWeddingMap(pid1=%d, pid2=%d) / regen_do(fileName=%s, mapIndex=%d, basePos=(%d, %d)) ok", dwPID1, dwPID2, st_weddingMapRegenFileName.c_str(), dwMapIndex, pkSectreeMap->m_setting.iBaseX, pkSectreeMap->m_setting.iBaseY);
		}
		// END_OF_LOCALE_SERVICE

		return dwMapIndex;
	}
	
	void WeddingManager::DestroyWeddingMap(WeddingMap* pMap)
	{
		sys_log(0, "DestroyWeddingMap(index=%u)", pMap->GetMapIndex());
		pMap->DestroyAll();
		m_mapWedding.erase(pMap->GetMapIndex());
		SECTREE_MANAGER::instance().DestroyPrivateMap(pMap->GetMapIndex());
		M2_DELETE(pMap);
	}

	bool WeddingManager::End(DWORD dwMapIndex)
	{
		itertype(m_mapWedding) it = m_mapWedding.find(dwMapIndex);

		if (it == m_mapWedding.end())
			return false;

		it->second->SetEnded();
		return true;
	}

	void WeddingManager::Request(DWORD dwPID1, DWORD dwPID2)
	{
		if (map_allow_find(WEDDING_MAP_INDEX))
		{
			DWORD dwMapIndex = __CreateWeddingMap(dwPID1, dwPID2);

			if (!dwMapIndex)
			{
				sys_err("cannot create wedding map for %u, %u", dwPID1, dwPID2);
				return;
			}

			TPacketWeddingReady p;
			p.dwPID1 = dwPID1;
			p.dwPID2 = dwPID2;
			p.dwMapIndex = dwMapIndex;

			db_clientdesc->DBPacket(HEADER_GD_WEDDING_READY, 0, &p, sizeof(p));
		}
	}

}
