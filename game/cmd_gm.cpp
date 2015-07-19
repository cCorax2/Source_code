#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "sectree_manager.h"
#include "mob_manager.h"
#include "packet.h"
#include "cmd.h"
#include "regen.h"
#include "guild.h"
#include "guild_manager.h"
#include "p2p.h"
#include "buffer_manager.h"
#include "fishing.h"
#include "mining.h"
#include "questmanager.h"
#include "vector.h"
#include "affect.h"
#include "db.h"
#include "priv_manager.h"
#include "building.h"
#include "battle.h"
#include "arena.h"
#include "start_position.h"
#include "party.h"
#include "monarch.h"
#include "castle.h"
#include "BattleArena.h"
#include "xmas_event.h"
#include "log.h"
#include "pcbang.h"
#include "threeway_war.h"
#include "unique_item.h"
#include "DragonSoul.h"

extern bool DropEvent_RefineBox_SetValue(const std::string& name, int value);

// ADD_COMMAND_SLOW_STUN
enum
{
	COMMANDAFFECT_STUN,
	COMMANDAFFECT_SLOW,
};

void Command_ApplyAffect(LPCHARACTER ch, const char* argument, const char* affectName, int cmdAffect)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	sys_log(0, arg1);

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: %s <name>", affectName);
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s is not in same map", arg1);
		return;
	}

	switch (cmdAffect)
	{
		case COMMANDAFFECT_STUN:
			SkillAttackAffect(tch, 1000, IMMUNE_STUN, AFFECT_STUN, POINT_NONE, 0, AFF_STUN, 30, "GM_STUN");
			break;
		case COMMANDAFFECT_SLOW:
			SkillAttackAffect(tch, 1000, IMMUNE_SLOW, AFFECT_SLOW, POINT_MOV_SPEED, -30, AFF_SLOW, 30, "GM_SLOW");
			break;
	}

	sys_log(0, "%s %s", arg1, affectName);

	ch->ChatPacket(CHAT_TYPE_INFO, "%s %s", arg1, affectName);
}
// END_OF_ADD_COMMAND_SLOW_STUN

ACMD(do_pcbang_update)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	unsigned long PCBangID = 0;

	if (*arg1 == '\0')
		PCBangID = 0;
	else
		str_to_number(PCBangID, arg1);

	if (PCBangID == 0)
	{
		CPCBangManager::instance().RequestUpdateIPList(0);
		ch->ChatPacket(CHAT_TYPE_INFO, "PCBang Info Update For All");
	}
	else
	{
		CPCBangManager::instance().RequestUpdateIPList(PCBangID);
		ch->ChatPacket(CHAT_TYPE_INFO, "PCBang Info Update For %u", PCBangID);
	}

	TPacketPCBangUpdate packet;
	packet.bHeader = HEADER_GG_PCBANG_UPDATE;
	packet.ulPCBangID = PCBangID;

	P2P_MANAGER::instance().Send(&packet, sizeof(TPacketPCBangUpdate));

}

ACMD(do_pcbang_check)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (CPCBangManager::instance().IsPCBangIP(arg1) == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s is a PCBang IP", arg1);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s is not a PCBang IP", arg1);
	}
}

ACMD(do_stun)
{
	Command_ApplyAffect(ch, argument, "stun", COMMANDAFFECT_STUN);
}

ACMD(do_slow)
{
	Command_ApplyAffect(ch, argument, "slow", COMMANDAFFECT_SLOW);
}

ACMD(do_transfer)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: transfer <name>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1); 
	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Target is in %d channel (my channel %d)", pkCCI->bChannel, g_bChannel);
				return;
			}

			TPacketGGTransfer pgg;

			pgg.bHeader = HEADER_GG_TRANSFER;
			strlcpy(pgg.szName, arg1, sizeof(pgg.szName));
			pgg.lX = ch->GetX();
			pgg.lY = ch->GetY();

			P2P_MANAGER::instance().Send(&pgg, sizeof(TPacketGGTransfer));
			ch->ChatPacket(CHAT_TYPE_INFO, "Transfer requested.");
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no character(%s) by that name", arg1);
			sys_log(0, "There is no character(%s) by that name", arg1);
		}

		return;
	}

	if (ch == tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Transfer me?!?");
		return;
	}

	//tch->Show(ch->GetMapIndex(), ch->GetX(), ch->GetY(), ch->GetZ());
	tch->WarpSet(ch->GetX(), ch->GetY(), ch->GetMapIndex());
}

// LUA_ADD_GOTO_INFO
struct GotoInfo
{
	std::string 	st_name;

	BYTE 	empire;
	int 	mapIndex;
	DWORD 	x, y;

	GotoInfo()
	{
		st_name 	= "";
		empire 		= 0;
		mapIndex 	= 0;

		x = 0;
		y = 0;
	}
	GotoInfo(const GotoInfo& c_src)
	{
		__copy__(c_src);
	}
	void operator = (const GotoInfo& c_src)
	{
		__copy__(c_src);
	}
	void __copy__(const GotoInfo& c_src)
	{
		st_name 	= c_src.st_name;
		empire 		= c_src.empire;
		mapIndex 	= c_src.mapIndex;

		x = c_src.x;
		y = c_src.y;
	}
};

static std::vector<GotoInfo> gs_vec_gotoInfo;

void CHARACTER_AddGotoInfo(const std::string& c_st_name, BYTE empire, int mapIndex, DWORD x, DWORD y)
{
	GotoInfo newGotoInfo;
	newGotoInfo.st_name = c_st_name;
	newGotoInfo.empire = empire;
	newGotoInfo.mapIndex = mapIndex;
	newGotoInfo.x = x;
	newGotoInfo.y = y;
	gs_vec_gotoInfo.push_back(newGotoInfo);

	sys_log(0, "AddGotoInfo(name=%s, empire=%d, mapIndex=%d, pos=(%d, %d))", c_st_name.c_str(), empire, mapIndex, x, y);
}

bool FindInString(const char * c_pszFind, const char * c_pszIn)
{
	const char * c = c_pszIn;
	const char * p;

	p = strchr(c, '|');

	if (!p)
		return (0 == strncasecmp(c_pszFind, c_pszIn, strlen(c_pszFind)));
	else
	{
		char sz[64 + 1];

		do
		{
			strlcpy(sz, c, MIN(sizeof(sz), (p - c) + 1));

			if (!strncasecmp(c_pszFind, sz, strlen(c_pszFind)))
				return true;

			c = p + 1;
		} while ((p = strchr(c, '|')));

		strlcpy(sz, c, sizeof(sz));

		if (!strncasecmp(c_pszFind, sz, strlen(c_pszFind)))
			return true;
	}

	return false;
}

bool CHARACTER_GoToName(LPCHARACTER ch, BYTE empire, int mapIndex, const char* gotoName)
{
	std::vector<GotoInfo>::iterator i;
	for (i = gs_vec_gotoInfo.begin(); i != gs_vec_gotoInfo.end(); ++i)
	{
		const GotoInfo& c_eachGotoInfo = *i;

		if (mapIndex != 0)
		{
			if (mapIndex != c_eachGotoInfo.mapIndex)
				continue;
		}
		else if (!FindInString(gotoName, c_eachGotoInfo.st_name.c_str()))
			continue;

		if (c_eachGotoInfo.empire == 0 || c_eachGotoInfo.empire == empire)
		{
			int x = c_eachGotoInfo.x * 100;
			int y = c_eachGotoInfo.y * 100;

			ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
			ch->WarpSet(x, y);
			ch->Stop();
			return true;
		}
	}
	return false;
}

// END_OF_LUA_ADD_GOTO_INFO

/*
   = {
   { "A1|영안읍성",		0, 1,  4693, 9642 },
   { "A3|자양현",		0, 3,  3608, 8776 },

   { "B1|조안읍성",		0, 21,  557, 1579 },
   { "B3|복정현",		0, 23, 1385, 2349 },

   { "C1|평무읍성",		0, 41, 9696, 2784 },
   { "C3|박라현",		0, 43, 8731, 2426 },

// Snow
{ "Snow|서한산",		1, 61, 4342, 2906 },
{ "Snow|서한산",		2, 61, 3752, 1749 },
{ "Snow|서한산",		3, 61, 4918, 1736 },

// Flame
{ "Flame|도염화지|화염",	1, 62, 5994, 7563 },
{ "Flame|도염화지|화염",	2, 62, 5978, 6222 },
{ "Flame|도염화지|화염",	3, 62, 7307, 6898 },

// Desert
{ "Desert|영비사막|사막",	1, 63, 2178, 6272 },
{ "Desert|영비사막|사막",	2, 63, 2219, 5027 },
{ "Desert|영비사막|사막",	3, 63, 3440, 5025 },

// Threeway
{ "Three|승룡곡",		1, 64, 4021, 6739 },
{ "Three|승룡곡",		2, 64, 2704, 7399 },
{ "Three|승룡곡",		3, 64, 3213, 8080 },

// 밀교사원
{ "Milgyo|밀교사원",	1, 65, 5536, 1436 },
{ "Milgyo|밀교사원",	2, 65, 5536, 1436 },
{ "Milgyo|밀교사원",	3, 65, 5536, 1436 },

// 사귀타워입구
{ "사귀타워입구",		1, 65, 5905, 1108 },
{ "사귀타워입구",		2, 65, 5905, 1108 },
{ "사귀타워입구",		3, 65, 5905, 1108 },

{ NULL,			0,  0,    0,    0 },
};
 */


ACMD(do_goto)
{
	char arg1[256], arg2[256];
	int x = 0, y = 0, z = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 && !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: goto <x meter> <y meter>");
		return;
	}

	if (isnhdigit(*arg1) && isnhdigit(*arg2))
	{
		str_to_number(x, arg1);
		str_to_number(y, arg2);

		PIXEL_POSITION p;

		if (SECTREE_MANAGER::instance().GetMapBasePosition(ch->GetX(), ch->GetY(), p))
		{
			x += p.x / 100;
			y += p.y / 100;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "You goto ( %d, %d )", x, y);
	}
	else
	{
		int mapIndex = 0;
		BYTE empire = 0;

		if (*arg1 == '#')
			str_to_number(mapIndex,  (arg1 + 1));

		if (*arg2 && isnhdigit(*arg2))
		{
			str_to_number(empire, arg2);
			empire = MINMAX(1, empire, 3);
		}
		else
			empire = ch->GetEmpire();

		if (CHARACTER_GoToName(ch, empire, mapIndex, arg1))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "Cannot find map command syntax: /goto <mapname> [empire]");
			return;
		}

		return;

		/*
		   int iMapIndex = 0;
		   for (int i = 0; aWarpInfo[i].c_pszName != NULL; ++i)
		   {
		   if (iMapIndex != 0)
		   {
		   if (iMapIndex != aWarpInfo[i].iMapIndex)
		   continue;
		   }
		   else if (!FindInString(arg1, aWarpInfo[i].c_pszName))
		   continue;

		   if (aWarpInfo[i].bEmpire == 0 || aWarpInfo[i].bEmpire == bEmpire)
		   {
		   x = aWarpInfo[i].x * 100;
		   y = aWarpInfo[i].y * 100;

		   ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
		   ch->WarpSet(x, y);
		   ch->Stop();
		   return;
		   }
		   }
		 */

	}

	x *= 100;
	y *= 100;

	ch->Show(ch->GetMapIndex(), x, y, z);
	ch->Stop();
}

ACMD(do_warp)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: warp <character name> | <x meter> <y meter>");
		return;
	}

	int x = 0, y = 0;

	if (isnhdigit(*arg1) && isnhdigit(*arg2))
	{
		str_to_number(x, arg1);
		str_to_number(y, arg2);
	}
	else
	{
		LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

		if (NULL == tch)
		{
			const CCI* pkCCI = P2P_MANAGER::instance().Find(arg1);

			if (NULL != pkCCI)
			{
				if (pkCCI->bChannel != g_bChannel)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Target is in %d channel (my channel %d)", pkCCI->bChannel, g_bChannel);
					return;
				}

				ch->WarpToPID( pkCCI->dwPID );
			}
			else
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "There is no one by that name");
			}

			return;
		}
		else
		{
			x = tch->GetX() / 100;
			y = tch->GetY() / 100;
		}
	}

	x *= 100;
	y *= 100;

	ch->ChatPacket(CHAT_TYPE_INFO, "You warp to ( %d, %d )", x, y);
	ch->WarpSet(x, y);
	ch->Stop();
}

