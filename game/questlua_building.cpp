#include "stdafx.h"
#include "config.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "char.h"
#include "guild.h"
#include "db.h"
#include "building.h"

namespace quest
{
	//
	// "building" Lua functions
	//
	int building_get_land_id(lua_State * L)
	{
		using namespace building;

		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument");
			lua_pushnumber(L, 0);
			return 1;
		}

		CLand * pkLand = CManager::instance().FindLand((int) lua_tonumber(L, 1), (int) lua_tonumber(L, 2), (int) lua_tonumber(L, 3));
		lua_pushnumber(L, pkLand ? pkLand->GetID() : 0);
		return 1;
	}

	int building_get_land_info(lua_State * L)
	{
		int price = 1000000000;
		int owner = 1000000000;
		int level_limit = 1000000000;

		if (lua_isnumber(L, 1))
		{
			using namespace building;

			CLand * pkLand = CManager::instance().FindLand((DWORD) lua_tonumber(L, 1));

			if (pkLand)
			{
				const TLand & t = pkLand->GetData();

				price = t.dwPrice;
				owner = t.dwGuildID;
				level_limit = t.bGuildLevelLimit;
			}
		}
		else
			sys_err("invalid argument");

		lua_pushnumber(L, price);
		lua_pushnumber(L, owner);
		lua_pushnumber(L, level_limit);
		return 3;
	}

	int building_set_land_owner(lua_State * L)
	{
		if (!lua_isnumber(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("invalid argument");
			return 0;
		}

		using namespace building;

		CLand * pkLand = CManager::instance().FindLand((DWORD) lua_tonumber(L, 1));

		if (pkLand)
		{
			if (pkLand->GetData().dwGuildID == 0)
				pkLand->SetOwner((DWORD) lua_tonumber(L, 2));
		}

		return 0;
	}

	int building_has_land(lua_State * L)
	{
		using namespace building;

		if (!lua_isnumber(L, 1))
		{
			sys_err("invalid argument");
			lua_pushboolean(L, true);
			return 1;
		}

		/*
		if (CManager::instance().FindLandByGuild((DWORD) lua_tonumber(L, 1)))
			lua_pushboolean(L, true);
		else
			lua_pushboolean(L, false);
		*/

		std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM land%s WHERE guild_id = %d", get_table_postfix(), (DWORD)lua_tonumber(L,1)));

		if ( pmsg->Get()->uiNumRows > 0 )
		{
			MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

			int	count = 0;
			str_to_number(count, row[0]);

			if (count == 0)
			{
				lua_pushboolean(L, false);
			}
			else
			{
				lua_pushboolean(L, true);
			}
		}
		else
		{
			lua_pushboolean(L, true);
		}

		return 1;
	}

	int building_reconstruct(lua_State* L)
	{
		using namespace building;

		DWORD dwNewBuilding = (DWORD)lua_tonumber(L, 1);

		CQuestManager& q = CQuestManager::instance();

		LPCHARACTER npc = q.GetCurrentNPCCharacterPtr();
		if (!npc)
			return 0;

		CGuild* pGuild = npc->GetGuild();
		if (!pGuild)
			return 0;

		CLand* pLand = CManager::instance().FindLandByGuild(pGuild->GetID());
		if (!pLand)
			return 0;

		LPOBJECT pObject = pLand->FindObjectByNPC(npc);
		if (!pObject)
			return 0;

		pObject->Reconstruct(dwNewBuilding);

		return 0;
	}

	void RegisterBuildingFunctionTable()
	{
		luaL_reg functions[] =
		{
			{ "get_land_id",	building_get_land_id	},
			{ "get_land_info",	building_get_land_info	},
			{ "set_land_owner",	building_set_land_owner	},
			{ "has_land",	building_has_land	},
			{ "reconstruct",	building_reconstruct	},
			{ NULL,		NULL			}
		};

		CQuestManager::instance().AddLuaFunctionTable("building", functions);
	}
};
