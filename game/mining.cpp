#include "stdafx.h"
#include "mining.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "item.h"
#include "config.h"
#include "db.h"
#include "log.h"
#include "skill.h"

namespace mining
{
	enum
	{
		MAX_ORE = 18,
		MAX_FRACTION_COUNT = 9,
		ORE_COUNT_FOR_REFINE = 100,
	};

	struct SInfo
	{
		DWORD dwLoadVnum;
		DWORD dwRawOreVnum;
		DWORD dwRefineVnum;
	};

	SInfo info[MAX_ORE] =
	{
		{ 20047, 50601, 50621 },
		{ 20048, 50602, 50622 },
		{ 20049, 50603, 50623 },
		{ 20050, 50604, 50624 },
		{ 20051, 50605, 50625 },
		{ 20052, 50606, 50626 },
		{ 20053, 50607, 50627 },
		{ 20054, 50608, 50628 },
		{ 20055, 50609, 50629 },
		{ 20056, 50610, 50630 },
		{ 20057, 50611, 50631 },
		{ 20058, 50612, 50632 },
		{ 20059, 50613, 50633 },
		{ 30301, 50614, 50634 },
		{ 30302, 50615, 50635 },
		{ 30303, 50616, 50636 },
		{ 30304, 50617, 50637 },
		{ 30305, 50618, 50638 },
	};

	int fraction_info[MAX_FRACTION_COUNT][3] =
	{
		{ 20,  1, 10 },
		{ 30, 11, 20 },
		{ 20, 21, 30 },
		{ 15, 31, 40 },
		{  5, 41, 50 },
		{  4, 51, 60 },
		{  3, 61, 70 },
		{  2, 71, 80 },
		{  1, 81, 90 },
	};

	int PickGradeAddPct[10] =
	{
		3, 5, 8, 11, 15, 20, 26, 32, 40, 50
	};

	int SkillLevelAddPct[SKILL_MAX_LEVEL + 1] =
	{
		0,
		1, 1, 1, 1,		//  1 - 4
		2, 2, 2, 2,		//  5 - 8
		3, 3, 3, 3,		//  9 - 12
		4, 4, 4, 4,		// 13 - 16
		5, 5, 5, 5,		// 17 - 20
		6, 6, 6, 6,		// 21 - 24
		7, 7, 7, 7,		// 25 - 28
		8, 8, 8, 8,		// 29 - 32
		9, 9, 9, 9,		// 33 - 36
		10, 10, 10, 	// 37 - 39
		11,				// 40
	};

	DWORD GetRawOreFromLoad(DWORD dwLoadVnum)
	{
		for (int i = 0; i < MAX_ORE; ++i)
		{
			if (info[i].dwLoadVnum == dwLoadVnum)
				return info[i].dwRawOreVnum;
		}
		return 0;
	}

	DWORD GetRefineFromRawOre(DWORD dwRawOreVnum)
	{
		for (int i = 0; i < MAX_ORE; ++i)
		{
			if (info[i].dwRawOreVnum == dwRawOreVnum)
				return info[i].dwRefineVnum;
		}
		return 0;
	}

	int GetFractionCount()
	{
		int r = number(1, 100);

		for (int i = 0; i < MAX_FRACTION_COUNT; ++i)
		{
			if (r <= fraction_info[i][0])
				return number(fraction_info[i][1], fraction_info[i][2]);
			else
				r -= fraction_info[i][0];
		}

		return 0; 
	}

	void OreDrop(LPCHARACTER ch, DWORD dwLoadVnum)
	{
		DWORD dwRawOreVnum = GetRawOreFromLoad(dwLoadVnum);

		int iFractionCount = GetFractionCount();

		if (iFractionCount == 0)
		{
			sys_err("Wrong ore fraction count");
			return;
		}

		LPITEM item = ITEM_MANAGER::instance().CreateItem(dwRawOreVnum, GetFractionCount());

		if (!item)
		{
			sys_err("cannot create item vnum %d", dwRawOreVnum);
			return;
		}

		PIXEL_POSITION pos;
		pos.x = ch->GetX() + number(-200, 200);
		pos.y = ch->GetY() + number(-200, 200);

		item->AddToGround(ch->GetMapIndex(), pos);
		item->StartDestroyEvent();
		item->SetOwnership(ch, 15);

		DBManager::instance().SendMoneyLog(MONEY_LOG_DROP, item->GetVnum(), item->GetCount());
	}