ACMD(do_item)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: item <item vnum>");
		return;
	}

	int iCount = 1;

	if (*arg2)
	{
		str_to_number(iCount, arg2);
		iCount = MINMAX(1, iCount, ITEM_MAX_COUNT);
	}

	DWORD dwVnum;

	if (isnhdigit(*arg1))
		str_to_number(dwVnum, arg1);
	else
	{
		if (!ITEM_MANAGER::instance().GetVnum(arg1, dwVnum))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%u item not exist by that vnum.", dwVnum);
			return;
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwVnum, iCount, 0, true);

	if (item)
	{
		if (item->IsDragonSoul())
		{
			int iEmptyPos = ch->GetEmptyDragonSoulInventory(item);

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
				LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
			}
			else
			{
				M2_DESTROY_ITEM(item);
				if (!ch->DragonSoul_IsQualified())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "인벤이 활성화 되지 않음.");
				}
				else
					ch->ChatPacket(CHAT_TYPE_INFO, "Not enough inventory space.");
			}
		}
		else
		{
			int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "Not enough inventory space.");
			}
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "#%u item not exist by that vnum.", dwVnum);
	}
}

ACMD(do_group_random)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: grrandom <group vnum>");
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);
	CHARACTER_MANAGER::instance().SpawnGroupGroup(dwVnum, ch->GetMapIndex(), ch->GetX() - 500, ch->GetY() - 500, ch->GetX() + 500, ch->GetY() + 500);
}

ACMD(do_group)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: group <group vnum>");
		return;
	}

	DWORD dwVnum = 0;
	str_to_number(dwVnum, arg1);

	if (test_server)
		sys_log(0, "COMMAND GROUP SPAWN %u at %u %u %u", dwVnum, ch->GetMapIndex(), ch->GetX(), ch->GetY());

	CHARACTER_MANAGER::instance().SpawnGroup(dwVnum, ch->GetMapIndex(), ch->GetX() - 500, ch->GetY() - 500, ch->GetX() + 500, ch->GetY() + 500);
}

ACMD(do_mob_coward)
{
	char	arg1[256], arg2[256];
	DWORD	vnum = 0;
	LPCHARACTER	tch;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mc <vnum>");
		return;
	}

	const CMob * pkMob;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	iCount = MIN(20, iCount);

	while (iCount--)
	{
		tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum, 
				ch->GetMapIndex(),
				ch->GetX() - number(200, 750), 
				ch->GetY() - number(200, 750), 
				ch->GetX() + number(200, 750), 
				ch->GetY() + number(200, 750), 
				true,
				pkMob->m_table.bType == CHAR_TYPE_STONE);
		if (tch)
			tch->SetCoward();
	}
}

ACMD(do_mob_map)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: mm <vnum>");
		return;
	}

	DWORD vnum = 0;
	str_to_number(vnum, arg1);
	LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRandomPosition(vnum, ch->GetMapIndex());

	if (tch)
		ch->ChatPacket(CHAT_TYPE_INFO, "%s spawned in %dx%d", tch->GetName(), tch->GetX(), tch->GetY());
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Spawn failed.");
}

ACMD(do_mob_aggresive)
{
	char	arg1[256], arg2[256];
	DWORD	vnum = 0;
	LPCHARACTER	tch;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob * pkMob;

	if (isdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	iCount = MIN(20, iCount);

	while (iCount--)
	{
		tch = CHARACTER_MANAGER::instance().SpawnMobRange(vnum, 
				ch->GetMapIndex(),
				ch->GetX() - number(200, 750), 
				ch->GetY() - number(200, 750), 
				ch->GetX() + number(200, 750), 
				ch->GetY() + number(200, 750), 
				true,
				pkMob->m_table.bType == CHAR_TYPE_STONE);
		if (tch)
			tch->SetAggressive();
	}
}

ACMD(do_mob)
{
	char	arg1[256], arg2[256];
	DWORD	vnum = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob* pkMob = NULL;

	if (isnhdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
		return;
	}

	int iCount = 0;

	if (*arg2)
		str_to_number(iCount, arg2);
	else
		iCount = 1;

	if (test_server)
		iCount = MIN(40, iCount);
	else
		iCount = MIN(20, iCount);

	while (iCount--)
	{
		CHARACTER_MANAGER::instance().SpawnMobRange(vnum, 
				ch->GetMapIndex(),
				ch->GetX() - number(200, 750), 
				ch->GetY() - number(200, 750), 
				ch->GetX() + number(200, 750), 
				ch->GetY() + number(200, 750), 
				true,
				pkMob->m_table.bType == CHAR_TYPE_STONE);
	}
}

ACMD(do_mob_ld)
{
	char	arg1[256], arg2[256], arg3[256], arg4[256];
	DWORD	vnum = 0;

	two_arguments(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: mob <mob vnum>");
		return;
	}

	const CMob* pkMob = NULL;

	if (isnhdigit(*arg1))
	{
		str_to_number(vnum, arg1);

		if ((pkMob = CMobManager::instance().Get(vnum)) == NULL)
			vnum = 0;
	}
	else
	{
		pkMob = CMobManager::Instance().Get(arg1, true);

		if (pkMob)
			vnum = pkMob->m_table.dwVnum;
	}

	if (vnum == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such mob by that vnum");
		return;
	}

	int dir = 1;
	long x,y;

	if (*arg2)
		str_to_number(x, arg2);
	if (*arg3)
		str_to_number(y, arg3);
	if (*arg4)
		str_to_number(dir, arg4);


	CHARACTER_MANAGER::instance().SpawnMob(vnum, 
		ch->GetMapIndex(),
		x*100, 
		y*100, 
		ch->GetZ(),
		pkMob->m_table.bType == CHAR_TYPE_STONE,
		dir);
}

struct FuncPurge
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncPurge(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY());

		if (!m_bAll && iDist >= 1000)	// 10미터 이상에 있는 것들은 purge 하지 않는다.
			return;

		sys_log(0, "PURGE: %s %d", pkChr->GetName(), iDist);

		if (pkChr->IsNPC() && !pkChr->IsPet() && pkChr->GetRider() == NULL)
		{
			M2_DESTROY_CHARACTER(pkChr);
		}
	}
};

ACMD(do_purge)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncPurge func(ch);

	if (*arg1 && !strcmp(arg1, "all"))
		func.m_bAll = true;

	LPSECTREE sectree = ch->GetSectree();
	if (sectree) // #431
		sectree->ForEachAround(func);
	else
		sys_err("PURGE_ERROR.NULL_SECTREE(mapIndex=%d, pos=(%d, %d)", ch->GetMapIndex(), ch->GetX(), ch->GetY());
}

ACMD(do_item_purge)
{
	int         i;
	LPITEM      item;

	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = ch->GetInventoryItem(i)))
		{
			ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
			ch->SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}   
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = ch->GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i ))))
		{
			ITEM_MANAGER::instance().RemoveItem(item, "PURGE");
		}
	}   
}

ACMD(do_state)
{
	char arg1[256];
	LPCHARACTER tch;

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		if (arg1[0] == '#')
		{
			tch = CHARACTER_MANAGER::instance().Find(strtoul(arg1+1, NULL, 10));
		}
		else
		{
			LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);

			if (!d)
				tch = NULL;
			else
				tch = d->GetCharacter();
		}
	}
	else
		tch = ch;

	if (!tch)
		return;

	char buf[256];

	snprintf(buf, sizeof(buf), "%s's State: ", tch->GetName());

	if (tch->IsPosition(POS_FIGHTING))
		strlcat(buf, "Battle", sizeof(buf));
	else if (tch->IsPosition(POS_DEAD))
		strlcat(buf, "Dead", sizeof(buf));
	else
		strlcat(buf, "Standing", sizeof(buf));

	if (ch->GetShop())
		strlcat(buf, ", Shop", sizeof(buf));

	if (ch->GetExchange())
		strlcat(buf, ", Exchange", sizeof(buf));

	ch->ChatPacket(CHAT_TYPE_INFO, "%s", buf);

	int len;
	len = snprintf(buf, sizeof(buf), "Coordinate %ldx%ld (%ldx%ld)", 
			tch->GetX(), tch->GetY(), tch->GetX() / 100, tch->GetY() / 100);

	if (len < 0 || len >= (int) sizeof(buf))
		len = sizeof(buf) - 1;

	LPSECTREE pSec = SECTREE_MANAGER::instance().Get(tch->GetMapIndex(), tch->GetX(), tch->GetY());

	if (pSec)
	{
		TMapSetting& map_setting = SECTREE_MANAGER::instance().GetMap(tch->GetMapIndex())->m_setting;
		snprintf(buf + len, sizeof(buf) - len, " MapIndex %ld Attribute %08X Local Position (%ld x %ld)", 
			tch->GetMapIndex(), pSec->GetAttribute(tch->GetX(), tch->GetY()), (tch->GetX() - map_setting.iBaseX)/100, (tch->GetY() - map_setting.iBaseY)/100);
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "%s", buf);

	ch->ChatPacket(CHAT_TYPE_INFO, "LEV %d", tch->GetLevel());
	ch->ChatPacket(CHAT_TYPE_INFO, "HP %d/%d", tch->GetHP(), tch->GetMaxHP());
	ch->ChatPacket(CHAT_TYPE_INFO, "SP %d/%d", tch->GetSP(), tch->GetMaxSP());
	ch->ChatPacket(CHAT_TYPE_INFO, "ATT %d MAGIC_ATT %d SPD %d CRIT %d%% PENE %d%% ATT_BONUS %d%%",
			tch->GetPoint(POINT_ATT_GRADE),
			tch->GetPoint(POINT_MAGIC_ATT_GRADE),
			tch->GetPoint(POINT_ATT_SPEED),
			tch->GetPoint(POINT_CRITICAL_PCT),
			tch->GetPoint(POINT_PENETRATE_PCT),
			tch->GetPoint(POINT_ATT_BONUS));
	ch->ChatPacket(CHAT_TYPE_INFO, "DEF %d MAGIC_DEF %d BLOCK %d%% DODGE %d%% DEF_BONUS %d%%", 
			tch->GetPoint(POINT_DEF_GRADE),
			tch->GetPoint(POINT_MAGIC_DEF_GRADE),
			tch->GetPoint(POINT_BLOCK),
			tch->GetPoint(POINT_DODGE),
			tch->GetPoint(POINT_DEF_BONUS));
	ch->ChatPacket(CHAT_TYPE_INFO, "RESISTANCES:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   WARR:%3d%% ASAS:%3d%% SURA:%3d%% SHAM:%3d%%",
			tch->GetPoint(POINT_RESIST_WARRIOR),
			tch->GetPoint(POINT_RESIST_ASSASSIN),
			tch->GetPoint(POINT_RESIST_SURA),
			tch->GetPoint(POINT_RESIST_SHAMAN));
	ch->ChatPacket(CHAT_TYPE_INFO, "   SWORD:%3d%% THSWORD:%3d%% DAGGER:%3d%% BELL:%3d%% FAN:%3d%% BOW:%3d%%",
			tch->GetPoint(POINT_RESIST_SWORD),
			tch->GetPoint(POINT_RESIST_TWOHAND),
			tch->GetPoint(POINT_RESIST_DAGGER),
			tch->GetPoint(POINT_RESIST_BELL),
			tch->GetPoint(POINT_RESIST_FAN),
			tch->GetPoint(POINT_RESIST_BOW));
	ch->ChatPacket(CHAT_TYPE_INFO, "   FIRE:%3d%% ELEC:%3d%% MAGIC:%3d%% WIND:%3d%% CRIT:%3d%% PENE:%3d%%",
			tch->GetPoint(POINT_RESIST_FIRE),
			tch->GetPoint(POINT_RESIST_ELEC),
			tch->GetPoint(POINT_RESIST_MAGIC),
			tch->GetPoint(POINT_RESIST_WIND),
			tch->GetPoint(POINT_RESIST_CRITICAL),
			tch->GetPoint(POINT_RESIST_PENETRATE));
	ch->ChatPacket(CHAT_TYPE_INFO, "   ICE:%3d%% EARTH:%3d%% DARK:%3d%%",
			tch->GetPoint(POINT_RESIST_ICE),
			tch->GetPoint(POINT_RESIST_EARTH),
			tch->GetPoint(POINT_RESIST_DARK));

	ch->ChatPacket(CHAT_TYPE_INFO, "MALL:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   ATT:%3d%% DEF:%3d%% EXP:%3d%% ITEMx%d GOLDx%d",
			tch->GetPoint(POINT_MALL_ATTBONUS),
			tch->GetPoint(POINT_MALL_DEFBONUS),
			tch->GetPoint(POINT_MALL_EXPBONUS),
			tch->GetPoint(POINT_MALL_ITEMBONUS) / 10,
			tch->GetPoint(POINT_MALL_GOLDBONUS) / 10);

	ch->ChatPacket(CHAT_TYPE_INFO, "BONUS:");
	ch->ChatPacket(CHAT_TYPE_INFO, "   SKILL:%3d%% NORMAL:%3d%% SKILL_DEF:%3d%% NORMAL_DEF:%3d%%",
			tch->GetPoint(POINT_SKILL_DAMAGE_BONUS),
			tch->GetPoint(POINT_NORMAL_HIT_DAMAGE_BONUS),
			tch->GetPoint(POINT_SKILL_DEFEND_BONUS),
			tch->GetPoint(POINT_NORMAL_HIT_DEFEND_BONUS));

	ch->ChatPacket(CHAT_TYPE_INFO, "   HUMAN:%3d%% ANIMAL:%3d%% ORC:%3d%% MILGYO:%3d%% UNDEAD:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_HUMAN),
			tch->GetPoint(POINT_ATTBONUS_ANIMAL),
			tch->GetPoint(POINT_ATTBONUS_ORC),
			tch->GetPoint(POINT_ATTBONUS_MILGYO),
			tch->GetPoint(POINT_ATTBONUS_UNDEAD));

	ch->ChatPacket(CHAT_TYPE_INFO, "   DEVIL:%3d%% INSECT:%3d%% FIRE:%3d%% ICE:%3d%% DESERT:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_DEVIL),
			tch->GetPoint(POINT_ATTBONUS_INSECT),
			tch->GetPoint(POINT_ATTBONUS_FIRE),
			tch->GetPoint(POINT_ATTBONUS_ICE),
			tch->GetPoint(POINT_ATTBONUS_DESERT));

	ch->ChatPacket(CHAT_TYPE_INFO, "   TREE:%3d%% MONSTER:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_TREE),
			tch->GetPoint(POINT_ATTBONUS_MONSTER));

	ch->ChatPacket(CHAT_TYPE_INFO, "   WARR:%3d%% ASAS:%3d%% SURA:%3d%% SHAM:%3d%%",
			tch->GetPoint(POINT_ATTBONUS_WARRIOR),
			tch->GetPoint(POINT_ATTBONUS_ASSASSIN),
			tch->GetPoint(POINT_ATTBONUS_SURA),
			tch->GetPoint(POINT_ATTBONUS_SHAMAN));

	for (int i = 0; i < MAX_PRIV_NUM; ++i)
		if (CPrivManager::instance().GetPriv(tch, i))
		{
			int iByEmpire = CPrivManager::instance().GetPrivByEmpire(tch->GetEmpire(), i);
			int iByGuild = 0;

			if (tch->GetGuild())
				iByGuild = CPrivManager::instance().GetPrivByGuild(tch->GetGuild()->GetID(), i);

			int iByPlayer = CPrivManager::instance().GetPrivByCharacter(tch->GetPlayerID(), i);

			if (iByEmpire)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for empire : %d", LC_TEXT(c_apszPrivNames[i]), iByEmpire);

			if (iByGuild)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for guild : %d", LC_TEXT(c_apszPrivNames[i]), iByGuild);

			if (iByPlayer)
				ch->ChatPacket(CHAT_TYPE_INFO, "%s for player : %d", LC_TEXT(c_apszPrivNames[i]), iByPlayer);
		}
}

