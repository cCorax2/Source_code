#include "stdafx.h"

#include "questlua.h"
#include "questmanager.h"
#include "horsename_manager.h"
#include "char.h"
#include "affect.h"
#include "config.h"
#include "utils.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

extern int (*check_name) (const char * str);

namespace quest
{
	//
	// "horse" Lua functions
	//
	int horse_is_riding(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch->IsHorseRiding())
			lua_pushnumber(L, 1);
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int horse_is_summon(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (NULL != ch)
		{
			lua_pushboolean(L, (ch->GetHorse() != NULL) ? true : false);
		}
		else
		{
			lua_pushboolean(L, false);
		}

		return 1;
	}

	int horse_ride(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->StartRiding();
		return 0;
	}

	int horse_unride(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->StopRiding();
		return 0;
	}

	int horse_summon(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		// 소환하면 멀리서부터 달려오는지 여부
		bool bFromFar = lua_isboolean(L, 1) ? lua_toboolean(L, 1) : false;

		// 소환수의 vnum
		DWORD horseVnum= lua_isnumber(L, 2) ? lua_tonumber(L, 2) : 0;

		const char* name = lua_isstring(L, 3) ? lua_tostring(L, 3) : 0;
		ch->HorseSummon(true, bFromFar, horseVnum, name);
		return 0;
	}

	int horse_unsummon(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		ch->HorseSummon(false);
		return 0;
	}

	int horse_is_mine(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		LPCHARACTER horse = CQuestManager::instance().GetCurrentNPCCharacterPtr();

		lua_pushboolean(L, horse && horse->GetRider() == ch);
		return 1;
	}

	int horse_set_level(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (!lua_isnumber(L, 1))
			return 0;

		int newlevel = MINMAX(0, (int)lua_tonumber(L, 1), HORSE_MAX_LEVEL);
		ch->SetHorseLevel(newlevel);
		ch->ComputePoints();
		ch->SkillLevelPacket();
		return 0;
	}

	int horse_get_level(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetHorseLevel());
		return 1;
	}

	int horse_advance(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch->GetHorseLevel() >= HORSE_MAX_LEVEL)
			return 0;

		ch->SetHorseLevel(ch->GetHorseLevel() + 1);
		ch->ComputePoints();
		ch->SkillLevelPacket();
		return 0;
	}

	int horse_get_health(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch->GetHorseLevel())
			lua_pushnumber(L, ch->GetHorseHealth());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int horse_get_health_pct(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		int pct = MINMAX(0, ch->GetHorseHealth() * 100 / ch->GetHorseMaxHealth(), 100);
		sys_log(1, "horse.get_health_pct %d", pct);

		if (ch->GetHorseLevel())
			lua_pushnumber(L, pct);
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int horse_get_stamina(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if (ch->GetHorseLevel())
			lua_pushnumber(L, ch->GetHorseStamina());
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int horse_get_stamina_pct(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int pct = MINMAX(0, ch->GetHorseStamina() * 100 / ch->GetHorseMaxStamina(), 100);
		sys_log(1, "horse.get_stamina_pct %d", pct);

		if (ch->GetHorseLevel())
			lua_pushnumber(L, pct);
		else
			lua_pushnumber(L, 0);

		return 1;
	}

	int horse_get_grade(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushnumber(L, ch->GetHorseGrade());
		return 1;
	}

	int horse_is_dead(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		lua_pushboolean(L, ch->GetHorseHealth()<=0);
		return 1;
	}

	int horse_revive(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (ch->GetHorseLevel() > 0 && ch->GetHorseHealth() <= 0)
		{
			ch->ReviveHorse();
		}
		return 0;
	}

	int horse_feed(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		//DWORD dwHorseFood = ch->GetHorseLevel() + ITEM_HORSE_FOOD_1 - 1;
		if (ch->GetHorseLevel() > 0 && ch->GetHorseHealth() > 0)
		{
			ch->FeedHorse();
		}
		return 0;
	}

	int horse_set_name(lua_State* L)
	{
		// 리턴값
		// 0 : 소유한 말이 없다
		// 1 : 잘못된 이름이다
		// 2 : 이름 바꾸기 성공

		if ( lua_isstring(L, -1) != true ) return 0;

		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch->GetHorseLevel() > 0 )
		{
			const char* pHorseName = lua_tostring(L, -1);

			if ( pHorseName == NULL || check_name(pHorseName) == 0 )
			{
				lua_pushnumber(L, 1);
			}
			else
			{
				int nHorseNameDuration = test_server == true ? 60*5 : 60*60*24*30;

				ch->SetQuestFlag("horse_name.valid_till", get_global_time() + nHorseNameDuration);
				ch->AddAffect(AFFECT_HORSE_NAME, 0, 0, 0, PASSES_PER_SEC(nHorseNameDuration), 0, true);

				CHorseNameManager::instance().UpdateHorseName(ch->GetPlayerID(), lua_tostring(L, -1), true);

				ch->HorseSummon(false, true);
				ch->HorseSummon(true, true);

				lua_pushnumber(L, 2);
			}
		}
		else
		{
			lua_pushnumber(L, 0);
		}

		return 1;
	}

	int horse_get_name(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();

		if ( ch != NULL )
		{
			const char* pHorseName = CHorseNameManager::instance().GetHorseName(ch->GetPlayerID());

			if ( pHorseName != NULL )
			{
				lua_pushstring(L, pHorseName);
				return 1;
			}
		}

		lua_pushstring(L, "");

		return 1;
	}

	void RegisterHorseFunctionTable()
	{
		luaL_reg horse_functions[] =
		{
			{ "is_mine",		horse_is_mine			},
			{ "is_riding",		horse_is_riding			},
			{ "is_summon",		horse_is_summon			},
			{ "ride",			horse_ride				},
			{ "unride",			horse_unride			},
			{ "summon",			horse_summon			},
			{ "unsummon",		horse_unsummon			},
			{ "advance",		horse_advance			},
			{ "get_level",		horse_get_level			},
			{ "set_level",		horse_set_level			},
			{ "get_health",		horse_get_health		},
			{ "get_health_pct",	horse_get_health_pct	},
			{ "get_stamina",	horse_get_stamina		},
			{ "get_stamina_pct",horse_get_stamina_pct	},
			{ "get_grade",      horse_get_grade         },
			{ "is_dead",		horse_is_dead			},
			{ "revive",			horse_revive			},
			{ "feed",			horse_feed				},
			{ "set_name",		horse_set_name			},
			{ "get_name",		horse_get_name			},

			{ NULL,				NULL					}
		};

		CQuestManager::instance().AddLuaFunctionTable("horse", horse_functions);
	}
}




