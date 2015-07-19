
#include "stdafx.h"

#include "BlueDragon_Binder.h"

#include "questmanager.h"

unsigned int BlueDragon_GetSkillFactor(const size_t cnt, ...)
{
	lua_State* L = quest::CQuestManager::instance().GetLuaState();

	const int stack_top = lua_gettop(L);

	lua_getglobal( L, "BlueDragonSetting" );

	if (false == lua_istable(L, -1))
	{
		lua_settop( L, stack_top );

		return 0;
	}

	va_list vl;

	va_start(vl, cnt);

	for( size_t i=0 ; i < cnt ; ++i )
	{
		const char* key = va_arg(vl, const char*);

		if (NULL == key)
		{
			va_end(vl);
			lua_settop( L, stack_top );
			sys_err("BlueDragon: wrong key list");
			return 0;
		}

		lua_pushstring( L, key );
		lua_gettable( L, -2 );

		if (false == lua_istable(L, -1) && i != cnt-1)
		{
			va_end(vl);
			lua_settop( L, stack_top );
			sys_err("BlueDragon: wrong key table %s", key);
			return 0;
		}
	}

	va_end(vl);

	if (false == lua_isnumber(L, -1))
	{
		lua_settop( L, stack_top );
		sys_err("BlueDragon: Last key is not a number");
		return 0;
	}

	int val = static_cast<int>(lua_tonumber( L, -1 ));

	lua_settop( L, stack_top );

	return val;
}

unsigned int BlueDragon_GetRangeFactor(const char* key, const int val)
{
	lua_State* L = quest::CQuestManager::instance().GetLuaState();

	const int stack_top = lua_gettop(L);

	lua_getglobal( L, "BlueDragonSetting" );

	if (false == lua_istable(L, -1))
	{
		lua_settop( L, stack_top );

		return 0;
	}

	lua_pushstring( L, key );
	lua_gettable( L, -2 );

	if (false == lua_istable(L, -1))
	{
		lua_settop( L, stack_top );

		sys_err("BlueDragon: no required table %s", key);
		return 0;
	}

	const size_t cnt = static_cast<size_t>(luaL_getn(L, -1));

	for( size_t i=1 ; i <= cnt ; ++i )
	{
		lua_rawgeti( L, -1, i );

		if (false == lua_istable(L, -1))
		{
			lua_settop( L, stack_top );

			sys_err("BlueDragon: wrong table index %s %d", key, i);
			return 0;
		}

		lua_pushstring( L, "min" );
		lua_gettable( L, -2 );

		if (false == lua_isnumber(L, -1))
		{
			lua_settop( L, stack_top );

			sys_err("BlueDragon: no min value set %s", key);
			return 0;
		}

		const int min = static_cast<int>(lua_tonumber(L, -1));

		lua_pop(L, 1);

		lua_pushstring( L, "max" );
		lua_gettable( L, -2 );

		if (false == lua_isnumber(L, -1))
		{
			lua_settop( L, stack_top );

			sys_err("BlueDragon: no max value set %s", key);
			return 0;
		}

		const int max = static_cast<int>(lua_tonumber(L, -1));

		lua_pop(L, 1);

		if (min <= val && val <= max)
		{
			lua_pushstring( L, "pct" );
			lua_gettable( L, -2 );

			if (false == lua_isnumber(L, -1))
			{
				lua_settop( L, stack_top );

				sys_err("BlueDragon: no pct value set %s", key);
				return 0;
			}

			const int pct = static_cast<int>(lua_tonumber(L, -1));

			lua_settop( L, stack_top );

			return pct;
		}

		lua_pop(L, 1);
	}

	lua_settop( L, stack_top );

	return 0;
}

unsigned int BlueDragon_GetIndexFactor(const char* container, const size_t idx, const char* key)
{
	lua_State* L = quest::CQuestManager::instance().GetLuaState();

	const int stack_top = lua_gettop(L);

	lua_getglobal( L, "BlueDragonSetting" );

	if (false == lua_istable(L, -1))
	{
		lua_settop( L, stack_top );

		return 0;
	}

	lua_pushstring( L, container );
	lua_gettable( L, -2 );

	if (false == lua_istable(L, -1))
	{
		lua_settop( L, stack_top );

		sys_err("BlueDragon: no required table %s", key);
		return 0;
	}

	lua_rawgeti( L, -1, idx );

	if (false == lua_istable(L, -1))
	{
		lua_settop( L, stack_top );

		sys_err("BlueDragon: wrong table index %s %d", key, idx);
		return 0;
	}

	lua_pushstring( L, key );
	lua_gettable( L, -2 );

	if (false == lua_isnumber(L, -1))
	{
		lua_settop( L, stack_top );

		sys_err("BlueDragon: no min value set %s", key);
		return 0;
	}

	const unsigned int ret = static_cast<unsigned int>(lua_tonumber(L, -1));

	lua_settop( L, stack_top );

	return ret;
}