struct notice_packet_func
{
	const char * m_str;

	notice_packet_func(const char * str) : m_str(str)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
	}
};

struct monarch_notice_packet_func
{
	const char * m_str;
	BYTE m_bEmpire;

	monarch_notice_packet_func(BYTE bEmpire, const char * str) : m_str(str), m_bEmpire(bEmpire)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (m_bEmpire == d->GetCharacter()->GetEmpire())
		{
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
		}
	}
};


void SendNotice(const char * c_pszBuf)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_packet_func(c_pszBuf));
}

void SendMonarchNotice(BYTE bEmpire, const char* c_pszBuf)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), monarch_notice_packet_func(bEmpire, c_pszBuf));
}

struct notice_map_packet_func
{
	const char* m_str;
	int m_mapIndex;
	bool m_bBigFont;

	notice_map_packet_func(const char* str, int idx, bool bBigFont) : m_str(str), m_mapIndex(idx), m_bBigFont(bBigFont)
	{
	}

	void operator() (LPDESC d)
	{
		if (d->GetCharacter() == NULL) return;
		if (d->GetCharacter()->GetMapIndex() != m_mapIndex) return;

		d->GetCharacter()->ChatPacket(m_bBigFont == true ? CHAT_TYPE_BIG_NOTICE : CHAT_TYPE_NOTICE, "%s", m_str);
	}
};

void SendNoticeMap(const char* c_pszBuf, int nMapIndex, bool bBigFont)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), notice_map_packet_func(c_pszBuf, nMapIndex, bBigFont));
}

struct log_packet_func
{
	const char * m_str;

	log_packet_func(const char * str) : m_str(str)
	{
	}

	void operator () (LPDESC d)
	{
		if (!d->GetCharacter())
			return;

		if (d->GetCharacter()->GetGMLevel() > GM_PLAYER)
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, "%s", m_str);
	}
};


void SendLog(const char * c_pszBuf)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	std::for_each(c_ref_set.begin(), c_ref_set.end(), log_packet_func(c_pszBuf));
}

void BroadcastNotice(const char * c_pszBuf)
{
	TPacketGGNotice p;
	p.bHeader = HEADER_GG_NOTICE;
	p.lSize = strlen(c_pszBuf) + 1;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(p));
	buf.write(c_pszBuf, p.lSize);

	P2P_MANAGER::instance().Send(buf.read_peek(), buf.size()); // HEADER_GG_NOTICE

	SendNotice(c_pszBuf);
}

void BroadcastMonarchNotice(BYTE bEmpire, const char * c_pszBuf)
{
	TPacketGGMonarchNotice p;
	p.bHeader = HEADER_GG_MONARCH_NOTICE;
	p.bEmpire = bEmpire;
	p.lSize = strlen(c_pszBuf) + 1;

	TEMP_BUFFER buf;
	buf.write(&p, sizeof(p));
	buf.write(c_pszBuf, p.lSize);

	P2P_MANAGER::instance().Send(buf.read_peek(), buf.size());

	SendMonarchNotice(bEmpire, c_pszBuf);
}

ACMD(do_notice)
{
	BroadcastNotice(argument);
}

ACMD(do_map_notice)
{
	SendNoticeMap(argument, ch->GetMapIndex(), false);
}

ACMD(do_big_notice)
{
	ch->ChatPacket(CHAT_TYPE_BIG_NOTICE, "%s", argument);
}

ACMD(do_monarch_notice)
{
	if (ch->IsMonarch() == true)
	{
		BroadcastMonarchNotice(ch->GetEmpire(), argument);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("군주만이 사용 가능한 기능입니다"));
	}
}

ACMD(do_who)
{
	int iTotal;
	int * paiEmpireUserCount;
	int iLocal;

	DESC_MANAGER::instance().GetUserCount(iTotal, &paiEmpireUserCount, iLocal);

	ch->ChatPacket(CHAT_TYPE_INFO, "Total [%d] %d / %d / %d (this server %d)", 
			iTotal, paiEmpireUserCount[1], paiEmpireUserCount[2], paiEmpireUserCount[3], iLocal);
}

class user_func
{
	public:
		LPCHARACTER	m_ch;
		static int count;
		static char str[128];
		static int str_len;

		user_func()
			: m_ch(NULL)
		{}

		void initialize(LPCHARACTER ch)
		{
			m_ch = ch;
			str_len = 0;
			count = 0;
			str[0] = '\0';
		}

		void operator () (LPDESC d)
		{
			if (!d->GetCharacter())
				return;

			int len = snprintf(str + str_len, sizeof(str) - str_len, "%-16s ", d->GetCharacter()->GetName());

			if (len < 0 || len >= (int) sizeof(str) - str_len)
				len = (sizeof(str) - str_len) - 1;

			str_len += len;
			++count;

			if (!(count % 4))
			{
				m_ch->ChatPacket(CHAT_TYPE_INFO, str);

				str[0] = '\0';
				str_len = 0;
			}
		}
};

int	user_func::count = 0;
char user_func::str[128] = { 0, };
int	user_func::str_len = 0;

ACMD(do_user)
{
	const DESC_MANAGER::DESC_SET & c_ref_set = DESC_MANAGER::instance().GetClientSet();
	user_func func;

	func.initialize(ch);
	std::for_each(c_ref_set.begin(), c_ref_set.end(), func);

	if (func.count % 4)
		ch->ChatPacket(CHAT_TYPE_INFO, func.str);

	ch->ChatPacket(CHAT_TYPE_INFO, "Total %d", func.count);
}

ACMD(do_disconnect)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "ex) /dc <player name>");
		return;
	}

	LPDESC d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER	tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s: no such a player.", arg1);
		return;
	}

	if (tch == ch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "cannot disconnect myself");
		return;
	}
	
	DESC_MANAGER::instance().DestroyDesc(d);
}

ACMD(do_kill)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "ex) /kill <player name>");
		return;
	}

	LPDESC	d = DESC_MANAGER::instance().FindByCharacterName(arg1);
	LPCHARACTER tch = d ? d->GetCharacter() : NULL;

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s: no such a player", arg1);
		return;
	}

	tch->Dead();
}

#define MISC    0
#define BINARY  1
#define NUMBER  2

const struct set_struct 
{
	const char *cmd;
	const char type;
} set_fields[] = {
	{ "gold",		NUMBER	},
	{ "race",		BINARY	},
	{ "sex",		BINARY	},
	{ "exp",		NUMBER	},
	{ "max_hp",		NUMBER	},
	{ "max_sp",		NUMBER	},
	{ "skill",		NUMBER	},
	{ "alignment",	NUMBER	},
	{ "align",		NUMBER	},
	{ "\n",		MISC	}
};

ACMD(do_set)
{
	char arg1[256], arg2[256], arg3[256];

	LPCHARACTER tch = NULL;

	int i, len;
	const char* line;

	line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(line, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: set <name> <field> <value>");
		return;
	}

	tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s not exist", arg1);
		return;
	}

	len = strlen(arg2);

	for (i = 0; *(set_fields[i].cmd) != '\n'; i++)
		if (!strncmp(arg2, set_fields[i].cmd, len))
			break;

	switch (i)
	{
		case 0:	// gold
			{
				int gold = 0;
				str_to_number(gold, arg3);
				DBManager::instance().SendMoneyLog(MONEY_LOG_MISC, 3, gold);
				int before_gold = tch->GetGold();
				tch->PointChange(POINT_GOLD, gold, true);
				int after_gold = tch->GetGold();
				if (0 == after_gold && 0 != before_gold)
				{
					LogManager::instance().CharLog(tch, gold, "ZERO_GOLD", "GM");
				}
			}
			break;

		case 1: // race
			break;

		case 2: // sex
			break;

		case 3: // exp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_EXP, amount, true);
			}
			break;

		case 4: // max_hp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_MAX_HP, amount, true);
			}
			break;

		case 5: // max_sp
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_MAX_SP, amount, true);
			}
			break;

		case 6: // active skill point
			{
				int amount = 0;
				str_to_number(amount, arg3);
				tch->PointChange(POINT_SKILL, amount, true);
			}
			break;

		case 7: // alignment
		case 8: // alignment
			{
				int	amount = 0;
				str_to_number(amount, arg3);
				tch->UpdateAlignment(amount - ch->GetRealAlignment());
			}
			break;
	}

	if (set_fields[i].type == NUMBER)
	{
		int	amount = 0;
		str_to_number(amount, arg3);
		ch->ChatPacket(CHAT_TYPE_INFO, "%s's %s set to [%d]", tch->GetName(), set_fields[i].cmd, amount);
	}
}

ACMD(do_reset)
{
	ch->PointChange(POINT_HP, ch->GetMaxHP() - ch->GetHP());
	ch->PointChange(POINT_SP, ch->GetMaxSP() - ch->GetSP());
	ch->Save();
}

ACMD(do_advance)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: advance <name> <level>");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s not exist", arg1);
		return;
	}

	int level = 0;
	str_to_number(level, arg2);

	tch->ResetPoint(MINMAX(0, level, PLAYER_MAX_LEVEL_CONST));
}

ACMD(do_respawn)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1 && !strcasecmp(arg1, "all"))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Respaw everywhere");
		regen_reset(0, 0);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Respaw around");
		regen_reset(ch->GetX(), ch->GetY());
	}
}

ACMD(do_safebox_size)
{

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int size = 0;

	if (*arg1)
		str_to_number(size, arg1);

	if (size > 3 || size < 0)
		size = 0;

	ch->ChatPacket(CHAT_TYPE_INFO, "Safebox size set to %d", size);
	ch->ChangeSafeboxSize(size);
}

ACMD(do_makeguild)
{
	if (ch->GetGuild())
		return;

	CGuildManager& gm = CGuildManager::instance();

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	TGuildCreateParameter cp;
	memset(&cp, 0, sizeof(cp));

	cp.master = ch;
	strlcpy(cp.name, arg1, sizeof(cp.name));

	if (!check_name(cp.name))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("적합하지 않은 길드 이름 입니다."));
		return;
	}

	gm.CreateGuild(cp);
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("(%s) 길드가 생성되었습니다. [임시]"), cp.name);
}

ACMD(do_deleteguild)
{
	if (ch->GetGuild())
		ch->GetGuild()->RequestDisband(ch->GetPlayerID());
}

ACMD(do_greset)
{
	if (ch->GetGuild())
		ch->GetGuild()->Reset();
}

// REFINE_ROD_HACK_BUG_FIX
ACMD(do_refine_rod)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
		fishing::RealRefineRod(ch, item);
}
// END_OF_REFINE_ROD_HACK_BUG_FIX