	int GetOrePct(LPCHARACTER ch)
	{
		int defaultPct = 20;
		int iSkillLevel = ch->GetSkillLevel(SKILL_MINING);

		LPITEM pick = ch->GetWear(WEAR_WEAPON);

		if (!pick || pick->GetType() != ITEM_PICK)
			return 0;

		return defaultPct + SkillLevelAddPct[MINMAX(0, iSkillLevel, 40)] + PickGradeAddPct[MINMAX(0, pick->GetRefineLevel(), 9)];
	}

	EVENTINFO(mining_event_info)
	{
		DWORD pid;
		DWORD vid_load;

		mining_event_info() 
		: pid( 0 )
		, vid_load( 0 )
		{
		}
	};

	// REFINE_PICK
	bool Pick_Check(CItem& item)
	{
		if (item.GetType() != ITEM_PICK)    
			return false;

		return true;
	}

	int Pick_GetMaxExp(CItem& pick)
	{
		return pick.GetValue(2);
	}

	int Pick_GetCurExp(CItem& pick)
	{
		return pick.GetSocket(0);
	}

	void Pick_IncCurExp(CItem& pick)
	{
		int cur = Pick_GetCurExp(pick);
		pick.SetSocket(0, cur + 1);
	}

	void Pick_MaxCurExp(CItem& pick)
	{
		int max = Pick_GetMaxExp(pick);
		pick.SetSocket(0, max);
	}

	bool Pick_Refinable(CItem& item)
	{
		if (Pick_GetCurExp(item) < Pick_GetMaxExp(item))
			return false;

		return true;
	}

	bool Pick_IsPracticeSuccess(CItem& pick)
	{
		return (number(1,pick.GetValue(1))==1);
	}
	
	bool Pick_IsRefineSuccess(CItem& pick)
	{
		return (number(1,100) <= pick.GetValue(3));
	}

