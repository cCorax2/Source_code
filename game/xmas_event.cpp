#include "stdafx.h"
#include "config.h"
#include "xmas_event.h"
#include "desc.h"
#include "desc_manager.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"

namespace xmas
{
	void ProcessEventFlag(const std::string& name, int prev_value, int value)
	{
		if (name == "xmas_snow" || name == "xmas_boom" || name == "xmas_song" || name == "xmas_tree")
		{
			// 뿌려준다
			const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();

			for (itertype(c_ref_set) it = c_ref_set.begin(); it != c_ref_set.end(); ++it)
			{
				LPCHARACTER ch = (*it)->GetCharacter();

				if (!ch)
					continue;

				ch->ChatPacket(CHAT_TYPE_COMMAND, "%s %d", name.c_str(), value);
			}

			if (name == "xmas_boom")
			{
				if (value && !prev_value)
				{
					SpawnEventHelper(true);
				}
				else if (!value && prev_value)
				{
					SpawnEventHelper(false);
				}
			}
			else if (name == "xmas_tree")
			{
				if (value > 0 && prev_value == 0)
				{
					CharacterVectorInteractor i;

					// 없으면 만들어준다
					if (!CHARACTER_MANAGER::instance().GetCharactersByRaceNum(MOB_XMAS_TREE_VNUM, i))
						CHARACTER_MANAGER::instance().SpawnMob(MOB_XMAS_TREE_VNUM, 61, 76500 + 358400, 60900 + 153600, 0, false, -1);
				}
				else if (prev_value > 0 && value == 0)
				{
					// 있으면 지워준다
					CharacterVectorInteractor i;

					if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(MOB_XMAS_TREE_VNUM, i))
					{
						CharacterVectorInteractor::iterator it = i.begin();

						while (it != i.end())
							M2_DESTROY_CHARACTER(*it++);
					}
				}
			}
		}
		else if (name == "xmas_santa")
		{
			switch (value)
			{
				case 0:
					// 있으면 지우는 코드
					{
						CharacterVectorInteractor i;

						if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(MOB_SANTA_VNUM, i))
						{
							CharacterVectorInteractor::iterator it = i.begin();

							while (it != i.end())
								M2_DESTROY_CHARACTER(*it++);
						}
					}

					break;

				case 1:
					// 내가 서한산이면 산타 없으면 만들고 상태를 2로 만든다.
					if (map_allow_find(61))
					{
						quest::CQuestManager::instance().RequestSetEventFlag("xmas_santa", 2);

						CharacterVectorInteractor i;

						if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(MOB_SANTA_VNUM, i))
							CHARACTER_MANAGER::instance().SpawnMobRandomPosition(MOB_SANTA_VNUM, 61);
					}
					break;

				case 2:
					break;
			}
		}
	}

	EVENTINFO(spawn_santa_info)
	{
		long lMapIndex;

		spawn_santa_info() 
		: lMapIndex( 0 )
		{
		}
	};

	EVENTFUNC(spawn_santa_event)
	{
		spawn_santa_info* info = dynamic_cast<spawn_santa_info*>( event->info );

		if ( info == NULL )
		{
			sys_err( "spawn_santa_event> <Factor> Null pointer" );
			return 0;
		}

		long lMapIndex = info->lMapIndex;

		if (quest::CQuestManager::instance().GetEventFlag("xmas_santa") == 0)
			return 0;

		CharacterVectorInteractor i;

		if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(MOB_SANTA_VNUM, i))
			return 0;

		if (CHARACTER_MANAGER::instance().SpawnMobRandomPosition(xmas::MOB_SANTA_VNUM, lMapIndex))
		{
			sys_log(0, "santa comes to town!");
			return 0;
		}

		return PASSES_PER_SEC(5);
	}

	void SpawnSanta(long lMapIndex, int iTimeGapSec)
	{
		if (test_server)
		{
			iTimeGapSec /= 60;
		}

		sys_log(0, "santa respawn time = %d", iTimeGapSec);
		spawn_santa_info* info = AllocEventInfo<spawn_santa_info>();

		info->lMapIndex = lMapIndex;

		event_create(spawn_santa_event, info, PASSES_PER_SEC(iTimeGapSec));
	}

	void SpawnEventHelper(bool spawn)
	{
		if ( spawn == true )
		{
			// 없으면 만들어준다
			struct SNPCSellFireworkPosition
			{
				long lMapIndex;
				int x;
				int y;
			} positions[] = {
				{  1,	615,	618 },
				{  3,	500,	625 },
				{ 21,	598,	665 },
				{ 23,	476,	360 },
				{ 41,	318,	629 },
				{ 43,	478,	375 },
				{ 0,	0,	0   },
			};

			SNPCSellFireworkPosition* p = positions;
			while (p->lMapIndex)
			{
				if (map_allow_find(p->lMapIndex))
				{
					PIXEL_POSITION posBase;
					if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(p->lMapIndex, posBase))
					{
						sys_err("cannot get map base position %d", p->lMapIndex);
						p++;
						continue;
					}

					CHARACTER_MANAGER::instance().SpawnMob(
							MOB_XMAS_FIRWORK_SELLER_VNUM, p->lMapIndex, posBase.x + p->x * 100, posBase.y + p->y * 100, 0, false, -1);
				}
				p++;
			}
		}
		else
		{
			CharacterVectorInteractor i;

			// 있으면 지워준다
			if (CHARACTER_MANAGER::instance().GetCharactersByRaceNum(MOB_XMAS_FIRWORK_SELLER_VNUM, i))
			{
				CharacterVectorInteractor::iterator it = i.begin();

				while (it != i.end())
					M2_DESTROY_CHARACTER(*it++);
			}
		}
	}
}