// REFINE_PICK
ACMD(do_refine_pick)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		mining::CHEAT_MAX_PICK(ch, item);
		mining::RealRefinePick(ch, item);
	}
}

ACMD(do_max_pick)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	BYTE cell = 0;
	str_to_number(cell, arg1);
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		mining::CHEAT_MAX_PICK(ch, item);
	}
}
// END_OF_REFINE_PICK


ACMD(do_fishing_simul)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];
	argument = one_argument(argument, arg1, sizeof(arg1));
	two_arguments(argument, arg2, sizeof(arg2), arg3, sizeof(arg3));

	int count = 1000;
	int prob_idx = 0;
	int level = 100;

	ch->ChatPacket(CHAT_TYPE_INFO, "Usage: fishing_simul <level> <prob index> <count>");

	if (*arg1)
		str_to_number(level, arg1);

	if (*arg2)
		str_to_number(prob_idx, arg2);

	if (*arg3)
		str_to_number(count, arg3);

	fishing::Simulation(level, count, prob_idx, ch);
}

ACMD(do_invisibility)
{
	if (ch->IsAffectFlag(AFF_INVISIBILITY))
	{
		ch->RemoveAffect(AFFECT_INVISIBILITY);
	}
	else
	{
		ch->AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, INFINITE_AFFECT_DURATION, 0, true);
	}
}

ACMD(do_event_flag)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!(*arg1) || !(*arg2))
		return;

	int value = 0;
	str_to_number(value, arg2);

	if (!strcmp(arg1, "mob_item") || 
			!strcmp(arg1, "mob_exp") ||
			!strcmp(arg1, "mob_gold") ||
			!strcmp(arg1, "mob_dam") ||
			!strcmp(arg1, "mob_gold_pct") ||
			!strcmp(arg1, "mob_item_buyer") || 
			!strcmp(arg1, "mob_exp_buyer") ||
			!strcmp(arg1, "mob_gold_buyer") ||
			!strcmp(arg1, "mob_gold_pct_buyer")
	   )
		value = MINMAX(0, value, 1000);

	//quest::CQuestManager::instance().SetEventFlag(arg1, atoi(arg2));
	quest::CQuestManager::instance().RequestSetEventFlag(arg1, value);
	ch->ChatPacket(CHAT_TYPE_INFO, "RequestSetEventFlag %s %d", arg1, value);
	sys_log(0, "RequestSetEventFlag %s %d", arg1, value);
}

ACMD(do_get_event_flag)
{
	quest::CQuestManager::instance().SendEventFlagList(ch);
}

ACMD(do_private)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: private <map index>");
		return;
	}

	long lMapIndex;
	long map_index = 0;
	str_to_number(map_index, arg1);
	if ((lMapIndex = SECTREE_MANAGER::instance().CreatePrivateMap(map_index)))
	{
		ch->SaveExitLocation();

		LPSECTREE_MAP pkSectreeMap = SECTREE_MANAGER::instance().GetMap(lMapIndex);
		ch->WarpSet(pkSectreeMap->m_setting.posSpawn.x, pkSectreeMap->m_setting.posSpawn.y, lMapIndex); 
	}
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Can't find map by index %d", map_index);
}

ACMD(do_qf)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	std::string questname = pPC->GetCurrentQuestName();

	if (!questname.empty())
	{
		int value = quest::CQuestManager::Instance().GetQuestStateIndex(questname, arg1);

		pPC->SetFlag(questname + ".__status", value);
		pPC->ClearTimer();

		quest::PC::QuestInfoIterator it = pPC->quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(questname);

		while (it!= pPC->quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = value;
				break;
			}

			++it;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag %s %s %d", questname.c_str(), arg1, value);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag failed");
	}
}

LPCHARACTER chHori, chForge, chLib, chTemple, chTraining, chTree, chPortal, chBall;

ACMD(do_b1)
{
	//호리병 478 579
	chHori = CHARACTER_MANAGER::instance().SpawnMobRange(14017, ch->GetMapIndex(), 304222, 742858, 304222, 742858, true, false);
	chHori->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_CONSTRUCTION_SMALL, 65535, 0, true);
	chHori->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);

	for (int i = 0; i < 30; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 800, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(number(701, 706), 
				ch->GetMapIndex(),
				304222 + (int)fx,
				742858 + (int)fy,
				304222 + (int)fx,
				742858 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}

	for (int i = 0; i < 5; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 800, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(8009, 
				ch->GetMapIndex(),
				304222 + (int)fx,
				742858 + (int)fy,
				304222 + (int)fx,
				742858 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}
}

ACMD(do_b2)
{
	chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
}

ACMD(do_b3)
{
	// 포지 492 547
	chForge = CHARACTER_MANAGER::instance().SpawnMobRange(14003, ch->GetMapIndex(), 307500, 746300, 307500, 746300, true, false);
	chForge->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//높은탑 509 589 -> 도서관
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14007, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//욕조 513 606 -> 힘의신전
	chTemple = CHARACTER_MANAGER::instance().SpawnMobRange(14004, ch->GetMapIndex(), 307700, 741600, 307700, 741600, true, false);
	chTemple->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//권투장 490 625
	chTraining= CHARACTER_MANAGER::instance().SpawnMobRange(14010, ch->GetMapIndex(), 307100, 739500, 307100, 739500, true, false);
	chTraining->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//나무 466 614
	chTree= CHARACTER_MANAGER::instance().SpawnMobRange(14013, ch->GetMapIndex(), 300800, 741600, 300800, 741600, true, false);
	chTree->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	//포탈 439 615
	chPortal= CHARACTER_MANAGER::instance().SpawnMobRange(14001, ch->GetMapIndex(), 300900, 744500, 300900, 744500, true, false);
	chPortal->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
	// 구슬 436 600
	chBall = CHARACTER_MANAGER::instance().SpawnMobRange(14012, ch->GetMapIndex(), 302500, 746600, 302500, 746600, true, false);
	chBall->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_b4)
{
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_UPGRADE, 65535, 0, true);

	for (int i = 0; i < 30; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 1200, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(number(701, 706), 
				ch->GetMapIndex(),
				307900 + (int)fx,
				744500 + (int)fy,
				307900 + (int)fx,
				744500 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}

	for (int i = 0; i < 5; ++i)
	{
		int rot = number(0, 359);
		float fx, fy;
		GetDeltaByDegree(rot, 1200, &fx, &fy);

		LPCHARACTER tch = CHARACTER_MANAGER::instance().SpawnMobRange(8009, 
				ch->GetMapIndex(),
				307900 + (int)fx,
				744500 + (int)fy,
				307900 + (int)fx,
				744500 + (int)fy,
				true,
				false);
		tch->SetAggressive();
	}

}

ACMD(do_b5)
{
	M2_DESTROY_CHARACTER(chLib);
	//chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14008, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_b6)
{
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_BUILDING_UPGRADE, 65535, 0, true);
}
ACMD(do_b7)
{
	M2_DESTROY_CHARACTER(chLib);
	//chHori->RemoveAffect(AFFECT_DUNGEON_UNIQUE);
	chLib = CHARACTER_MANAGER::instance().SpawnMobRange(14009, ch->GetMapIndex(), 307900, 744500, 307900, 744500, true, false);
	chLib->AddAffect(AFFECT_DUNGEON_UNIQUE, POINT_NONE, 0, AFF_DUNGEON_UNIQUE, 65535, 0, true);
}

ACMD(do_book)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	CSkillProto * pkProto;

	if (isnhdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pkProto = CSkillManager::instance().Get(vnum);
	}
	else 
		pkProto = CSkillManager::instance().Get(arg1);

	if (!pkProto)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such a skill.");
		return;
	}

	LPITEM item = ch->AutoGiveItem(50300);
	item->SetSocket(0, pkProto->dwVnum);
}

ACMD(do_setskillother)
{
	char arg1[256], arg2[256], arg3[256];
	argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	one_argument(argument, arg3, sizeof(arg3));

	if (!*arg1 || !*arg2 || !*arg3 || !isdigit(*arg3))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setskillother <target> <skillname> <lev>");
		return;
	}

	LPCHARACTER tch;

	tch = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	CSkillProto * pk;

	if (isdigit(*arg2))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg2);
		pk = CSkillManager::instance().Get(vnum);
	}
	else
		pk = CSkillManager::instance().Get(arg2);

	if (!pk)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such a skill by that name.");
		return;
	}

	BYTE level = 0;
	str_to_number(level, arg3);
	tch->SetSkillLevel(pk->dwVnum, level);
	tch->ComputePoints();
	tch->SkillLevelPacket();
}

ACMD(do_setskill)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2 || !isdigit(*arg2))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setskill <name> <lev>");
		return;
	}

	CSkillProto * pk;

	if (isdigit(*arg1))
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		pk = CSkillManager::instance().Get(vnum);
	}

	else
		pk = CSkillManager::instance().Get(arg1);

	if (!pk)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "No such a skill by that name.");
		return;
	}

	BYTE level = 0;
	str_to_number(level, arg2);
	ch->SetSkillLevel(pk->dwVnum, level);
	ch->ComputePoints();
	ch->SkillLevelPacket();
}

ACMD(do_set_skill_point)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int skill_point = 0;
	if (*arg1)
		str_to_number(skill_point, arg1);

	ch->SetRealPoint(POINT_SKILL, skill_point);
	ch->SetPoint(POINT_SKILL, ch->GetRealPoint(POINT_SKILL));
	ch->PointChange(POINT_SKILL, 0);
}

ACMD(do_set_skill_group)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int skill_group = 0;
	if (*arg1)
		str_to_number(skill_group, arg1);

	ch->SetSkillGroup(skill_group);
	
	ch->ClearSkill();
	ch->ChatPacket(CHAT_TYPE_INFO, "skill group to %d.", skill_group);
}

ACMD(do_reload)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		switch (LOWER(*arg1))
		{
			case 'u':
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading state_user_count.");
				LoadStateUserCount();
				break;

			case 'p':
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading prototype tables,");
				db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);
				break;

			case 's':
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading notice string.");
				DBManager::instance().LoadDBString();
				break;

			case 'q':
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading quest.");
				quest::CQuestManager::instance().Reload();
				break;

			case 'f':
				fishing::Initialize();
				break;

				//RELOAD_ADMIN
			case 'a':
				ch->ChatPacket(CHAT_TYPE_INFO, "Reloading Admin infomation.");
				db_clientdesc->DBPacket(HEADER_GD_RELOAD_ADMIN, 0, NULL, 0);
				sys_log(0, "Reloading admin infomation.");
				break;
				//END_RELOAD_ADMIN
			case 'c':	// cube
				// 로컬 프로세스만 갱산한다.
				Cube_init ();
				break;
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Reloading state_user_count.");
		LoadStateUserCount();

		ch->ChatPacket(CHAT_TYPE_INFO, "Reloading prototype tables,");
		db_clientdesc->DBPacket(HEADER_GD_RELOAD_PROTO, 0, NULL, 0);

		ch->ChatPacket(CHAT_TYPE_INFO, "Reloading notice string.");
		DBManager::instance().LoadDBString();    
	}
}

ACMD(do_cooltime)
{
	ch->DisableCooltime();
}

ACMD(do_level)
{
	char arg2[256];
	one_argument(argument, arg2, sizeof(arg2));

	if (!*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: level <level>");
		return;
	}

	int	level = 0;
	str_to_number(level, arg2);
	ch->ResetPoint(MINMAX(1, level, PLAYER_MAX_LEVEL_CONST));

	ch->ClearSkill();
	ch->ClearSubSkill();
}

ACMD(do_gwlist)
{
	ch->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("현재 전쟁중인 길드 입니다"));
	CGuildManager::instance().ShowGuildWarList(ch);
}

ACMD(do_stop_guild_war)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	int id1 = 0, id2 = 0;

	str_to_number(id1, arg1);
	str_to_number(id2, arg2);
	
	if (!id1 || !id2)
		return;

	if (id1 > id2)
	{
		std::swap(id1, id2);
	}

	ch->ChatPacket(CHAT_TYPE_TALKING, "%d %d", id1, id2);
	CGuildManager::instance().RequestEndWar(id1, id2);
}

ACMD(do_cancel_guild_war)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int id1 = 0, id2 = 0;
	str_to_number(id1, arg1);
	str_to_number(id2, arg2);

	if (id1 > id2)
		std::swap(id1, id2);

	CGuildManager::instance().RequestCancelWar(id1, id2);
}

ACMD(do_guild_state)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	CGuild* pGuild = CGuildManager::instance().FindGuildByName(arg1);
	if (pGuild != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "GuildID: %d", pGuild->GetID());
		ch->ChatPacket(CHAT_TYPE_INFO, "GuildMasterPID: %d", pGuild->GetMasterPID());
		ch->ChatPacket(CHAT_TYPE_INFO, "IsInWar: %d", pGuild->UnderAnyWar());
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%s: 존재하지 않는 길드 입니다."), arg1);
	}
}