	int RealRefinePick(LPCHARACTER ch, LPITEM item)
	{
		if (!ch || !item)
			return 2;

		LogManager& rkLogMgr = LogManager::instance();
		ITEM_MANAGER& rkItemMgr = ITEM_MANAGER::instance();

		if (!Pick_Check(*item))
		{
			sys_err("REFINE_PICK_HACK pid(%u) item(%s:%d) type(%d)", ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetType());
			rkLogMgr.RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), -1, 1, "PICK_HACK");
			return 2;
		}

		CItem& rkOldPick = *item;

		if (!Pick_Refinable(rkOldPick))
			return 2;

		int iAdv = rkOldPick.GetValue(0) / 10;

		if (rkOldPick.IsEquipped() == true)
			return 2;

		if (Pick_IsRefineSuccess(rkOldPick))
		{
			rkLogMgr.RefineLog(ch->GetPlayerID(), rkOldPick.GetName(), rkOldPick.GetID(), iAdv, 1, "PICK");

			LPITEM pkNewPick = ITEM_MANAGER::instance().CreateItem(rkOldPick.GetRefinedVnum(), 1);
			if (pkNewPick)
			{
				BYTE bCell = rkOldPick.GetCell();
				rkItemMgr.RemoveItem(item, "REMOVE (REFINE PICK)");
				pkNewPick->AddToCharacter(ch, TItemPos(INVENTORY, bCell));
				LogManager::instance().ItemLog(ch, pkNewPick, "REFINE PICK SUCCESS", pkNewPick->GetName());
				return 1;
			}

			return 2;
		}
		else
		{
			rkLogMgr.RefineLog(ch->GetPlayerID(), rkOldPick.GetName(), rkOldPick.GetID(), iAdv, 0, "PICK");

			LPITEM pkNewPick = ITEM_MANAGER::instance().CreateItem(rkOldPick.GetValue(4), 1);

			if (pkNewPick)
			{
				BYTE bCell = rkOldPick.GetCell();
				rkItemMgr.RemoveItem(item, "REMOVE (REFINE PICK)");
				pkNewPick->AddToCharacter(ch, TItemPos(INVENTORY, bCell));
				rkLogMgr.ItemLog(ch, pkNewPick, "REFINE PICK FAIL", pkNewPick->GetName());
				return 0;
			}

			return 2;
		}
	}

	void CHEAT_MAX_PICK(LPCHARACTER ch, LPITEM item)
	{
		if (!item)
			return;

		if (!Pick_Check(*item))
			return;

		CItem& pick = *item;
		Pick_MaxCurExp(pick);

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("곡괭이 수련도가 최대(%d)가 되었습니다."), Pick_GetCurExp(pick));
	}

	void PracticePick(LPCHARACTER ch, LPITEM item)
	{
		if (!item)
			return;

		if (!Pick_Check(*item))
			return;

		CItem& pick = *item;
		if (pick.GetRefinedVnum()<=0)
			return;

		if (Pick_IsPracticeSuccess(pick))
		{

			if (Pick_Refinable(pick))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("곡괭이가 최대 수련도에 도달하였습니다."));
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("나무꾼를 통해 다음 레벨의 곡괭이로 업그레이드 할 수 있습니다."));
			}
			else
			{
				Pick_IncCurExp(pick);	

				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("곡괭이의 수련도가 증가하였습니다! (%d/%d)"),
						Pick_GetCurExp(pick), Pick_GetMaxExp(pick));

				if (Pick_Refinable(pick))
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("곡괭이가 최대 수련도에 도달하였습니다."));
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("나무꾼를 통해 다음 레벨의 곡괭이로 업그레이드 할 수 있습니다."));
				}
			}
		}
	}
	// END_OF_REFINE_PICK

	EVENTFUNC(mining_event)
	{
		mining_event_info* info = dynamic_cast<mining_event_info*>( event->info );

		if ( info == NULL )
		{
			sys_err( "mining_event_info> <Factor> Null pointer" );
			return 0;
		}

		LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(info->pid);
		LPCHARACTER load = CHARACTER_MANAGER::instance().Find(info->vid_load);

		if (!ch)
			return 0;

		ch->mining_take();

		LPITEM pick = ch->GetWear(WEAR_WEAPON);

		// REFINE_PICK
		if (!pick || !Pick_Check(*pick))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("곡괭이를 들고 있지 않아서 캘 수 없습니다."));
			return 0;
		}
		// END_OF_REFINE_PICK

		if (!load)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("더이상 캐낼 수 없습니다."));
			return 0;
		}

		int iPct = GetOrePct(ch);

		if (number(1, 100) <= iPct)
		{
			OreDrop(ch, load->GetRaceNum());
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("채광에 성공하였습니다."));
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("채광에 실패하였습니다."));
		}

		PracticePick(ch, pick);

		return 0;
	}

	LPEVENT CreateMiningEvent(LPCHARACTER ch, LPCHARACTER load, int count)
	{
		mining_event_info* info = AllocEventInfo<mining_event_info>();
		info->pid = ch->GetPlayerID();
		info->vid_load = load->GetVID();

		return event_create(mining_event, info, PASSES_PER_SEC(2 * count));
	}

	bool OreRefine(LPCHARACTER ch, LPCHARACTER npc, LPITEM item, int cost, int pct, LPITEM metinstone_item)
	{
		if (!ch || !npc)
			return false;

		if (item->GetOwner() != ch)
		{
			sys_err("wrong owner");
			return false;
		}

		if (item->GetCount() < ORE_COUNT_FOR_REFINE)
		{
			sys_err("not enough count");
			return false;
		}

		DWORD dwRefinedVnum = GetRefineFromRawOre(item->GetVnum());

		if (dwRefinedVnum == 0)
			return false;

		ch->SetRefineNPC(npc);
		item->SetCount(item->GetCount() - ORE_COUNT_FOR_REFINE);
		int iCost = ch->ComputeRefineFee(cost, 1);

		if (ch->GetGold() < iCost)
			return false;

		ch->PayRefineFee(iCost);

		if (metinstone_item)
			ITEM_MANAGER::instance().RemoveItem(metinstone_item, "REMOVE (MELT)");

		if (number(1, 100) <= pct)
		{
			ch->AutoGiveItem(dwRefinedVnum, 1);
			return true;
		}

		return false;
	}

	bool IsVeinOfOre (DWORD vnum)
	{
		for (int i = 0; i < MAX_ORE; i++)
		{
			if (info[i].dwLoadVnum == vnum)
				return true;
		}
		return false;
	}
}

