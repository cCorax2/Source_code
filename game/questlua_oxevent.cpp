
#include "stdafx.h"
#include "questmanager.h"
#include "char.h"
#include "char_manager.h"
#include "OXEvent.h"
#include "config.h"
#include "locale_service.h"

namespace quest
{
	int oxevent_get_status(lua_State* L)
	{
		OXEventStatus ret = COXEventManager::instance().GetStatus();

		lua_pushnumber(L, (int)ret);

		return 1;
	}

	int oxevent_open(lua_State* L)
	{
		COXEventManager::instance().ClearQuiz();

		char script[256];
		snprintf(script, sizeof(script), "%s/oxquiz.lua", LocaleService_GetBasePath().c_str());
		int result = lua_dofile(quest::CQuestManager::instance().GetLuaState(), script);

		if (result != 0)
		{
			lua_pushnumber(L, 0);
			return 1;
		}
		else
		{
			lua_pushnumber(L, 1);
		}
		
		COXEventManager::instance().SetStatus(OXEVENT_OPEN);

		return 1;
	}
	
	int oxevent_close(lua_State* L)
	{
		COXEventManager::instance().SetStatus(OXEVENT_CLOSE);
		
		return 0;
	}
	
	int oxevent_quiz(lua_State* L)
	{
		if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
		{
			bool ret = COXEventManager::instance().Quiz((int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2));

			if (ret == false)
			{
				lua_pushnumber(L, 0);
			}
			else
			{
				lua_pushnumber(L, 1);
			}
		}

		return 1;
	}
	
	int oxevent_get_attender(lua_State* L)
	{
		lua_pushnumber(L, (int)COXEventManager::instance().GetAttenderCount());
		return 1;
	}

	EVENTINFO(end_oxevent_info)
	{
		int empty;

		end_oxevent_info() 
		: empty( 0 )
		{
		}
	};

	EVENTFUNC(end_oxevent)
	{
		COXEventManager::instance().CloseEvent();
		return 0;
	}

	int oxevent_end_event(lua_State* L)
	{
		COXEventManager::instance().SetStatus(OXEVENT_FINISH);

		end_oxevent_info* info = AllocEventInfo<end_oxevent_info>();
		event_create(end_oxevent, info, PASSES_PER_SEC(5));

		return 0;
	}

	int oxevent_end_event_force(lua_State* L)
	{
		COXEventManager::instance().CloseEvent();
		COXEventManager::instance().SetStatus(OXEVENT_FINISH);

		return 0;
	}

	int oxevent_give_item(lua_State* L)
	{
		if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
		{
			COXEventManager::instance().GiveItemToAttender((int)lua_tonumber(L, 1), (int)lua_tonumber(L, 2));
		}

		return 0;
	}
	
	void RegisterOXEventFunctionTable()
	{
		luaL_reg oxevent_functions[] = 
		{
			{	"get_status",	oxevent_get_status	},
			{	"open",			oxevent_open		},
			{	"close",		oxevent_close		},
			{	"quiz",			oxevent_quiz		},
			{	"get_attender",	oxevent_get_attender},
			{	"end_event",	oxevent_end_event	},
			{	"end_event_force",	oxevent_end_event_force	},
			{	"give_item",	oxevent_give_item	},

			{ NULL, NULL}
		};

		CQuestManager::instance().AddLuaFunctionTable("oxevent", oxevent_functions);
	}
}