struct FuncWeaken
{
	LPCHARACTER m_pkGM;
	bool	m_bAll;

	FuncWeaken(LPCHARACTER ch) : m_pkGM(ch), m_bAll(false)
	{
	}

	void operator () (LPENTITY ent)
	{
		if (!ent->IsType(ENTITY_CHARACTER))
			return;

		LPCHARACTER pkChr = (LPCHARACTER) ent;

		int iDist = DISTANCE_APPROX(pkChr->GetX() - m_pkGM->GetX(), pkChr->GetY() - m_pkGM->GetY());

		if (!m_bAll && iDist >= 1000)	// 10미터 이상에 있는 것들은 purge 하지 않는다.
			return;

		if (pkChr->IsNPC())
			pkChr->PointChange(POINT_HP, (10 - pkChr->GetHP()));
	}
};

ACMD(do_weaken)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	FuncWeaken func(ch);

	if (*arg1 && !strcmp(arg1, "all"))
		func.m_bAll = true;

	ch->GetSectree()->ForEachAround(func);
}

ACMD(do_getqf)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	LPCHARACTER tch;

	if (!*arg1)
		tch = ch;
	else
	{
		tch = CHARACTER_MANAGER::instance().FindPC(arg1);

		if (!tch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
			return;
		}
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
		pPC->SendFlagList(ch);
}

ACMD(do_set_state)
{
	char arg1[256];
	char arg2[256];

	//argument = one_argument(argument, arg1, sizeof(arg1));
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	std::string questname = arg1;
	std::string statename = arg2;

	if (!questname.empty())
	{
		int value = quest::CQuestManager::Instance().GetQuestStateIndex(questname, statename);

		pPC->SetFlag(questname + ".__status", value);
		pPC->ClearTimer();

		quest::PC::QuestInfoIterator it = pPC->quest_begin();
		unsigned int questindex = quest::CQuestManager::instance().GetQuestIndexByName(questname);

		while (it!= pPC->quest_end())
		{
			if (it->first == questindex)
			{
				it->second.st = value;
				break;
			}

			++it;
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag %s %s %d", questname.c_str(), arg1, value);
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "setting quest state flag failed");
	}
}

ACMD(do_setqf)
{
	char arg1[256];
	char arg2[256];
	char arg3[256];

	one_argument(two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: setqf <flagname> <value> [<character name>]");
		return;
	}

	LPCHARACTER tch = ch;

	if (*arg3)
		tch = CHARACTER_MANAGER::instance().FindPC(arg3);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
	{
		int value = 0;
		str_to_number(value, arg2);
		pPC->SetFlag(arg1, value);
		ch->ChatPacket(CHAT_TYPE_INFO, "Quest flag set: %s %d", arg1, value);
	}
}

ACMD(do_delqf)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: delqf <flagname> [<character name>]");
		return;
	}

	LPCHARACTER tch = ch;

	if (*arg2)
		tch = CHARACTER_MANAGER::instance().FindPC(arg2);

	if (!tch)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "There is no such character.");
		return;
	}

	quest::PC* pPC = quest::CQuestManager::instance().GetPC(tch->GetPlayerID());

	if (pPC)
	{
		if (pPC->DeleteFlag(arg1))
			ch->ChatPacket(CHAT_TYPE_INFO, "Delete success.");
		else
			ch->ChatPacket(CHAT_TYPE_INFO, "Delete failed. Quest flag does not exist.");
	}
}

ACMD(do_forgetme)
{
	ch->ForgetMyAttacker();
}

ACMD(do_aggregate)
{
	ch->AggregateMonster();
}

ACMD(do_attract_ranger)
{
	ch->AttractRanger();
}

ACMD(do_pull_monster)
{
	ch->PullMonster();
}

ACMD(do_polymorph)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);
		bool bMaintainStat = false;
		if (*arg2)
		{
			int value = 0;
			str_to_number(value, arg2);
			bMaintainStat = (value>0);
		}

		ch->SetPolymorph(dwVnum, bMaintainStat);
	}
}

ACMD(do_polymorph_item)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);

		LPITEM item = ITEM_MANAGER::instance().CreateItem(70104, 1, 0, true);
		if (item)
		{
			item->SetSocket(0, dwVnum);
			int iEmptyPos = ch->GetEmptyInventory(item->GetSize());

			if (iEmptyPos != -1)
			{
				item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));
				LogManager::instance().ItemLog(ch, item, "GM", item->GetName());
			}
			else
			{
				M2_DESTROY_ITEM(item);
				ch->ChatPacket(CHAT_TYPE_INFO, "Not enough inventory space.");
			}
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%d item not exist by that vnum.", 70103);
		}
		//ch->SetPolymorph(dwVnum, bMaintainStat);
	}
}

ACMD(do_priv_empire)
{
	char arg1[256] = {0};
	char arg2[256] = {0};
	char arg3[256] = {0};
	char arg4[256] = {0};
	int empire = 0;
	int type = 0;
	int value = 0;
	int duration = 0;

	const char* line = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
		goto USAGE;

	if (!line)
		goto USAGE;

	two_arguments(line, arg3, sizeof(arg3), arg4, sizeof(arg4));

	if (!*arg3 || !*arg4)
		goto USAGE;

	str_to_number(empire, arg1);
	str_to_number(type,	arg2);
	str_to_number(value,	arg3);
	value = MINMAX(0, value, 1000);
	str_to_number(duration, arg4);

	if (empire < 0 || 3 < empire)
		goto USAGE;

	if (type < 1 || 4 < type)
		goto USAGE;

	if (value < 0)
		goto USAGE;

	if (duration < 0)
		goto USAGE;

	// 시간 단위로 변경
	duration = duration * (60*60);

	sys_log(0, "_give_empire_privileage(empire=%d, type=%d, value=%d, duration=%d) by command", 
			empire, type, value, duration);
	CPrivManager::instance().RequestGiveEmpirePriv(empire, type, value, duration);
	return;

USAGE:
	ch->ChatPacket(CHAT_TYPE_INFO, "usage : priv_empire <empire> <type> <value> <duration>");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <empire>    0 - 3 (0==all)");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <type>      1:item_drop, 2:gold_drop, 3:gold10_drop, 4:exp");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <value>     percent");
	ch->ChatPacket(CHAT_TYPE_INFO, "  <duration>  hour");
}

/**
 * @version 05/06/08	Bang2ni - 길드 보너스 퀘스트 진행 안되는 문제 수정.(스크립트가 작성안됨.)
 * 			          quest/priv_guild.quest 로 부터 스크립트 읽어오게 수정됨
 */
ACMD(do_priv_guild)
{
	static const char msg[] = { '\0' };

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		CGuild * g = CGuildManager::instance().FindGuildByName(arg1);

		if (!g)
		{
			DWORD guild_id = 0;
			str_to_number(guild_id, arg1);
			g = CGuildManager::instance().FindGuild(guild_id);
		}

		if (!g)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("그런 이름 또는 번호의 길드가 없습니다."));
		else
		{
			char buf[1024+1];
			snprintf(buf, sizeof(buf), msg, g->GetID());

			using namespace quest;
			PC * pc = CQuestManager::instance().GetPC(ch->GetPlayerID());
			QuestState qs = CQuestManager::instance().OpenState("ADMIN_QUEST", QUEST_FISH_REFINE_STATE_INDEX);
			luaL_loadbuffer(qs.co, buf, strlen(buf), "ADMIN_QUEST");
			pc->SetQuest("ADMIN_QUEST", qs);

			QuestState & rqs = *pc->GetRunningQuestState();

			if (!CQuestManager::instance().RunState(rqs))
			{
				CQuestManager::instance().CloseState(rqs);
				pc->EndRunning();
				return;
			}
		}
	}
}

ACMD(do_mount_test)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (*arg1)
	{
		DWORD vnum = 0;
		str_to_number(vnum, arg1);
		ch->MountVnum(vnum);
	}
}

ACMD(do_observer)
{
	ch->SetObserverMode(!ch->IsObserverMode());
}

ACMD(do_socket_item)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1)
	{
		DWORD dwVnum = 0;
		str_to_number(dwVnum, arg1);
	
		int iSocketCount = 0;
		str_to_number(iSocketCount, arg2);
	
		if (!iSocketCount || iSocketCount >= ITEM_SOCKET_MAX_NUM)
			iSocketCount = 3;
	
		if (!dwVnum)
		{
			if (!ITEM_MANAGER::instance().GetVnum(arg1, dwVnum))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "#%d item not exist by that vnum.", dwVnum);
				return;
			}
		}

		LPITEM item = ch->AutoGiveItem(dwVnum);
	
		if (item)
		{
			for (int i = 0; i < iSocketCount; ++i)
				item->SetSocket(i, 1);
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "#%d cannot create item.", dwVnum);
		}
	}
}

ACMD(do_xmas)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int flag = 0;

	if (*arg1)
		str_to_number(flag, arg1);

	switch (subcmd)
	{
		case SCMD_XMAS_SNOW:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_snow", flag);
			break;

		case SCMD_XMAS_BOOM:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_boom", flag);
			break;

		case SCMD_XMAS_SANTA:
			quest::CQuestManager::instance().RequestSetEventFlag("xmas_santa", flag);
			break;
	}
}


// BLOCK_CHAT
ACMD(do_block_chat_list)
{
	// GM이 아니거나 block_chat_privilege가 없는 사람은 명령어 사용 불가
	if (!ch || (ch->GetGMLevel() < GM_HIGH_WIZARD && ch->GetQuestFlag("chat_privilege.block") <= 0))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("그런 명령어는 없습니다"));
		return;
	}

	DBManager::instance().ReturnQuery(QID_BLOCK_CHAT_LIST, ch->GetPlayerID(), NULL, 
			"SELECT p.name, a.lDuration FROM affect%s as a, player%s as p WHERE a.bType = %d AND a.dwPID = p.id",
			get_table_postfix(), get_table_postfix(), AFFECT_BLOCK_CHAT);
}

ACMD(do_vote_block_chat)
{
	return;

	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: vote_block_chat <name>");
		return;
	}

	const char* name = arg1;
	long lBlockDuration = 10;
	sys_log(0, "vote_block_chat %s %d", name, lBlockDuration);

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(name);

		if (pkCCI)
		{
			TPacketGGBlockChat p;

			p.bHeader = HEADER_GG_BLOCK_CHAT;
			strlcpy(p.szName, name, sizeof(p.szName));
			p.lBlockDuration = lBlockDuration;
			P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGBlockChat));
		}
		else
		{
			TPacketBlockChat p;

			strlcpy(p.szName, name, sizeof(p.szName));
			p.lDuration = lBlockDuration;
			db_clientdesc->DBPacket(HEADER_GD_BLOCK_CHAT, ch ? ch->GetDesc()->GetHandle() : 0, &p, sizeof(p));

		}

		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Chat block requested.");

		return;
	}

	if (tch && ch != tch)
		tch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, lBlockDuration, 0, true);
}

ACMD(do_block_chat)
{
	// GM이 아니거나 block_chat_privilege가 없는 사람은 명령어 사용 불가
	if (ch && (ch->GetGMLevel() < GM_HIGH_WIZARD && ch->GetQuestFlag("chat_privilege.block") <= 0))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("그런 명령어는 없습니다"));
		return;
	}

	char arg1[256];
	argument = one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Usage: block_chat <name> <time> (0 to off)");

		return;
	}

	const char* name = arg1;
	long lBlockDuration = parse_time_str(argument);

	if (lBlockDuration < 0)
	{
		if (ch)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, "잘못된 형식의 시간입니다. h, m, s를 붙여서 지정해 주십시오.");
			ch->ChatPacket(CHAT_TYPE_INFO, "예) 10s, 10m, 1m 30s");
		}
		return;
	}

	sys_log(0, "BLOCK CHAT %s %d", name, lBlockDuration);

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(name);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(name);

		if (pkCCI)
		{
			TPacketGGBlockChat p;

			p.bHeader = HEADER_GG_BLOCK_CHAT;
			strlcpy(p.szName, name, sizeof(p.szName));
			p.lBlockDuration = lBlockDuration;
			P2P_MANAGER::instance().Send(&p, sizeof(TPacketGGBlockChat));
		}
		else
		{
			TPacketBlockChat p;

			strlcpy(p.szName, name, sizeof(p.szName));
			p.lDuration = lBlockDuration;
			db_clientdesc->DBPacket(HEADER_GD_BLOCK_CHAT, ch ? ch->GetDesc()->GetHandle() : 0, &p, sizeof(p));
		}

		if (ch)
			ch->ChatPacket(CHAT_TYPE_INFO, "Chat block requested.");

		return;
	}

	if (tch && ch != tch)
		tch->AddAffect(AFFECT_BLOCK_CHAT, POINT_NONE, 0, AFF_NONE, lBlockDuration, 0, true);
}
// END_OF_BLOCK_CHAT

