#include "stdafx.h"
#include "questmanager.h"
#include "char.h"
#include "sectree_manager.h"
#include "target.h"

namespace quest 
{
	//
	// "target" Lua functions
	//
	int target_pos(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int iQuestIndex = CQuestManager::instance().GetCurrentPC()->GetCurrentQuestIndex();

		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2) || !lua_isnumber(L, 3))
		{
			sys_err("invalid argument, name: %s, quest_index %d", ch->GetName(), iQuestIndex);
			return 0;
		}

		PIXEL_POSITION pos;

		if (!SECTREE_MANAGER::instance().GetMapBasePositionByMapIndex(ch->GetMapIndex(), pos))
		{
			sys_err("cannot find base position in this map %d", ch->GetMapIndex());
			return 0;
		}

		int x = pos.x + (int) lua_tonumber(L, 2) * 100;
		int y = pos.y + (int) lua_tonumber(L, 3) * 100;

		CTargetManager::instance().CreateTarget(ch->GetPlayerID(),
				iQuestIndex,
				lua_tostring(L, 1),
				TARGET_TYPE_POS,
				x,
				y,
				(int) lua_tonumber(L, 4),
				lua_isstring(L, 5) ? lua_tostring(L, 5) : NULL,
				lua_isnumber(L, 6) ? (int)lua_tonumber(L, 6): 1);

		return 0;
	}

	int target_vid(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int iQuestIndex = CQuestManager::instance().GetCurrentPC()->GetCurrentQuestIndex();

		if (!lua_isstring(L, 1) || !lua_isnumber(L, 2))
		{
			sys_err("invalid argument, name: %s, quest_index %d", ch->GetName(), iQuestIndex);
			return 0;
		}


		CTargetManager::instance().CreateTarget(ch->GetPlayerID(),
				iQuestIndex,
				lua_tostring(L, 1),
				TARGET_TYPE_VID,
				(int) lua_tonumber(L, 2),
				0,
				ch->GetMapIndex(),
				lua_isstring(L, 3) ? lua_tostring(L, 3) : NULL,
				lua_isnumber(L, 4) ? (int)lua_tonumber(L, 4): 1);

		return 0;
	}

	// 현재 퀘스트에 등록된 타겟을 삭제 한다.
	int target_delete(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int iQuestIndex = CQuestManager::instance().GetCurrentPC()->GetCurrentQuestIndex();

		if (!lua_isstring(L, 1))
		{
			sys_err("invalid argument, name: %s, quest_index %d", ch->GetName(), iQuestIndex);
			return 0;
		}

		CTargetManager::instance().DeleteTarget(ch->GetPlayerID(), iQuestIndex, lua_tostring(L, 1));

		return 0;
	}

	// 현재 퀘스트 인덱스로 되어있는 타겟을 모두 삭제한다.
	int target_clear(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		int iQuestIndex = CQuestManager::instance().GetCurrentPC()->GetCurrentQuestIndex();

		CTargetManager::instance().DeleteTarget(ch->GetPlayerID(), iQuestIndex, NULL);

		return 0;
	}

	int target_id(lua_State* L)
	{
		LPCHARACTER ch = CQuestManager::instance().GetCurrentCharacterPtr();
		DWORD dwQuestIndex = CQuestManager::instance().GetCurrentPC()->GetCurrentQuestIndex();

		if (!lua_isstring(L, 1))
		{
			sys_err("invalid argument, name: %s, quest_index %u", ch->GetName(), dwQuestIndex);
			lua_pushnumber(L, 0);
			return 1;
		}

		LPEVENT pkEvent = CTargetManager::instance().GetTargetEvent(ch->GetPlayerID(), dwQuestIndex, (const char *) lua_tostring(L, 1));

		if (pkEvent)
		{
			TargetInfo * pInfo = dynamic_cast<TargetInfo *>(pkEvent->info);

			if ( pInfo == NULL )
			{
				sys_err( "target_id> <Factor> Null pointer" );
				lua_pushnumber(L, 0);
				return 1;
			}

			if (pInfo->iType == TARGET_TYPE_VID)
			{
				lua_pushnumber(L, pInfo->iArg1);
				return 1;
			}
		}

		lua_pushnumber(L, 0);
		return 1;
	}

	void RegisterTargetFunctionTable()
	{
		luaL_reg target_functions[] =
		{
			{ "pos",			target_pos		},
			{ "vid",			target_vid		},
			{ "npc",			target_vid		}, // TODO: delete this
			{ "pc",			target_vid		}, // TODO: delete this
			{ "delete",			target_delete		},
			{ "clear",			target_clear		},
			{ "id",			target_id		},
			{ NULL,			NULL			},
		};

		CQuestManager::instance().AddLuaFunctionTable("target", target_functions);
	}
};

