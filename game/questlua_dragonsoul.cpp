#include "stdafx.h"

#include "config.h"
#include "questmanager.h"
#include "char.h"

#undef sys_err
#ifndef __WIN32__
#define sys_err(fmt, args...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, ##args)
#else
#define sys_err(fmt, ...) quest::CQuestManager::instance().QuestError(__FUNCTION__, __LINE__, fmt, __VA_ARGS__)
#endif

namespace quest 
{
	int ds_open_refine_window(lua_State* L)
	{
		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (NULL == ch)
		{
			sys_err ("NULL POINT ERROR");
			return 0;
		}
		if (ch->DragonSoul_IsQualified())
			ch->DragonSoul_RefineWindow_Open(CQuestManager::instance().GetCurrentNPCCharacterPtr());

		return 0;
	}

	int ds_give_qualification(lua_State* L)
	{
		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (NULL == ch)
		{
			sys_err ("NULL POINT ERROR");
			return 0;
		}
		ch->DragonSoul_GiveQualification();

		return 0;
	}

	int ds_is_qualified(lua_State* L)
	{
		const LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		if (NULL == ch)
		{
			sys_err ("NULL POINT ERROR");
			lua_pushnumber(L, 0);
			return 1;
		}

		lua_pushnumber(L, ch->DragonSoul_IsQualified());
		return 1;
	}

	void RegisterDragonSoulFunctionTable()
	{
		luaL_reg ds_functions[] = 
		{
			{ "open_refine_window"	, ds_open_refine_window },
			{ "give_qualification"	, ds_give_qualification },
			{ "is_qualified"		, ds_is_qualified		},
			{ NULL					, NULL					}
		};

		CQuestManager::instance().AddLuaFunctionTable("ds", ds_functions);
	}
};