// BUILD_BUILDING
ACMD(do_build)
{
	using namespace building;

	char arg1[256], arg2[256], arg3[256], arg4[256];
	const char * line = one_argument(argument, arg1, sizeof(arg1));
	BYTE GMLevel = ch->GetGMLevel();

	CLand * pkLand = CManager::instance().FindLand(ch->GetMapIndex(), ch->GetX(), ch->GetY());

	// NOTE: 조건 체크들은 클라이언트와 서버가 함께 하기 때문에 문제가 있을 때는
	//       메세지를 전송하지 않고 에러를 출력한다.
	if (!pkLand)
	{
		sys_err("%s trying to build on not buildable area.", ch->GetName());
		return;
	}

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax: no command");
		return;
	}

	// 건설 권한 체크
	if (GMLevel == GM_PLAYER)
	{
		// 플레이어가 집을 지을 때는 땅이 내껀지 확인해야 한다.
		if ((!ch->GetGuild() || ch->GetGuild()->GetID() != pkLand->GetOwner()))
		{
			sys_err("%s trying to build on not owned land.", ch->GetName());
			return;
		}

		// 내가 길마인가?
		if (ch->GetGuild()->GetMasterPID() != ch->GetPlayerID())
		{
			sys_err("%s trying to build while not the guild master.", ch->GetName());
			return;
		}
	}

	switch (LOWER(*arg1))
	{
		case 'c':
			{
				// /build c vnum x y x_rot y_rot z_rot
				char arg5[256], arg6[256];
				line = one_argument(two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2)), arg3, sizeof(arg3)); // vnum x y
				one_argument(two_arguments(line, arg4, sizeof(arg4), arg5, sizeof(arg5)), arg6, sizeof(arg6)); // x_rot y_rot z_rot

				if (!*arg1 || !*arg2 || !*arg3 || !*arg4 || !*arg5 || !*arg6)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax");
					return;
				}

				DWORD dwVnum = 0;
				str_to_number(dwVnum,  arg1);

				using namespace building;

				const TObjectProto * t = CManager::instance().GetObjectProto(dwVnum);
				if (!t)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("존재하지 않는 건물입니다."));
					return;
				}

				const DWORD BUILDING_MAX_PRICE = 100000000;

				if (t->dwGroupVnum)
				{
					if (pkLand->FindObjectByGroup(t->dwGroupVnum))
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("같이 지을 수 없는 종류의 건물이 지어져 있습니다."));
						return;
					}
				}

				// 건물 종속성 체크 (이 건물이 지어져 있어야함)
				if (t->dwDependOnGroupVnum)
				{
					//		const TObjectProto * dependent = CManager::instance().GetObjectProto(dwVnum);
					//		if (dependent)
					{
						// 지어져있는가?
						if (!pkLand->FindObjectByGroup(t->dwDependOnGroupVnum))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("건설에 필요한 건물이 지어져 있지 않습니다."));
							return;
						}
					}
				}

				if (test_server || GMLevel == GM_PLAYER)
				{
					// GM이 아닐경우만 (테섭에서는 GM도 소모)
					// 건설 비용 체크
					if (t->dwPrice > BUILDING_MAX_PRICE)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("건물 비용 정보 이상으로 건설 작업에 실패했습니다."));
						return;
					}

					if (ch->GetGold() < (int)t->dwPrice)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("건설 비용이 부족합니다."));
						return;
					}

					// 아이템 자재 개수 체크

					int i;
					for (i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
					{
						DWORD dwItemVnum = t->kMaterials[i].dwItemVnum;
						DWORD dwItemCount = t->kMaterials[i].dwCount;

						if (dwItemVnum == 0)
							break;

						if ((int) dwItemCount > ch->CountSpecifyItem(dwItemVnum))
						{
							ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("자재가 부족하여 건설할 수 없습니다."));
							return;
						}
					}
				}

				float x_rot = atof(arg4);
				float y_rot = atof(arg5);
				float z_rot = atof(arg6);
				// 20050811.myevan.건물 회전 기능 봉인 해제
				/*
				   if (x_rot != 0.0f || y_rot != 0.0f || z_rot != 0.0f)
				   {
				   ch->ChatPacket(CHAT_TYPE_INFO, "건물 회전 기능은 아직 제공되지 않습니다");
				   return;
				   }
				 */

				long map_x = 0;
				str_to_number(map_x, arg2);
				long map_y = 0;
				str_to_number(map_y, arg3);

				bool isSuccess = pkLand->RequestCreateObject(dwVnum, 
						ch->GetMapIndex(),
						map_x,
						map_y,
						x_rot,
						y_rot,
						z_rot, true);

				if (!isSuccess)
				{
					if (test_server)
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("건물을 지을 수 없는 위치입니다."));
					return;
				}

				if (test_server || GMLevel == GM_PLAYER)
					// 건설 재료 소모하기 (테섭에서는 GM도 소모)
				{
					// 건설 비용 소모
					ch->PointChange(POINT_GOLD, -t->dwPrice);

					// 아이템 자재 사용하기 
					{
						int i;
						for (i = 0; i < OBJECT_MATERIAL_MAX_NUM; ++i)
						{
							DWORD dwItemVnum = t->kMaterials[i].dwItemVnum;
							DWORD dwItemCount = t->kMaterials[i].dwCount;

							if (dwItemVnum == 0)
								break;

							sys_log(0, "BUILD: material %d %u %u", i, dwItemVnum, dwItemCount);
							ch->RemoveSpecifyItem(dwItemVnum, dwItemCount);
						}
					}
				}
			}
			break;

		case 'd' :
			// build (d)elete ObjectID
			{
				one_argument(line, arg1, sizeof(arg1));

				if (!*arg1)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "Invalid syntax");
					return;
				}

				DWORD vid = 0;
				str_to_number(vid, arg1);
				pkLand->RequestDeleteObjectByVID(vid);
			}
			break;

			// BUILD_WALL	

			// build w n/e/w/s
		case 'w' :
			if (GMLevel > GM_PLAYER) 
			{
				int mapIndex = ch->GetMapIndex();

				one_argument(line, arg1, sizeof(arg1));
				
				sys_log(0, "guild.wall.build map[%d] direction[%s]", mapIndex, arg1);

				switch (arg1[0])
				{
					case 's':
						pkLand->RequestCreateWall(mapIndex,   0.0f);
						break;
					case 'n':
						pkLand->RequestCreateWall(mapIndex, 180.0f);
						break;
					case 'e':
						pkLand->RequestCreateWall(mapIndex,  90.0f);
						break;
					case 'w':
						pkLand->RequestCreateWall(mapIndex, 270.0f);
						break;
					default:
						ch->ChatPacket(CHAT_TYPE_INFO, "guild.wall.build unknown_direction[%s]", arg1);
						sys_err("guild.wall.build unknown_direction[%s]", arg1);
						break;
				}

			}
			break;

		case 'e':
			if (GMLevel > GM_PLAYER)
			{
				pkLand->RequestDeleteWall();
			}
			break;

		case 'W' :
			// 담장 세우기
			// build (w)all 담장번호 담장크기 대문동 대문서 대문남 대문북

			if (GMLevel >  GM_PLAYER) 
			{
				int setID = 0, wallSize = 0;
				char arg5[256], arg6[256];
				line = two_arguments(line, arg1, sizeof(arg1), arg2, sizeof(arg2));
				line = two_arguments(line, arg3, sizeof(arg3), arg4, sizeof(arg4));
				two_arguments(line, arg5, sizeof(arg5), arg6, sizeof(arg6));

				str_to_number(setID, arg1);
				str_to_number(wallSize, arg2);

				if (setID != 14105 && setID != 14115 && setID != 14125)
				{
					sys_log(0, "BUILD_WALL: wrong wall set id %d", setID);
					break;
				}
				else 
				{
					bool door_east = false;
					str_to_number(door_east, arg3);
					bool door_west = false;
					str_to_number(door_west, arg4);
					bool door_south = false;
					str_to_number(door_south, arg5);
					bool door_north = false;
					str_to_number(door_north, arg6);
					pkLand->RequestCreateWallBlocks(setID, ch->GetMapIndex(), wallSize, door_east, door_west, door_south, door_north);
				}
			}
			break;

		case 'E' :
			// 담장 지우기
			// build (e)rase 담장셋ID
			if (GMLevel > GM_PLAYER) 
			{
				one_argument(line, arg1, sizeof(arg1));
				DWORD id = 0;
				str_to_number(id, arg1);
				pkLand->RequestDeleteWallBlocks(id);
			}
			break;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, "Invalid command %s", arg1);
			break;
	}
}
// END_OF_BUILD_BUILDING

ACMD(do_clear_quest)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	quest::PC* pPC = quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID());
	pPC->ClearQuest(arg1);
}

ACMD(do_horse_state)
{
	ch->ChatPacket(CHAT_TYPE_INFO, "Horse Information:");
	ch->ChatPacket(CHAT_TYPE_INFO, "    Level  %d", ch->GetHorseLevel());
	ch->ChatPacket(CHAT_TYPE_INFO, "    Health %d/%d (%d%%)", ch->GetHorseHealth(), ch->GetHorseMaxHealth(), ch->GetHorseHealth() * 100 / ch->GetHorseMaxHealth());
	ch->ChatPacket(CHAT_TYPE_INFO, "    Stam   %d/%d (%d%%)", ch->GetHorseStamina(), ch->GetHorseMaxStamina(), ch->GetHorseStamina() * 100 / ch->GetHorseMaxStamina());
}

ACMD(do_horse_level)
{
	char arg1[256] = {0};
	char arg2[256] = {0};
	LPCHARACTER victim;
	int	level = 0;

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage : /horse_level <name> <level>");
		return;
	}

	victim = CHARACTER_MANAGER::instance().FindPC(arg1);

	if (NULL == victim)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("존재하지 않는 캐릭터 입니다."));
		return;
	}

	str_to_number(level, arg2);
	level = MINMAX(0, level, HORSE_MAX_LEVEL);

	ch->ChatPacket(CHAT_TYPE_INFO, "horse level set (%s: %d)", victim->GetName(), level);

	victim->SetHorseLevel(level);
	victim->ComputePoints();
	victim->SkillLevelPacket();
	return;

/*-----
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	int level = MINMAX(0, atoi(arg1), HORSE_MAX_LEVEL);

	ch->ChatPacket(CHAT_TYPE_INFO, "horse level set to %d.", level);
	ch->SetHorseLevel(level);
	ch->ComputePoints();
	ch->SkillLevelPacket();
	return;
-----*/
}

ACMD(do_horse_ride)
{
	if (ch->IsHorseRiding())
		ch->StopRiding(); 
	else
		ch->StartRiding();
}

ACMD(do_horse_summon)
{
	ch->HorseSummon(true, true);
}

ACMD(do_horse_unsummon)
{
	ch->HorseSummon(false, true);
}

ACMD(do_horse_set_stat)
{
	char arg1[256], arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (*arg1 && *arg2)
	{
		int hp = 0;
		str_to_number(hp, arg1);
		int stam = 0;
		str_to_number(stam, arg2);
		ch->UpdateHorseHealth(hp - ch->GetHorseHealth());
		ch->UpdateHorseStamina(stam - ch->GetHorseStamina());
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage : /horse_set_stat <hp> <stamina>");
	}
}

ACMD(do_save_attribute_to_image) // command "/saveati" for alias
{
	char szFileName[256];
	char szMapIndex[256];

	two_arguments(argument, szMapIndex, sizeof(szMapIndex), szFileName, sizeof(szFileName));

	if (!*szMapIndex || !*szFileName)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /saveati <map_index> <filename>");
		return;
	}

	long lMapIndex = 0;
	str_to_number(lMapIndex, szMapIndex);

	if (SECTREE_MANAGER::instance().SaveAttributeToImage(lMapIndex, szFileName))
		ch->ChatPacket(CHAT_TYPE_INFO, "Save done.");
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Save failed.");
}

ACMD(do_affect_remove)
{
	char arg1[256];
	char arg2[256];

	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (!*arg1 || !*arg2)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /affect_remove <player name>");
		ch->ChatPacket(CHAT_TYPE_INFO, "Syntax: /affect_remove <type> <point>");

		LPCHARACTER tch = ch;

		if (*arg1)
			if (!(tch = CHARACTER_MANAGER::instance().FindPC(arg1)))
				tch = ch;

		ch->ChatPacket(CHAT_TYPE_INFO, "-- Affect List of %s -------------------------------", tch->GetName());
		ch->ChatPacket(CHAT_TYPE_INFO, "Type Point Modif Duration Flag");

		const std::list<CAffect *> & cont = tch->GetAffectContainer();

		itertype(cont) it = cont.begin();

		while (it != cont.end())
		{
			CAffect * pkAff = *it++;

			ch->ChatPacket(CHAT_TYPE_INFO, "%4d %5d %5d %8d %u", 
					pkAff->dwType, pkAff->bApplyOn, pkAff->lApplyValue, pkAff->lDuration, pkAff->dwFlag);
		}
		return;
	}

	bool removed = false;

	CAffect * af;

	DWORD	type = 0;
	str_to_number(type, arg1);
	BYTE	point = 0;
	str_to_number(point, arg2);
	while ((af = ch->FindAffect(type, point)))
	{
		ch->RemoveAffect(af);
		removed = true;
	}

	if (removed)
		ch->ChatPacket(CHAT_TYPE_INFO, "Affect successfully removed.");
	else
		ch->ChatPacket(CHAT_TYPE_INFO, "Not affected by that type and point.");
}

