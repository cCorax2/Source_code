#include "stdafx.h"
#include "SpeedServer.h"
#include "questlua.h"
#include "questmanager.h"

namespace quest
{
	// "sun", "mon", "tue", "wed", "thu", "fri", "sat", "week", "weekend"

	int speedserver_get_wday (lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2))
		{
			sys_err("wrong argument");
		}

		BYTE empire = lua_tonumber(L,1);

		if (empire > 3)
		{
			sys_err("invalid empire");
			return 0;
		}

		int wday = lua_tonumber(L,2) - 1;

		if (wday < 0 || wday > 6)
		{
			sys_err ("wrong day");
			return 0;
		}

		sys_log (0, "empire %d wday %d",empire, wday);

		std::list <HME> time_lst = CSpeedServerManager::instance().GetWdayExpTableOfEmpire(empire, wday);

		int i = 0;
		for (std::list <HME>::iterator it = time_lst.begin();
				it != time_lst.end(); it++)
		{
			sys_log (0, "%d",i);
			lua_pushnumber (L, it->hour);
			lua_pushnumber (L, it->min);
			lua_pushnumber (L, it->exp);
			i++;
		}

		return i * 3;
	}

	int speedserver_set_wday (lua_State* L)
	{
		BYTE empire = lua_tonumber (L, 1);
		int wday = lua_tonumber (L, 2);
		BYTE end_hour = lua_tonumber(L, 3);
		BYTE end_minite = lua_tonumber(L, 4);
		int exp_percent = lua_tonumber(L, 5);

		CSpeedServerManager::instance().SetWdayExpTableOfEmpire (empire, wday - 1, HME (end_hour, end_minite, exp_percent));

		return 0;
	}

	int speedserver_init_wday (lua_State* L)
	{
		if (!lua_isnumber (L, 1) || !lua_isnumber (L, 2))
		{
			sys_err ("invalid argument.");
			return 0;
		}

		BYTE empire = lua_tonumber (L, 1);
		int wday = lua_tonumber (L, 2);
		
		sys_log (0, "init_wday %d %d",empire, wday);

		CSpeedServerManager::instance().InitWdayExpTableOfEmpire (empire, wday - 1);
		
		return 0;
	}

	int speedserver_get_holiday (lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2) || !lua_isnumber(L,3) || !lua_isnumber(L,4))
		{
			sys_err("wrong argument");
		}

		BYTE empire = lua_tonumber(L,1);

		if (empire > 4)
		{
			sys_err("invalid empire");
			return 0;
		}

		Date date = Date (lua_tonumber(L,2) - 1900, lua_tonumber(L,3) - 1, lua_tonumber(L,4));
		
		sys_log (0, "empire %d date %d %d %d", empire, date.year, date.mon, date.day);
		
		bool is_exist;

		std::list <HME> time_lst = CSpeedServerManager::instance().GetHolidayExpTableOfEmpire(empire, date, is_exist);

		int i = 0;
		if (is_exist)
		{
			for (std::list <HME>::iterator it = time_lst.begin();
					it != time_lst.end(); it++)
			{
				lua_pushnumber (L, it->hour);
				lua_pushnumber (L, it->min);
				lua_pushnumber (L, it->exp);
				i++;
			}
			return i * 3;
		}
		else
			return 0;

	}

	int speedserver_set_holiday (lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2) || !lua_isnumber(L,3) || !lua_isnumber(L,4) 
				|| !lua_isnumber(L,5) || !lua_isnumber(L,6) || !lua_isnumber(L,7))
		{
			sys_err("wrong argument");
		}

		BYTE empire = lua_tonumber (L, 1);
		Date date = Date (lua_tonumber(L,2) - 1900, lua_tonumber(L,3) - 1, lua_tonumber(L,4));
		BYTE end_hour = lua_tonumber(L, 5);
		BYTE end_minite = lua_tonumber(L, 6);
		int exp_percent = lua_tonumber(L, 7);

		sys_log (0,"h %d m %d e %d", end_hour, end_minite, exp_percent);

		CSpeedServerManager::instance().SetHolidayExpTableOfEmpire (empire, date, HME (end_hour, end_minite, exp_percent));

		return 0;
	}

	int speedserver_init_holiday (lua_State* L)
	{
		if (!lua_isnumber(L,1) || !lua_isnumber(L,2) || !lua_isnumber(L,3) || !lua_isnumber(L,4))
		{
			sys_err("wrong argument");
		}

		BYTE empire = lua_tonumber (L, 1);
		Date date = Date (lua_tonumber(L,2) - 1900, lua_tonumber(L,3) - 1, lua_tonumber(L,4));
	
		CSpeedServerManager::instance().InitHolidayExpTableOfEmpire (empire, date);
		
		return 0;
	}

	int speedserver_get_current_exp_priv (lua_State* L)
	{
		if (!lua_isnumber (L, 1))
		{
			sys_err ("invalid empire");
			return 0;
		}
		BYTE empire = lua_tonumber(L, 1);

		int duration;
		bool is_change;

		HME hme = CSpeedServerManager::instance().GetCurrentExpPrivOfEmpire (empire, duration, is_change);

		lua_pushnumber (L, hme.hour);
		lua_pushnumber (L, hme.min);
		lua_pushnumber (L, hme.exp);
		lua_pushnumber (L, duration);
		lua_pushboolean (L, is_change);

		sys_log (0, "empire : %d is_change : %d",empire, is_change);

		return 5;
	}

	void RegisterSpeedServerFunctionTable()
	{
		luaL_reg speed_server_functions[] = 
		{
			{	"get_holiday",	speedserver_get_holiday	},
			{	"set_holiday",	speedserver_set_holiday	},
			{	"get_wday",	speedserver_get_wday	},
			{	"set_wday",	speedserver_set_wday	},
			{	"init_holiday", speedserver_init_holiday	},
			{	"init_wday", speedserver_init_wday	},
			{	"get_current_exp_priv", speedserver_get_current_exp_priv	},

			{ NULL, NULL}
		};

		CQuestManager::instance().AddLuaFunctionTable("speedserver", speed_server_functions);
	}
}

