#include "stdafx.h"
#include "constants.h"
#include "questmanager.h"
#include "monarch.h"
#include "desc_client.h"

namespace quest
{
	int mgmt_monarch_state(lua_State* L)
	{
		if ( lua_isnumber(L, 1) == false )
		{
			return 0;
		}

		int idx = (int)lua_tonumber(L, 1);

		if ( idx < 1 || idx > 3 )
		{
			return 0;
		}

		TMonarchInfo* info = CMonarch::instance().GetMonarch();

		lua_pushstring(L, info->name[idx]);
		lua_pushnumber(L, info->pid[idx]);
		lua_pushstring(L, info->date[idx]);
		lua_pushnumber(L, info->money[idx]);

		return 4;
	}

	int mgmt_monarch_change_lord(lua_State* L)
	{
		if ( lua_isnumber(L, 1) == false || lua_isnumber(L, 2) == false )
		{
			return 0;
		}

		TPacketChangeMonarchLord info;
		info.bEmpire = (BYTE)lua_tonumber(L, 1);
		info.dwPID = (DWORD)lua_tonumber(L, 2);

		db_clientdesc->DBPacket(HEADER_GD_CHANGE_MONARCH_LORD, 0, &info, sizeof(info));

		return 0;
	}
	
	void RegisterMgmtFunctionTable()
	{
		luaL_reg mgmt_functions[] =
		{
			{	"monarch_state",		mgmt_monarch_state	},
			{	"monarch_change_lord",	mgmt_monarch_change_lord	},

			{	NULL,	NULL}
		};
		
		CQuestManager::instance().AddLuaFunctionTable("mgmt", mgmt_functions);
	}
}