ACMD(do_change_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->ChangeAttribute();
}

ACMD(do_add_attr)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddAttribute();
}

ACMD(do_add_socket)
{
	LPITEM weapon = ch->GetWear(WEAR_WEAPON);
	if (weapon)
		weapon->AddSocket();
}

ACMD(do_show_arena_list)
{
	CArenaManager::instance().SendArenaMapListTo(ch);
}

ACMD(do_end_all_duel)
{
	CArenaManager::instance().EndAllDuel();
}

ACMD(do_end_duel)
{
	char szName[256];

	one_argument(argument, szName, sizeof(szName));

	LPCHARACTER pChar = CHARACTER_MANAGER::instance().FindPC(szName);
	if (pChar == NULL)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("존재하지 않는 캐릭터 입니다."));
		return;
	}

	if (CArenaManager::instance().EndDuel(pChar->GetPlayerID()) == false)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 강제 종료 실패"));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 강제 종료 성공"));
	}
}

ACMD(do_duel)
{
	char szName1[256];
	char szName2[256];
	char szSet[256];
	char szMinute[256];
	int set = 0;
	int minute = 0;

	argument = two_arguments(argument, szName1, sizeof(szName1), szName2, sizeof(szName2));
	two_arguments(argument, szSet, sizeof(szSet), szMinute, sizeof(szMinute));

	str_to_number(set, szSet);
	
	if (set < 0) set = 1;
	if (set > 5) set = 5;

	if (!str_to_number(minute, szMinute))
	{
		// 캐나다는 기본 10분.
		if (LC_IsCanada() == true)
		{
			minute = 10;
		}
		else
		{
			minute = 5;
		}
	}
	if (minute < 5)
		minute = 5;

	LPCHARACTER pChar1 = CHARACTER_MANAGER::instance().FindPC(szName1);
	LPCHARACTER pChar2 = CHARACTER_MANAGER::instance().FindPC(szName2);

	if (pChar1 != NULL && pChar2 != NULL)
	{
		pChar1->RemoveGoodAffect();
		pChar2->RemoveGoodAffect();

		pChar1->RemoveBadAffect();
		pChar2->RemoveBadAffect();

		LPPARTY pParty = pChar1->GetParty();
		if (pParty != NULL)
		{
			if (pParty->GetMemberCount() == 2)
			{
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				pChar1->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티에서 나가셨습니다."));
				pParty->Quit(pChar1->GetPlayerID());
			}
		}
	
		pParty = pChar2->GetParty();
		if (pParty != NULL)
		{
			if (pParty->GetMemberCount() == 2)
			{
				CPartyManager::instance().DeleteParty(pParty);
			}
			else
			{
				pChar2->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티에서 나가셨습니다."));
				pParty->Quit(pChar2->GetPlayerID());
			}
		}
		
		if (CArenaManager::instance().StartDuel(pChar1, pChar2, set, minute) == true)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련이 성공적으로 시작 되었습니다."));
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련 시작에 문제가 있습니다."));
		}
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련자가 없습니다."));
	}
}

ACMD(do_stat_plus_amount)
{
	char szPoint[256];

	one_argument(argument, szPoint, sizeof(szPoint));

	if (*szPoint == '\0')
		return;

	if (ch->IsPolymorphed())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
		return;
	}

	int nRemainPoint = ch->GetPoint(POINT_STAT);

	if (nRemainPoint <= 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("남은 스탯 포인트가 없습니다."));
		return;
	}

	int nPoint = 0;
	str_to_number(nPoint, szPoint);

	if (nRemainPoint < nPoint)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("남은 스탯 포인트가 적습니다."));
		return;
	}

	if (nPoint < 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("값을 잘못 입력하였습니다."));
		return;
	}
	
	switch (subcmd)
	{
		case POINT_HT : // 체력
			if (nPoint + ch->GetPoint(POINT_HT) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_HT);
			}
			break;

		case POINT_IQ : // 지능
			if (nPoint + ch->GetPoint(POINT_IQ) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_IQ);
			}
			break;
			
		case POINT_ST : // 근력
			if (nPoint + ch->GetPoint(POINT_ST) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_ST);
			}
			break;
			
		case POINT_DX : // 민첩
			if (nPoint + ch->GetPoint(POINT_DX) > 90)
			{
				nPoint = 90 - ch->GetPoint(POINT_DX);
			}
			break;

		default :
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("명령어의 서브 커맨드가 잘못 되었습니다."));
			return;
			break;
	}

	if (nPoint != 0)
	{
		ch->SetRealPoint(subcmd, ch->GetRealPoint(subcmd) + nPoint);
		ch->SetPoint(subcmd, ch->GetPoint(subcmd) + nPoint);
		ch->ComputePoints();
		ch->PointChange(subcmd, 0);

		ch->PointChange(POINT_STAT, -nPoint);
		ch->ComputePoints();
	}
}

struct tTwoPID
{
	int pid1;
	int pid2;
};

ACMD(do_break_marriage)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	tTwoPID pids = { 0, 0 };

	str_to_number(pids.pid1, arg1);
	str_to_number(pids.pid2, arg2);
	
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("플레이어 %d 와 플레이어  %d를 파혼시킵니다.."), pids.pid1, pids.pid2);
	db_clientdesc->DBPacket(HEADER_GD_BREAK_MARRIAGE, 0, &pids, sizeof(pids));
}

ACMD(do_effect)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	int	effect_type = 0;
	str_to_number(effect_type, arg1);
	ch->EffectPacket(effect_type);
}


struct FCountInMap
{
	int m_Count[4];
	FCountInMap() { memset(m_Count, 0, sizeof(int) * 4); }
	void operator()(LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER ch = (LPCHARACTER) ent;
			if (ch && ch->IsPC())
				++m_Count[ch->GetEmpire()];
		}
	}
	int GetCount(BYTE bEmpire) { return m_Count[bEmpire]; } 
};

ACMD(do_threeway_war_info)
{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("각제국 진행 정보"));
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("선택 맵 정보 성지 %d 통로 %d %d %d"), GetSungziMapIndex(), GetPassMapIndex(1), GetPassMapIndex(2), GetPassMapIndex(3)); 
	ch->ChatPacket(CHAT_TYPE_INFO, "ThreewayPhase %d", CThreeWayWar::instance().GetRegenFlag());

	for (int n = 1; n < 4; ++n)
	{
		LPSECTREE_MAP pSecMap = SECTREE_MANAGER::instance().GetMap(GetSungziMapIndex());

		FCountInMap c;

		if (pSecMap)
		{
			pSecMap->for_each(c);
		}

		ch->ChatPacket(CHAT_TYPE_INFO, "%s killscore %d usercount %d",
				EMPIRE_NAME(n),
			   	CThreeWayWar::instance().GetKillScore(n),
				c.GetCount(n));
	}
}

ACMD(do_threeway_war_myinfo)
{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("나의 삼거리 진행정보"));
	ch->ChatPacket(CHAT_TYPE_INFO, "Deadcount %d",
			CThreeWayWar::instance().GetReviveTokenForPlayer(ch->GetPlayerID()));
}

ACMD(do_rmcandidacy)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: rmcandidacy <name>");
		return;
	}
		
	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1); 

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(arg1);

		if (pkCCI)
		{
			if (pkCCI->bChannel != g_bChannel)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "Target is in %d channel (my channel %d)", pkCCI->bChannel, g_bChannel);
				return;
			}
		}
	}

	db_clientdesc->DBPacket(HEADER_GD_RMCANDIDACY, 0, NULL, 32);
	db_clientdesc->Packet(arg1, 32); 
}

ACMD(do_setmonarch)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: setmonarch <name>");
		return;
	}

	db_clientdesc->DBPacket(HEADER_GD_SETMONARCH, 0, NULL, 32);
	db_clientdesc->Packet(arg1, 32); 
}

ACMD(do_rmmonarch)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: rmmonarch <name>");
		return;
	}

	db_clientdesc->DBPacket(HEADER_GD_RMMONARCH, 0, NULL, 32);
	db_clientdesc->Packet(arg1, 32); 
}

ACMD(do_check_monarch_money)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	int empire = 0;
	str_to_number(empire, arg1);
	int NationMoney = CMonarch::instance().GetMoney(empire);

	ch->ChatPacket(CHAT_TYPE_INFO, "국고: %d 원", NationMoney);
}

ACMD(do_reset_subskill)
{
	char arg1[256];

	one_argument(argument, arg1, sizeof(arg1));
	
	if (!*arg1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: reset_subskill <name>");
		return;
	}
	
	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(arg1);
	
	if (tch == NULL)
		return;

	tch->ClearSubSkill();
	ch->ChatPacket(CHAT_TYPE_INFO, "Subskill of [%s] was reset", tch->GetName());
}

ACMD(do_siege)
{
	char arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	int	empire = strtol(arg1, NULL, 10);
	int tower_count = strtol(arg2, NULL, 10);

	if (empire == 0) empire = number(1, 3);
	if (tower_count < 5 || tower_count > 10) tower_count = number(5, 10);

	TPacketGGSiege packet;
	packet.bHeader = HEADER_GG_SIEGE;
	packet.bEmpire = empire;
	packet.bTowerCount = tower_count;

	P2P_MANAGER::instance().Send(&packet, sizeof(TPacketGGSiege));

	switch (castle_siege(empire, tower_count))
	{
		case 0 :
			ch->ChatPacket(CHAT_TYPE_INFO, "SIEGE FAILED");
			break;
		case 1 :
			ch->ChatPacket(CHAT_TYPE_INFO, "SIEGE START Empire(%d) Tower(%d)", empire, tower_count);
			break;
		case 2 :
			ch->ChatPacket(CHAT_TYPE_INFO, "SIEGE END");
			break;
	}
}

ACMD(do_temp)
{
	if (false == test_server)
		return;

	char	arg1[256], arg2[256];
	two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

	if (0 == arg1[0] || 0 == arg2[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: empire money");
		return;
	}

	int	empire = 0;
	str_to_number(empire, arg1);
	int	money = 0;
	str_to_number(money, arg2);

	CMonarch::instance().SendtoDBAddMoney(money, empire, ch);
}

ACMD(do_frog)
{
	char	arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "Usage: empire(1, 2, 3)");
		return;
	}

	int	empire = 0;
	str_to_number(empire, arg1);

	switch (empire)
	{
		case 1:
		case 2:
		case 3:
			if (IS_CASTLE_MAP(ch->GetMapIndex()))
			{
				castle_spawn_frog(empire);
				castle_save();
			}
			else
				ch->ChatPacket(CHAT_TYPE_INFO, "You must spawn frog in castle");
			break;

		default:
			ch->ChatPacket(CHAT_TYPE_INFO, "Usage: empire(1, 2, 3)");
			break;
	}

}

ACMD(do_flush)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (0 == arg1[0])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "usage : /flush player_id");
		return;
	}

	DWORD pid = (DWORD) strtoul(arg1, NULL, 10);

	db_clientdesc->DBPacketHeader(HEADER_GD_FLUSH_CACHE, 0, sizeof(DWORD));
	db_clientdesc->Packet(&pid, sizeof(DWORD));
}

ACMD(do_eclipse)
{
	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (strtol(arg1, NULL, 10) == 1)
	{
		quest::CQuestManager::instance().RequestSetEventFlag("eclipse", 1);
	}
	else
	{
		quest::CQuestManager::instance().RequestSetEventFlag("eclipse", 0);
	}
}

ACMD(do_weeklyevent)
{
	char arg1[256];
	int empire = 0;
	
	if (CBattleArena::instance().IsRunning() == false)
	{
		one_argument(argument, arg1, sizeof(arg1));

		empire = strtol(arg1, NULL, 10);

		if (empire == 1 || empire == 2 || empire == 3)
		{
			CBattleArena::instance().Start(empire);
		}
		else
		{
			CBattleArena::instance().Start(rand()%3 + 1);
		}
		ch->ChatPacket(CHAT_TYPE_INFO, "Weekly Event Start");
	}
	else
	{
		CBattleArena::instance().ForceEnd();
		ch->ChatPacket(CHAT_TYPE_INFO, "Weekly Event End");
	}
}

ACMD(do_event_helper)
{
	char arg1[256];
	int mode = 0;

	one_argument(argument, arg1, sizeof(arg1));
	str_to_number(mode, arg1);

	if (mode == 1)
	{
		xmas::SpawnEventHelper(true);
		ch->ChatPacket(CHAT_TYPE_INFO, "Event Helper Spawn");
	}
	else
	{
		xmas::SpawnEventHelper(false);
		ch->ChatPacket(CHAT_TYPE_INFO, "Event Helper Delete");
	}
}

struct FMobCounter
{
	int nCount;

	void operator () (LPENTITY ent)
	{
		if (ent->IsType(ENTITY_CHARACTER))
		{
			LPCHARACTER pChar = static_cast<LPCHARACTER>(ent);

			if (pChar->IsMonster() == true || pChar->IsStone())
			{
				nCount++;
			}
		}
	}
};

ACMD(do_get_mob_count)
{
	LPSECTREE_MAP pSectree = SECTREE_MANAGER::instance().GetMap(ch->GetMapIndex());

	if (pSectree == NULL)
		return;

	FMobCounter f;
	f.nCount = 0;

	pSectree->for_each(f);

	ch->ChatPacket(CHAT_TYPE_INFO, "MapIndex: %d MobCount %d", ch->GetMapIndex(), f.nCount);
}

ACMD(do_clear_land)
{
	const building::CLand* pLand = building::CManager::instance().FindLand(ch->GetMapIndex(), ch->GetX(), ch->GetY());

	if( NULL == pLand )
	{
		return;
	}

	ch->ChatPacket(CHAT_TYPE_INFO, "Guild Land(%d) Cleared", pLand->GetID());

	building::CManager::instance().ClearLand(pLand->GetID());
}

ACMD(do_special_item)
{
    ITEM_MANAGER::instance().ConvSpecialDropItemFile();
}

ACMD(do_set_stat)
{
	char szName [256];
	char szChangeAmount[256];

	two_arguments (argument, szName, sizeof (szName), szChangeAmount, sizeof(szChangeAmount));

	if (*szName == NULL || *szChangeAmount == '\0')
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "Invalid argument.");
		return;
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(szName);

	if (!tch)
	{
		CCI * pkCCI = P2P_MANAGER::instance().Find(szName);

		if (pkCCI)
		{
			ch->ChatPacket (CHAT_TYPE_INFO, "Cannot find player(%s). %s is not in your game server.", szName, szName);
			return;
		}
		else
		{
			ch->ChatPacket (CHAT_TYPE_INFO, "Cannot find player(%s). Perhaps %s doesn't login or exist.", szName, szName);
			return;
		}
	}
	else
	{
		if (tch->IsPolymorphed())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("둔갑 중에는 능력을 올릴 수 없습니다."));
			return;
		}

		if (subcmd != POINT_HT && subcmd != POINT_IQ && subcmd != POINT_ST && subcmd != POINT_DX)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("명령어의 서브 커맨드가 잘못 되었습니다."));
			return;
		}
		int nRemainPoint = tch->GetPoint(POINT_STAT);
		int nCurPoint = tch->GetRealPoint(subcmd);
		int nChangeAmount = 0;
		str_to_number(nChangeAmount, szChangeAmount);
		int nPoint = nCurPoint + nChangeAmount;
		
		int n;
		switch (subcmd)
		{
		case POINT_HT:
			if (nPoint < JobInitialPoints[tch->GetJob()].ht)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 0;
			break;
		case POINT_IQ:
			if (nPoint < JobInitialPoints[tch->GetJob()].iq)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 1;
			break;
		case POINT_ST:
			if (nPoint < JobInitialPoints[tch->GetJob()].st)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 2;
			break;
		case POINT_DX:
			if (nPoint < JobInitialPoints[tch->GetJob()].dx)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Cannot set stat under initial stat."));
				return;
			}
			n = 3;
			break;
		}

		if (nPoint > 90)
		{
			nChangeAmount -= nPoint - 90;
			nPoint = 90;
		}

		if (nRemainPoint < nChangeAmount)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("남은 스탯 포인트가 적습니다."));
			return;
		}

		tch->SetRealPoint(subcmd, nPoint);
		tch->SetPoint(subcmd, tch->GetPoint(subcmd) + nChangeAmount);
		tch->ComputePoints();
		tch->PointChange(subcmd, 0);

		tch->PointChange(POINT_STAT, -nChangeAmount);
		tch->ComputePoints();

		const char* stat_name[4] = {"con", "int", "str", "dex"};

		ch->ChatPacket(CHAT_TYPE_INFO, "%s's %s change %d to %d", szName, stat_name[n], nCurPoint, nPoint);
	}
}

ACMD(do_get_item_id_list)
{
	for (int i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; i++)
	{
		LPITEM item = ch->GetInventoryItem(i);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "cell : %d, name : %s, id : %d", item->GetCell(), item->GetName(), item->GetID());
	}
}

ACMD(do_set_socket)
{
	char arg1 [256];
	char arg2 [256];
	char arg3 [256];

	one_argument (two_arguments (argument, arg1, sizeof (arg1), arg2, sizeof(arg2)), arg3, sizeof (arg3));
	
	int item_id, socket_num, value;
	if (!str_to_number (item_id, arg1) || !str_to_number (socket_num, arg2) || !str_to_number (value, arg3))
		return;
	
	LPITEM item = ITEM_MANAGER::instance().Find (item_id);
	if (item)
		item->SetSocket (socket_num, value);
}

ACMD (do_can_dead)
{
	if (subcmd)
		ch->SetArmada();
	else
		ch->ResetArmada();
}

ACMD (do_full_set)
{
	extern void do_all_skill_master(LPCHARACTER ch, const char *argument, int cmd, int subcmd);
	do_all_skill_master(ch, NULL, 0, 0);
	extern void do_item_full_set(LPCHARACTER ch, const char *argument, int cmd, int subcmd);
	do_item_full_set(ch, NULL, 0, 0);
	extern void do_attr_full_set(LPCHARACTER ch, const char *argument, int cmd, int subcmd);
	do_attr_full_set(ch, NULL, 0, 0);

}

ACMD (do_all_skill_master)
{
	ch->SetHorseLevel(SKILL_MAX_LEVEL);
	for (int i = 0; i < SKILL_MAX_NUM; i++)
	{
		if (true == ch->CanUseSkill(i))
		{
			ch->SetSkillLevel(i, SKILL_MAX_LEVEL);
		}
		else
		{
			switch(i)
			{
			case SKILL_HORSE_WILDATTACK:
			case SKILL_HORSE_CHARGE:
			case SKILL_HORSE_ESCAPE:
			case SKILL_HORSE_WILDATTACK_RANGE:
				ch->SetSkillLevel(i, SKILL_MAX_LEVEL);
				break;
			}
		}
	}
	ch->ComputePoints();
	ch->SkillLevelPacket();
}

ACMD (do_item_full_set)
{
	BYTE job = ch->GetJob();
	LPITEM item;
	for (int i = 0; i < 6; i++)
	{
		item = ch->GetWear(i);
		if (item != NULL)
			ch->UnequipItem(item);
	}
	item = ch->GetWear(WEAR_SHIELD);
	if (item != NULL)
		ch->UnequipItem(item);

	switch (job)
	{
	case JOB_SURA:
		{
			
			item = ITEM_MANAGER::instance().CreateItem(11699);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12529 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_WARRIOR:
		{
			
			item = ITEM_MANAGER::instance().CreateItem(11299);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(3159 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12249 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_SHAMAN:
		{
			
			item = ITEM_MANAGER::instance().CreateItem(11899);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(7159 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12669 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16209 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	case JOB_ASSASSIN:
		{
			
			item = ITEM_MANAGER::instance().CreateItem(11499);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(13049);
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(15189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(1139 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(12389 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(14109 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(17189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
			item = ITEM_MANAGER::instance().CreateItem(16189 );
			if (!item || !item->EquipTo(ch, item->FindEquipCell(ch)))
				M2_DESTROY_ITEM(item);
		}
		break;
	}
}

ACMD (do_attr_full_set)
{
	BYTE job = ch->GetJob();
	LPITEM item;

	switch (job)
	{
	case JOB_WARRIOR:
	case JOB_ASSASSIN:
	case JOB_SURA:
	case JOB_SHAMAN:
		{
			// 무사 몸빵 셋팅.
			// 이것만 나와 있어서 임시로 모든 직군 다 이런 속성 따름.
			item = ch->GetWear(WEAR_HEAD);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_ATT_SPEED, 8);
				item->SetForceAttribute( 1, APPLY_HP_REGEN, 30);
				item->SetForceAttribute( 2, APPLY_SP_REGEN, 30);
				item->SetForceAttribute( 3, APPLY_DODGE, 15);
				item->SetForceAttribute( 4, APPLY_STEAL_SP, 10);
			}

			item = ch->GetWear(WEAR_WEAPON);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_CAST_SPEED, 20);
				item->SetForceAttribute( 1, APPLY_CRITICAL_PCT, 10);
				item->SetForceAttribute( 2, APPLY_PENETRATE_PCT, 10);
				item->SetForceAttribute( 3, APPLY_ATTBONUS_DEVIL, 20);
				item->SetForceAttribute( 4, APPLY_STR, 12);
			}

			item = ch->GetWear(WEAR_SHIELD);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_CON, 12);
				item->SetForceAttribute( 1, APPLY_BLOCK, 15);
				item->SetForceAttribute( 2, APPLY_REFLECT_MELEE, 10);
				item->SetForceAttribute( 3, APPLY_IMMUNE_STUN, 1);
				item->SetForceAttribute( 4, APPLY_IMMUNE_SLOW, 1);
			}

			item = ch->GetWear(WEAR_BODY);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MAX_HP, 2000);
				item->SetForceAttribute( 1, APPLY_CAST_SPEED, 20);
				item->SetForceAttribute( 2, APPLY_STEAL_HP, 10);
				item->SetForceAttribute( 3, APPLY_REFLECT_MELEE, 10);
				item->SetForceAttribute( 4, APPLY_ATT_GRADE_BONUS, 50);
			}

			item = ch->GetWear(WEAR_FOOTS);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MAX_HP, 2000);
				item->SetForceAttribute( 1, APPLY_MAX_SP, 80);
				item->SetForceAttribute( 2, APPLY_MOV_SPEED, 8);
				item->SetForceAttribute( 3, APPLY_ATT_SPEED, 8);
				item->SetForceAttribute( 4, APPLY_CRITICAL_PCT, 10);
			}

			item = ch->GetWear(WEAR_WRIST);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MAX_HP, 2000);
				item->SetForceAttribute( 1, APPLY_MAX_SP, 80);
				item->SetForceAttribute( 2, APPLY_PENETRATE_PCT, 10);
				item->SetForceAttribute( 3, APPLY_STEAL_HP, 10);
				item->SetForceAttribute( 4, APPLY_MANA_BURN_PCT, 10);
			}
			item = ch->GetWear(WEAR_NECK);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MAX_HP, 2000);
				item->SetForceAttribute( 1, APPLY_MAX_SP, 80);
				item->SetForceAttribute( 2, APPLY_CRITICAL_PCT, 10);
				item->SetForceAttribute( 3, APPLY_PENETRATE_PCT, 10);
				item->SetForceAttribute( 4, APPLY_STEAL_SP, 10);
			}
			item = ch->GetWear(WEAR_EAR);
			if (item != NULL)
			{
				item->ClearAttribute();
				item->SetForceAttribute( 0, APPLY_MOV_SPEED, 20);
				item->SetForceAttribute( 1, APPLY_MANA_BURN_PCT, 10);
				item->SetForceAttribute( 2, APPLY_POISON_REDUCE, 5);
				item->SetForceAttribute( 3, APPLY_ATTBONUS_DEVIL, 20);
				item->SetForceAttribute( 4, APPLY_ATTBONUS_UNDEAD, 20);
			}
		}
		break;
	}
}

ACMD (do_use_item)
{
	char arg1 [256];

	one_argument (argument, arg1, sizeof (arg1));

	int cell;
	str_to_number(cell, arg1);
	
	LPITEM item = ch->GetInventoryItem(cell);
	if (item)
	{
		ch->UseItem(TItemPos (INVENTORY, cell));
	}
	else
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "아이템이 없어서 착용할 수 없어.");
	}
}

ACMD (do_clear_affect)
{
	ch->ClearAffect(true);
}

ACMD (do_dragon_soul)
{
	char arg1[512];
	const char* rest = one_argument (argument, arg1, sizeof(arg1));
	switch (arg1[0])
	{
	case 'a':
		{
			one_argument (rest, arg1, sizeof(arg1));
			int deck_idx;
			if (str_to_number(deck_idx, arg1) == false)
			{
				return;
			}
			ch->DragonSoul_ActivateDeck(deck_idx);
		}
		break;
	case 'd':
		{
			ch->DragonSoul_DeactivateAll();
		}
		break;
	}
}

ACMD (do_ds_list)
{
	for (int i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; i++)
	{
		TItemPos cell(DRAGON_SOUL_INVENTORY, i);
		
		LPITEM item = ch->GetItem(cell);
		if (item != NULL)
			ch->ChatPacket(CHAT_TYPE_INFO, "cell : %d, name : %s, id : %d", item->GetCell(), item->GetName(), item->GetID());
	}
}
