#include "stdafx.h"

#include <stack>

#include "utils.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "desc.h"
#include "desc_client.h"
#include "desc_manager.h"
#include "packet.h"
#include "affect.h"
#include "skill.h"
#include "start_position.h"
#include "mob_manager.h"
#include "db.h"
#include "log.h"
#include "vector.h"
#include "buffer_manager.h"
#include "questmanager.h"
#include "fishing.h"
#include "party.h"
#include "dungeon.h"
#include "refine.h"
#include "unique_item.h"
#include "war_map.h"
#include "xmas_event.h"
#include "marriage.h"
#include "monarch.h"
#include "polymorph.h"
#include "blend_item.h"
#include "castle.h"
#include "BattleArena.h"
#include "arena.h"
#include "dev_log.h"
#include "pcbang.h"
#include "threeway_war.h"

#include "safebox.h"
#include "shop.h"

#include "../../common/VnumHelper.h"
#include "DragonSoul.h"
#include "buff_on_attributes.h"
#include "belt_inventory_helper.h"

//auction_temp
#ifdef __AUCTION__
#include "auction_manager.h"
#endif
const int ITEM_BROKEN_METIN_VNUM = 28960;

// CHANGE_ITEM_ATTRIBUTES
const DWORD CHARACTER::msc_dwDefaultChangeItemAttrCycle = 10;
const char CHARACTER::msc_szLastChangeItemAttrFlag[] = "Item.LastChangeItemAttr";
const char CHARACTER::msc_szChangeItemAttrCycleFlag[] = "change_itemattr_cycle";
// END_OF_CHANGE_ITEM_ATTRIBUTES
const BYTE g_aBuffOnAttrPoints[] = { POINT_ENERGY, POINT_COSTUME_ATTR_BONUS };

struct FFindStone
{
	std::map<DWORD, LPCHARACTER> m_mapStone;

	void operator()(LPENTITY pEnt)
	{
		if (pEnt->IsType(ENTITY_CHARACTER) == true)
		{
			LPCHARACTER pChar = (LPCHARACTER)pEnt;

			if (pChar->IsStone() == true)
			{
				m_mapStone[(DWORD)pChar->GetVID()] = pChar;
			}
		}
	}
};


//±ÍÈ¯ºÎ, ±ÍÈ¯±â¾ïºÎ, °áÈ¥¹ÝÁö
static bool IS_SUMMON_ITEM(int vnum)
{
	switch (vnum)
	{
		case 22000:
		case 22010:
		case 22011:
		case 22020:
		case ITEM_MARRIAGE_RING:
			return true;
	}

	return false;
}

static bool IS_MONKEY_DUNGEON(int map_index)
{
	switch (map_index)
	{
		case 5:
		case 25:
		case 45:
		case 108:
		case 109:
			return true;;
	}

	return false;
}

bool IS_SUMMONABLE_ZONE(int map_index)
{
	// ¸ùÅ°´øÀü
	if (IS_MONKEY_DUNGEON(map_index))
		return false;
	// ¼º
	if (IS_CASTLE_MAP(map_index))
		return false;

	switch (map_index)
	{
		case 66 : // »ç±ÍÅ¸¿ö
		case 71 : // °Å¹Ì ´øÀü 2Ãþ
		case 72 : // ÃµÀÇ µ¿±¼
		case 73 : // ÃµÀÇ µ¿±¼ 2Ãþ
		case 193 : // °Å¹Ì ´øÀü 2-1Ãþ
#if 0
		case 184 : // ÃµÀÇ µ¿±¼(½Å¼ö)
		case 185 : // ÃµÀÇ µ¿±¼ 2Ãþ(½Å¼ö)
		case 186 : // ÃµÀÇ µ¿±¼(ÃµÁ¶)
		case 187 : // ÃµÀÇ µ¿±¼ 2Ãþ(ÃµÁ¶)
		case 188 : // ÃµÀÇ µ¿±¼(Áø³ë)
		case 189 : // ÃµÀÇ µ¿±¼ 2Ãþ(Áø³ë)
#endif
//		case 206 : // ¾Æ±Íµ¿±¼
		case 216 : // ¾Æ±Íµ¿±¼
		case 217 : // °Å¹Ì ´øÀü 3Ãþ
		case 208 : // ÃµÀÇ µ¿±¼ (¿ë¹æ)
			return false;
	}

	if (CBattleArena::IsBattleArenaMap(map_index)) return false;

	// ¸ðµç private ¸ÊÀ¸·Ð ¿öÇÁ ºÒ°¡´É
	if (map_index > 10000) return false;

	return true;
}

bool IS_BOTARYABLE_ZONE(int nMapIndex)
{
	if (LC_IsYMIR() == false && LC_IsKorea() == false) return true;

	switch (nMapIndex)
	{
		case 1 :
		case 3 :
		case 21 :
		case 23 :
		case 41 :
		case 43 :
			return true;
	}
	
	return false;
}

// item socket ÀÌ ÇÁ·ÎÅäÅ¸ÀÔ°ú °°ÀºÁö Ã¼Å© -- by mhh
static bool FN_check_item_socket(LPITEM item)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		if (item->GetSocket(i) != item->GetProto()->alSockets[i])
			return false;
	}

	return true;
}

// item socket º¹»ç -- by mhh
static void FN_copy_item_socket(LPITEM dest, LPITEM src)
{
	for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
	{
		dest->SetSocket(i, src->GetSocket(i));
	}
}
static bool FN_check_item_sex(LPCHARACTER ch, LPITEM item)
{
	// ³²ÀÚ ±ÝÁö
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_MALE))
	{
		if (SEX_MALE==GET_SEX(ch))
			return false;
	}
	// ¿©ÀÚ±ÝÁö
	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_FEMALE)) 
	{
		if (SEX_FEMALE==GET_SEX(ch))
			return false;
	}

	return true;
}


/////////////////////////////////////////////////////////////////////////////
// ITEM HANDLING
/////////////////////////////////////////////////////////////////////////////
bool CHARACTER::CanHandleItem(bool bSkipCheckRefine, bool bSkipObserver)
{
	if (!bSkipObserver)
		if (m_bIsObserver)
			return false;

	if (GetMyShop())
		return false;

	if (!bSkipCheckRefine)
		if (m_bUnderRefine)
			return false;

	if (IsCubeOpen() || NULL != DragonSoul_RefineWindow_GetOpener())
		return false;

	if (IsWarping())
		return false;

	return true;
}

LPITEM CHARACTER::GetInventoryItem(WORD wCell) const
{
	return GetItem(TItemPos(INVENTORY, wCell));
}
LPITEM CHARACTER::GetItem(TItemPos Cell) const
{
	if (!IsValidItemPosition(Cell))
		return NULL;
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	switch (window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pItems[wCell];
	case DRAGON_SOUL_INVENTORY:
		if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
		{
			sys_err("CHARACTER::GetInventoryItem: invalid DS item cell %d", wCell);
			return NULL;
		}
		return m_pointsInstant.pDSItems[wCell];

	default:
		return NULL;
	}
	return NULL;
}

void CHARACTER::SetItem(TItemPos Cell, LPITEM pItem)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	if ((unsigned long)((CItem*)pItem) == 0xff || (unsigned long)((CItem*)pItem) == 0xffffffff)
	{
		sys_err("!!! FATAL ERROR !!! item == 0xff (char: %s cell: %u)", GetName(), wCell);
		core_dump();
		return;
	}

	if (pItem && pItem->GetOwner())
	{
		assert(!"GetOwner exist");
		return;
	}
	// ±âº» ÀÎº¥Åä¸®
	switch(window_type)
	{
	case INVENTORY:
	case EQUIPMENT:
		{
			if (wCell >= INVENTORY_AND_EQUIP_SLOT_MAX)
			{
				sys_err("CHARACTER::SetItem: invalid item cell %d", wCell);
				return;
			}

			LPITEM pOld = m_pointsInstant.pItems[wCell];

			if (pOld)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pItems[p] && m_pointsInstant.pItems[p] != pOld)
							continue;

						m_pointsInstant.bItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.bItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell < INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * 5);

						if (p >= INVENTORY_MAX_NUM)
							continue;

						// wCell + 1 ·Î ÇÏ´Â °ÍÀº ºó°÷À» Ã¼Å©ÇÒ ¶§ °°Àº
						// ¾ÆÀÌÅÛÀº ¿¹¿ÜÃ³¸®ÇÏ±â À§ÇÔ
						m_pointsInstant.bItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.bItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pItems[wCell] = pItem;
		}
		break;
	// ¿ëÈ¥¼® ÀÎº¥Åä¸®
	case DRAGON_SOUL_INVENTORY:
		{
			LPITEM pOld = m_pointsInstant.pDSItems[wCell];

			if (pOld)
			{
				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pOld->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						if (m_pointsInstant.pDSItems[p] && m_pointsInstant.pDSItems[p] != pOld)
							continue;

						m_pointsInstant.wDSItemGrid[p] = 0;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = 0;
			}

			if (pItem)
			{
				if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					sys_err("CHARACTER::SetItem: invalid DS item cell %d", wCell);
					return;
				}

				if (wCell < DRAGON_SOUL_INVENTORY_MAX_NUM)
				{
					for (int i = 0; i < pItem->GetSize(); ++i)
					{
						int p = wCell + (i * DRAGON_SOUL_BOX_COLUMN_NUM);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							continue;

						// wCell + 1 ·Î ÇÏ´Â °ÍÀº ºó°÷À» Ã¼Å©ÇÒ ¶§ °°Àº
						// ¾ÆÀÌÅÛÀº ¿¹¿ÜÃ³¸®ÇÏ±â À§ÇÔ
						m_pointsInstant.wDSItemGrid[p] = wCell + 1;
					}
				}
				else
					m_pointsInstant.wDSItemGrid[wCell] = wCell + 1;
			}

			m_pointsInstant.pDSItems[wCell] = pItem;
		}
		break;
	default:
		sys_err ("Invalid Inventory type %d", window_type);
		return;
	}

	if (GetDesc())
	{
		// È®Àå ¾ÆÀÌÅÛ: ¼­¹ö¿¡¼­ ¾ÆÀÌÅÛ ÇÃ·¡±× Á¤º¸¸¦ º¸³½´Ù
		if (pItem)
		{
			TPacketGCItemSet pack;
			pack.header = HEADER_GC_ITEM_SET;
			pack.Cell = Cell;

			pack.count = pItem->GetCount();
			pack.vnum = pItem->GetVnum();
			pack.flags = pItem->GetFlag();
			pack.anti_flags	= pItem->GetAntiFlag();
			pack.highlight = (Cell.window_type == DRAGON_SOUL_INVENTORY);


			thecore_memcpy(pack.alSockets, pItem->GetSockets(), sizeof(pack.alSockets));
			thecore_memcpy(pack.aAttr, pItem->GetAttributes(), sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemSet));
		}
		else
		{
			TPacketGCItemDelDeprecated pack;
			pack.header = HEADER_GC_ITEM_DEL;
			pack.Cell = Cell;
			pack.count = 0;
			pack.vnum = 0;
			memset(pack.alSockets, 0, sizeof(pack.alSockets));
			memset(pack.aAttr, 0, sizeof(pack.aAttr));

			GetDesc()->Packet(&pack, sizeof(TPacketGCItemDelDeprecated));
		}
	}

	if (pItem)
	{
		pItem->SetCell(this, wCell);
		switch (window_type)
		{
		case INVENTORY:
		case EQUIPMENT:
			if ((wCell < INVENTORY_MAX_NUM) || (BELT_INVENTORY_SLOT_START <= wCell && BELT_INVENTORY_SLOT_END > wCell))
				pItem->SetWindow(INVENTORY);
			else
				pItem->SetWindow(EQUIPMENT);
			break;
		case DRAGON_SOUL_INVENTORY:
			pItem->SetWindow(DRAGON_SOUL_INVENTORY);
			break;
		}
	}
}

LPITEM CHARACTER::GetWear(BYTE bCell) const
{
	// > WEAR_MAX_NUM : ¿ëÈ¥¼® ½½·Ôµé.
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::GetWear: invalid wear cell %d", bCell);
		return NULL;
	}

	return m_pointsInstant.pItems[INVENTORY_MAX_NUM + bCell];
}

void CHARACTER::SetWear(BYTE bCell, LPITEM item)
{
	// > WEAR_MAX_NUM : ¿ëÈ¥¼® ½½·Ôµé.
	if (bCell >= WEAR_MAX_NUM + DRAGON_SOUL_DECK_MAX_NUM * DS_SLOT_MAX)
	{
		sys_err("CHARACTER::SetItem: invalid item cell %d", bCell);
		return;
	}

	SetItem(TItemPos (INVENTORY, INVENTORY_MAX_NUM + bCell), item);

	if (!item && bCell == WEAR_WEAPON)
	{
		// ±Í°Ë »ç¿ë ½Ã ¹þ´Â °ÍÀÌ¶ó¸é È¿°ú¸¦ ¾ø¾Ö¾ß ÇÑ´Ù.
		if (IsAffectFlag(AFF_GWIGUM))
			RemoveAffect(SKILL_GWIGEOM);

		if (IsAffectFlag(AFF_GEOMGYEONG))
			RemoveAffect(SKILL_GEOMKYUNG);
	}
}

void CHARACTER::ClearItem()
{
	int		i;
	LPITEM	item;
	
	for (i = 0; i < INVENTORY_AND_EQUIP_SLOT_MAX; ++i)
	{
		if ((item = GetInventoryItem(i)))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);

			SyncQuickslot(QUICKSLOT_TYPE_ITEM, i, 255);
		}
	}
	for (i = 0; i < DRAGON_SOUL_INVENTORY_MAX_NUM; ++i)
	{
		if ((item = GetItem(TItemPos(DRAGON_SOUL_INVENTORY, i))))
		{
			item->SetSkipSave(true);
			ITEM_MANAGER::instance().FlushDelayedSave(item);

			item->RemoveFromCharacter();
			M2_DESTROY_ITEM(item);
		}
	}
}

bool CHARACTER::IsEmptyItemGrid(TItemPos Cell, BYTE bSize, int iExceptionCell) const
{
	switch (Cell.window_type)
	{
	case INVENTORY:
		{
			BYTE bCell = Cell.cell;

			// bItemCellÀº 0ÀÌ falseÀÓÀ» ³ªÅ¸³»±â À§ÇØ + 1 ÇØ¼­ Ã³¸®ÇÑ´Ù.
			// µû¶ó¼­ iExceptionCell¿¡ 1À» ´õÇØ ºñ±³ÇÑ´Ù.
			++iExceptionCell;

			if (Cell.IsBeltInventoryPosition())
			{
				LPITEM beltItem = GetWear(WEAR_BELT);

				if (NULL == beltItem)
					return false;

				if (false == CBeltInventoryHelper::IsAvailableCell(bCell - BELT_INVENTORY_SLOT_START, beltItem->GetValue(0)))
					return false;

				if (m_pointsInstant.bItemGrid[bCell])
				{
					if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
						return true;

					return false;
				}

				if (bSize == 1)
					return true;

			}
			else if (bCell >= INVENTORY_MAX_NUM)
				return false;

			if (m_pointsInstant.bItemGrid[bCell])
			{
				if (m_pointsInstant.bItemGrid[bCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;
					BYTE bPage = bCell / (INVENTORY_MAX_NUM / 2);

					do
					{
						BYTE p = bCell + (5 * j);

						if (p >= INVENTORY_MAX_NUM)
							return false;

						if (p / (INVENTORY_MAX_NUM / 2) != bPage)
							return false;

						if (m_pointsInstant.bItemGrid[p])
							if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// Å©±â°¡ 1ÀÌ¸é ÇÑÄ­À» Â÷ÁöÇÏ´Â °ÍÀÌ¹Ç·Î ±×³É ¸®ÅÏ
			if (1 == bSize)
				return true;
			else
			{
				int j = 1;
				BYTE bPage = bCell / (INVENTORY_MAX_NUM / 2);

				do
				{
					BYTE p = bCell + (5 * j);

					if (p >= INVENTORY_MAX_NUM)
						return false;

					if (p / (INVENTORY_MAX_NUM / 2) != bPage)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.bItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
		break;
	case DRAGON_SOUL_INVENTORY:
		{
			WORD wCell = Cell.cell;
			if (wCell >= DRAGON_SOUL_INVENTORY_MAX_NUM)
				return false;

			// bItemCellÀº 0ÀÌ falseÀÓÀ» ³ªÅ¸³»±â À§ÇØ + 1 ÇØ¼­ Ã³¸®ÇÑ´Ù.
			// µû¶ó¼­ iExceptionCell¿¡ 1À» ´õÇØ ºñ±³ÇÑ´Ù.
			iExceptionCell++;

			if (m_pointsInstant.wDSItemGrid[wCell])
			{
				if (m_pointsInstant.wDSItemGrid[wCell] == iExceptionCell)
				{
					if (bSize == 1)
						return true;

					int j = 1;

					do
					{
						BYTE p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

						if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
							return false;

						if (m_pointsInstant.wDSItemGrid[p])
							if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
								return false;
					}
					while (++j < bSize);

					return true;
				}
				else
					return false;
			}

			// Å©±â°¡ 1ÀÌ¸é ÇÑÄ­À» Â÷ÁöÇÏ´Â °ÍÀÌ¹Ç·Î ±×³É ¸®ÅÏ
			if (1 == bSize)
				return true;
			else
			{
				int j = 1;

				do
				{
					BYTE p = wCell + (DRAGON_SOUL_BOX_COLUMN_NUM * j);

					if (p >= DRAGON_SOUL_INVENTORY_MAX_NUM)
						return false;

					if (m_pointsInstant.bItemGrid[p])
						if (m_pointsInstant.wDSItemGrid[p] != iExceptionCell)
							return false;
				}
				while (++j < bSize);

				return true;
			}
		}
	}
}

int CHARACTER::GetEmptyInventory(BYTE size) const
{
	// NOTE: ÇöÀç ÀÌ ÇÔ¼ö´Â ¾ÆÀÌÅÛ Áö±Þ, È¹µæ µîÀÇ ÇàÀ§¸¦ ÇÒ ¶§ ÀÎº¥Åä¸®ÀÇ ºó Ä­À» Ã£±â À§ÇØ »ç¿ëµÇ°í ÀÖ´Âµ¥,
	//		º§Æ® ÀÎº¥Åä¸®´Â Æ¯¼ö ÀÎº¥Åä¸®ÀÌ¹Ç·Î °Ë»çÇÏÁö ¾Êµµ·Ï ÇÑ´Ù. (±âº» ÀÎº¥Åä¸®: INVENTORY_MAX_NUM ±îÁö¸¸ °Ë»ç)
	for ( int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (IsEmptyItemGrid(TItemPos (INVENTORY, i), size))
			return i;
	return -1;
}

int CHARACTER::GetEmptyDragonSoulInventory(LPITEM pItem) const
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return -1;
	if (!DragonSoul_IsQualified())
	{
		return -1;
	}
	BYTE bSize = pItem->GetSize();
	WORD wBaseCell = DSManager::instance().GetBasePosition(pItem);

	if (WORD_MAX == wBaseCell)
		return -1;

	for (int i = 0; i < DRAGON_SOUL_BOX_SIZE; ++i)
		if (IsEmptyItemGrid(TItemPos(DRAGON_SOUL_INVENTORY, i + wBaseCell), bSize))
			return i + wBaseCell;

	return -1;
}

void CHARACTER::CopyDragonSoulItemGrid(std::vector<WORD>& vDragonSoulItemGrid) const
{
	vDragonSoulItemGrid.resize(DRAGON_SOUL_INVENTORY_MAX_NUM);

	std::copy(m_pointsInstant.wDSItemGrid, m_pointsInstant.wDSItemGrid + DRAGON_SOUL_INVENTORY_MAX_NUM, vDragonSoulItemGrid.begin());
}

int CHARACTER::CountEmptyInventory() const
{
	int	count = 0;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i))
			count += GetInventoryItem(i)->GetSize();

	return (INVENTORY_MAX_NUM - count);
}

void TransformRefineItem(LPITEM pkOldItem, LPITEM pkNewItem)
{
	// ACCESSORY_REFINE
	if (pkOldItem->IsAccessoryForSocket())
	{
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			pkNewItem->SetSocket(i, pkOldItem->GetSocket(i));
		}
		//pkNewItem->StartAccessorySocketExpireEvent();
	}
	// END_OF_ACCESSORY_REFINE
	else
	{
		// ¿©±â¼­ ±úÁø¼®ÀÌ ÀÚµ¿ÀûÀ¸·Î Ã»¼Ò µÊ
		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			if (!pkOldItem->GetSocket(i))
				break;
			else
				pkNewItem->SetSocket(i, 1);
		}

		// ¼ÒÄÏ ¼³Á¤
		int slot = 0;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
		{
			long socket = pkOldItem->GetSocket(i);

			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				pkNewItem->SetSocket(slot++, socket);
		}

	}

	// ¸ÅÁ÷ ¾ÆÀÌÅÛ ¼³Á¤
	pkOldItem->CopyAttributeTo(pkNewItem);
}

void NotifyRefineSuccess(LPCHARACTER ch, LPITEM item, const char* way)
{
	if (NULL != ch && item != NULL)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineSuceeded");

		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), 1, way);
	}
}

void NotifyRefineFail(LPCHARACTER ch, LPITEM item, const char* way, int success = 0)
{
	if (NULL != ch && NULL != item)
	{
		ch->ChatPacket(CHAT_TYPE_COMMAND, "RefineFailed");

		LogManager::instance().RefineLog(ch->GetPlayerID(), item->GetName(), item->GetID(), item->GetRefineLevel(), success, way);
	}
}

void CHARACTER::SetRefineNPC(LPCHARACTER ch)
{
	if ( ch != NULL )
	{
		m_dwRefineNPCVID = ch->GetVID();
	}
	else
	{
		m_dwRefineNPCVID = 0;
	}
}

bool CHARACTER::DoRefine(LPITEM item, bool bMoneyOnly)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}
	
	//°³·® ½Ã°£Á¦ÇÑ : upgrade_refine_scroll.quest ¿¡¼­ °³·®ÈÄ 5ºÐÀÌ³»¿¡ ÀÏ¹Ý °³·®À» 
	//ÁøÇàÇÒ¼ö ¾øÀ½
	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	DWORD result_vnum = item->GetRefinedVnum();

	// REFINE_COST
	int cost = ComputeRefineFee(prt->cost);

	int RefineChance = GetQuestFlag("main_quest_lv7.refine_chance");

	if (RefineChance > 0)
	{
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«·á °³·® ±âÈ¸´Â 20 ÀÌÇÏÀÇ ¹«±â¸¸ °¡´ÉÇÕ´Ï´Ù"));
			return false;
		}

		cost = 0;
		SetQuestFlag("main_quest_lv7.refine_chance", RefineChance - 1);
	}
	// END_OF_REFINE_COST

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
		return false;

	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefine NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// Check level limit in korea only
	if (!g_iUseLocale)
	{
		for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
		{
			long limit = pProto->aLimits[i].lValue;

			switch (pProto->aLimits[i].bType)
			{
				case LIMIT_LEVEL:
					if (GetLevel() < limit)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®µÈ ÈÄ ¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
						return false;
					}
					break;
			}
		}
	}

	// REFINE_COST
	if (GetGold() < cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return false;
	}

	if (!bMoneyOnly && !RefineChance)
	{
		for (int i = 0; i < prt->material_count; ++i)
		{
			if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
			{
				if (test_server)
				{
					ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
				}
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®À» ÇÏ±â À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
				return false;
			}
		}

		for (int i = 0; i < prt->material_count; ++i)
			RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);
	}

	int prob = number(1, 100);

	if (IsRefineThroughGuild() || bMoneyOnly)
		prob -= 10;

	// END_OF_REFINE_COST

	if (prob <= prt->prob)
	{
		// ¼º°ø! ¸ðµç ¾ÆÀÌÅÛÀÌ »ç¶óÁö°í, °°Àº ¼Ó¼ºÀÇ ´Ù¸¥ ¾ÆÀÌÅÛ È¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			BYTE bCell = item->GetCell();

			// DETAIL_REFINE_LOG
			NotifyRefineSuccess(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");
			// END_OF_DETAIL_REFINE_LOG

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell)); 
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			sys_log(0, "Refine Success %d", cost);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -cost);
			sys_log(0, "PayPee %d", cost);
			PayRefineFee(cost);
			sys_log(0, "PayPee End %d", cost);
		}
		else
		{
			// DETAIL_REFINE_LOG
			// ¾ÆÀÌÅÛ »ý¼º¿¡ ½ÇÆÐ -> °³·® ½ÇÆÐ·Î °£ÁÖ
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
			// END_OF_DETAIL_REFINE_LOG
		}
	}
	else
	{
		// ½ÇÆÐ! ¸ðµç ¾ÆÀÌÅÛÀÌ »ç¶óÁü.
		DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -cost);
		NotifyRefineFail(this, item, IsRefineThroughGuild() ? "GUILD" : "POWER");
		item->AttrLog();
		ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

		//PointChange(POINT_GOLD, -cost);
		PayRefineFee(cost);
	}

	return true;
}

enum enum_RefineScrolls
{
	CHUKBOK_SCROLL = 0,
	HYUNIRON_CHN   = 1, // Áß±¹¿¡¼­¸¸ »ç¿ë
	YONGSIN_SCROLL = 2,
	MUSIN_SCROLL   = 3,
	YAGONG_SCROLL  = 4,
	MEMO_SCROLL	   = 5,
	BDRAGON_SCROLL	= 6,
};

bool CHARACTER::DoRefineWithScroll(LPITEM item)
{
	if (!CanHandleItem(true))
	{
		ClearRefineMode();
		return false;
	}

	ClearRefineMode();

	//°³·® ½Ã°£Á¦ÇÑ : upgrade_refine_scroll.quest ¿¡¼­ °³·®ÈÄ 5ºÐÀÌ³»¿¡ ÀÏ¹Ý °³·®À» 
	//ÁøÇàÇÒ¼ö ¾øÀ½
	if (quest::CQuestManager::instance().GetEventFlag("update_refine_time") != 0)
	{
		if (get_global_time() < quest::CQuestManager::instance().GetEventFlag("update_refine_time") + (60 * 5))
		{
			sys_log(0, "can't refine %d %s", GetPlayerID(), GetName());
			return false;
		}
	}

	const TRefineTable * prt = CRefineManager::instance().GetRefineRecipe(item->GetRefineSet());

	if (!prt)
		return false;

	LPITEM pkItemScroll;

	// °³·®¼­ Ã¼Å©
	if (m_iRefineAdditionalCell < 0)
		return false;

	pkItemScroll = GetInventoryItem(m_iRefineAdditionalCell);

	if (!pkItemScroll)
		return false;

	if (!(pkItemScroll->GetType() == ITEM_USE && pkItemScroll->GetSubType() == USE_TUNING))
		return false;

	if (pkItemScroll->GetVnum() == item->GetVnum())
		return false;

	DWORD result_vnum = item->GetRefinedVnum();
	DWORD result_fail_vnum = item->GetRefineFromVnum();

	if (result_vnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// MUSIN_SCROLL
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL)
	{
		if (item->GetRefineLevel() >= 4)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ °³·®¼­·Î ´õ ÀÌ»ó °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	// END_OF_MUSIC_SCROLL

	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		if (item->GetRefineLevel() != pkItemScroll->GetValue(1))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ °³·®¼­·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		if (item->GetType() != ITEM_METIN || item->GetRefineLevel() != 4)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	TItemTable * pProto = ITEM_MANAGER::instance().GetTable(item->GetRefinedVnum());

	if (!pProto)
	{
		sys_err("DoRefineWithScroll NOT GET ITEM PROTO %d", item->GetRefinedVnum());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// Check level limit in korea only
	if (!g_iUseLocale)
	{
		for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
		{
			long limit = pProto->aLimits[i].lValue;

			switch (pProto->aLimits[i].bType)
			{
				case LIMIT_LEVEL:
					if (GetLevel() < limit)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®µÈ ÈÄ ¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
						return false;
					}
					break;
			}
		}
	}

	if (GetGold() < prt->cost)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®À» ÇÏ±â À§ÇÑ µ·ÀÌ ºÎÁ·ÇÕ´Ï´Ù."));
		return false;
	}

	for (int i = 0; i < prt->material_count; ++i)
	{
		if (CountSpecifyItem(prt->materials[i].vnum) < prt->materials[i].count)
		{
			if (test_server)
			{
				ChatPacket(CHAT_TYPE_INFO, "Find %d, count %d, require %d", prt->materials[i].vnum, CountSpecifyItem(prt->materials[i].vnum), prt->materials[i].count);
			}
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³·®À» ÇÏ±â À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
			return false;
		}
	}

	for (int i = 0; i < prt->material_count; ++i)
		RemoveSpecifyItem(prt->materials[i].vnum, prt->materials[i].count);

	int prob = number(1, 100);
	int success_prob = prt->prob;
	bool bDestroyWhenFail = false;

	const char* szRefineType = "SCROLL";

	if (pkItemScroll->GetValue(0) == HYUNIRON_CHN || 
		pkItemScroll->GetValue(0) == YONGSIN_SCROLL || 
		pkItemScroll->GetValue(0) == YAGONG_SCROLL) // ÇöÃ¶, ¿ë½ÅÀÇ Ãàº¹¼­, ¾ß°øÀÇ ºñÀü¼­  Ã³¸®
	{
		const char hyuniron_prob[9] = { 100, 75, 65, 55, 45, 40, 35, 25, 20 };
		const char hyuniron_prob_euckr[9] = { 100, 75, 65, 55, 45, 40, 35, 30, 25 };

		const char yagong_prob[9] = { 100, 100, 90, 80, 70, 60, 50, 30, 20 };
		const char yagong_prob_euckr[9] = { 100, 100, 90, 80, 70, 60, 50, 40, 30 };

		if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			if (LC_IsYMIR() == true || LC_IsKorea() == true)
				success_prob = hyuniron_prob_euckr[MINMAX(0, item->GetRefineLevel(), 8)];
			else
				success_prob = hyuniron_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			if (LC_IsYMIR() == true || LC_IsKorea() == true)
				success_prob = yagong_prob_euckr[MINMAX(0, item->GetRefineLevel(), 8)];
			else
				success_prob = yagong_prob[MINMAX(0, item->GetRefineLevel(), 8)];
		}
		else
		{
			sys_err("REFINE : Unknown refine scroll item. Value0: %d", pkItemScroll->GetValue(0));
		}

		if (test_server) 
		{
			ChatPacket(CHAT_TYPE_INFO, "[Only Test] Success_Prob %d, RefineLevel %d ", success_prob, item->GetRefineLevel());
		}
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN) // ÇöÃ¶Àº ¾ÆÀÌÅÛÀÌ ºÎ¼­Á®¾ß ÇÑ´Ù.
			bDestroyWhenFail = true;

		// DETAIL_REFINE_LOG
		if (pkItemScroll->GetValue(0) == HYUNIRON_CHN)
		{
			szRefineType = "HYUNIRON";
		}
		else if (pkItemScroll->GetValue(0) == YONGSIN_SCROLL)
		{
			szRefineType = "GOD_SCROLL";
		}
		else if (pkItemScroll->GetValue(0) == YAGONG_SCROLL)
		{
			szRefineType = "YAGONG_SCROLL";
		}
		// END_OF_DETAIL_REFINE_LOG
	}

	// DETAIL_REFINE_LOG
	if (pkItemScroll->GetValue(0) == MUSIN_SCROLL) // ¹«½ÅÀÇ Ãàº¹¼­´Â 100% ¼º°ø (+4±îÁö¸¸)
	{
		success_prob = 100;

		szRefineType = "MUSIN_SCROLL";
	}
	// END_OF_DETAIL_REFINE_LOG
	else if (pkItemScroll->GetValue(0) == MEMO_SCROLL)
	{
		success_prob = 100;
		szRefineType = "MEMO_SCROLL";
	}
	else if (pkItemScroll->GetValue(0) == BDRAGON_SCROLL)
	{
		success_prob = 80;
		szRefineType = "BDRAGON_SCROLL";
	}

	pkItemScroll->SetCount(pkItemScroll->GetCount() - 1);

	if (prob <= success_prob)
	{
		// ¼º°ø! ¸ðµç ¾ÆÀÌÅÛÀÌ »ç¶óÁö°í, °°Àº ¼Ó¼ºÀÇ ´Ù¸¥ ¾ÆÀÌÅÛ È¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE SUCCESS", pkNewItem->GetName());

			BYTE bCell = item->GetCell();

			NotifyRefineSuccess(this, item, szRefineType);
			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE SUCCESS)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell)); 
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);
			pkNewItem->AttrLog();
			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			// ¾ÆÀÌÅÛ »ý¼º¿¡ ½ÇÆÐ -> °³·® ½ÇÆÐ·Î °£ÁÖ
			sys_err("cannot create item %u", result_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else if (!bDestroyWhenFail && result_fail_vnum)
	{
		// ½ÇÆÐ! ¸ðµç ¾ÆÀÌÅÛÀÌ »ç¶óÁö°í, °°Àº ¼Ó¼ºÀÇ ³·Àº µî±ÞÀÇ ¾ÆÀÌÅÛ È¹µæ
		LPITEM pkNewItem = ITEM_MANAGER::instance().CreateItem(result_fail_vnum, 1, 0, false);

		if (pkNewItem)
		{
			ITEM_MANAGER::CopyAllAttrTo(item, pkNewItem);
			LogManager::instance().ItemLog(this, pkNewItem, "REFINE FAIL", pkNewItem->GetName());

			BYTE bCell = item->GetCell();

			DBManager::instance().SendMoneyLog(MONEY_LOG_REFINE, item->GetVnum(), -prt->cost);
			NotifyRefineFail(this, item, szRefineType, -1);
			ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (REFINE FAIL)");

			pkNewItem->AddToCharacter(this, TItemPos(INVENTORY, bCell)); 
			ITEM_MANAGER::instance().FlushDelayedSave(pkNewItem);

			pkNewItem->AttrLog();

			//PointChange(POINT_GOLD, -prt->cost);
			PayRefineFee(prt->cost);
		}
		else
		{
			// ¾ÆÀÌÅÛ »ý¼º¿¡ ½ÇÆÐ -> °³·® ½ÇÆÐ·Î °£ÁÖ
			sys_err("cannot create item %u", result_fail_vnum);
			NotifyRefineFail(this, item, szRefineType);
		}
	}
	else
	{
		NotifyRefineFail(this, item, szRefineType); // °³·®½Ã ¾ÆÀÌÅÛ »ç¶óÁöÁö ¾ÊÀ½
		
		PayRefineFee(prt->cost);
	}

	return true;
}

bool CHARACTER::RefineInformation(BYTE bCell, BYTE bType, int iAdditionalCell)
{
	if (bCell > INVENTORY_MAX_NUM)
		return false;

	LPITEM item = GetInventoryItem(bCell);

	if (!item)
		return false;

	// REFINE_COST
	if (bType == REFINE_TYPE_MONEY_ONLY && !GetQuestFlag("deviltower_zone.can_refine"))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç±Í Å¸¿ö ¿Ï·á º¸»óÀº ÇÑ¹ø±îÁö »ç¿ë°¡´ÉÇÕ´Ï´Ù."));
		return false;
	}
	// END_OF_REFINE_COST

	TPacketGCRefineInformation p;

	p.header = HEADER_GC_REFINE_INFORMATION;
	p.pos = bCell;
	p.src_vnum = item->GetVnum();
	p.result_vnum = item->GetRefinedVnum();
	p.type = bType;

	if (p.result_vnum == 0)
	{
		sys_err("RefineInformation p.result_vnum == 0");
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (item->GetType() == ITEM_USE && item->GetSubType() == USE_TUNING)
	{
		if (bType == 0)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº ÀÌ ¹æ½ÄÀ¸·Î´Â °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
		else
		{
			LPITEM itemScroll = GetInventoryItem(iAdditionalCell);
			if (!itemScroll || item->GetVnum() == itemScroll->GetVnum())
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°°Àº °³·®¼­¸¦ ÇÕÄ¥ ¼ö´Â ¾ø½À´Ï´Ù."));
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ãàº¹ÀÇ ¼­¿Í ÇöÃ¶À» ÇÕÄ¥ ¼ö ÀÖ½À´Ï´Ù."));
				return false;
			}
		}
	}

	CRefineManager & rm = CRefineManager::instance();

	const TRefineTable* prt = rm.GetRefineRecipe(item->GetRefineSet());

	if (!prt)
	{
		sys_err("RefineInformation NOT GET REFINE SET %d", item->GetRefineSet());
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// REFINE_COST
	
	//MAIN_QUEST_LV7
	if (GetQuestFlag("main_quest_lv7.refine_chance") > 0)
	{
		// ÀÏº»Àº Á¦¿Ü
		if (!item->CheckItemUseLevel(20) || item->GetType() != ITEM_WEAPON)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«·á °³·® ±âÈ¸´Â 20 ÀÌÇÏÀÇ ¹«±â¸¸ °¡´ÉÇÕ´Ï´Ù"));
			return false;
		}
		p.cost = 0;
	}
	else
		p.cost = ComputeRefineFee(prt->cost);
	
	//END_MAIN_QUEST_LV7
	p.prob = prt->prob;
	if (bType == REFINE_TYPE_MONEY_ONLY)
	{
		p.material_count = 0;
		memset(p.materials, 0, sizeof(p.materials));
	}
	else
	{
		p.material_count = prt->material_count;
		thecore_memcpy(&p.materials, prt->materials, sizeof(prt->materials));
	}
	// END_OF_REFINE_COST

	GetDesc()->Packet(&p, sizeof(TPacketGCRefineInformation));

	SetRefineMode(iAdditionalCell);
	return true;
}

bool CHARACTER::RefineItem(LPITEM pkItem, LPITEM pkTarget)
{
	if (!CanHandleItem())
		return false;

	if (pkItem->GetSubType() == USE_TUNING)
	{
		// XXX ¼º´É, ¼ÒÄÏ °³·®¼­´Â »ç¶óÁ³½À´Ï´Ù...
		// XXX ¼º´É°³·®¼­´Â Ãàº¹ÀÇ ¼­°¡ µÇ¾ú´Ù!
		// MUSIN_SCROLL
		if (pkItem->GetValue(0) == MUSIN_SCROLL)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_MUSIN, pkItem->GetCell());
		// END_OF_MUSIN_SCROLL
		else if (pkItem->GetValue(0) == HYUNIRON_CHN)
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_HYUNIRON, pkItem->GetCell());
		else if (pkItem->GetValue(0) == BDRAGON_SCROLL)
		{
			if (pkTarget->GetRefineSet() != 702) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_BDRAGON, pkItem->GetCell());
		}
		else
		{
			if (pkTarget->GetRefineSet() == 501) return false;
			RefineInformation(pkTarget->GetCell(), REFINE_TYPE_SCROLL, pkItem->GetCell());
		}
	}
	else if (pkItem->GetSubType() == USE_DETACHMENT && IS_SET(pkTarget->GetFlag(), ITEM_FLAG_REFINEABLE))
	{
		LogManager::instance().ItemLog(this, pkTarget, "USE_DETACHMENT", pkTarget->GetName());

		bool bHasMetinStone = false;

		for (int i = 0; i < ITEM_SOCKET_MAX_NUM; i++)
		{
			long socket = pkTarget->GetSocket(i);
			if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
			{
				bHasMetinStone = true;
				break;
			}
		}

		if (bHasMetinStone)
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
			{
				long socket = pkTarget->GetSocket(i);
				if (socket > 2 && socket != ITEM_BROKEN_METIN_VNUM)
				{
					AutoGiveItem(socket);
					//TItemTable* pTable = ITEM_MANAGER::instance().GetTable(pkTarget->GetSocket(i));
					//pkTarget->SetSocket(i, pTable->alValues[2]);
					// ±úÁøµ¹·Î ´ëÃ¼ÇØÁØ´Ù
					pkTarget->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
				}
			}
			pkItem->SetCount(pkItem->GetCount() - 1);
			return true;
		}
		else
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»©³¾ ¼ö ÀÖ´Â ¸ÞÆ¾¼®ÀÌ ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	return false;
}

EVENTFUNC(kill_campfire_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "kill_campfire_event> <Factor> Null pointer" );
		return 0;
	}

	LPCHARACTER	ch = info->ch;

	if (ch == NULL) { // <Factor>
		return 0;
	}
	ch->m_pkMiningEvent = NULL;
	M2_DESTROY_CHARACTER(ch);
	return 0;
}

bool CHARACTER::GiveRecallItem(LPITEM item)
{
	int idx = GetMapIndex();
	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±â¾ïÇØ µÑ ¼ö ¾ø´Â À§Ä¡ ÀÔ´Ï´Ù."));
		return false;
	}

	int pos;

	if (item->GetCount() == 1)	// ¾ÆÀÌÅÛÀÌ ÇÏ³ª¶ó¸é ±×³É ¼ÂÆÃ.
	{
		item->SetSocket(0, GetX());
		item->SetSocket(1, GetY());
	}
	else if ((pos = GetEmptyInventory(item->GetSize())) != -1) // ±×·¸Áö ¾Ê´Ù¸é ´Ù¸¥ ÀÎº¥Åä¸® ½½·ÔÀ» Ã£´Â´Ù.
	{
		LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), 1);

		if (NULL != item2)
		{
			item2->SetSocket(0, GetX());
			item2->SetSocket(1, GetY());
			item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

			item->SetCount(item->GetCount() - 1);
		}
	}
	else
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
		return false;
	}

	return true;
}

void CHARACTER::ProcessRecallItem(LPITEM item)
{
	int idx;

	if ((idx = SECTREE_MANAGER::instance().GetMapIndex(item->GetSocket(0), item->GetSocket(1))) == 0)
		return;

	int iEmpireByMapIndex = -1;

	if (idx < 20)
		iEmpireByMapIndex = 1;
	else if (idx < 40)
		iEmpireByMapIndex = 2;
	else if (idx < 60)
		iEmpireByMapIndex = 3;
	else if (idx < 10000)
		iEmpireByMapIndex = 0;

	switch (idx)
	{
		case 66:
		case 216:
			iEmpireByMapIndex = -1;
			break;
		// ¾Ç·æ±ºµµ ÀÏ¶§
		case 301:
		case 302:
		case 303:
		case 304:
			if( GetLevel() < 90 )
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
				return;
			}
			else
				break;
	}

	if (iEmpireByMapIndex && GetEmpire() != iEmpireByMapIndex)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±â¾ïµÈ À§Ä¡°¡ Å¸Á¦±¹¿¡ ¼ÓÇØ ÀÖ¾î¼­ ±ÍÈ¯ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		item->SetSocket(0, 0);
		item->SetSocket(1, 0);
	}
	else
	{
		sys_log(1, "Recall: %s %d %d -> %d %d", GetName(), GetX(), GetY(), item->GetSocket(0), item->GetSocket(1));
		WarpSet(item->GetSocket(0), item->GetSocket(1));
		item->SetCount(item->GetCount() - 1);
	}
}

void CHARACTER::__OpenPrivateShop()
{
	unsigned bodyPart = GetPart(PART_MAIN);
	switch (bodyPart)
	{
		case 0:
		case 1:
		case 2:
			ChatPacket(CHAT_TYPE_COMMAND, "OpenPrivateShop");
			break;
		default:
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°©¿ÊÀ» ¹þ¾î¾ß °³ÀÎ »óÁ¡À» ¿­ ¼ö ÀÖ½À´Ï´Ù."));
			break;
	}
}

// MYSHOP_PRICE_LIST
void CHARACTER::SendMyShopPriceListCmd(DWORD dwItemVnum, DWORD dwItemPrice)
{
	char szLine[256];
	snprintf(szLine, sizeof(szLine), "MyShopPriceList %u %u", dwItemVnum, dwItemPrice);
	ChatPacket(CHAT_TYPE_COMMAND, szLine);
	sys_log(0, szLine);
}

//
// DB Ä³½Ã·Î ºÎÅÍ ¹ÞÀº ¸®½ºÆ®¸¦ User ¿¡°Ô Àü¼ÛÇÏ°í »óÁ¡À» ¿­¶ó´Â Ä¿¸Çµå¸¦ º¸³½´Ù.
//
void CHARACTER::UseSilkBotaryReal(const TPacketMyshopPricelistHeader* p)
{
	const TItemPriceInfo* pInfo = (const TItemPriceInfo*)(p + 1);

	if (!p->byCount)
		// °¡°Ý ¸®½ºÆ®°¡ ¾ø´Ù. dummy µ¥ÀÌÅÍ¸¦ ³ÖÀº Ä¿¸Çµå¸¦ º¸³»ÁØ´Ù.
		SendMyShopPriceListCmd(1, 0);
	else {
		for (int idx = 0; idx < p->byCount; idx++)
			SendMyShopPriceListCmd(pInfo[ idx ].dwVnum, pInfo[ idx ].dwPrice);
	}

	__OpenPrivateShop();
}

//
// ÀÌ¹ø Á¢¼Ó ÈÄ Ã³À½ »óÁ¡À» Open ÇÏ´Â °æ¿ì ¸®½ºÆ®¸¦ Load ÇÏ±â À§ÇØ DB Ä³½Ã¿¡ °¡°ÝÁ¤º¸ ¸®½ºÆ® ¿äÃ» ÆÐÅ¶À» º¸³½´Ù.
// ÀÌÈÄºÎÅÍ´Â ¹Ù·Î »óÁ¡À» ¿­¶ó´Â ÀÀ´äÀ» º¸³½´Ù.
//
void CHARACTER::UseSilkBotary(void)
{
	if (m_bNoOpenedShop) {
		DWORD dwPlayerID = GetPlayerID();
		db_clientdesc->DBPacket(HEADER_GD_MYSHOP_PRICELIST_REQ, GetDesc()->GetHandle(), &dwPlayerID, sizeof(DWORD));
		m_bNoOpenedShop = false;
	} else {
		__OpenPrivateShop();
	}
}
// END_OF_MYSHOP_PRICE_LIST


int CalculateConsume(LPCHARACTER ch)
{
	static const int WARP_NEED_LIFE_PERCENT	= 30;
	static const int WARP_MIN_LIFE_PERCENT	= 10;
	// CONSUME_LIFE_WHEN_USE_WARP_ITEM
	int consumeLife = 0;
	{
		// CheckNeedLifeForWarp
		const int curLife		= ch->GetHP();
		const int needPercent	= WARP_NEED_LIFE_PERCENT;
		const int needLife = ch->GetMaxHP() * needPercent / 100;
		if (curLife < needLife)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³²Àº »ý¸í·Â ¾çÀÌ ¸ðÀÚ¶ó »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return -1;
		}

		consumeLife = needLife;


		// CheckMinLifeForWarp: µ¶¿¡ ÀÇÇØ¼­ Á×À¸¸é ¾ÈµÇ¹Ç·Î »ý¸í·Â ÃÖ¼Ò·®´Â ³²°ÜÁØ´Ù
		const int minPercent	= WARP_MIN_LIFE_PERCENT;
		const int minLife	= ch->GetMaxHP() * minPercent / 100;
		if (curLife - needLife < minLife)
			consumeLife = curLife - minLife;

		if (consumeLife < 0)
			consumeLife = 0;
	}
	// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM
	return consumeLife;
}

int CalculateConsumeSP(LPCHARACTER lpChar)
{
	static const int NEED_WARP_SP_PERCENT = 30;

	const int curSP = lpChar->GetSP();
	const int needSP = lpChar->GetMaxSP() * NEED_WARP_SP_PERCENT / 100;

	if (curSP < needSP)
	{
		lpChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³²Àº Á¤½Å·Â ¾çÀÌ ¸ðÀÚ¶ó »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return -1;
	}

	return needSP;
}

bool CHARACTER::UseItemEx(LPITEM item, TItemPos DestCell)
{
	int iLimitRealtimeStartFirstUseFlagIndex = -1;
	int iLimitTimerBasedOnWearFlagIndex = -1;

	WORD wDestCell = DestCell.cell;
	BYTE bDestInven = DestCell.window_type;
	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limitValue = item->GetProto()->aLimits[i].lValue;

		switch (item->GetProto()->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limitValue)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛÀÇ ·¹º§ Á¦ÇÑº¸´Ù ·¹º§ÀÌ ³·½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_REAL_TIME_START_FIRST_USE:
				iLimitRealtimeStartFirstUseFlagIndex = i;
				break;

			case LIMIT_TIMER_BASED_ON_WEAR:
				iLimitTimerBasedOnWearFlagIndex = i;
				break;
		}
	}

	if (test_server)
	{
		sys_log(0, "USE_ITEM %s, Inven %d, Cell %d, ItemType %d, SubType %d", item->GetName(), bDestInven, wDestCell, item->GetType(), item->GetSubType());
	}

	if ( CArenaManager::instance().IsLimitedItem( GetMapIndex(), item->GetVnum() ) == true )
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
		return false;
	}

	// ¾ÆÀÌÅÛ ÃÖÃÊ »ç¿ë ÀÌÈÄºÎÅÍ´Â »ç¿ëÇÏÁö ¾Ê¾Æµµ ½Ã°£ÀÌ Â÷°¨µÇ´Â ¹æ½Ä Ã³¸®. 
	if (-1 != iLimitRealtimeStartFirstUseFlagIndex)
	{
		// ÇÑ ¹øÀÌ¶óµµ »ç¿ëÇÑ ¾ÆÀÌÅÛÀÎÁö ¿©ºÎ´Â Socket1À» º¸°í ÆÇ´ÜÇÑ´Ù. (Socket1¿¡ »ç¿ëÈ½¼ö ±â·Ï)
		if (0 == item->GetSocket(1))
		{
			// »ç¿ë°¡´É½Ã°£Àº Default °ªÀ¸·Î Limit Value °ªÀ» »ç¿ëÇÏµÇ, Socket0¿¡ °ªÀÌ ÀÖÀ¸¸é ±× °ªÀ» »ç¿ëÇÏµµ·Ï ÇÑ´Ù. (´ÜÀ§´Â ÃÊ)
			long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[iLimitRealtimeStartFirstUseFlagIndex].lValue;

			if (0 == duration)
				duration = 60 * 60 * 24 * 7;

			item->SetSocket(0, time(0) + duration);
			item->StartRealTimeExpireEvent();
		}	

		if (false == item->IsEquipped())
			item->SetSocket(1, item->GetSocket(1) + 1);		
	}

	switch (item->GetType())
	{
		case ITEM_HAIR:
			return ItemProcess_Hair(item, wDestCell);

		case ITEM_POLYMORPH:
			return ItemProcess_Polymorph(item);

		case ITEM_QUEST:
			if (GetArena() != NULL || IsObserverMode() == true)
			{
				if (item->GetVnum() == 50051 || item->GetVnum() == 50052 || item->GetVnum() == 50053)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
					return false;
				}
			}

			if (!IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE | ITEM_FLAG_QUEST_USE_MULTIPLE))
			{
				if (item->GetSIGVnum() == 0)
				{
					quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
				}
				else
				{
					quest::CQuestManager::instance().SIGUse(GetPlayerID(), item->GetSIGVnum(), item, false);
				}
			}
			break;

		case ITEM_CAMPFIRE:
			{
				float fx, fy;
				GetDeltaByDegree(GetRotation(), 100.0f, &fx, &fy);

				LPSECTREE tree = SECTREE_MANAGER::instance().Get(GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy));

				if (!tree)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ð´ÚºÒÀ» ÇÇ¿ï ¼ö ¾ø´Â ÁöÁ¡ÀÔ´Ï´Ù."));
					return false;
				}

				if (tree->IsAttr((long)(GetX()+fx), (long)(GetY()+fy), ATTR_WATER))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹° ¼Ó¿¡ ¸ð´ÚºÒÀ» ÇÇ¿ï ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				LPCHARACTER campfire = CHARACTER_MANAGER::instance().SpawnMob(fishing::CAMPFIRE_MOB, GetMapIndex(), (long)(GetX()+fx), (long)(GetY()+fy), 0, false, number(0, 359));

				char_event_info* info = AllocEventInfo<char_event_info>();

				info->ch = campfire;

				campfire->m_pkMiningEvent = event_create(kill_campfire_event, info, PASSES_PER_SEC(40));

				item->SetCount(item->GetCount() - 1);
			}
			break;

		case ITEM_UNIQUE:
			{
				switch (item->GetSubType())
				{
					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true, true);
									break;

								case APPLY_STR:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_RESIST_MAGIC:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_RESIST_MAGIC, item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_ATT_GRADE_BONUS, 
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_UNIQUE_ABILITY, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					default:
						{
							if (item->GetSubType() == USE_SPECIAL)
							{
								sys_log(0, "ITEM_UNIQUE: USE_SPECIAL %u", item->GetVnum());

								switch (item->GetVnum())
								{
									case 71049: // ºñ´Üº¸µû¸®
										if (LC_IsYMIR() == true || LC_IsKorea() == true)
										{
											if (IS_BOTARYABLE_ZONE(GetMapIndex()) == true)
											{
												UseSilkBotary();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³ÀÎ »óÁ¡À» ¿­ ¼ö ¾ø´Â Áö¿ªÀÔ´Ï´Ù"));
											}
										}
										else
										{
											UseSilkBotary();
										}
										break;
								}
							}
							else
							{
								if (!item->IsEquipped())
									EquipItem(item);
								else
									UnequipItem(item);
							}
						}
						break;
				}
			}
			break;

		case ITEM_COSTUME:
		case ITEM_WEAPON:
		case ITEM_ARMOR:
		case ITEM_ROD:
		case ITEM_RING:		// ½Å±Ô ¹ÝÁö ¾ÆÀÌÅÛ
		case ITEM_BELT:		// ½Å±Ô º§Æ® ¾ÆÀÌÅÛ
			// MINING
		case ITEM_PICK:
			// END_OF_MINING
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
			// Âø¿ëÇÏÁö ¾ÊÀº ¿ëÈ¥¼®Àº »ç¿ëÇÒ ¼ö ¾ø´Ù.
			// Á¤»óÀûÀÎ Å¬¶ó¶ó¸é, ¿ëÈ¥¼®¿¡ °üÇÏ¿© item use ÆÐÅ¶À» º¸³¾ ¼ö ¾ø´Ù.
			// ¿ëÈ¥¼® Âø¿ëÀº item move ÆÐÅ¶À¸·Î ÇÑ´Ù.
			// Âø¿ëÇÑ ¿ëÈ¥¼®Àº ÃßÃâÇÑ´Ù.
		case ITEM_DS:
			{
				if (!item->IsEquipped())
					return false;
				return DSManager::instance().PullOut(this, NPOS, item);
			break;
			}
		case ITEM_SPECIAL_DS:
			if (!item->IsEquipped())
				EquipItem(item);
			else
				UnequipItem(item);
			break;
		
		case ITEM_FISH:
			{
				if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
					return false;
				}

				if (item->GetSubType() == FISH_ALIVE)
					fishing::UseFish(this, item);
			}
			break;

		case ITEM_TREASURE_BOX:
			{
				return false;
				//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è·Î Àá°Ü ÀÖ¾î¼­ ¿­¸®Áö ¾Ê´Â°Í °°´Ù. ¿­¼è¸¦ ±¸ÇØº¸ÀÚ."));
			}
			break;

		case ITEM_TREASURE_KEY:
			{
				LPITEM item2;

				if (!GetItem(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging())
					return false;

				if (item2->GetType() != ITEM_TREASURE_BOX)
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è·Î ¿©´Â ¹°°ÇÀÌ ¾Æ´Ñ°Í °°´Ù."));
					return false;
				}

				if (item->GetValue(0) == item2->GetValue(0))
				{
					//ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è´Â ¸ÂÀ¸³ª ¾ÆÀÌÅÛ ÁÖ´Â ºÎºÐ ±¸ÇöÀÌ ¾ÈµÇ¾ú½À´Ï´Ù."));
					DWORD dwBoxVnum = item2->GetVnum();
					std::vector <DWORD> dwVnums;
					std::vector <DWORD> dwCounts;
					std::vector <LPITEM> item_gets(NULL);
					int count = 0;

					if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
					{
						ITEM_MANAGER::instance().RemoveItem(item);
						ITEM_MANAGER::instance().RemoveItem(item2);
						
						for (int i = 0; i < count; i++){
							switch (dwVnums[i])
							{
								case CSpecialItemGroup::GOLD:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
									break;
								case CSpecialItemGroup::EXP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
									break;
								case CSpecialItemGroup::MOB:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::SLOW:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
									break;
								case CSpecialItemGroup::DRAIN_HP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
									break;
								case CSpecialItemGroup::POISON:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
									break;
								case CSpecialItemGroup::MOB_GROUP:
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
									break;
								default:
									if (item_gets[i])
									{
										if (dwCounts[i] > 1)
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(), dwCounts[i]);
										else
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName());

									}
							}
						}
					}
					else
					{
						ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è°¡ ¸ÂÁö ¾Ê´Â °Í °°´Ù."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¿­¼è°¡ ¸ÂÁö ¾Ê´Â °Í °°´Ù."));
					return false;
				}
			}
			break;

		case ITEM_GIFTBOX:
			{
				DWORD dwBoxVnum = item->GetVnum();
				std::vector <DWORD> dwVnums;
				std::vector <DWORD> dwCounts;
				std::vector <LPITEM> item_gets(NULL);
				int count = 0;

				if (dwBoxVnum == 50033 && LC_IsYMIR()) // ¾Ë¼ö¾ø´Â »óÀÚ
				{
					if (GetLevel() < 15)
					{
						ChatPacket(CHAT_TYPE_INFO, "15·¹º§ ÀÌÇÏ¿¡¼­´Â »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù.");
						return false;
					}
				}

				if( (dwBoxVnum > 51500 && dwBoxVnum < 52000) || (dwBoxVnum >= 50255 && dwBoxVnum <= 50260) )	// ¿ëÈ¥¿ø¼®µé
				{
					if( !(this->DragonSoul_IsQualified()) )
					{
						ChatPacket(CHAT_TYPE_INFO,LC_TEXT("¸ÕÀú ¿ëÈ¥¼® Äù½ºÆ®¸¦ ¿Ï·áÇÏ¼Å¾ß ÇÕ´Ï´Ù."));
						return false;
					}
				}

				if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
				{
					item->SetCount(item->GetCount()-1);

					for (int i = 0; i < count; i++){
						switch (dwVnums[i])
						{
						case CSpecialItemGroup::GOLD:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
							break;
						case CSpecialItemGroup::EXP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
							break;
						case CSpecialItemGroup::MOB:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
							break;
						case CSpecialItemGroup::SLOW:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
							break;
						case CSpecialItemGroup::DRAIN_HP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
							break;
						case CSpecialItemGroup::POISON:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
							break;
						case CSpecialItemGroup::MOB_GROUP:
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
							break;
						default:
							if (item_gets[i])
							{
								if (dwCounts[i] > 1)
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(), dwCounts[i]);
								else
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName());
							}
						}
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¾Æ¹«°Íµµ ¾òÀ» ¼ö ¾ø¾ú½À´Ï´Ù."));
					return false;
				}
			}
			break;

		case ITEM_SKILLFORGET:
			{
				if (!item->GetSocket(0))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					return false;
				}

				DWORD dwVnum = item->GetSocket(0);

				if (SkillLevelDown(dwVnum))
				{
					ITEM_MANAGER::instance().RemoveItem(item);
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½ºÅ³ ·¹º§À» ³»¸®´Âµ¥ ¼º°øÇÏ¿´½À´Ï´Ù."));
				}
				else
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½ºÅ³ ·¹º§À» ³»¸± ¼ö ¾ø½À´Ï´Ù."));
			}
			break;

		case ITEM_SKILLBOOK:
			{
				if (IsPolymorphed())
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				DWORD dwVnum = 0;

				if (item->GetVnum() == 50300)
				{
					dwVnum = item->GetSocket(0);
				}
				else
				{
					// »õ·Î¿î ¼ö·Ã¼­´Â value 0 ¿¡ ½ºÅ³ ¹øÈ£°¡ ÀÖÀ¸¹Ç·Î ±×°ÍÀ» »ç¿ë.
					dwVnum = item->GetValue(0);
				}

				if (0 == dwVnum)
				{
					ITEM_MANAGER::instance().RemoveItem(item);

					return false;
				}

				if (true == LearnSkillByBook(dwVnum))
				{
					item->SetCount(item->GetCount() - 1);

					int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);

					if (distribution_test_server)
						iReadDelay /= 3;

					//ÇÑ±¹ º»¼·ÀÇ °æ¿ì¿¡´Â ½Ã°£À» 24½Ã°£ °íÁ¤
					if (LC_IsKorea())
						iReadDelay = 86400;

					SetSkillNextReadTime(dwVnum, get_global_time() + iReadDelay);
				}
			}
			break;

		case ITEM_USE:
			{
				if (item->GetVnum() > 50800 && item->GetVnum() <= 50820)
				{
					if (test_server)
						sys_log (0, "ADD addtional effect : vnum(%d) subtype(%d)", item->GetOriginalVnum(), item->GetSubType());

					int affect_type = AFFECT_EXP_BONUS_EURO_FREE;
					int apply_type = aApplyInfo[item->GetValue(0)].bPointType;
					int apply_value = item->GetValue(2);
					int apply_duration = item->GetValue(1);

					switch (item->GetSubType())
					{
						case USE_ABILITY_UP:
							if (FindAffect(affect_type, apply_type))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
								return false;
							}

							{
								switch (item->GetValue(0))
								{
									case APPLY_MOV_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_MOV_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_ATT_SPEED:
										AddAffect(affect_type, apply_type, apply_value, AFF_ATT_SPEED_POTION, apply_duration, 0, true, true);
										break;

									case APPLY_STR:
									case APPLY_DEX:
									case APPLY_CON:
									case APPLY_INT:
									case APPLY_CAST_SPEED:
									case APPLY_RESIST_MAGIC:
									case APPLY_ATT_GRADE_BONUS:
									case APPLY_DEF_GRADE_BONUS:
										AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, true, true);
										break;
								}
							}

							if (GetDungeon())
								GetDungeon()->UsePotion(this);

							if (GetWarMap())
								GetWarMap()->UsePotion(this, item);

							item->SetCount(item->GetCount() - 1);
							break;

					case USE_AFFECT :
						{
							if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
							}
							else
							{
								// PC_BANG_ITEM_ADD
								if (item->IsPCBangItem() == true)
								{
									// PC¹æÀÎÁö Ã¼Å©ÇØ¼­ Ã³¸®
									if (CPCBangManager::instance().IsPCBangIP(GetDesc()->GetHostName()) == false)
									{
										// PC¹æÀÌ ¾Æ´Ô!
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº PC¹æ¿¡¼­¸¸ »ç¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
										return false;
									}
								}
								// END_PC_BANG_ITEM_ADD

								AddAffect(AFFECT_EXP_BONUS_EURO_FREE, aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false, true);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
										break;
								}
							}

							bool used = false;

							if (item->GetValue(0) != 0) // HP Àý´ë°ª È¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP Àý´ë°ª È¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP % È¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP % È¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿ùº´ ¶Ç´Â Á¾ÀÚ ¸¦ »ç¿ëÇÏ¿´½À´Ï´Ù"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;
					}

					return true;
				}


				if (item->GetVnum() >= 27863 && item->GetVnum() <= 27883)
				{
					if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
						return false;
					}
				}

				if (test_server)
				{
					 sys_log (0, "USE_ITEM %s Type %d SubType %d vnum %d", item->GetName(), item->GetType(), item->GetSubType(), item->GetOriginalVnum());
				}

				switch (item->GetSubType())
				{
					case USE_TIME_CHARGE_PER:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}
							// ¿ì¼± ¿ëÈ¥¼®¿¡ °üÇØ¼­¸¸ ÇÏµµ·Ï ÇÑ´Ù.
							if (pDestItem->IsDragonSoul())
							{
								int ret;
								char buf[128];
								if (item->GetVnum() == DRAGON_HEART_VNUM)
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
								}
								else
								{
									ret = pDestItem->GiveMoreTime_Per((float)item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								}
								if (ret > 0)
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "Inc %ds by item{VN:%d SOC%d:%d}", ret, item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "Inc %ds by item{VN:%d VAL%d:%d}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÃÊ ¸¸Å­ ÃæÀüµÇ¾ú½À´Ï´Ù."), ret);
									item->SetCount(item->GetCount() - 1);
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									return true;
								}
								else
								{
									if (item->GetVnum() == DRAGON_HEART_VNUM)
									{
										sprintf(buf, "No change by item{VN:%d SOC%d:%d}", item->GetVnum(), ITEM_SOCKET_CHARGING_AMOUNT_IDX, item->GetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX));
									}
									else
									{
										sprintf(buf, "No change by item{VN:%d VAL%d:%d}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÃæÀüÇÒ ¼ö ¾ø½À´Ï´Ù."));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_TIME_CHARGE_FIX:
						{
							LPITEM pDestItem = GetItem(DestCell);
							if (NULL == pDestItem)
							{
								return false;
							}
							// ¿ì¼± ¿ëÈ¥¼®¿¡ °üÇØ¼­¸¸ ÇÏµµ·Ï ÇÑ´Ù.
							if (pDestItem->IsDragonSoul())
							{
								int ret = pDestItem->GiveMoreTime_Fix(item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
								char buf[128];
								if (ret)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÃÊ ¸¸Å­ ÃæÀüµÇ¾ú½À´Ï´Ù."), ret);
									sprintf(buf, "Increase %ds by item{VN:%d VAL%d:%d}", ret, item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_SUCCESS", buf);
									item->SetCount(item->GetCount() - 1);
									return true;
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÃæÀüÇÒ ¼ö ¾ø½À´Ï´Ù."));
									sprintf(buf, "No change by item{VN:%d VAL%d:%d}", item->GetVnum(), ITEM_VALUE_CHARGING_AMOUNT_IDX, item->GetValue(ITEM_VALUE_CHARGING_AMOUNT_IDX));
									LogManager::instance().ItemLog(this, item, "DS_CHARGING_FAILED", buf);
									return false;
								}
							}
							else
								return false;
						}
						break;
					case USE_SPECIAL:
						
						switch (item->GetVnum())
						{
							//Å©¸®½º¸¶½º ¶õÁÖ
							case ITEM_NOG_POCKET:
								{
									/*
									¶õÁÖ´É·ÂÄ¡ : item_proto value ÀÇ¹Ì
										ÀÌµ¿¼Óµµ  value 1
										°ø°Ý·Â	  value 2
										°æÇèÄ¡    value 3
										Áö¼Ó½Ã°£  value 0 (´ÜÀ§ ÃÊ)

									*/
									if (FindAffect(AFFECT_NOG_ABILITY))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
										return false;
									}
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_NOG_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;
								
							//¶ó¸¶´Ü¿ë »çÅÁ
							case ITEM_RAMADAN_CANDY:
								{
									/*
									»çÅÁ´É·ÂÄ¡ : item_proto value ÀÇ¹Ì
										ÀÌµ¿¼Óµµ  value 1
										°ø°Ý·Â	  value 2
										°æÇèÄ¡    value 3
										Áö¼Ó½Ã°£  value 0 (´ÜÀ§ ÃÊ)

									*/
									long time = item->GetValue(0);
									long moveSpeedPer	= item->GetValue(1);
									long attPer	= item->GetValue(2);
									long expPer			= item->GetValue(3);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MOV_SPEED, moveSpeedPer, AFF_MOV_SPEED_POTION, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_ATTBONUS, attPer, AFF_NONE, time, 0, true, true);
									AddAffect(AFFECT_RAMADAN_ABILITY, POINT_MALL_EXPBONUS, expPer, AFF_NONE, time, 0, true, true);
									item->SetCount(item->GetCount() - 1);
								}
								break;
							case ITEM_MARRIAGE_RING:
								{
									marriage::TMarriage* pMarriage = marriage::CManager::instance().Get(GetPlayerID());
									if (pMarriage)
									{
										if (pMarriage->ch1 != NULL)
										{
											if (CArenaManager::instance().IsArenaMap(pMarriage->ch1->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
												break;
											}
										}

										if (pMarriage->ch2 != NULL)
										{
											if (CArenaManager::instance().IsArenaMap(pMarriage->ch2->GetMapIndex()) == true)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
												break;
											}
										}

										int consumeSP = CalculateConsumeSP(this);

										if (consumeSP < 0)
											return false;

										PointChange(POINT_SP, -consumeSP, false);

										WarpToPID(pMarriage->GetOther(GetPlayerID()));
									}
									else
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°áÈ¥ »óÅÂ°¡ ¾Æ´Ï¸é °áÈ¥¹ÝÁö¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
								}
								break;

								//±âÁ¸ ¿ë±âÀÇ ¸ÁÅä
							case UNIQUE_ITEM_CAPE_OF_COURAGE:
								//¶ó¸¶´Ü º¸»ó¿ë ¿ë±âÀÇ ¸ÁÅä
							case 70057:
							case REWARD_BOX_UNIQUE_ITEM_CAPE_OF_COURAGE:
								AggregateMonster();
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_WHITE_FLAG:
								ForgetMyAttacker();
								item->SetCount(item->GetCount()-1);
								break;

							case UNIQUE_ITEM_TREASURE_BOX:
								break;

							case 30093:
							case 30094:
							case 30095:
							case 30096:
								// º¹ÁÖ¸Ó´Ï
								{
									const int MAX_BAG_INFO = 26;
									static struct LuckyBagInfo
									{
										DWORD count;
										int prob;
										DWORD vnum;
									} b1[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27003 },
										{ 10,	100,	27005 },
										{ 10,	50,	27006 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};

									struct LuckyBagInfo b2[MAX_BAG_INFO] =
									{
										{ 1000,	302,	1 },
										{ 10,	150,	27002 },
										{ 10,	75,	27002 },
										{ 10,	100,	27005 },
										{ 10,	50,	27005 },
										{ 10,	80,	27001 },
										{ 10,	50,	27002 },
										{ 10,	80,	27004 },
										{ 10,	50,	27005 },
										{ 1,	10,	50300 },
										{ 1,	6,	92 },
										{ 1,	2,	132 },
										{ 1,	6,	1052 },
										{ 1,	2,	1092 },
										{ 1,	6,	2082 },
										{ 1,	2,	2122 },
										{ 1,	6,	3082 },
										{ 1,	2,	3122 },
										{ 1,	6,	5052 },
										{ 1,	2,	5082 },
										{ 1,	6,	7082 },
										{ 1,	2,	7122 },
										{ 1,	1,	11282 },
										{ 1,	1,	11482 },
										{ 1,	1,	11682 },
										{ 1,	1,	11882 },
									};
	
									LuckyBagInfo * bi = NULL;
									if (LC_IsHongKong())
										bi = b2;
									else
										bi = b1;

									int pct = number(1, 1000);

									int i;
									for (i=0;i<MAX_BAG_INFO;i++)
									{
										if (pct <= bi[i].prob)
											break;
										pct -= bi[i].prob;
									}
									if (i>=MAX_BAG_INFO)
										return false;

									if (bi[i].vnum == 50300)
									{
										// ½ºÅ³¼ö·Ã¼­´Â Æ¯¼öÇÏ°Ô ÁØ´Ù.
										GiveRandomSkillBook();
									}
									else if (bi[i].vnum == 1)
									{
										PointChange(POINT_GOLD, 1000, true);
									}
									else
									{
										AutoGiveItem(bi[i].vnum, bi[i].count);
									}
									ITEM_MANAGER::instance().RemoveItem(item);
								}
								break;

							case 50004: // ÀÌº¥Æ®¿ë °¨Áö±â
								{
									if (item->GetSocket(0))
									{
										item->SetSocket(0, item->GetSocket(0) + 1);
									}
									else
									{
										// Ã³À½ »ç¿ë½Ã
										int iMapIndex = GetMapIndex();

										PIXEL_POSITION pos;

										if (SECTREE_MANAGER::instance().GetRandomLocation(iMapIndex, pos, 700))
										{
											item->SetSocket(0, 1);
											item->SetSocket(1, pos.x);
											item->SetSocket(2, pos.y);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ °÷¿¡¼± ÀÌº¥Æ®¿ë °¨Áö±â°¡ µ¿ÀÛÇÏÁö ¾Ê´Â°Í °°½À´Ï´Ù."));
											return false;
										}
									}

									int dist = 0;
									float distance = (DISTANCE_SQRT(GetX()-item->GetSocket(1), GetY()-item->GetSocket(2)));

									if (distance < 1000.0f)
									{
										// ¹ß°ß!
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌº¥Æ®¿ë °¨Áö±â°¡ ½Åºñ·Î¿î ºûÀ» ³»¸ç »ç¶óÁý´Ï´Ù."));

										// »ç¿ëÈ½¼ö¿¡ µû¶ó ÁÖ´Â ¾ÆÀÌÅÛÀ» ´Ù¸£°Ô ÇÑ´Ù.
										struct TEventStoneInfo
										{
											DWORD dwVnum;
											int count;
											int prob;
										};
										const int EVENT_STONE_MAX_INFO = 15;
										TEventStoneInfo info_10[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  8 },
											{ 27004, 10,  6 },
											{ 27002, 10, 12 },
											{ 27005, 10, 12 },
											{ 27100,  1,  9 },
											{ 27103,  1,  9 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 12 },

											{ 25040,  1,  4 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1,  0 },

											{ 50001,  1,  0 },
											{ 50003,  1,  1 },
										};
										TEventStoneInfo info_7[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  1 },
											{ 27004, 10,  1 },
											{ 27004, 10,  9 },
											{ 27005, 10,  9 },
											{ 27100,  1,  5 },
											{ 27103,  1,  5 },
											{ 27101,  1, 10 },
											{ 27104,  1, 10 },
											{ 27999,  1, 14 },

											{ 25040,  1,  5 },

											{ 27410,  1,  5 },
											{ 27600,  1,  5 },
											{ 25100,  1,  5 },

											{ 50001,  1,  0 },
											{ 50003,  1,  5 },

										};
										TEventStoneInfo info_4[EVENT_STONE_MAX_INFO] =
										{ 
											{ 27001, 10,  0 },
											{ 27004, 10,  0 },
											{ 27002, 10,  0 },
											{ 27005, 10,  0 },
											{ 27100,  1,  0 },
											{ 27103,  1,  0 },
											{ 27101,  1,  0 },
											{ 27104,  1,  0 },
											{ 27999,  1, 25 },

											{ 25040,  1,  0 },

											{ 27410,  1,  0 },
											{ 27600,  1,  0 },
											{ 25100,  1, 15 },

											{ 50001,  1, 10 },
											{ 50003,  1, 50 },

										};

										{
											TEventStoneInfo* info;
											if (item->GetSocket(0) <= 4)
												info = info_4;
											else if (item->GetSocket(0) <= 7)
												info = info_7;
											else
												info = info_10;

											int prob = number(1, 100);

											for (int i = 0; i < EVENT_STONE_MAX_INFO; ++i)
											{
												if (!info[i].prob)
													continue;

												if (prob <= info[i].prob)
												{
													if (info[i].dwVnum == 50001)
													{
														DWORD * pdw = M2_NEW DWORD[2];

														pdw[0] = info[i].dwVnum;
														pdw[1] = info[i].count;

														// ÃßÃ·¼­´Â ¼ÒÄÏÀ» ¼³Á¤ÇÑ´Ù
														DBManager::instance().ReturnQuery(QID_LOTTO, GetPlayerID(), pdw,
																"INSERT INTO lotto_list VALUES(0, 'server%s', %u, NOW())", 
																get_table_postfix(), GetPlayerID());
													}
													else
														AutoGiveItem(info[i].dwVnum, info[i].count);

													break;
												}
												prob -= info[i].prob;
											}
										}

										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf), "StoneDetect %u 0 0", (DWORD)GetVID());

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // \0 ¹®ÀÚ±îÁö º¸³»±â

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id	= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());

										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 1");
										return true;
									}
									else if (distance < 20000)
										dist = 1;
									else if (distance < 70000)
										dist = 2;
									else
										dist = 3;

									// ¸¹ÀÌ »ç¿ëÇßÀ¸¸é »ç¶óÁø´Ù.
									const int STONE_DETECT_MAX_TRY = 10;
									if (item->GetSocket(0) >= STONE_DETECT_MAX_TRY)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌº¥Æ®¿ë °¨Áö±â°¡ ÈçÀûµµ ¾øÀÌ »ç¶óÁý´Ï´Ù."));
										ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (DETECT_EVENT_STONE) 0");
										AutoGiveItem(27002);
										return true;
									}

									if (dist)
									{
										char chatbuf[CHAT_MAX_LEN + 1];
										int len = snprintf(chatbuf, sizeof(chatbuf),
												"StoneDetect %u %d %d",
											   	(DWORD)GetVID(), dist, (int)GetDegreeFromPositionXY(GetX(), item->GetSocket(2), item->GetSocket(1), GetY()));

										if (len < 0 || len >= (int) sizeof(chatbuf))
											len = sizeof(chatbuf) - 1;

										++len;  // \0 ¹®ÀÚ±îÁö º¸³»±â

										TPacketGCChat pack_chat;
										pack_chat.header	= HEADER_GC_CHAT;
										pack_chat.size		= sizeof(TPacketGCChat) + len;
										pack_chat.type		= CHAT_TYPE_COMMAND;
										pack_chat.id		= 0;
										pack_chat.bEmpire	= GetDesc()->GetEmpire();
										//pack_chat.id		= vid;

										TEMP_BUFFER buf;
										buf.write(&pack_chat, sizeof(TPacketGCChat));
										buf.write(chatbuf, len);

										PacketAround(buf.read_peek(), buf.size());
									}

								}
								break;

							case 27989: // ¿µ¼®°¨Áö±â
							case 76006: // ¼±¹°¿ë ¿µ¼®°¨Áö±â
								{
									LPSECTREE_MAP pMap = SECTREE_MANAGER::instance().GetMap(GetMapIndex());

									if (pMap != NULL)
									{
										item->SetSocket(0, item->GetSocket(0) + 1);

										FFindStone f;

										// <Factor> SECTREE::for_each -> SECTREE::for_each_entity
										pMap->for_each(f);

										if (f.m_mapStone.size() > 0)
										{
											std::map<DWORD, LPCHARACTER>::iterator stone = f.m_mapStone.begin();

											DWORD max = UINT_MAX;
											LPCHARACTER pTarget = stone->second;

											while (stone != f.m_mapStone.end())
											{
												DWORD dist = (DWORD)DISTANCE_SQRT(GetX()-stone->second->GetX(), GetY()-stone->second->GetY());

												if (dist != 0 && max > dist)
												{
													max = dist;
													pTarget = stone->second;
												}
												stone++;
											}

											if (pTarget != NULL)
											{
												int val = 3;

												if (max < 10000) val = 2;
												else if (max < 70000) val = 1;

												ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u %d %d", (DWORD)GetVID(), val,
														(int)GetDegreeFromPositionXY(GetX(), pTarget->GetY(), pTarget->GetX(), GetY()));
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°¨Áö±â¸¦ ÀÛ¿ëÇÏ¿´À¸³ª °¨ÁöµÇ´Â ¿µ¼®ÀÌ ¾ø½À´Ï´Ù."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°¨Áö±â¸¦ ÀÛ¿ëÇÏ¿´À¸³ª °¨ÁöµÇ´Â ¿µ¼®ÀÌ ¾ø½À´Ï´Ù."));
										}

										if (item->GetSocket(0) >= 6)
										{
											ChatPacket(CHAT_TYPE_COMMAND, "StoneDetect %u 0 0", (DWORD)GetVID());
											ITEM_MANAGER::instance().RemoveItem(item);
										}
									}
									break;
								}
								break;

							case 27996: // µ¶º´
								item->SetCount(item->GetCount() - 1);
								/*if (GetSkillLevel(SKILL_CREATE_POISON))
								  AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE, 3, AFF_DRINK_POISON, 15*60, 0, true);
								  else
								  {
								// µ¶´Ù·ç±â°¡ ¾øÀ¸¸é 50% Áï»ç 50% °ø°Ý·Â +2
								if (number(0, 1))
								{
								if (GetHP() > 100)
								PointChange(POINT_HP, -(GetHP() - 1));
								else
								Dead();
								}
								else
								AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE, 2, AFF_DRINK_POISON, 15*60, 0, true);
								}*/
								break;

							case 27987: // Á¶°³
								// 50  µ¹Á¶°¢ 47990
								// 30  ²Î
								// 10  ¹éÁøÁÖ 47992
								// 7   Ã»ÁøÁÖ 47993
								// 3   ÇÇÁøÁÖ 47994
								{
									item->SetCount(item->GetCount() - 1);

									int r = number(1, 100);

									if (r <= 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ µ¹Á¶°¢ÀÌ ³ª¿Ô½À´Ï´Ù."));
										AutoGiveItem(27990);
									}
									else
									{
										const int prob_table_euckr[] =
										{
											80, 90, 97
										};

										const int prob_table_gb2312[] =
										{
											95, 97, 99
										};

										const int * prob_table = !g_iUseLocale ? prob_table_euckr : prob_table_gb2312;

										if (r <= prob_table[0])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³°¡ ÈçÀûµµ ¾øÀÌ »ç¶óÁý´Ï´Ù."));
										}
										else if (r <= prob_table[1])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ ¹éÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27992);
										}
										else if (r <= prob_table[2])
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ Ã»ÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27993);
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á¶°³¿¡¼­ ÇÇÁøÁÖ°¡ ³ª¿Ô½À´Ï´Ù."));
											AutoGiveItem(27994);
										}
									}
								}
								break;

							case 71013: // ÃàÁ¦¿ëÆøÁ×
								CreateFly(number(FLY_FIREWORK1, FLY_FIREWORK6), this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50100: // ÆøÁ×
							case 50101:
							case 50102:
							case 50103:
							case 50104:
							case 50105:
							case 50106:
								CreateFly(item->GetVnum() - 50100 + FLY_FIREWORK1, this);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50200: // º¸µû¸®
								if (LC_IsYMIR() == true || LC_IsKorea() == true)
								{
									if (IS_BOTARYABLE_ZONE(GetMapIndex()) == true)
									{
										__OpenPrivateShop();
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³ÀÎ »óÁ¡À» ¿­ ¼ö ¾ø´Â Áö¿ªÀÔ´Ï´Ù"));
									}
								}
								else
								{
									__OpenPrivateShop();
								}
								break;

							case fishing::FISH_MIND_PILL_VNUM:
								AddAffect(AFFECT_FISH_MIND_PILL, POINT_NONE, 0, AFF_FISH_MIND, 20*60, 0, true);
								item->SetCount(item->GetCount() - 1);
								break;

							case 50301: // Åë¼Ö·Â ¼ö·Ã¼­
							case 50302:
							case 50303:
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									int lv = GetSkillLevel(SKILL_LEADERSHIP);

									if (lv < item->GetValue(0))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
										return false;
									}

									if (lv >= item->GetValue(1))
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ¾Æ¹«¸® ºÁµµ µµ¿òÀÌ µÉ °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(SKILL_LEADERSHIP))
									{
										ITEM_MANAGER::instance().RemoveItem(item);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_LEADERSHIP, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50304: // ¿¬°è±â ¼ö·Ã¼­
							case 50305:
							case 50306:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									if (GetSkillLevel(SKILL_COMBO) == 0 && GetLevel() < 30)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·¹º§ 30ÀÌ µÇ±â Àü¿¡´Â ½ÀµæÇÒ ¼ö ÀÖÀ» °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) == 1 && GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·¹º§ 50ÀÌ µÇ±â Àü¿¡´Â ½ÀµæÇÒ ¼ö ÀÖÀ» °Í °°Áö ¾Ê½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(SKILL_COMBO) >= 2)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿¬°è±â´Â ´õÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									int iPct = item->GetValue(0);

									if (LearnSkillByBook(SKILL_COMBO, iPct))
									{
										ITEM_MANAGER::instance().RemoveItem(item);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(SKILL_COMBO, get_global_time() + iReadDelay);
									}
								}
								break;
							case 50311: // ¾ð¾î ¼ö·Ã¼­
							case 50312:
							case 50313:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);
									if (GetSkillLevel(dwSkillVnum)>=20 || dwSkillVnum-SKILL_LANGUAGE1+1 == GetEmpire())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì ¿Ïº®ÇÏ°Ô ¾Ë¾ÆµéÀ» ¼ö ÀÖ´Â ¾ð¾îÀÌ´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::instance().RemoveItem(item);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50061 : // ÀÏº» ¸» ¼ÒÈ¯ ½ºÅ³ ¼ö·Ã¼­
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = item->GetValue(0);
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum) >= 10)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::instance().RemoveItem(item);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50314: case 50315: case 50316: // º¯½Å ¼ö·Ã¼­
							case 50323: case 50324: // ÁõÇ÷ ¼ö·Ã¼­
							case 50325: case 50326: // Ã¶Åë ¼ö·Ã¼­
								{
									if (IsPolymorphed() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â ´É·ÂÀ» ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}
									
									int iSkillLevelLowLimit = item->GetValue(0);
									int iSkillLevelHighLimit = item->GetValue(1);
									int iPct = MINMAX(0, item->GetValue(2), 100);
									int iLevelLimit = item->GetValue(3);
									DWORD dwSkillVnum = 0;
									
									switch (item->GetVnum())
									{
										case 50314: case 50315: case 50316:
											dwSkillVnum = SKILL_POLYMORPH;
											break;

										case 50323: case 50324:
											dwSkillVnum = SKILL_ADD_HP;
											break;

										case 50325: case 50326:
											dwSkillVnum = SKILL_RESIST_PENETRATE;
											break;

										default:
											return false;
									}

									if (0 == dwSkillVnum)
										return false;

									if (GetLevel() < iLevelLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥À» ÀÐÀ¸·Á¸é ·¹º§À» ´õ ¿Ã·Á¾ß ÇÕ´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= 40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) < iSkillLevelLowLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥Àº ³Ê¹« ¾î·Á¿ö ÀÌÇØÇÏ±â°¡ Èûµì´Ï´Ù."));
										return false;
									}

									if (GetSkillLevel(dwSkillVnum) >= iSkillLevelHighLimit)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ Ã¥À¸·Î´Â ´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::instance().RemoveItem(item);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;

							case 50902:
							case 50903:
							case 50904:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = SKILL_CREATE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::instance().RemoveItem(item);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);

										if (test_server) 
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Success to learn skill ");
										}
									}
									else
									{
										if (test_server) 
										{
											ChatPacket(CHAT_TYPE_INFO, "[TEST_SERVER] Failed to learn skill ");
										}
									}
								}
								break;

								// MINING
							case ITEM_MINING_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = SKILL_MINING;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetSkillLevel(dwSkillVnum)>=40)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ¼ö·ÃÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (LearnSkillByBook(dwSkillVnum, iPct))
									{
										ITEM_MANAGER::instance().RemoveItem(item);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
								}
								break;
								// END_OF_MINING

							case ITEM_HORSE_SKILL_TRAIN_BOOK:
								{
									if (IsPolymorphed())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯½ÅÁß¿¡´Â Ã¥À» ÀÐÀ»¼ö ¾ø½À´Ï´Ù."));
										return false;
										
									}
									DWORD dwSkillVnum = SKILL_HORSE;
									int iPct = MINMAX(0, item->GetValue(1), 100);

									if (GetLevel() < 50)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ ½Â¸¶ ½ºÅ³À» ¼ö·ÃÇÒ ¼ö ÀÖ´Â ·¹º§ÀÌ ¾Æ´Õ´Ï´Ù."));
										return false;
									}

									if (!test_server && get_global_time() < GetSkillNextReadTime(dwSkillVnum))
									{
										if (FindAffect(AFFECT_SKILL_NO_BOOK_DELAY))
										{
											// ÁÖ¾È¼ú¼­ »ç¿ëÁß¿¡´Â ½Ã°£ Á¦ÇÑ ¹«½Ã
											RemoveAffect(AFFECT_SKILL_NO_BOOK_DELAY);
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÁÖ¾È¼ú¼­¸¦ ÅëÇØ ÁÖÈ­ÀÔ¸¶¿¡¼­ ºüÁ®³ª¿Ô½À´Ï´Ù."));
										}
										else
										{
											SkillLearnWaitMoreTimeMessage(GetSkillNextReadTime(dwSkillVnum) - get_global_time());
											return false;
										}
									}

									if (GetPoint(POINT_HORSE_SKILL) >= 20 || 
											GetSkillLevel(SKILL_HORSE_WILDATTACK) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60 ||
											GetSkillLevel(SKILL_HORSE_WILDATTACK_RANGE) + GetSkillLevel(SKILL_HORSE_CHARGE) + GetSkillLevel(SKILL_HORSE_ESCAPE) >= 60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ½Â¸¶ ¼ö·Ã¼­¸¦ ÀÐÀ» ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									if (number(1, 100) <= iPct)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½Â¸¶ ¼ö·Ã¼­¸¦ ÀÐ¾î ½Â¸¶ ½ºÅ³ Æ÷ÀÎÆ®¸¦ ¾ò¾ú½À´Ï´Ù."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾òÀº Æ÷ÀÎÆ®·Î´Â ½Â¸¶ ½ºÅ³ÀÇ ·¹º§À» ¿Ã¸± ¼ö ÀÖ½À´Ï´Ù."));
										PointChange(POINT_HORSE_SKILL, 1);

										int iReadDelay = number(SKILLBOOK_DELAY_MIN, SKILLBOOK_DELAY_MAX);
										if (distribution_test_server) iReadDelay /= 3;

										if (!test_server)
											SetSkillNextReadTime(dwSkillVnum, get_global_time() + iReadDelay);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("½Â¸¶ ¼ö·Ã¼­ ÀÌÇØ¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
									}

									ITEM_MANAGER::instance().RemoveItem(item);
								}
								break;

							case 70102: // ¼±µÎ
							case 70103: // ¼±µÎ
								{
									if (GetAlignment() >= 0)
										return false;

									int delta = MIN(-GetAlignment(), item->GetValue(0));

									sys_log(0, "%s ALIGNMENT ITEM %d", GetName(), delta);

									UpdateAlignment(delta);
									item->SetCount(item->GetCount() - 1);

									if (delta / 10 > 0)
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¸¶À½ÀÌ ¸¼¾ÆÁö´Â±º. °¡½¿À» Áþ´©¸£´ø ¹«¾ð°¡°¡ Á» °¡º­¿öÁø ´À³¦ÀÌ¾ß."));
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼±¾ÇÄ¡°¡ %d Áõ°¡ÇÏ¿´½À´Ï´Ù."), delta/10);
									}
								}
								break;

							case 71107: // Ãµµµº¹¼þ¾Æ
								{
									int val = item->GetValue(0);
									int interval = item->GetValue(1);
									quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());
									int last_use_time = pPC->GetFlag("mythical_peach.last_use_time");

									if (get_global_time() - last_use_time < interval * 60 * 60)
									{
										if (test_server == false)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
											return false;
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Å×½ºÆ® ¼­¹ö ½Ã°£Á¦ÇÑ Åë°ú"));
										}
									}
									
									if (GetAlignment() == 200000)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼±¾ÇÄ¡¸¦ ´õ ÀÌ»ó ¿Ã¸± ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}
									
									if (200000 - GetAlignment() < val * 10)
									{
										val = (200000 - GetAlignment()) / 10;
									}
									
									int old_alignment = GetAlignment() / 10;

									UpdateAlignment(val*10);
									
									item->SetCount(item->GetCount()-1);
									pPC->SetFlag("mythical_peach.last_use_time", get_global_time());

									ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¸¶À½ÀÌ ¸¼¾ÆÁö´Â±º. °¡½¿À» Áþ´©¸£´ø ¹«¾ð°¡°¡ Á» °¡º­¿öÁø ´À³¦ÀÌ¾ß."));
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼±¾ÇÄ¡°¡ %d Áõ°¡ÇÏ¿´½À´Ï´Ù."), val);

									char buf[256 + 1];
									snprintf(buf, sizeof(buf), "%d %d", old_alignment, GetAlignment() / 10);
									LogManager::instance().CharLog(this, val, "MYTHICAL_PEACH", buf);
								}
								break;

							case 71109: // Å»¼®¼­
							case 72719:
								{
									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetSocketCount() == 0)
										return false;

									switch( item2->GetType() )
									{
										case ITEM_WEAPON:
											break;
										case ITEM_ARMOR:
											switch (item2->GetSubType())
											{
											case ARMOR_EAR:
											case ARMOR_WRIST:
											case ARMOR_NECK:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»©³¾ ¿µ¼®ÀÌ ¾ø½À´Ï´Ù"));
												return false;
											}
											break;

										default:
											return false;
									}

									std::stack<long> socket;

									for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										socket.push(item2->GetSocket(i));

									int idx = ITEM_SOCKET_MAX_NUM - 1;

									while (socket.size() > 0)
									{
										if (socket.top() > 2 && socket.top() != ITEM_BROKEN_METIN_VNUM)
											break;

										idx--;
										socket.pop();
									}

									if (socket.size() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»©³¾ ¿µ¼®ÀÌ ¾ø½À´Ï´Ù"));
										return false;
									}

									LPITEM pItemReward = AutoGiveItem(socket.top());

									if (pItemReward != NULL)
									{
										item2->SetSocket(idx, 1);

										char buf[256+1];
										snprintf(buf, sizeof(buf), "%s(%u) %s(%u)", 
												item2->GetName(), item2->GetID(), pItemReward->GetName(), pItemReward->GetID());
										LogManager::instance().ItemLog(this, item, "USE_DETACHMENT_ONE", buf);

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case 70201:   // Å»»öÁ¦
							case 70202:   // ¿°»ö¾à(Èò»ö)
							case 70203:   // ¿°»ö¾à(±Ý»ö)
							case 70204:   // ¿°»ö¾à(»¡°£»ö)
							case 70205:   // ¿°»ö¾à(°¥»ö)
							case 70206:   // ¿°»ö¾à(°ËÀº»ö)
								{
									// NEW_HAIR_STYLE_ADD
									if (GetPart(PART_HAIR) >= 1001)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÇöÀç Çì¾î½ºÅ¸ÀÏ¿¡¼­´Â ¿°»ö°ú Å»»öÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
									}
									// END_NEW_HAIR_STYLE_ADD
									else
									{
										quest::CQuestManager& q = quest::CQuestManager::instance();
										quest::PC* pPC = q.GetPC(GetPlayerID());

										if (pPC)
										{
											int last_dye_level = pPC->GetFlag("dyeing_hair.last_dye_level");

											if (last_dye_level == 0 ||
													last_dye_level+3 <= GetLevel() ||
													item->GetVnum() == 70201)
											{
												SetPart(PART_HAIR, item->GetVnum() - 70201);

												if (item->GetVnum() == 70201)
													pPC->SetFlag("dyeing_hair.last_dye_level", 0);
												else
													pPC->SetFlag("dyeing_hair.last_dye_level", GetLevel());

												item->SetCount(item->GetCount() - 1);
												UpdatePacket();
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%d ·¹º§ÀÌ µÇ¾î¾ß ´Ù½Ã ¿°»öÇÏ½Ç ¼ö ÀÖ½À´Ï´Ù."), last_dye_level+3);
											}
										}
									}
								}
								break;

							case ITEM_NEW_YEAR_GREETING_VNUM:
								{
									DWORD dwBoxVnum = ITEM_NEW_YEAR_GREETING_VNUM;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets;
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int i = 0; i < count; i++)
										{
											if (dwVnums[i] == CSpecialItemGroup::GOLD)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
										}

										item->SetCount(item->GetCount() - 1);
									}
								}
								break;

							case ITEM_VALENTINE_ROSE:
							case ITEM_VALENTINE_CHOCOLATE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(NULL);
									int count = 0;


									if (item->GetVnum() == ITEM_VALENTINE_ROSE && SEX_MALE==GET_SEX(this) ||
										item->GetVnum() == ITEM_VALENTINE_CHOCOLATE && SEX_FEMALE==GET_SEX(this))
									{
										// ¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ¾µ ¼ö ¾ø´Ù.
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}


									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
										item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WHITEDAY_CANDY:
							case ITEM_WHITEDAY_ROSE:
								{
									DWORD dwBoxVnum = item->GetVnum();
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(NULL);
									int count = 0;


									if (item->GetVnum() == ITEM_WHITEDAY_CANDY && SEX_MALE==GET_SEX(this) ||
										item->GetVnum() == ITEM_WHITEDAY_ROSE && SEX_FEMALE==GET_SEX(this))
									{
										// ¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ¾µ ¼ö ¾ø´Ù.
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» ¿­ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}


									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
										item->SetCount(item->GetCount()-1);
								}
								break;

							case 50011: // ¿ù±¤º¸ÇÕ
								{
									DWORD dwBoxVnum = 50011;
									std::vector <DWORD> dwVnums;
									std::vector <DWORD> dwCounts;
									std::vector <LPITEM> item_gets(NULL);
									int count = 0;

									if (GiveItemFromSpecialItemGroup(dwBoxVnum, dwVnums, dwCounts, item_gets, count))
									{
										for (int i = 0; i < count; i++)
										{
											char buf[50 + 1];
											snprintf(buf, sizeof(buf), "%u %u", dwVnums[i], dwCounts[i]);
											LogManager::instance().ItemLog(this, item, "MOONLIGHT_GET", buf);

											//ITEM_MANAGER::instance().RemoveItem(item);
											item->SetCount(item->GetCount() - 1);

											switch (dwVnums[i])
											{
											case CSpecialItemGroup::GOLD:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
												break;

											case CSpecialItemGroup::EXP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ºÎÅÍ ½ÅºñÇÑ ºûÀÌ ³ª¿É´Ï´Ù."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("%dÀÇ °æÇèÄ¡¸¦ È¹µæÇß½À´Ï´Ù."), dwCounts[i]);
												break;

											case CSpecialItemGroup::MOB:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
												break;

											case CSpecialItemGroup::SLOW:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â »¡°£ ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ ¿òÁ÷ÀÌ´Â ¼Óµµ°¡ ´À·ÁÁ³½À´Ï´Ù!"));
												break;

											case CSpecialItemGroup::DRAIN_HP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ°¡ °©ÀÚ±â Æø¹ßÇÏ¿´½À´Ï´Ù! »ý¸í·ÂÀÌ °¨¼ÒÇß½À´Ï´Ù."));
												break;

											case CSpecialItemGroup::POISON:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ³ª¿Â ³ì»ö ¿¬±â¸¦ µéÀÌ¸¶½ÃÀÚ µ¶ÀÌ ¿Â¸öÀ¸·Î ÆÛÁý´Ï´Ù!"));
												break;

											case CSpecialItemGroup::MOB_GROUP:
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ ¸ó½ºÅÍ°¡ ³ªÅ¸³µ½À´Ï´Ù!"));
												break;

											default:
												if (item_gets[i])
												{
													if (dwCounts[i] > 1)
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ %d °³ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName(), dwCounts[i]);
													else
														ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»óÀÚ¿¡¼­ %s °¡ ³ª¿Ô½À´Ï´Ù."), item_gets[i]->GetName());
												}
												break;
											}
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_TALKING, LC_TEXT("¾Æ¹«°Íµµ ¾òÀ» ¼ö ¾ø¾ú½À´Ï´Ù."));
										return false;
									}
								}
								break;

							case ITEM_GIVE_STAT_RESET_COUNT_VNUM:
								{
									//PointChange(POINT_GOLD, -iCost);
									PointChange(POINT_STAT_RESET_COUNT, 1);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50107:
								{
									EffectPacket(SE_CHINA_FIREWORK);
									// ½ºÅÏ °ø°ÝÀ» ¿Ã·ÁÁØ´Ù
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case 50108:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
										return false;
									}

									EffectPacket(SE_SPIN_TOP);
									// ½ºÅÏ °ø°ÝÀ» ¿Ã·ÁÁØ´Ù
									AddAffect(AFFECT_CHINA_FIREWORK, POINT_STUN_PCT, 30, AFF_CHINA_FIREWORK, 5*60, 0, true);
									item->SetCount(item->GetCount()-1);
								}
								break;

							case ITEM_WONSO_BEAN_VNUM:
								PointChange(POINT_HP, GetMaxHP() - GetHP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_SUGAR_VNUM:
								PointChange(POINT_SP, GetMaxSP() - GetSP());
								item->SetCount(item->GetCount()-1);
								break;

							case ITEM_WONSO_FRUIT_VNUM:
								PointChange(POINT_STAMINA, GetMaxStamina()-GetStamina());
								item->SetCount(item->GetCount()-1);
								break;

							case 90008: // VCARD
							case 90009: // VCARD
								VCardUse(this, this, item);
								break;

							case ITEM_ELK_VNUM: // µ·²Ù·¯¹Ì
								{
									int iGold = item->GetSocket(0);
									ITEM_MANAGER::instance().RemoveItem(item);
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ· %d ³ÉÀ» È¹µæÇß½À´Ï´Ù."), iGold);
									PointChange(POINT_GOLD, iGold);
								}
								break;

								//±ºÁÖÀÇ ÁõÇ¥ 
							case 70021:
								{
									int HealPrice = quest::CQuestManager::instance().GetEventFlag("MonarchHealGold");
									if (HealPrice == 0)
										HealPrice = 2000000;

									if (CMonarch::instance().HealMyEmpire(this, HealPrice))
									{
										char szNotice[256];
										snprintf(szNotice, sizeof(szNotice), LC_TEXT("±ºÁÖÀÇ Ãàº¹À¸·Î ÀÌÁö¿ª %s À¯Àú´Â HP,SP°¡ ¸ðµÎ Ã¤¿öÁý´Ï´Ù."), EMPIRE_NAME(GetEmpire()));
										SendNoticeMap(szNotice, GetMapIndex(), false);
										
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁÖÀÇ Ãàº¹À» »ç¿ëÇÏ¿´½À´Ï´Ù."));
									}
								}
								break;

							case 27995:
								{
								}
								break;

							case 71092 : // º¯½Å ÇØÃ¼ºÎ ÀÓ½Ã
								{
									if (m_pkChrTarget != NULL)
									{
										if (m_pkChrTarget->IsPolymorphed())
										{
											m_pkChrTarget->SetPolymorph(0);
											m_pkChrTarget->RemoveAffect(AFFECT_POLYMORPH);
										}
									}
									else
									{
										if (IsPolymorphed())
										{
											SetPolymorph(0);
											RemoveAffect(AFFECT_POLYMORPH);
										}
									}
								}
								break;

							case 71051 : // ÁøÀç°¡
								{
									// À¯·´, ½Ì°¡Æú, º£Æ®³² ÁøÀç°¡ »ç¿ë±ÝÁö
									//if (LC_IsEurope() || LC_IsSingapore() || LC_IsVietnam())
										return false;

									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetInventoryItem(wDestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetType() == ITEM_COSTUME) //6-7 javítás kosztümön
									{
										return false;
									}

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->AddRareAttribute() == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼º°øÀûÀ¸·Î ¼Ó¼ºÀÌ Ãß°¡ µÇ¾ú½À´Ï´Ù"));

										int iAddedIdx = item2->GetRareAttrCount() + 4;
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										LogManager::instance().ItemLog(
												GetPlayerID(),
												item2->GetAttributeType(iAddedIdx),
												item2->GetAttributeValue(iAddedIdx),
												item->GetID(),
												"ADD_RARE_ATTR",
												buf,
												GetDesc()->GetHostName(),
												item->GetOriginalVnum());

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ¸·Î ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù"));
									}
								}
								break;

							case 71052 : // ÁøÀç°æ
								{
									// À¯·´, ½Ì°¡Æú, º£Æ®³² ÁøÀç°¡ »ç¿ë±ÝÁö
									//if (LC_IsEurope() || LC_IsSingapore() || LC_IsVietnam())
										return false;

									LPITEM item2;

									if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
										return false;

									if (item2->IsExchanging() == true)
										return false;

									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->ChangeRareAttribute() == true)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_RARE_ATTR", buf);

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æ ½ÃÅ³ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù"));
									}
								}
								break;

							case ITEM_AUTO_HP_RECOVERY_S:
							case ITEM_AUTO_HP_RECOVERY_M:
							case ITEM_AUTO_HP_RECOVERY_L:
							case ITEM_AUTO_HP_RECOVERY_X:
							case ITEM_AUTO_SP_RECOVERY_S:
							case ITEM_AUTO_SP_RECOVERY_M:
							case ITEM_AUTO_SP_RECOVERY_L:
							case ITEM_AUTO_SP_RECOVERY_X:
							// ¹«½Ã¹«½ÃÇÏÁö¸¸ ÀÌÀü¿¡ ÇÏ´ø °É °íÄ¡±â´Â ¹«¼·°í...
							// ±×·¡¼­ ±×³É ÇÏµå ÄÚµù. ¼±¹° »óÀÚ¿ë ÀÚµ¿¹°¾à ¾ÆÀÌÅÛµé.
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS: 
							case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S: 
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS: 
							case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
							case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
								{
									if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									EAffectTypes type = AFFECT_NONE;
									bool isSpecialPotion = false;

									switch (item->GetVnum())
									{
										case ITEM_AUTO_HP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_HP_RECOVERY_S:
										case ITEM_AUTO_HP_RECOVERY_M:
										case ITEM_AUTO_HP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_HP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_HP_RECOVERY_S:
											type = AFFECT_AUTO_HP_RECOVERY;
											break;

										case ITEM_AUTO_SP_RECOVERY_X:
											isSpecialPotion = true;

										case ITEM_AUTO_SP_RECOVERY_S:
										case ITEM_AUTO_SP_RECOVERY_M:
										case ITEM_AUTO_SP_RECOVERY_L:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_XS:
										case REWARD_BOX_ITEM_AUTO_SP_RECOVERY_S:
										case FUCKING_BRAZIL_ITEM_AUTO_SP_RECOVERY_S:
											type = AFFECT_AUTO_SP_RECOVERY;
											break;
									}

									if (AFFECT_NONE == type)
										break;

									if (item->GetCount() > 1)
									{
										int pos = GetEmptyInventory(item->GetSize());

										if (-1 == pos)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù."));
											break;
										}

										item->SetCount( item->GetCount() - 1 );

										LPITEM item2 = ITEM_MANAGER::instance().CreateItem( item->GetVnum(), 1 );
										item2->AddToCharacter(this, TItemPos(INVENTORY, pos));

										if (item->GetSocket(1) != 0)
										{
											item2->SetSocket(1, item->GetSocket(1));
										}

										item = item2;
									}

									CAffect* pAffect = FindAffect( type );

									if (NULL == pAffect)
									{
										EPointTypes bonus = POINT_NONE;

										if (true == isSpecialPotion)
										{
											if (type == AFFECT_AUTO_HP_RECOVERY)
											{
												bonus = POINT_MAX_HP_PCT;
											}
											else if (type == AFFECT_AUTO_SP_RECOVERY)
											{
												bonus = POINT_MAX_SP_PCT;
											}
										}

										AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

										item->Lock(true);
										item->SetSocket(0, true);

										AutoRecoveryItemProcess( type );
									}
									else
									{
										if (item->GetID() == pAffect->dwFlag)
										{
											RemoveAffect( pAffect );

											item->Lock(false);
											item->SetSocket(0, false);
										}
										else
										{
											LPITEM old = FindItemByID( pAffect->dwFlag );

											if (NULL != old)
											{
												old->Lock(false);
												old->SetSocket(0, false);
											}

											RemoveAffect( pAffect );

											EPointTypes bonus = POINT_NONE;

											if (true == isSpecialPotion)
											{
												if (type == AFFECT_AUTO_HP_RECOVERY)
												{
													bonus = POINT_MAX_HP_PCT;
												}
												else if (type == AFFECT_AUTO_SP_RECOVERY)
												{
													bonus = POINT_MAX_SP_PCT;
												}
											}

											AddAffect( type, bonus, 4, item->GetID(), INFINITE_AFFECT_DURATION, 0, true, false);

											item->Lock(true);
											item->SetSocket(0, true);

											AutoRecoveryItemProcess( type );
										}
									}
								}
								break;
						}
						break;

					case USE_CLEAR:
						{
							RemoveBadAffect();
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_INVISIBILITY:
						{
							if (item->GetVnum() == 70026)
							{
								quest::CQuestManager& q = quest::CQuestManager::instance();
								quest::PC* pPC = q.GetPC(GetPlayerID());

								if (pPC != NULL)
								{
									int last_use_time = pPC->GetFlag("mirror_of_disapper.last_use_time");

									if (get_global_time() - last_use_time < 10*60)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
										return false;
									}

									pPC->SetFlag("mirror_of_disapper.last_use_time", get_global_time());
								}
							}

							AddAffect(AFFECT_INVISIBILITY, POINT_NONE, 0, AFF_INVISIBILITY, 300, 0, true);
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_POTION_NODELAY:
						{
							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
								}

								switch (item->GetVnum())
								{
									case 70020 :
									case 71018 :
									case 71019 :
									case 71020 :
										if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
										{
											if (m_nPotionLimit <= 0)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
												return false;
											}
										}
										break;

									default :
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
										return false;
								}
							}

							bool used = false;

							if (item->GetValue(0) != 0) // HP Àý´ë°ª È¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(0) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(1) != 0)	// SP Àý´ë°ª È¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(1) * (100 + GetPoint(POINT_POTION_BONUS)) / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (item->GetValue(3) != 0) // HP % È¸º¹
							{
								if (GetHP() < GetMaxHP())
								{
									PointChange(POINT_HP, item->GetValue(3) * GetMaxHP() / 100);
									EffectPacket(SE_HPUP_RED);
									used = TRUE;
								}
							}

							if (item->GetValue(4) != 0) // SP % È¸º¹
							{
								if (GetSP() < GetMaxSP())
								{
									PointChange(POINT_SP, item->GetValue(4) * GetMaxSP() / 100);
									EffectPacket(SE_SPUP_BLUE);
									used = TRUE;
								}
							}

							if (used)
							{
								if (item->GetVnum() == 50085 || item->GetVnum() == 50086)
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿ùº´ ¶Ç´Â Á¾ÀÚ ¸¦ »ç¿ëÇÏ¿´½À´Ï´Ù"));
									SetUseSeedOrMoonBottleTime();
								}
								if (GetDungeon())
									GetDungeon()->UsePotion(this);

								if (GetWarMap())
									GetWarMap()->UsePotion(this, item);

								m_nPotionLimit--;

								//RESTRICT_USE_SEED_OR_MOONBOTTLE
								item->SetCount(item->GetCount() - 1);
								//END_RESTRICT_USE_SEED_OR_MOONBOTTLE
							}
						}
						break;

					case USE_POTION:
						if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
						{
							if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit") > 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}
						
							switch (item->GetVnum())
							{
								case 27001 :
								case 27002 :
								case 27003 :
								case 27004 :
								case 27005 :
								case 27006 :
									if (quest::CQuestManager::instance().GetEventFlag("arena_potion_limit_count") < 10000)
									{
										if (m_nPotionLimit <= 0)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ë Á¦ÇÑ·®À» ÃÊ°úÇÏ¿´½À´Ï´Ù."));
											return false;
										}
									}
									break;

								default :
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·ÃÀå¿¡¼­ »ç¿ëÇÏ½Ç ¼ö ¾ø½À´Ï´Ù."));
									return false;
							}
						}
						
						if (item->GetValue(1) != 0)
						{
							if (GetPoint(POINT_SP_RECOVERY) + GetSP() >= GetMaxSP())
							{
								return false;
							}

							PointChange(POINT_SP_RECOVERY, item->GetValue(1) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_SPUP_BLUE);
						}

						if (item->GetValue(0) != 0)
						{
							if (GetPoint(POINT_HP_RECOVERY) + GetHP() >= GetMaxHP())
							{
								return false;
							}

							PointChange(POINT_HP_RECOVERY, item->GetValue(0) * MIN(200, (100 + GetPoint(POINT_POTION_BONUS))) / 100);
							StartAffectEvent();
							EffectPacket(SE_HPUP_RED);
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						m_nPotionLimit--;
						break;

					case USE_POTION_CONTINUE:
						{
							if (item->GetValue(0) != 0)
							{
								AddAffect(AFFECT_HP_RECOVER_CONTINUE, POINT_HP_RECOVER_CONTINUE, item->GetValue(0), 0, item->GetValue(2), 0, true);
							}
							else if (item->GetValue(1) != 0)
							{
								AddAffect(AFFECT_SP_RECOVER_CONTINUE, POINT_SP_RECOVER_CONTINUE, item->GetValue(1), 0, item->GetValue(2), 0, true);
							}
							else
								return false;
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_ABILITY_UP:
						{
							switch (item->GetValue(0))
							{
								case APPLY_MOV_SPEED:
									AddAffect(AFFECT_MOV_SPEED, POINT_MOV_SPEED, item->GetValue(2), AFF_MOV_SPEED_POTION, item->GetValue(1), 0, true);
									break;

								case APPLY_ATT_SPEED:
									AddAffect(AFFECT_ATT_SPEED, POINT_ATT_SPEED, item->GetValue(2), AFF_ATT_SPEED_POTION, item->GetValue(1), 0, true);
									break;

								case APPLY_STR:
									AddAffect(AFFECT_STR, POINT_ST, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEX:
									AddAffect(AFFECT_DEX, POINT_DX, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CON:
									AddAffect(AFFECT_CON, POINT_HT, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_INT:
									AddAffect(AFFECT_INT, POINT_IQ, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_CAST_SPEED:
									AddAffect(AFFECT_CAST_SPEED, POINT_CASTING_SPEED, item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_ATT_GRADE_BONUS:
									AddAffect(AFFECT_ATT_GRADE, POINT_ATT_GRADE_BONUS, 
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;

								case APPLY_DEF_GRADE_BONUS:
									AddAffect(AFFECT_DEF_GRADE, POINT_DEF_GRADE_BONUS,
											item->GetValue(2), 0, item->GetValue(1), 0, true);
									break;
							}
						}

						if (GetDungeon())
							GetDungeon()->UsePotion(this);

						if (GetWarMap())
							GetWarMap()->UsePotion(this, item);

						item->SetCount(item->GetCount() - 1);
						break;

					case USE_TALISMAN:
						{
							const int TOWN_PORTAL	= 1;
							const int MEMORY_PORTAL = 2;


							// gm_guild_build, oxevent ¸Ê¿¡¼­ ±ÍÈ¯ºÎ ±ÍÈ¯±â¾ïºÎ ¸¦ »ç¿ë¸øÇÏ°Ô ¸·À½
							if (GetMapIndex() == 200 || GetMapIndex() == 113)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÇöÀç À§Ä¡¿¡¼­ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ë·Ã Áß¿¡´Â ÀÌ¿ëÇÒ ¼ö ¾ø´Â ¹°Ç°ÀÔ´Ï´Ù."));
								return false;
							}

							if (m_pkWarpEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌµ¿ÇÒ ÁØºñ°¡ µÇ¾îÀÖÀ½À¸·Î ±ÍÈ¯ºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù"));
								return false;
							}

							// CONSUME_LIFE_WHEN_USE_WARP_ITEM
							int consumeLife = CalculateConsume(this);

							if (consumeLife < 0)
								return false;
							// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

							if (item->GetValue(0) == TOWN_PORTAL) // ±ÍÈ¯ºÎ
							{
								if (item->GetSocket(0) == 0)
								{
									if (!GetDungeon())
										if (!GiveRecallItem(item))
											return false;

									PIXEL_POSITION posWarp;

									if (SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp))
									{
										// CONSUME_LIFE_WHEN_USE_WARP_ITEM
										PointChange(POINT_HP, -consumeLife, false);
										// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

										WarpSet(posWarp.x, posWarp.y);
									}
									else
									{
										sys_err("CHARACTER::UseItem : cannot find spawn position (name %s, %d x %d)", GetName(), GetX(), GetY());
									}
								}
								else
								{
									if (test_server)
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿ø·¡ À§Ä¡·Î º¹±Í"));	

									ProcessRecallItem(item);
								}
							}
							else if (item->GetValue(0) == MEMORY_PORTAL) // ±ÍÈ¯±â¾ïºÎ
							{
								if (item->GetSocket(0) == 0)
								{
									if (GetDungeon())
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´øÀü ¾È¿¡¼­´Â %s%s »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."),
												item->GetName(),
												g_iUseLocale ? "" : (under_han(item->GetName()) ? LC_TEXT("À»") : LC_TEXT("¸¦")));
										return false;
									}

									if (!GiveRecallItem(item))
										return false;
								}
								else
								{
									// CONSUME_LIFE_WHEN_USE_WARP_ITEM
									PointChange(POINT_HP, -consumeLife, false);
									// END_OF_CONSUME_LIFE_WHEN_USE_WARP_ITEM

									ProcessRecallItem(item);
								}
							}
						}
						break;

					case USE_TUNING:
					case USE_DETACHMENT:
						{
							LPITEM item2;

							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsExchanging())
								return false;
	
							if (item2->GetVnum() >= 28330 && item2->GetVnum() <= 28343) // ¿µ¼®+3
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("+3 ¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù"));
								return false;
							}
							
							if (item2->GetVnum() >= 28430 && item2->GetVnum() <= 28443)  // ¿µ¼®+4
							{
								if (item->GetVnum() == 71056) // Ã»·æÀÇ¼û°á
								{
									RefineItem(item, item2);
								}
								else
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¿µ¼®Àº ÀÌ ¾ÆÀÌÅÛÀ¸·Î °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù"));
								}
							}
							else
							{
								RefineItem(item, item2);
							}
						}
						break;

						//  ACCESSORY_REFINE & ADD/CHANGE_ATTRIBUTES
					case USE_PUT_INTO_BELT_SOCKET:
					case USE_PUT_INTO_RING_SOCKET:
					case USE_PUT_INTO_ACCESSORY_SOCKET:
					case USE_ADD_ACCESSORY_SOCKET:
					case USE_CLEAN_SOCKET:
					case USE_CHANGE_ATTRIBUTE:
					case USE_CHANGE_ATTRIBUTE2 :
					case USE_ADD_ATTRIBUTE:
					case USE_ADD_ATTRIBUTE2:
						{
							LPITEM item2;
							if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
								return false;

							if (item2->IsEquipped())
							{
								BuffOnAttr_RemoveBuffsFromItem(item2);
							}

							// [NOTE] ÄÚ½ºÆ¬ ¾ÆÀÌÅÛ¿¡´Â ¾ÆÀÌÅÛ ÃÖÃÊ »ý¼º½Ã ·£´ý ¼Ó¼ºÀ» ºÎ¿©ÇÏµÇ, Àç°æÀç°¡ µîµîÀº ¸·¾Æ´Þ¶ó´Â ¿äÃ»ÀÌ ÀÖ¾úÀ½.
							// ¿ø·¡ ANTI_CHANGE_ATTRIBUTE °°Àº ¾ÆÀÌÅÛ Flag¸¦ Ãß°¡ÇÏ¿© ±âÈ¹ ·¹º§¿¡¼­ À¯¿¬ÇÏ°Ô ÄÁÆ®·Ñ ÇÒ ¼ö ÀÖµµ·Ï ÇÒ ¿¹Á¤ÀÌ¾úÀ¸³ª
							// ±×µý°Å ÇÊ¿ä¾øÀ¸´Ï ´ÚÄ¡°í »¡¸® ÇØ´Þ·¡¼­ ±×³É ¿©±â¼­ ¸·À½... -_-
							if (ITEM_COSTUME == item2->GetType())
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
								return false;
							}

							if (item2->IsExchanging())
								return false;

							if (item2->IsEquipped())
								return false;

							switch (item->GetSubType())
							{
								case USE_CLEAN_SOCKET:
									{
										int i;
										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) == ITEM_BROKEN_METIN_VNUM)
												break;
										}

										if (i == ITEM_SOCKET_MAX_NUM)
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ã»¼ÒÇÒ ¼®ÀÌ ¹ÚÇôÀÖÁö ¾Ê½À´Ï´Ù."));
											return false;
										}

										int j = 0;

										for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
										{
											if (item2->GetSocket(i) != ITEM_BROKEN_METIN_VNUM && item2->GetSocket(i) != 0)
												item2->SetSocket(j++, item2->GetSocket(i));
										}

										for (; j < ITEM_SOCKET_MAX_NUM; ++j)
										{
											if (item2->GetSocket(j) > 0)
												item2->SetSocket(j, 1);
										}

										{
											char buf[21];
											snprintf(buf, sizeof(buf), "%u", item2->GetID());
											LogManager::instance().ItemLog(this, item, "CLEAN_SOCKET", buf);
										}

										item->SetCount(item->GetCount() - 1);

									}
									break;

								case USE_CHANGE_ATTRIBUTE :
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->GetAttributeCount() == 0)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("º¯°æÇÒ ¼Ó¼ºÀÌ ¾ø½À´Ï´Ù."));
										return false;
									}

									if (GM_PLAYER == GetGMLevel() && false == test_server)
									{
										//
										// Event Flag ¸¦ ÅëÇØ ÀÌÀü¿¡ ¾ÆÀÌÅÛ ¼Ó¼º º¯°æÀ» ÇÑ ½Ã°£À¸·Î ºÎÅÍ ÃæºÐÇÑ ½Ã°£ÀÌ Èê·¶´ÂÁö °Ë»çÇÏ°í
										// ½Ã°£ÀÌ ÃæºÐÈ÷ Èê·¶´Ù¸é ÇöÀç ¼Ó¼ºº¯°æ¿¡ ´ëÇÑ ½Ã°£À» ¼³Á¤ÇØ ÁØ´Ù.
										//

										DWORD dwChangeItemAttrCycle = quest::CQuestManager::instance().GetEventFlag(msc_szChangeItemAttrCycleFlag);
										if (dwChangeItemAttrCycle < msc_dwDefaultChangeItemAttrCycle)
											dwChangeItemAttrCycle = msc_dwDefaultChangeItemAttrCycle;

										quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetPlayerID());

										if (pPC)
										{
											DWORD dwNowMin = get_global_time() / 60;

											//DWORD dwLastChangeItemAttrMin = pPC->GetFlag(msc_szLastChangeItemAttrFlag);

											//if (dwLastChangeItemAttrMin + dwChangeItemAttrCycle > dwNowMin)
											//{
												//ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» ¹Ù²ÛÁö %dºÐ ÀÌ³»¿¡´Â ´Ù½Ã º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù.(%d ºÐ ³²À½)"),
												//		dwChangeItemAttrCycle, dwChangeItemAttrCycle - (dwNowMin - dwLastChangeItemAttrMin));
												//return false;
											//}

											pPC->SetFlag(msc_szLastChangeItemAttrFlag, dwNowMin);
										}
									}

									if (item->GetSubType() == USE_CHANGE_ATTRIBUTE2)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] = 
										{
											0, 0, 30, 40, 3
										};

										item2->ChangeAttribute(aiChangeProb);
									}
									else if (item->GetVnum() == 76014)
									{
										int aiChangeProb[ITEM_ATTRIBUTE_MAX_LEVEL] = 
										{
											0, 10, 50, 39, 1
										};

										item2->ChangeAttribute(aiChangeProb);
									}

									else
									{
										// ¿¬Àç°æ Æ¯¼öÃ³¸®
										// Àý´ë·Î ¿¬Àç°¡ Ãß°¡ ¾ÈµÉ°Å¶ó ÇÏ¿© ÇÏµå ÄÚµùÇÔ.
										if (item->GetVnum() == 71151 || item->GetVnum() == 76023)
										{
											if ((item2->GetType() == ITEM_WEAPON)
												|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àû¿ë ·¹º§º¸´Ù ³ô¾Æ »ç¿ëÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«±â¿Í °©¿Ê¿¡¸¸ »ç¿ë °¡´ÉÇÕ´Ï´Ù."));
												break;
											}
										}
										item2->ChangeAttribute();
									}

									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÏ¿´½À´Ï´Ù."));
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());
										LogManager::instance().ItemLog(this, item, "CHANGE_ATTRIBUTE", buf);
									}

									item->SetCount(item->GetCount() - 1);
									break;

								case USE_ADD_ATTRIBUTE :
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									if (item2->GetAttributeCount() < 4)
									{
										// ¿¬Àç°¡ Æ¯¼öÃ³¸®
										// Àý´ë·Î ¿¬Àç°¡ Ãß°¡ ¾ÈµÉ°Å¶ó ÇÏ¿© ÇÏµå ÄÚµùÇÔ.
										if (item->GetVnum() == 71152 || item->GetVnum() == 76024)
										{
											if ((item2->GetType() == ITEM_WEAPON)
												|| (item2->GetType() == ITEM_ARMOR && item2->GetSubType() == ARMOR_BODY))
											{
												bool bCanUse = true;
												for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
												{
													if (item2->GetLimitType(i) == LIMIT_LEVEL && item2->GetLimitValue(i) > 40)
													{
														bCanUse = false;
														break;
													}
												}
												if (false == bCanUse)
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àû¿ë ·¹º§º¸´Ù ³ô¾Æ »ç¿ëÀÌ ºÒ°¡´ÉÇÕ´Ï´Ù."));
													break;
												}
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹«±â¿Í °©¿Ê¿¡¸¸ »ç¿ë °¡´ÉÇÕ´Ï´Ù."));
												break;
											}
										}
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

											int iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::instance().ItemLog(
													GetPlayerID(), 
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(), 
													"ADD_ATTRIBUTE_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum());
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
											LogManager::instance().ItemLog(this, item, "ADD_ATTRIBUTE_FAIL", buf);
										}

										item->SetCount(item->GetCount() - 1);
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									break;

								case USE_ADD_ATTRIBUTE2 :
									// Ãàº¹ÀÇ ±¸½½ 
									// Àç°¡ºñ¼­¸¦ ÅëÇØ ¼Ó¼ºÀ» 4°³ Ãß°¡ ½ÃÅ² ¾ÆÀÌÅÛ¿¡ ´ëÇØ¼­ ÇÏ³ªÀÇ ¼Ó¼ºÀ» ´õ ºÙ¿©ÁØ´Ù.
									if (item2->GetAttributeSetIndex() == -1)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼ºÀ» º¯°æÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										return false;
									}

									// ¼Ó¼ºÀÌ ÀÌ¹Ì 4°³ Ãß°¡ µÇ¾úÀ» ¶§¸¸ ¼Ó¼ºÀ» Ãß°¡ °¡´ÉÇÏ´Ù.
									if (item2->GetAttributeCount() == 4)
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (number(1, 100) <= aiItemAttributeAddPercent[item2->GetAttributeCount()])
										{
											item2->AddAttribute();
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));

											int iAddedIdx = item2->GetAttributeCount() - 1;
											LogManager::instance().ItemLog(
													GetPlayerID(),
													item2->GetAttributeType(iAddedIdx),
													item2->GetAttributeValue(iAddedIdx),
													item->GetID(),
													"ADD_ATTRIBUTE2_SUCCESS",
													buf,
													GetDesc()->GetHostName(),
													item->GetOriginalVnum());
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼Ó¼º Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
											LogManager::instance().ItemLog(this, item, "ADD_ATTRIBUTE2_FAIL", buf);
										}

										item->SetCount(item->GetCount() - 1);
									}
									else if (item2->GetAttributeCount() == 5)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´õ ÀÌ»ó ÀÌ ¾ÆÀÌÅÛÀ» ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡ÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									else if (item2->GetAttributeCount() < 4)
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ÕÀú Àç°¡ºñ¼­¸¦ ÀÌ¿ëÇÏ¿© ¼Ó¼ºÀ» Ãß°¡½ÃÄÑ ÁÖ¼¼¿ä."));
									}
									else
									{
										// wtf ?!
										sys_err("ADD_ATTRIBUTE2 : Item has wrong AttributeCount(%d)", item2->GetAttributeCount());
									}
									break;

								case USE_ADD_ACCESSORY_SOCKET:
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->IsAccessoryForSocket())
										{
											if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												if (number(1, 100) <= 50)
												{
													item2->SetAccessorySocketMaxGrade(item2->GetAccessorySocketMaxGrade() + 1);
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÄÏÀÌ ¼º°øÀûÀ¸·Î Ãß°¡µÇ¾ú½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_SUCCESS", buf);
												}
												else
												{
													ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÄÏ Ãß°¡¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
													LogManager::instance().ItemLog(this, item, "ADD_SOCKET_FAIL", buf);
												}

												item->SetCount(item->GetCount() - 1);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ¼ÒÄÏÀ» Ãß°¡ÇÒ °ø°£ÀÌ ¾ø½À´Ï´Ù."));
											}
										}
										else
										{
											ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀ¸·Î ¼ÒÄÏÀ» Ãß°¡ÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
										}
									}
									break;

								case USE_PUT_INTO_BELT_SOCKET:
								case USE_PUT_INTO_ACCESSORY_SOCKET:
									if (item2->IsAccessoryForSocket() && item->CanPutInto(item2))
									{
										char buf[21];
										snprintf(buf, sizeof(buf), "%u", item2->GetID());

										if (item2->GetAccessorySocketGrade() < item2->GetAccessorySocketMaxGrade())
										{
											if (number(1, 100) <= aiAccessorySocketPutPct[item2->GetAccessorySocketGrade()])
											{
												item2->SetAccessorySocketGrade(item2->GetAccessorySocketGrade() + 1);
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀåÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
												LogManager::instance().ItemLog(this, item, "PUT_SOCKET_SUCCESS", buf);
											}
											else
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀåÂø¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
												LogManager::instance().ItemLog(this, item, "PUT_SOCKET_FAIL", buf);
											}

											item->SetCount(item->GetCount() - 1);
										}
										else
										{
											if (item2->GetAccessorySocketMaxGrade() == 0)
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ÕÀú ´ÙÀÌ¾Æ¸óµå·Î ¾Ç¼¼¼­¸®¿¡ ¼ÒÄÏÀ» Ãß°¡ÇØ¾ßÇÕ´Ï´Ù."));
											else if (item2->GetAccessorySocketMaxGrade() < ITEM_ACCESSORY_SOCKET_MAX_NUM)
											{
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó ÀåÂøÇÒ ¼ÒÄÏÀÌ ¾ø½À´Ï´Ù."));
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("´ÙÀÌ¾Æ¸óµå·Î ¼ÒÄÏÀ» Ãß°¡ÇØ¾ßÇÕ´Ï´Ù."));
											}
											else
												ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾×¼¼¼­¸®¿¡´Â ´õÀÌ»ó º¸¼®À» ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
										}
									}
									else
									{
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀ» ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
									}
									break;
							}
							if (item2->IsEquipped())
							{
								BuffOnAttr_AddBuffsFromItem(item2);
							}
						}
						break;
						//  END_OF_ACCESSORY_REFINE & END_OF_ADD_ATTRIBUTES & END_OF_CHANGE_ATTRIBUTES

					case USE_BAIT:
						{

							if (m_pkFishingEvent)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³¬½Ã Áß¿¡ ¹Ì³¢¸¦ °¥¾Æ³¢¿ï ¼ö ¾ø½À´Ï´Ù."));
								return false;
							}

							LPITEM weapon = GetWear(WEAR_WEAPON);

							if (!weapon || weapon->GetType() != ITEM_ROD)
								return false;

							if (weapon->GetSocket(2))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì ²ÈÇôÀÖ´ø ¹Ì³¢¸¦ »©°í %s¸¦ ³¢¿ó´Ï´Ù."), item->GetName());
							}
							else
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³¬½Ã´ë¿¡ %s¸¦ ¹Ì³¢·Î ³¢¿ó´Ï´Ù."), item->GetName());
							}

							weapon->SetSocket(2, item->GetValue(0));
							item->SetCount(item->GetCount() - 1);
						}
						break;

					case USE_MOVE:
					case USE_TREASURE_BOX:
					case USE_MONEYBAG:
						break;

					case USE_AFFECT :
						{
							if (FindAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
							}
							else
							{
								// PC_BANG_ITEM_ADD
								if (item->IsPCBangItem() == true)
								{
									// PC¹æÀÎÁö Ã¼Å©ÇØ¼­ Ã³¸®
									if (CPCBangManager::instance().IsPCBangIP(GetDesc()->GetHostName()) == false)
									{
										// PC¹æÀÌ ¾Æ´Ô!
										ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº PC¹æ¿¡¼­¸¸ »ç¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
										return false;
									}
								}
								// END_PC_BANG_ITEM_ADD

								AddAffect(item->GetValue(0), aApplyInfo[item->GetValue(1)].bPointType, item->GetValue(2), 0, item->GetValue(3), 0, false);
								item->SetCount(item->GetCount() - 1);
							}
						}
						break;

					case USE_CREATE_STONE:
						AutoGiveItem(number(28000, 28013));
						item->SetCount(item->GetCount() - 1);
						break;

					// ¹°¾à Á¦Á¶ ½ºÅ³¿ë ·¹½ÃÇÇ Ã³¸®	
					case USE_RECIPE :
						{
							LPITEM pSource1 = FindSpecifyItem(item->GetValue(1));
							DWORD dwSourceCount1 = item->GetValue(2);

							LPITEM pSource2 = FindSpecifyItem(item->GetValue(3));
							DWORD dwSourceCount2 = item->GetValue(4);

							if (dwSourceCount1 != 0)
							{
								if (pSource1 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹°¾à Á¶ÇÕÀ» À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
									return false;
								}
							}

							if (dwSourceCount2 != 0)
							{
								if (pSource2 == NULL)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹°¾à Á¶ÇÕÀ» À§ÇÑ Àç·á°¡ ºÎÁ·ÇÕ´Ï´Ù."));
									return false;
								}
							}

							if (pSource1 != NULL)
							{
								if (pSource1->GetCount() < dwSourceCount1)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àç·á(%s)°¡ ºÎÁ·ÇÕ´Ï´Ù."), pSource1->GetName());
									return false;
								}

								pSource1->SetCount(pSource1->GetCount() - dwSourceCount1);
							}

							if (pSource2 != NULL)
							{
								if (pSource2->GetCount() < dwSourceCount2)
								{
									ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àç·á(%s)°¡ ºÎÁ·ÇÕ´Ï´Ù."), pSource2->GetName());
									return false;
								}

								pSource2->SetCount(pSource2->GetCount() - dwSourceCount2);
							}

							LPITEM pBottle = FindSpecifyItem(50901);

							if (!pBottle || pBottle->GetCount() < 1)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ºó º´ÀÌ ¸ðÀÚ¸¨´Ï´Ù."));
								return false;
							}

							pBottle->SetCount(pBottle->GetCount() - 1);

							if (number(1, 100) > item->GetValue(5))
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹°¾à Á¦Á¶¿¡ ½ÇÆÐÇß½À´Ï´Ù."));
								return false;
							}

							AutoGiveItem(item->GetValue(0));
						}
						break;
				}
			}
			break;

		case ITEM_METIN:
			{
				LPITEM item2;

				if (!IsValidItemPosition(DestCell) || !(item2 = GetItem(DestCell)))
					return false;

				if (item2->IsExchanging())
					return false;

				if (item2->GetType() == ITEM_PICK) return false;
				if (item2->GetType() == ITEM_ROD) return false;

				int i;

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				{
					DWORD dwVnum;   

					if ((dwVnum = item2->GetSocket(i)) <= 2)
						continue;

					TItemTable * p = ITEM_MANAGER::instance().GetTable(dwVnum);

					if (!p)
						continue;

					if (item->GetValue(5) == p->alValues[5])
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°°Àº Á¾·ùÀÇ ¸ÞÆ¾¼®Àº ¿©·¯°³ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}

				if (item2->GetType() == ITEM_ARMOR)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_BODY) || !IS_SET(item2->GetWearFlag(), WEARABLE_BODY))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¸ÞÆ¾¼®Àº Àåºñ¿¡ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}
				else if (item2->GetType() == ITEM_WEAPON)
				{
					if (!IS_SET(item->GetWearFlag(), WEARABLE_WEAPON))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¸ÞÆ¾¼®Àº ¹«±â¿¡ ºÎÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
						return false;
					}
				}
				else
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ºÎÂøÇÒ ¼ö ÀÖ´Â ½½·ÔÀÌ ¾ø½À´Ï´Ù."));
					return false;
				}

				for (i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
					if (item2->GetSocket(i) >= 1 && item2->GetSocket(i) <= 2 && item2->GetSocket(i) >= item->GetValue(2))
					{
						// ¼® È®·ü
						if (number(1, 100) <= 30)
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ÞÆ¾¼® ºÎÂø¿¡ ¼º°øÇÏ¿´½À´Ï´Ù."));
							item2->SetSocket(i, item->GetVnum());
						}
						else
						{
							ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸ÞÆ¾¼® ºÎÂø¿¡ ½ÇÆÐÇÏ¿´½À´Ï´Ù."));
							item2->SetSocket(i, ITEM_BROKEN_METIN_VNUM);
						}

						LogManager::instance().ItemLog(this, item2, "SOCKET", item->GetName());
						ITEM_MANAGER::instance().RemoveItem(item, "REMOVE (METIN)");
						break;
					}

				if (i == ITEM_SOCKET_MAX_NUM)
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ºÎÂøÇÒ ¼ö ÀÖ´Â ½½·ÔÀÌ ¾ø½À´Ï´Ù."));
			}
			break;

		case ITEM_AUTOUSE:
		case ITEM_MATERIAL:
		case ITEM_SPECIAL:
		case ITEM_TOOL:
		case ITEM_LOTTERY:
			break;

		case ITEM_TOTEM:
			{
				if (!item->IsEquipped())
					EquipItem(item);
			}
			break;

		case ITEM_BLEND:
			// »õ·Î¿î ¾àÃÊµé
			sys_log(0,"ITEM_BLEND!!");
			if (Blend_Item_find(item->GetVnum()))
			{
				int		affect_type		= AFFECT_BLEND;
				if (item->GetSocket(0) >= _countof(aApplyInfo))
				{
					sys_err ("INVALID BLEND ITEM(id : %d, vnum : %d). APPLY TYPE IS %d.", item->GetID(), item->GetVnum(), item->GetSocket(0));
					return false;
				}
				int		apply_type		= aApplyInfo[item->GetSocket(0)].bPointType;
				int		apply_value		= item->GetSocket(1);
				int		apply_duration	= item->GetSocket(2);
				
				if (FindAffect(affect_type, apply_type))
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
				}
				else
				{
					if (FindAffect(AFFECT_EXP_BONUS_EURO_FREE, POINT_RESIST_MAGIC))
					{
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì È¿°ú°¡ °É·Á ÀÖ½À´Ï´Ù."));
					}
					else
					{
						AddAffect(affect_type, apply_type, apply_value, 0, apply_duration, 0, false);
						item->SetCount(item->GetCount() - 1);
					}
				}
			}
			break;
		case ITEM_EXTRACT:
			{
				LPITEM pDestItem = GetItem(DestCell);
				if (NULL == pDestItem)
				{
					return false;
				}
				switch (item->GetSubType())
				{
				case EXTRACT_DRAGON_SOUL:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().PullOut(this, NPOS, pDestItem, item);
					}
					return false;
				case EXTRACT_DRAGON_HEART:
					if (pDestItem->IsDragonSoul())
					{
						return DSManager::instance().ExtractDragonHeart(this, pDestItem, item);
					}
					return false;
				default:
					return false;
				}
			}
			break;

		case ITEM_NONE:
			sys_err("Item type NONE %s", item->GetName());
			break;

		default:
			sys_log(0, "UseItemEx: Unknown type %s %d", item->GetName(), item->GetType());
			return false;
	}

	return true;
}

int g_nPortalLimitTime = 10;

bool CHARACTER::UseItem(TItemPos Cell, TItemPos DestCell)
{
	WORD wCell = Cell.cell;
	BYTE window_type = Cell.window_type;
	WORD wDestCell = DestCell.cell;
	BYTE bDestInven = DestCell.window_type;
	LPITEM item;

	if (!CanHandleItem())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
			return false;

	sys_log(0, "%s: USE_ITEM %s (inven %d, cell: %d)", GetName(), item->GetName(), window_type, wCell);

	if (item->IsExchanging())
		return false;

	if (!item->CanUsedBy(this))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±ºÁ÷ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (IsStun())
		return false;

	if (false == FN_check_item_sex(this, item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	//PREVENT_TRADE_WINDOW
	if (IS_SUMMON_ITEM(item->GetVnum()))
	{
		if (false == IS_SUMMONABLE_ZONE(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		// °æÈ¥¹ÝÁö »ç¿ëÁö »ó´ë¹æÀÌ SUMMONABLE_ZONE¿¡ ÀÖ´Â°¡´Â WarpToPC()¿¡¼­ Ã¼Å©
		
		//»ï°Å¸® °ü·Á ¸Ê¿¡¼­´Â ±ÍÈ¯ºÎ¸¦ ¸·¾Æ¹ö¸°´Ù.
		if (CThreeWayWar::instance().IsThreeWayWarMapIndex(GetMapIndex()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("»ï°Å¸® ÀüÅõ Âü°¡Áß¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
		int iPulse = thecore_pulse();

		//Ã¢°í ¿¬ÈÄ Ã¼Å©
		if (iPulse - GetSafeboxLoadTime() < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ã¢°í¸¦ ¿¬ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);

			if (test_server)
				ChatPacket(CHAT_TYPE_INFO, "[TestOnly]Pulse %d LoadTime %d PASS %d", iPulse, GetSafeboxLoadTime(), PASSES_PER_SEC(g_nPortalLimitTime));
			return false; 
		}

		//°Å·¡°ü·Ã Ã¢ Ã¼Å©
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°Å·¡Ã¢,Ã¢°í µîÀ» ¿¬ »óÅÂ¿¡¼­´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		//PREVENT_REFINE_HACK
		//°³·®ÈÄ ½Ã°£Ã¼Å© 
		{
			if (iPulse - GetRefineTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ °³·®ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
				return false;
			}
		}
		//END_PREVENT_REFINE_HACK
		

		//PREVENT_ITEM_COPY
		{
			if (iPulse - GetMyShopTime() < PASSES_PER_SEC(g_nPortalLimitTime))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°³ÀÎ»óÁ¡ »ç¿ëÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎ¸¦ »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
				return false;
			}
			
		}
		//END_PREVENT_ITEM_COPY
		

		//±ÍÈ¯ºÎ °Å¸®Ã¼Å©
		if (item->GetVnum() != 70302)
		{
			PIXEL_POSITION posWarp;

			int x = 0;
			int y = 0;

			double nDist = 0;
			const double nDistant = 5000.0;
			//±ÍÈ¯±â¾ïºÎ 
			if (item->GetVnum() == 22010)
			{
				x = item->GetSocket(0) - GetX();
				y = item->GetSocket(1) - GetY();
			}
			//±ÍÈ¯ºÎ
			else if (item->GetVnum() == 22000) 
			{
				SECTREE_MANAGER::instance().GetRecallPositionByEmpire(GetMapIndex(), GetEmpire(), posWarp);

				if (item->GetSocket(0) == 0)
				{
					x = posWarp.x - GetX();
					y = posWarp.y - GetY();
				}
				else
				{
					x = item->GetSocket(0) - GetX();
					y = item->GetSocket(1) - GetY();
				}
			}

			nDist = sqrt(pow((float)x,2) + pow((float)y,2));

			if (nDistant > nDist)
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌµ¿ µÇ¾îÁú À§Ä¡¿Í ³Ê¹« °¡±î¿ö ±ÍÈ¯ºÎ¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));				
				if (test_server)
					ChatPacket(CHAT_TYPE_INFO, "PossibleDistant %f nNowDist %f", nDistant,nDist); 
				return false;
			}
		}

		//PREVENT_PORTAL_AFTER_EXCHANGE
		//±³È¯ ÈÄ ½Ã°£Ã¼Å©
		if (iPulse - GetExchangeTime()  < PASSES_PER_SEC(g_nPortalLimitTime))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°Å·¡ ÈÄ %dÃÊ ÀÌ³»¿¡´Â ±ÍÈ¯ºÎ,±ÍÈ¯±â¾ïºÎµîÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."), g_nPortalLimitTime);
			return false;
		}
		//END_PREVENT_PORTAL_AFTER_EXCHANGE

	}

	//º¸µû¸® ºñ´Ü »ç¿ë½Ã °Å·¡Ã¢ Á¦ÇÑ Ã¼Å© 
	if (item->GetVnum() == 50200 | item->GetVnum() == 71049)
	{
		if (GetExchange() || GetMyShop() || GetShopOwner() || IsOpenSafebox() || IsCubeOpen())
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°Å·¡Ã¢,Ã¢°í µîÀ» ¿¬ »óÅÂ¿¡¼­´Â º¸µû¸®,ºñ´Üº¸µû¸®¸¦ »ç¿ëÇÒ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

	}
	//END_PREVENT_TRADE_WINDOW

	if (IS_SET(item->GetFlag(), ITEM_FLAG_LOG)) // »ç¿ë ·Î±×¸¦ ³²±â´Â ¾ÆÀÌÅÛ Ã³¸®
	{
		DWORD vid = item->GetVID();
		DWORD oldCount = item->GetCount();
		DWORD vnum = item->GetVnum();

		char hint[ITEM_NAME_MAX_LEN + 32 + 1];
		int len = snprintf(hint, sizeof(hint) - 32, "%s", item->GetName());

		if (len < 0 || len >= (int) sizeof(hint) - 32)
			len = (sizeof(hint) - 32) - 1;

		bool ret = UseItemEx(item, DestCell);

		if (NULL == ITEM_MANAGER::instance().FindByVID(vid)) // UseItemEx¿¡¼­ ¾ÆÀÌÅÛÀÌ »èÁ¦ µÇ¾ú´Ù. »èÁ¦ ·Î±×¸¦ ³²±è
		{
			LogManager::instance().ItemLog(this, vid, vnum, "REMOVE", hint);
		}
		else if (oldCount != item->GetCount())
		{
			snprintf(hint + len, sizeof(hint) - len, " %u", oldCount - 1);
			LogManager::instance().ItemLog(this, vid, vnum, "USE_ITEM", hint);
		}
		return (ret);
	}
	else
		return UseItemEx(item, DestCell);
}

bool CHARACTER::DropItem(TItemPos Cell, BYTE bCount)
{
	LPITEM item = NULL; 

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°­È­Ã¢À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (IsDead())
		return false;

	if (!IsValidItemPosition(Cell) || !(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (true == item->isLocked())
		return false;

	if (quest::CQuestManager::instance().GetPCForce(GetPlayerID())->IsRunning() == true)
		return false;

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP | ITEM_ANTIFLAG_GIVE))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹ö¸± ¼ö ¾ø´Â ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		return false;
	}

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, 255);	// Quickslot ¿¡¼­ Áö¿ò

	LPITEM pkItemToDrop;

	if (bCount == item->GetCount())
	{
		item->RemoveFromCharacter();
		pkItemToDrop = item;
	}
	else
	{
		if (bCount == 0)
		{
			if (test_server)
				sys_log(0, "[DROP_ITEM] drop item count == 0");
			return false;
		}
	
		// check non-split items for china
		//if (LC_IsNewCIBN())
		//	if (item->GetVnum() == 71095 || item->GetVnum() == 71050 || item->GetVnum() == 70038)
		//		return false;

		item->SetCount(item->GetCount() - bCount);
		ITEM_MANAGER::instance().FlushDelayedSave(item);

		pkItemToDrop = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), bCount);

		// copy item socket -- by mhh
		FN_copy_item_socket(pkItemToDrop, item);

		char szBuf[51 + 1];
		snprintf(szBuf, sizeof(szBuf), "%u %u", pkItemToDrop->GetID(), pkItemToDrop->GetCount());
		LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
	}

	PIXEL_POSITION pxPos = GetXYZ();

	if (pkItemToDrop->AddToGround(GetMapIndex(), pxPos))
	{
		// ÇÑ±¹¿¡´Â ¾ÆÀÌÅÛÀ» ¹ö¸®°í º¹±¸ÇØ´Þ¶ó´Â Áø»óÀ¯ÀúµéÀÌ ¸¹¾Æ¼­
		// ¾ÆÀÌÅÛÀ» ¹Ù´Ú¿¡ ¹ö¸± ½Ã ¼Ó¼º·Î±×¸¦ ³²±ä´Ù.
		if (LC_IsYMIR())
			item->AttrLog();

		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¶³¾îÁø ¾ÆÀÌÅÛÀº 3ºÐ ÈÄ »ç¶óÁý´Ï´Ù."));
		pkItemToDrop->StartDestroyEvent(10);

		ITEM_MANAGER::instance().FlushDelayedSave(pkItemToDrop);
		
		char szHint[32 + 1];
		snprintf(szHint, sizeof(szHint), "%s %u %u", pkItemToDrop->GetName(), pkItemToDrop->GetCount(), pkItemToDrop->GetOriginalVnum());
		LogManager::instance().ItemLog(this, pkItemToDrop, "DROP", szHint);
		//Motion(MOTION_PICKUP);
	}

	return true;
}

bool CHARACTER::DropGold(int gold)
{
	if (gold <= 0 || gold > GetGold())
		return false;

	if (!CanHandleItem())
		return false;

	if (0 != g_GoldDropTimeLimitValue)
	{
		if (get_dword_time() < m_dwLastGoldDropTime+g_GoldDropTimeLimitValue)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ °ñµå¸¦ ¹ö¸± ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}
	}

	m_dwLastGoldDropTime = get_dword_time();

	LPITEM item = ITEM_MANAGER::instance().CreateItem(1, gold);

	if (item)
	{
		PIXEL_POSITION pos = GetXYZ();

		if (item->AddToGround(GetMapIndex(), pos))
		{
			//Motion(MOTION_PICKUP);
			PointChange(POINT_GOLD, -gold, true);

			// ºê¶óÁú¿¡ µ·ÀÌ ¾ø¾îÁø´Ù´Â ¹ö±×°¡ ÀÖ´Âµ¥,
			// °¡´ÉÇÑ ½Ã³ª¸®¿À Áß¿¡ ÇÏ³ª´Â,
			// ¸ÞÅ©·Î³ª, ÇÙÀ» ½á¼­ 1000¿ø ÀÌÇÏÀÇ µ·À» °è¼Ó ¹ö·Á °ñµå¸¦ 0À¸·Î ¸¸µé°í, 
			// µ·ÀÌ ¾ø¾îÁ³´Ù°í º¹±¸ ½ÅÃ»ÇÏ´Â °ÍÀÏ ¼öµµ ÀÖ´Ù.
			// µû¶ó¼­ ±×·± °æ¿ì¸¦ Àâ±â À§ÇØ ³·Àº ¼öÄ¡ÀÇ °ñµå¿¡ ´ëÇØ¼­µµ ·Î±×¸¦ ³²±è.
			if (LC_IsBrazil() == true)
			{
				if (gold >= 213)
					LogManager::instance().CharLog(this, gold, "DROP_GOLD", "");
			}
			else
			{
				if (gold > 1000) // Ãµ¿ø ÀÌ»ó¸¸ ±â·ÏÇÑ´Ù.
					LogManager::instance().CharLog(this, gold, "DROP_GOLD", "");
			}

			if (false == LC_IsBrazil())
			{
				item->StartDestroyEvent(150);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¶³¾îÁø ¾ÆÀÌÅÛÀº %dºÐ ÈÄ »ç¶óÁý´Ï´Ù."), 150/60);
			}
			else
			{
				item->StartDestroyEvent(60);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¶³¾îÁø ¾ÆÀÌÅÛÀº %dºÐ ÈÄ »ç¶óÁý´Ï´Ù."), 1);
			}
		}

		Save();
		return true;
	}

	return false;
}

bool CHARACTER::MoveItem(TItemPos Cell, TItemPos DestCell, BYTE count)
{
	LPITEM item = NULL;

	if (!IsValidItemPosition(Cell))
		return false;

	if (!(item = GetItem(Cell)))
		return false;

	if (item->IsExchanging())
		return false;

	if (item->GetCount() < count)
		return false;

	if (INVENTORY == Cell.window_type && Cell.cell >= INVENTORY_MAX_NUM && IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	if (true == item->isLocked())
		return false;

	if (!IsValidItemPosition(DestCell))
	{
		return false;
	}

	if (!CanHandleItem())
	{
		if (NULL != DragonSoul_RefineWindow_GetOpener())
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°­È­Ã¢À» ¿¬ »óÅÂ¿¡¼­´Â ¾ÆÀÌÅÛÀ» ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	// ±âÈ¹ÀÚÀÇ ¿äÃ»À¸·Î º§Æ® ÀÎº¥Åä¸®¿¡´Â Æ¯Á¤ Å¸ÀÔÀÇ ¾ÆÀÌÅÛ¸¸ ³ÖÀ» ¼ö ÀÖ´Ù.
	if (DestCell.IsBeltInventoryPosition() && false == CBeltInventoryHelper::CanMoveIntoBeltInventory(item))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº º§Æ® ÀÎº¥Åä¸®·Î ¿Å±æ ¼ö ¾ø½À´Ï´Ù."));			
		return false;
	}

	if(DestCell.cell >= 256 && DestCell.window_type == INVENTORY)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("BELT_255_256"));			
		return false;
	}

	// ÀÌ¹Ì Âø¿ëÁßÀÎ ¾ÆÀÌÅÛÀ» ´Ù¸¥ °÷À¸·Î ¿Å±â´Â °æ¿ì, 'ÀåÃ¥ ÇØÁ¦' °¡´ÉÇÑ Áö È®ÀÎÇÏ°í ¿Å±è
	if (Cell.IsEquipPosition() && !CanUnequipNow(item))
		return false;

	if (DestCell.IsEquipPosition())
	{
		if (GetItem(DestCell))	// ÀåºñÀÏ °æ¿ì ÇÑ °÷¸¸ °Ë»çÇØµµ µÈ´Ù.
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì Àåºñ¸¦ Âø¿ëÇÏ°í ÀÖ½À´Ï´Ù."));
			
			return false;
		}

		EquipItem(item, DestCell.cell - INVENTORY_MAX_NUM);
	}
	else
	{
		if (item->IsDragonSoul())
		{
			if (item->IsEquipped())
			{
				return DSManager::instance().PullOut(this, DestCell, item);
			}
			else
			{
				if (DestCell.window_type != DRAGON_SOUL_INVENTORY)
				{
					return false;
				}

				if (!DSManager::instance().IsValidCellForThisItem(item, DestCell))
					return false;
			}
		}
		// ¿ëÈ¥¼®ÀÌ ¾Æ´Ñ ¾ÆÀÌÅÛÀº ¿ëÈ¥¼® ÀÎº¥¿¡ µé¾î°¥ ¼ö ¾ø´Ù.
		else if (DRAGON_SOUL_INVENTORY == DestCell.window_type)
			return false;

		LPITEM item2;

		if ((item2 = GetItem(DestCell)) && item != item2 && item2->IsStackable() &&
				!IS_SET(item2->GetAntiFlag(), ITEM_ANTIFLAG_STACK) &&
				item2->GetVnum() == item->GetVnum()) // ÇÕÄ¥ ¼ö ÀÖ´Â ¾ÆÀÌÅÛÀÇ °æ¿ì
		{
			for (int i = 0; i < ITEM_SOCKET_MAX_NUM; ++i)
				if (item2->GetSocket(i) != item->GetSocket(i))
					return false;

			if (count == 0)
				count = item->GetCount();

			sys_log(0, "%s: ITEM_STACK %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);

			count = MIN(200 - item2->GetCount(), count);

			item->SetCount(item->GetCount() - count);
			item2->SetCount(item2->GetCount() + count);
			return true;
		}

		if (!IsEmptyItemGrid(DestCell, item->GetSize(), Cell.cell))
			return false;

		if (count == 0 || count >= item->GetCount() || !item->IsStackable() || IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
		{
			sys_log(0, "%s: ITEM_MOVE %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);
			
			item->RemoveFromCharacter();
			SetItem(DestCell, item);

			if (INVENTORY == Cell.window_type && INVENTORY == DestCell.window_type)
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, Cell.cell, DestCell.cell);
		}
		else if (count < item->GetCount())
		{
			//check non-split items 
			//if (LC_IsNewCIBN())
			//{
			//	if (item->GetVnum() == 71095 || item->GetVnum() == 71050 || item->GetVnum() == 70038)
			//	{
			//		return false;
			//	}
			//}

			sys_log(0, "%s: ITEM_SPLIT %s (window: %d, cell : %d) -> (window:%d, cell %d) count %d", GetName(), item->GetName(), Cell.window_type, Cell.cell, 
				DestCell.window_type, DestCell.cell, count);

			item->SetCount(item->GetCount() - count);
			LPITEM item2 = ITEM_MANAGER::instance().CreateItem(item->GetVnum(), count);

			// copy socket -- by mhh
			FN_copy_item_socket(item2, item);

			item2->AddToCharacter(this, DestCell);

			char szBuf[51+1];
			snprintf(szBuf, sizeof(szBuf), "%u %u %u %u ", item2->GetID(), item2->GetCount(), item->GetCount(), item->GetCount() + item2->GetCount());
			LogManager::instance().ItemLog(this, item, "ITEM_SPLIT", szBuf);
		}
	}

	return true;
}

namespace NPartyPickupDistribute
{
	struct FFindOwnership
	{
		LPITEM item;
		LPCHARACTER owner;

		FFindOwnership(LPITEM item) 
			: item(item), owner(NULL)
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (item->IsOwnership(ch))
				owner = ch;
		}
	};

	struct FCountNearMember
	{
		int		total;
		int		x, y;

		FCountNearMember(LPCHARACTER center )
			: total(0), x(center->GetX()), y(center->GetY())
		{
		}

		void operator () (LPCHARACTER ch)
		{
			if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				total += 1;
		}
	};

	struct FMoneyDistributor
	{
		int		total;
		LPCHARACTER	c;
		int		x, y;
		int		iMoney;

		FMoneyDistributor(LPCHARACTER center, int iMoney) 
			: total(0), c(center), x(center->GetX()), y(center->GetY()), iMoney(iMoney) 
		{
		}

		void operator ()(LPCHARACTER ch)
		{
			if (ch!=c)
				if (DISTANCE_APPROX(ch->GetX() - x, ch->GetY() - y) <= PARTY_DEFAULT_RANGE)
				{
					ch->PointChange(POINT_GOLD, iMoney, true);

					if (iMoney > 1000) // Ãµ¿ø ÀÌ»ó¸¸ ±â·ÏÇÑ´Ù.
						LogManager::instance().CharLog(ch, iMoney, "GET_GOLD", "");
				}
		}
	};
}

void CHARACTER::GiveGold(int iAmount)
{
	if (iAmount <= 0)
		return;

	sys_log(0, "GIVE_GOLD: %s %d", GetName(), iAmount);

	if (GetParty())
	{
		LPPARTY pParty = GetParty();

		// ÆÄÆ¼°¡ ÀÖ´Â °æ¿ì ³ª´©¾î °¡Áø´Ù.
		DWORD dwTotal = iAmount;
		DWORD dwMyAmount = dwTotal;

		NPartyPickupDistribute::FCountNearMember funcCountNearMember(this);
		pParty->ForEachOnlineMember(funcCountNearMember);

		if (funcCountNearMember.total > 1)
		{
			DWORD dwShare = dwTotal / funcCountNearMember.total;
			dwMyAmount -= dwShare * (funcCountNearMember.total - 1);

			NPartyPickupDistribute::FMoneyDistributor funcMoneyDist(this, dwShare);

			pParty->ForEachOnlineMember(funcMoneyDist);
		}

		PointChange(POINT_GOLD, dwMyAmount, true);

		if (dwMyAmount > 1000) // Ãµ¿ø ÀÌ»ó¸¸ ±â·ÏÇÑ´Ù.
			LogManager::instance().CharLog(this, dwMyAmount, "GET_GOLD", "");
	}
	else
	{
		PointChange(POINT_GOLD, iAmount, true);

		// ºê¶óÁú¿¡ µ·ÀÌ ¾ø¾îÁø´Ù´Â ¹ö±×°¡ ÀÖ´Âµ¥,
		// °¡´ÉÇÑ ½Ã³ª¸®¿À Áß¿¡ ÇÏ³ª´Â,
		// ¸ÞÅ©·Î³ª, ÇÙÀ» ½á¼­ 1000¿ø ÀÌÇÏÀÇ µ·À» °è¼Ó ¹ö·Á °ñµå¸¦ 0À¸·Î ¸¸µé°í, 
		// µ·ÀÌ ¾ø¾îÁ³´Ù°í º¹±¸ ½ÅÃ»ÇÏ´Â °ÍÀÏ ¼öµµ ÀÖ´Ù.
		// µû¶ó¼­ ±×·± °æ¿ì¸¦ Àâ±â À§ÇØ ³·Àº ¼öÄ¡ÀÇ °ñµå¿¡ ´ëÇØ¼­µµ ·Î±×¸¦ ³²±è.
		if (LC_IsBrazil() == true)
		{
			if (iAmount >= 213)
				LogManager::instance().CharLog(this, iAmount, "GET_GOLD", "");
		}
		else
		{
			if (iAmount > 1000) // Ãµ¿ø ÀÌ»ó¸¸ ±â·ÏÇÑ´Ù.
				LogManager::instance().CharLog(this, iAmount, "GET_GOLD", "");
		}
	}
}

bool CHARACTER::PickupItem(DWORD dwVID)
{
	LPITEM item = ITEM_MANAGER::instance().FindByVID(dwVID);

	if (IsObserverMode())
		return false;

	if (!item || !item->GetSectree())
		return false;

	if (item->DistanceValid(this))
	{
		if (item->IsOwnership(this))
		{
			// ¸¸¾à ÁÖÀ¸·Á ÇÏ´Â ¾ÆÀÌÅÛÀÌ ¿¤Å©¶ó¸é
			if (item->GetType() == ITEM_ELK)
			{
				GiveGold(item->GetCount());
				item->RemoveFromGround();

				M2_DESTROY_ITEM(item);

				Save();
			}
			// Æò¹üÇÑ ¾ÆÀÌÅÛÀÌ¶ó¸é
			else
			{
				if (item->IsStackable() && !IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_STACK))
				{
					BYTE bCount = item->GetCount();

					for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
					{
						LPITEM item2 = GetInventoryItem(i);

						if (!item2)
							continue;

						if (item2->GetVnum() == item->GetVnum())
						{
							int j;

							for (j = 0; j < ITEM_SOCKET_MAX_NUM; ++j)
								if (item2->GetSocket(j) != item->GetSocket(j))
									break;

							if (j != ITEM_SOCKET_MAX_NUM)
								continue;

							BYTE bCount2 = MIN(200 - item2->GetCount(), bCount);
							bCount -= bCount2;

							item2->SetCount(item2->GetCount() + bCount2);

							if (bCount == 0)
							{
								ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s"), item2->GetName());
								M2_DESTROY_ITEM(item);
								if (item2->GetType() == ITEM_QUEST)
									quest::CQuestManager::instance().PickupItem (GetPlayerID(), item2);
								return true;
							}
						}
					}

					item->SetCount(bCount);
				}

				int iEmptyCell;
				if (item->IsDragonSoul())
				{
					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						sys_log(0, "No empty ds inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
				else
				{
					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						sys_log(0, "No empty inventory pid %u size %ud itemid %u", GetPlayerID(), item->GetSize(), item->GetID());
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}

				item->RemoveFromGround();
				
				if (item->IsDragonSoul())
					item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
				else
					item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));

				char szHint[32+1];
				snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
				LogManager::instance().ItemLog(this, item, "GET", szHint);
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName());

				if (item->GetType() == ITEM_QUEST)
					quest::CQuestManager::instance().PickupItem (GetPlayerID(), item);
			}

			//Motion(MOTION_PICKUP);
			return true;
		}
		else if (!IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_GIVE | ITEM_ANTIFLAG_DROP) && GetParty())
		{
			// ´Ù¸¥ ÆÄÆ¼¿ø ¼ÒÀ¯±Ç ¾ÆÀÌÅÛÀ» ÁÖÀ¸·Á°í ÇÑ´Ù¸é
			NPartyPickupDistribute::FFindOwnership funcFindOwnership(item);

			GetParty()->ForEachOnlineMember(funcFindOwnership);

			LPCHARACTER owner = funcFindOwnership.owner;

			int iEmptyCell;

			if (item->IsDragonSoul())
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyDragonSoulInventory(item)) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyDragonSoulInventory(item)) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}
			else
			{
				if (!(owner && (iEmptyCell = owner->GetEmptyInventory(item->GetSize())) != -1))
				{
					owner = this;

					if ((iEmptyCell = GetEmptyInventory(item->GetSize())) == -1)
					{
						owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ÒÁöÇÏ°í ÀÖ´Â ¾ÆÀÌÅÛÀÌ ³Ê¹« ¸¹½À´Ï´Ù."));
						return false;
					}
				}
			}

			item->RemoveFromGround();

			if (item->IsDragonSoul())
				item->AddToCharacter(owner, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
			else
				item->AddToCharacter(owner, TItemPos(INVENTORY, iEmptyCell));

			char szHint[32+1];
			snprintf(szHint, sizeof(szHint), "%s %u %u", item->GetName(), item->GetCount(), item->GetOriginalVnum());
			LogManager::instance().ItemLog(owner, item, "GET", szHint);

			if (owner == this)
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName());
			else
			{
				owner->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s ´ÔÀ¸·ÎºÎÅÍ %s"), GetName(), item->GetName());
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ Àü´Þ: %s ´Ô¿¡°Ô %s"), owner->GetName(), item->GetName());
			}

			if (item->GetType() == ITEM_QUEST)
				quest::CQuestManager::instance().PickupItem (owner->GetPlayerID(), item);

			return true;
		}
	}

	return false;
}

bool CHARACTER::SwapItem(BYTE bCell, BYTE bDestCell)
{
	if (!CanHandleItem())
		return false;

	TItemPos srcCell(INVENTORY, bCell), destCell(INVENTORY, bDestCell);

	// ¿Ã¹Ù¸¥ Cell ÀÎÁö °Ë»ç
	// ¿ëÈ¥¼®Àº SwapÇÒ ¼ö ¾øÀ¸¹Ç·Î, ¿©±â¼­ °É¸².
	//if (bCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM || bDestCell >= INVENTORY_MAX_NUM + WEAR_MAX_NUM)
	if (srcCell.IsDragonSoulEquipPosition() || destCell.IsDragonSoulEquipPosition())
		return false;

	// °°Àº CELL ÀÎÁö °Ë»ç
	if (bCell == bDestCell)
		return false;

	// µÑ ´Ù ÀåºñÃ¢ À§Ä¡¸é Swap ÇÒ ¼ö ¾ø´Ù.
	if (srcCell.IsEquipPosition() && destCell.IsEquipPosition())
		return false;

	LPITEM item1, item2;

	// item2°¡ ÀåºñÃ¢¿¡ ÀÖ´Â °ÍÀÌ µÇµµ·Ï.
	if (srcCell.IsEquipPosition())
	{
		item1 = GetInventoryItem(bDestCell);
		item2 = GetInventoryItem(bCell);
	}
	else
	{
		item1 = GetInventoryItem(bCell);
		item2 = GetInventoryItem(bDestCell);
	}

	if (!item1 || !item2)
		return false;
	
	if (item1 == item2)
	{
	    sys_log(0, "[WARNING][WARNING][HACK USER!] : %s %d %d", m_stName.c_str(), bCell, bDestCell);
	    return false;
	}

	// item2°¡ bCellÀ§Ä¡¿¡ µé¾î°¥ ¼ö ÀÖ´ÂÁö È®ÀÎÇÑ´Ù.
	if (!IsEmptyItemGrid(TItemPos (INVENTORY, item1->GetCell()), item2->GetSize(), item1->GetCell()))
		return false;

	// ¹Ù²Ü ¾ÆÀÌÅÛÀÌ ÀåºñÃ¢¿¡ ÀÖÀ¸¸é
	if (TItemPos(EQUIPMENT, item2->GetCell()).IsEquipPosition())
	{
		BYTE bEquipCell = item2->GetCell() - INVENTORY_MAX_NUM;
		BYTE bInvenCell = item1->GetCell();

		// Âø¿ëÁßÀÎ ¾ÆÀÌÅÛÀ» ¹þÀ» ¼ö ÀÖ°í, Âø¿ë ¿¹Á¤ ¾ÆÀÌÅÛÀÌ Âø¿ë °¡´ÉÇÑ »óÅÂ¿©¾ß¸¸ ÁøÇà
		if (false == CanUnequipNow(item2) || false == CanEquipNow(item1))
			return false;

		if (bEquipCell != item1->FindEquipCell(this)) // °°Àº À§Ä¡ÀÏ¶§¸¸ Çã¿ë
			return false;

		item2->RemoveFromCharacter();

		if (item1->EquipTo(this, bEquipCell))
			item2->AddToCharacter(this, TItemPos(INVENTORY, bInvenCell));
		else
			sys_err("SwapItem cannot equip %s! item1 %s", item2->GetName(), item1->GetName());
	}
	else
	{
		BYTE bCell1 = item1->GetCell();
		BYTE bCell2 = item2->GetCell();
		
		item1->RemoveFromCharacter();
		item2->RemoveFromCharacter();

		item1->AddToCharacter(this, TItemPos(INVENTORY, bCell2));
		item2->AddToCharacter(this, TItemPos(INVENTORY, bCell1));
	}

	return true;
}

bool CHARACTER::UnequipItem(LPITEM item)
{
	int pos;

	if (false == CanUnequipNow(item))
		return false;

	if (item->IsDragonSoul())
		pos = GetEmptyDragonSoulInventory(item);
	else
		pos = GetEmptyInventory(item->GetSize());

	// HARD CODING
	if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
		ShowAlignment(true);

	item->RemoveFromCharacter();
	if (item->IsDragonSoul())
	{
		item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, pos));
	}
	else
		item->AddToCharacter(this, TItemPos(INVENTORY, pos));

	CheckMaximumPoints();

	return true;
}

//
// @version	05/07/05 Bang2ni - Skill »ç¿ëÈÄ 1.5 ÃÊ ÀÌ³»¿¡ Àåºñ Âø¿ë ±ÝÁö
//
bool CHARACTER::EquipItem(LPITEM item, int iCandidateCell)
{
	if (item->IsExchanging())
		return false;

	if (false == item->IsEquipable())
		return false;

	if (false == CanEquipNow(item))
		return false;

	int iWearCell = item->FindEquipCell(this, iCandidateCell);
	TItemPos srcCell(INVENTORY, item->GetCell()), destCell(INVENTORY, INVENTORY_MAX_NUM + iWearCell);

	LPITEM belt1,belt2;

	if (iWearCell < 0)
		return false;

	// ¹«¾ð°¡¸¦ Åº »óÅÂ¿¡¼­ ÅÎ½Ãµµ ÀÔ±â ±ÝÁö
	if (iWearCell == WEAR_BODY && IsRiding() && (item->GetVnum() >= 11901 && item->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»À» Åº »óÅÂ¿¡¼­ ¿¹º¹À» ÀÔÀ» ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (iWearCell != WEAR_ARROW && IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°© Áß¿¡´Â Âø¿ëÁßÀÎ Àåºñ¸¦ º¯°æÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	if (FN_check_item_sex(this, item) == false)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¼ºº°ÀÌ ¸ÂÁö¾Ê¾Æ ÀÌ ¾ÆÀÌÅÛÀ» »ç¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return false;
	}

	//½Å±Ô Å»°Í »ç¿ë½Ã ±âÁ¸ ¸» »ç¿ë¿©ºÎ Ã¼Å©
	if(item->IsRideItem() && IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì Å»°ÍÀ» ÀÌ¿ëÁßÀÔ´Ï´Ù."));
		return false;
	}

	// È­»ì ÀÌ¿Ü¿¡´Â ¸¶Áö¸· °ø°Ý ½Ã°£ ¶Ç´Â ½ºÅ³ »ç¿ë 1.5 ÈÄ¿¡ Àåºñ ±³Ã¼°¡ °¡´É
	DWORD dwCurTime = get_dword_time();

	if (iWearCell != WEAR_ARROW 
		&& (dwCurTime - GetLastAttackTime() <= 1500 || dwCurTime - m_dwLastSkillTime <= 1500))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°¡¸¸È÷ ÀÖÀ» ¶§¸¸ Âø¿ëÇÒ ¼ö ÀÖ½À´Ï´Ù."));
		return false;
	}

	// ¿ëÈ¥¼® Æ¯¼ö Ã³¸®
	if (item->IsDragonSoul())
	{
		// °°Àº Å¸ÀÔÀÇ ¿ëÈ¥¼®ÀÌ ÀÌ¹Ì µé¾î°¡ ÀÖ´Ù¸é Âø¿ëÇÒ ¼ö ¾ø´Ù.
		// ¿ëÈ¥¼®Àº swapÀ» Áö¿øÇÏ¸é ¾ÈµÊ.
		if(GetInventoryItem(INVENTORY_MAX_NUM + iWearCell))
		{
			ChatPacket(CHAT_TYPE_INFO, "ÀÌ¹Ì °°Àº Á¾·ùÀÇ ¿ëÈ¥¼®À» Âø¿ëÇÏ°í ÀÖ½À´Ï´Ù.");
			return false;
		}
		
		if (!item->EquipTo(this, iWearCell))
		{
			return false;
		}
	}
	// ¿ëÈ¥¼®ÀÌ ¾Æ´Ô.
	else
	{
		// Âø¿ëÇÒ °÷¿¡ ¾ÆÀÌÅÛÀÌ ÀÖ´Ù¸é,
		if (GetWear(iWearCell) && !IS_SET(GetWear(iWearCell)->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		{
			// ÀÌ ¾ÆÀÌÅÛÀº ÇÑ¹ø ¹ÚÈ÷¸é º¯°æ ºÒ°¡. swap ¿ª½Ã ¿ÏÀü ºÒ°¡
			if (item->GetWearFlag() == WEARABLE_ABILITY) 
				return false;

			if (srcCell.IsEquipPosition())
			{
				belt1 = GetInventoryItem(INVENTORY_MAX_NUM + iWearCell);
				belt2 = GetInventoryItem(item->GetCell());
			}
			else
			{
				belt1 = GetInventoryItem(item->GetCell());
				belt2 = GetInventoryItem(INVENTORY_MAX_NUM + iWearCell);
			}

			if ((ITEM_BELT == belt2->GetType()) && (CBeltInventoryHelper::IsExistItemInBeltInventory(this) == true))
			{
				ChatPacket(CHAT_TYPE_INFO, LC_TEXT("?§?® ???Y???®?? ???????? ???????? ???¦?? ?? ??1?????."));
				return false;
			}

			if (false == SwapItem(item->GetCell(), INVENTORY_MAX_NUM + iWearCell))
			{
				return false;
			}
		}
		else
		{
			BYTE bOldCell = item->GetCell();

			if (item->EquipTo(this, iWearCell))
			{
				SyncQuickslot(QUICKSLOT_TYPE_ITEM, bOldCell, iWearCell);
			}
		}
	}

	if (true == item->IsEquipped())
	{
		// ¾ÆÀÌÅÛ ÃÖÃÊ »ç¿ë ÀÌÈÄºÎÅÍ´Â »ç¿ëÇÏÁö ¾Ê¾Æµµ ½Ã°£ÀÌ Â÷°¨µÇ´Â ¹æ½Ä Ã³¸®. 
		if (-1 != item->GetProto()->cLimitRealTimeFirstUseIndex)
		{
			// ÇÑ ¹øÀÌ¶óµµ »ç¿ëÇÑ ¾ÆÀÌÅÛÀÎÁö ¿©ºÎ´Â Socket1À» º¸°í ÆÇ´ÜÇÑ´Ù. (Socket1¿¡ »ç¿ëÈ½¼ö ±â·Ï)
			if (0 == item->GetSocket(1))
			{
				// »ç¿ë°¡´É½Ã°£Àº Default °ªÀ¸·Î Limit Value °ªÀ» »ç¿ëÇÏµÇ, Socket0¿¡ °ªÀÌ ÀÖÀ¸¸é ±× °ªÀ» »ç¿ëÇÏµµ·Ï ÇÑ´Ù. (´ÜÀ§´Â ÃÊ)
				long duration = (0 != item->GetSocket(0)) ? item->GetSocket(0) : item->GetProto()->aLimits[item->GetProto()->cLimitRealTimeFirstUseIndex].lValue;

				if (0 == duration)
					duration = 60 * 60 * 24 * 7;

				item->SetSocket(0, time(0) + duration);
				item->StartRealTimeExpireEvent();
			}

			item->SetSocket(1, item->GetSocket(1) + 1);
		}

		if (item->GetVnum() == UNIQUE_ITEM_HIDE_ALIGNMENT_TITLE)
			ShowAlignment(false);

		const DWORD& dwVnum = item->GetVnum();

		// ¶ó¸¶´Ü ÀÌº¥Æ® ÃÊ½Â´ÞÀÇ ¹ÝÁö(71135) Âø¿ë½Ã ÀÌÆåÆ® ¹ßµ¿
		if (true == CItemVnumHelper::IsRamadanMoonRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_RAMADAN_RING);
		}
		// ÇÒ·ÎÀ© »çÅÁ(71136) Âø¿ë½Ã ÀÌÆåÆ® ¹ßµ¿
		else if (true == CItemVnumHelper::IsHalloweenCandy(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HALLOWEEN_CANDY);
		}
		// Çàº¹ÀÇ ¹ÝÁö(71143) Âø¿ë½Ã ÀÌÆåÆ® ¹ßµ¿
		else if (true == CItemVnumHelper::IsHappinessRing(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_HAPPINESS_RING);
		}
		// »ç¶ûÀÇ ÆÒ´øÆ®(71145) Âø¿ë½Ã ÀÌÆåÆ® ¹ßµ¿
		else if (true == CItemVnumHelper::IsLovePendant(dwVnum))
		{
			this->EffectPacket(SE_EQUIP_LOVE_PENDANT);
		}
		// ITEM_UNIQUEÀÇ °æ¿ì, SpecialItemGroup¿¡ Á¤ÀÇµÇ¾î ÀÖ°í, (item->GetSIGVnum() != NULL)
		// 
		else if (ITEM_UNIQUE == item->GetType() && 0 != item->GetSIGVnum())
		{
			const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(item->GetSIGVnum());
			if (NULL != pGroup)
			{
				const CSpecialAttrGroup* pAttrGroup = ITEM_MANAGER::instance().GetSpecialAttrGroup(pGroup->GetAttrVnum(item->GetVnum()));
				if (NULL != pAttrGroup)
				{
					const std::string& std = pAttrGroup->m_stEffectFileName;
					SpecificEffectPacket(std.c_str());
				}
			}
		}

		if (UNIQUE_SPECIAL_RIDE == item->GetSubType() && IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_USE))
		{
			quest::CQuestManager::instance().UseItem(GetPlayerID(), item, false);
		}
	}

	return true;
}

void CHARACTER::BuffOnAttr_AddBuffsFromItem(LPITEM pItem)
{
	for (int i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->AddBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_RemoveBuffsFromItem(LPITEM pItem)
{
	for (int i = 0; i < sizeof(g_aBuffOnAttrPoints)/sizeof(g_aBuffOnAttrPoints[0]); i++)
	{
		TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(g_aBuffOnAttrPoints[i]);
		if (it != m_map_buff_on_attrs.end())
		{
			it->second->RemoveBuffFromItem(pItem);
		}
	}
}

void CHARACTER::BuffOnAttr_ClearAll()
{
	for (TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.begin(); it != m_map_buff_on_attrs.end(); it++)
	{
		CBuffOnAttributes* pBuff = it->second;
		if (pBuff)
		{
			pBuff->Initialize();
		}
	}
}

void CHARACTER::BuffOnAttr_ValueChange(BYTE bType, BYTE bOldValue, BYTE bNewValue)
{
	TMapBuffOnAttrs::iterator it = m_map_buff_on_attrs.find(bType);

	if (0 == bNewValue)
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->Off();
	}
	else if(0 == bOldValue)
	{
		CBuffOnAttributes* pBuff;
		if (m_map_buff_on_attrs.end() == it)
		{
			switch (bType)
			{
			case POINT_ENERGY:
				{
					static BYTE abSlot[] = { WEAR_BODY, WEAR_HEAD, WEAR_FOOTS, WEAR_WRIST, WEAR_WEAPON, WEAR_NECK, WEAR_EAR, WEAR_SHIELD };
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			case POINT_COSTUME_ATTR_BONUS:
				{
					static BYTE abSlot[] = { WEAR_COSTUME_BODY, WEAR_COSTUME_HAIR };
					static std::vector <BYTE> vec_slots (abSlot, abSlot + _countof(abSlot));
					pBuff = M2_NEW CBuffOnAttributes(this, bType, &vec_slots);
				}
				break;
			default:
				break;
			}
			m_map_buff_on_attrs.insert(TMapBuffOnAttrs::value_type(bType, pBuff));

		}
		else
			pBuff = it->second;
			
		pBuff->On(bNewValue);
	}
	else
	{
		if (m_map_buff_on_attrs.end() == it)
			return;
		else
			it->second->ChangeBuffValue(bNewValue);
	}
}


LPITEM CHARACTER::FindSpecifyItem(DWORD vnum) const
{
	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		if (GetInventoryItem(i) && GetInventoryItem(i)->GetVnum() == vnum)
			return GetInventoryItem(i);

	return NULL;
}

LPITEM CHARACTER::FindItemByID(DWORD id) const
{
	for (int i=0 ; i < INVENTORY_MAX_NUM ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	for (int i=BELT_INVENTORY_SLOT_START; i < BELT_INVENTORY_SLOT_END ; ++i)
	{
		if (NULL != GetInventoryItem(i) && GetInventoryItem(i)->GetID() == id)
			return GetInventoryItem(i);
	}

	return NULL;
}

int CHARACTER::CountSpecifyItem(DWORD vnum) const
{
	int	count = 0;
	LPITEM item;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		item = GetInventoryItem(i);
		if (NULL != item && item->GetVnum() == vnum)
		{
			// °³ÀÎ »óÁ¡¿¡ µî·ÏµÈ ¹°°ÇÀÌ¸é ³Ñ¾î°£´Ù.
			if (m_pkMyShop && m_pkMyShop->IsSellingItem(item->GetID()))
			{
				continue;
			}
			else
			{
				count += item->GetCount();
			}
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyItem(DWORD vnum, DWORD count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetVnum() != vnum)
			continue;

		//°³ÀÎ »óÁ¡¿¡ µî·ÏµÈ ¹°°ÇÀÌ¸é ³Ñ¾î°£´Ù. (°³ÀÎ »óÁ¡¿¡¼­ ÆÇ¸ÅµÉ¶§ ÀÌ ºÎºÐÀ¸·Î µé¾î¿Ã °æ¿ì ¹®Á¦!)
		if(m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (vnum >= 80003 && vnum <= 80007)
			LogManager::instance().GoldBarLog(GetPlayerID(), GetInventoryItem(i)->GetID(), QUEST, "RemoveSpecifyItem");

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}

	// ¿¹¿ÜÃ³¸®°¡ ¾àÇÏ´Ù.
	if (count)
		sys_log(0, "CHARACTER::RemoveSpecifyItem cannot remove enough item vnum %u, still remain %d", vnum, count);
}

int CHARACTER::CountSpecifyTypeItem(BYTE type) const
{
	int	count = 0;

	for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		LPITEM pItem = GetInventoryItem(i);
		if (pItem != NULL && pItem->GetType() == type)
		{
			count += pItem->GetCount();
		}
	}

	return count;
}

void CHARACTER::RemoveSpecifyTypeItem(BYTE type, DWORD count)
{
	if (0 == count)
		return;

	for (UINT i = 0; i < INVENTORY_MAX_NUM; ++i)
	{
		if (NULL == GetInventoryItem(i))
			continue;

		if (GetInventoryItem(i)->GetType() != type)
			continue;

		//°³ÀÎ »óÁ¡¿¡ µî·ÏµÈ ¹°°ÇÀÌ¸é ³Ñ¾î°£´Ù. (°³ÀÎ »óÁ¡¿¡¼­ ÆÇ¸ÅµÉ¶§ ÀÌ ºÎºÐÀ¸·Î µé¾î¿Ã °æ¿ì ¹®Á¦!)
		if(m_pkMyShop)
		{
			bool isItemSelling = m_pkMyShop->IsSellingItem(GetInventoryItem(i)->GetID());
			if (isItemSelling)
				continue;
		}

		if (count >= GetInventoryItem(i)->GetCount())
		{
			count -= GetInventoryItem(i)->GetCount();
			GetInventoryItem(i)->SetCount(0);

			if (0 == count)
				return;
		}
		else
		{
			GetInventoryItem(i)->SetCount(GetInventoryItem(i)->GetCount() - count);
			return;
		}
	}
}

void CHARACTER::AutoGiveItem(LPITEM item, bool longOwnerShip)
{
	if (NULL == item)
	{
		sys_err ("NULL point.");
		return;
	}
	if (item->GetOwner())
	{
		sys_err ("item %d 's owner exists!",item->GetID());
		return;
	}
	
	int cell;
	if (item->IsDragonSoul())
	{
		cell = GetEmptyDragonSoulInventory(item);
	}
	else
	{
		cell = GetEmptyInventory (item->GetSize());
	}

	if (cell != -1)
	{
		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, cell));
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, cell));

		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = cell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround (GetMapIndex(), GetXYZ());
		item->StartDestroyEvent();

		if (longOwnerShip)
			item->SetOwnership (this, 300);
		else
			item->SetOwnership (this, 60);
		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}
}

LPITEM CHARACTER::AutoGiveItem(DWORD dwItemVnum, BYTE bCount, int iRarePct, bool bMsg)
{
	TItemTable * p = ITEM_MANAGER::instance().GetTable(dwItemVnum);

	if (!p)
		return NULL;

	DBManager::instance().SendMoneyLog(MONEY_LOG_DROP, dwItemVnum, bCount);

	if (p->dwFlags & ITEM_FLAG_STACKABLE && p->bType != ITEM_BLEND) 
	{
		for (int i = 0; i < INVENTORY_MAX_NUM; ++i)
		{
			LPITEM item = GetInventoryItem(i);

			if (!item)
				continue;

			if (item->GetVnum() == dwItemVnum && FN_check_item_socket(item))
			{
				if (IS_SET(p->dwFlags, ITEM_FLAG_MAKECOUNT))
				{
					if (bCount < p->alValues[1])
						bCount = p->alValues[1];
				}

				BYTE bCount2 = MIN(200 - item->GetCount(), bCount);
				bCount -= bCount2;

				item->SetCount(item->GetCount() + bCount2);

				if (bCount == 0)
				{
					if (bMsg)
						ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName());

					return item;
				}
			}
		}
	}

	LPITEM item = ITEM_MANAGER::instance().CreateItem(dwItemVnum, bCount, 0, true);

	if (!item)
	{
		sys_err("cannot create item by vnum %u (name: %s)", dwItemVnum, GetName());
		return NULL;
	}

	if (item->GetType() == ITEM_BLEND)
	{
		for (int i=0; i < INVENTORY_MAX_NUM; i++)
		{
			LPITEM inv_item = GetInventoryItem(i);

			if (inv_item == NULL) continue;

			if (inv_item->GetType() == ITEM_BLEND)
			{
				if (inv_item->GetVnum() == item->GetVnum())
				{
					if (inv_item->GetSocket(0) == item->GetSocket(0) &&
							inv_item->GetSocket(1) == item->GetSocket(1) &&
							inv_item->GetSocket(2) == item->GetSocket(2) &&
							inv_item->GetCount() < ITEM_MAX_COUNT)
					{
						inv_item->SetCount(inv_item->GetCount() + item->GetCount());
						return inv_item;
					}
				}
			}
		}
	}

	int iEmptyCell;
	if (item->IsDragonSoul())
	{
		iEmptyCell = GetEmptyDragonSoulInventory(item);
	}
	else
		iEmptyCell = GetEmptyInventory(item->GetSize());

	if (iEmptyCell != -1)
	{
		if (bMsg)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÀÌÅÛ È¹µæ: %s"), item->GetName());

		if (item->IsDragonSoul())
			item->AddToCharacter(this, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyCell));
		else
			item->AddToCharacter(this, TItemPos(INVENTORY, iEmptyCell));
		LogManager::instance().ItemLog(this, item, "SYSTEM", item->GetName());

		if (item->GetType() == ITEM_USE && item->GetSubType() == USE_POTION)
		{
			TQuickslot * pSlot;

			if (GetQuickslot(0, &pSlot) && pSlot->type == QUICKSLOT_TYPE_NONE)
			{
				TQuickslot slot;
				slot.type = QUICKSLOT_TYPE_ITEM;
				slot.pos = iEmptyCell;
				SetQuickslot(0, slot);
			}
		}
	}
	else
	{
		item->AddToGround(GetMapIndex(), GetXYZ());
		item->StartDestroyEvent();
		// ¾ÈÆ¼ µå¶ø flag°¡ °É·ÁÀÖ´Â ¾ÆÀÌÅÛÀÇ °æ¿ì, 
		// ÀÎº¥¿¡ ºó °ø°£ÀÌ ¾ø¾î¼­ ¾îÂ¿ ¼ö ¾øÀÌ ¶³¾îÆ®¸®°Ô µÇ¸é,
		// ownershipÀ» ¾ÆÀÌÅÛÀÌ »ç¶óÁú ¶§±îÁö(300ÃÊ) À¯ÁöÇÑ´Ù.
		if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_DROP))
			item->SetOwnership(this, 300);
		else
			item->SetOwnership(this, 60);
		LogManager::instance().ItemLog(this, item, "SYSTEM_DROP", item->GetName());
	}

	sys_log(0, 
		"7: %d %d", dwItemVnum, bCount);
	return item;
}

bool CHARACTER::GiveItem(LPCHARACTER victim, TItemPos Cell)
{
	if (!CanHandleItem())
		return false;

	LPITEM item = GetItem(Cell);

	if (item && !item->IsExchanging())
	{
		if (victim->CanReceiveItem(this, item))
		{
			victim->ReceiveItem(this, item);
			return true;
		}
	}

	return false;
}

bool CHARACTER::CanReceiveItem(LPCHARACTER from, LPITEM item) const
{
	if (IsPC())
		return false;

	// TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX
	if (DISTANCE_APPROX(GetX() - from->GetX(), GetY() - from->GetY()) > 2000)
		return false;
	// END_OF_TOO_LONG_DISTANCE_EXCHANGE_BUG_FIX

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && 
					(item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				return true;
			break;

		case fishing::FISHER_MOB:
			if (item->GetType() == ITEM_ROD)
				return true;
			break;

			// BUILDING_NPC
		case BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
			if (item->GetType() == ITEM_WEAPON && 
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
			if (item->GetType() == ITEM_ARMOR && 
					(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;

		case BLACKSMITH_ACCESSORY_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetType() == ITEM_ARMOR &&
					!(item->GetSubType() == ARMOR_BODY || item->GetSubType() == ARMOR_SHIELD || item->GetSubType() == ARMOR_HEAD) &&
					item->GetRefinedVnum())
				return true;
			else
				return false;
			break;
			// END_OF_BUILDING_NPC

		case BLACKSMITH_MOB:
			if (item->GetRefinedVnum() && item->GetRefineSet() < 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case BLACKSMITH2_MOB:
			if (item->GetRefineSet() >= 500)
			{
				return true;
			}
			else
			{
				return false;
			}

		case ALCHEMIST_MOB:
			if (item->GetRefinedVnum())
				return true;
			break;

		case 20101:
		case 20102:
		case 20103:
			// ÃÊ±Þ ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20104:
		case 20105:
		case 20106:
			// Áß±Þ ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_2)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				return false;
			}
			break;
		case 20107:
		case 20108:
		case 20109:
			// °í±Þ ¸»
			if (item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				if (!IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Áö ¾ÊÀº ¸»¿¡°Ô ¼±ÃÊ¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				if (IsDead())
				{
					from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Á×Àº ¸»¿¡°Ô »ç·á¸¦ ¸ÔÀÏ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				return true;
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 || item->GetVnum() == ITEM_HORSE_FOOD_2)
			{
				return false;
			}
			break;
	}

	//if (IS_SET(item->GetFlag(), ITEM_FLAG_QUEST_GIVE))
	{
		return true;
	}

	return false;
}

void CHARACTER::ReceiveItem(LPCHARACTER from, LPITEM item)
{
	if (IsPC())
		return;

	switch (GetRaceNum())
	{
		case fishing::CAMPFIRE_MOB:
			if (item->GetType() == ITEM_FISH && (item->GetSubType() == FISH_ALIVE || item->GetSubType() == FISH_DEAD))
				fishing::Grill(from, item);
			else
			{
				// TAKE_ITEM_BUG_FIX
				from->SetQuestNPCID(GetVID());
				// END_OF_TAKE_ITEM_BUG_FIX
				quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			}
			break;

			// DEVILTOWER_NPC 
		case DEVILTOWER_BLACKSMITH_WEAPON_MOB:
		case DEVILTOWER_BLACKSMITH_ARMOR_MOB:
		case DEVILTOWER_BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum() != 0 && item->GetRefineSet() != 0 && item->GetRefineSet() < 500)
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_MONEY_ONLY);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			}
			break;
			// END_OF_DEVILTOWER_NPC

		case BLACKSMITH_MOB:
		case BLACKSMITH2_MOB:
		case BLACKSMITH_WEAPON_MOB:
		case BLACKSMITH_ARMOR_MOB:
		case BLACKSMITH_ACCESSORY_MOB:
			if (item->GetRefinedVnum())
			{
				from->SetRefineNPC(this);
				from->RefineInformation(item->GetCell(), REFINE_TYPE_NORMAL);
			}
			else
			{
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ ¾ÆÀÌÅÛÀº °³·®ÇÒ ¼ö ¾ø½À´Ï´Ù."));
			}
			break;

		case 20101:
		case 20102:
		case 20103:
		case 20104:
		case 20105:
		case 20106:
		case 20107:
		case 20108:
		case 20109:
			if (item->GetVnum() == ITEM_REVIVE_HORSE_1 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_2 ||
					item->GetVnum() == ITEM_REVIVE_HORSE_3)
			{
				from->ReviveHorse();
				item->SetCount(item->GetCount()-1);
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»¿¡°Ô ¼±ÃÊ¸¦ ÁÖ¾ú½À´Ï´Ù."));
			}
			else if (item->GetVnum() == ITEM_HORSE_FOOD_1 ||
					item->GetVnum() == ITEM_HORSE_FOOD_2 ||
					item->GetVnum() == ITEM_HORSE_FOOD_3)
			{
				from->FeedHorse();
				from->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¸»¿¡°Ô »ç·á¸¦ ÁÖ¾ú½À´Ï´Ù."));
				item->SetCount(item->GetCount()-1);
				EffectPacket(SE_HPUP_RED);
			}
			break;

		default:
			sys_log(0, "TakeItem %s %d %s", from->GetName(), GetRaceNum(), item->GetName());
			from->SetQuestNPCID(GetVID());
			quest::CQuestManager::instance().TakeItem(from->GetPlayerID(), GetRaceNum(), item);
			break;
	}
}

bool CHARACTER::IsEquipUniqueItem(DWORD dwItemVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetVnum() == dwItemVnum)
			return true;
	}

	// ¾ð¾î¹ÝÁöÀÎ °æ¿ì ¾ð¾î¹ÝÁö(°ßº») ÀÎÁöµµ Ã¼Å©ÇÑ´Ù.
	if (dwItemVnum == UNIQUE_ITEM_RING_OF_LANGUAGE)
		return IsEquipUniqueItem(UNIQUE_ITEM_RING_OF_LANGUAGE_SAMPLE);

	return false;
}

// CHECK_UNIQUE_GROUP
bool CHARACTER::IsEquipUniqueGroup(DWORD dwGroupVnum) const
{
	{
		LPITEM u = GetWear(WEAR_UNIQUE1);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	{
		LPITEM u = GetWear(WEAR_UNIQUE2);

		if (u && u->GetSpecialGroup() == (int) dwGroupVnum)
			return true;
	}

	return false;
}
// END_OF_CHECK_UNIQUE_GROUP

void CHARACTER::SetRefineMode(int iAdditionalCell)
{
	m_iRefineAdditionalCell = iAdditionalCell;
	m_bUnderRefine = true;
}

void CHARACTER::ClearRefineMode()
{
	m_bUnderRefine = false;
	SetRefineNPC( NULL );
}

bool CHARACTER::GiveItemFromSpecialItemGroup(DWORD dwGroupNum, std::vector<DWORD> &dwItemVnums, 
											std::vector<DWORD> &dwItemCounts, std::vector <LPITEM> &item_gets, int &count)
{
	const CSpecialItemGroup* pGroup = ITEM_MANAGER::instance().GetSpecialItemGroup(dwGroupNum);

	if (!pGroup)
	{
		sys_err("cannot find special item group %d", dwGroupNum);
		return false;
	}

	std::vector <int> idxes;
	int n = pGroup->GetMultiIndex(idxes);

	bool bSuccess;

	for (int i = 0; i < n; i++)
	{
		bSuccess = false;
		int idx = idxes[i];
		DWORD dwVnum = pGroup->GetVnum(idx);
		DWORD dwCount = pGroup->GetCount(idx);
		int	iRarePct = pGroup->GetRarePct(idx);
		LPITEM item_get = NULL;
		switch (dwVnum)
		{
			case CSpecialItemGroup::GOLD:
				PointChange(POINT_GOLD, dwCount);
				LogManager::instance().CharLog(this, dwCount, "TREASURE_GOLD", "");

				bSuccess = true;
				break;
			case CSpecialItemGroup::EXP:
				{
					PointChange(POINT_EXP, dwCount);
					LogManager::instance().CharLog(this, dwCount, "TREASURE_EXP", "");

					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB:
				{
					sys_log(0, "CSpecialItemGroup::MOB %d", dwCount);
					int x = GetX() + number(-500, 500);
					int y = GetY() + number(-500, 500);

					LPCHARACTER ch = CHARACTER_MANAGER::instance().SpawnMob(dwCount, GetMapIndex(), x, y, 0, true, -1);
					if (ch)
						ch->SetAggressive();
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::SLOW:
				{
					sys_log(0, "CSpecialItemGroup::SLOW %d", -(int)dwCount);
					AddAffect(AFFECT_SLOW, POINT_MOV_SPEED, -(int)dwCount, AFF_SLOW, 300, 0, true);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::DRAIN_HP:
				{
					int iDropHP = GetMaxHP()*dwCount/100;
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					iDropHP = MIN(iDropHP, GetHP()-1);
					sys_log(0, "CSpecialItemGroup::DRAIN_HP %d", -iDropHP);
					PointChange(POINT_HP, -iDropHP);
					bSuccess = true;
				}
				break;
			case CSpecialItemGroup::POISON:
				{
					AttackedByPoison(NULL);
					bSuccess = true;
				}
				break;

			case CSpecialItemGroup::MOB_GROUP:
				{
					int sx = GetX() - number(300, 500);
					int sy = GetY() - number(300, 500);
					int ex = GetX() + number(300, 500);
					int ey = GetY() + number(300, 500);
					CHARACTER_MANAGER::instance().SpawnGroup(dwCount, GetMapIndex(), sx, sy, ex, ey, NULL, true);

					bSuccess = true;
				}
				break;
			default:
				{
					item_get = AutoGiveItem(dwVnum, dwCount, iRarePct);

					if (item_get)
					{
						bSuccess = true;
					}
				}
				break;
		}
	
		if (bSuccess)
		{
			dwItemVnums.push_back(dwVnum);
			dwItemCounts.push_back(dwCount);
			item_gets.push_back(item_get);
			count++;

		}
		else
		{
			return false;
		}
	}
	return bSuccess;
}

// NEW_HAIR_STYLE_ADD
bool CHARACTER::ItemProcess_Hair(LPITEM item, int iDestCell)
{
	if (item->CheckItemUseLevel(GetLevel()) == false)
	{
		// ·¹º§ Á¦ÇÑ¿¡ °É¸²
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¾ÆÁ÷ ÀÌ ¸Ó¸®¸¦ »ç¿ëÇÒ ¼ö ¾ø´Â ·¹º§ÀÔ´Ï´Ù."));
		return false;
	}

	DWORD hair = item->GetVnum();

	switch (GetJob())
	{
		case JOB_WARRIOR :
			hair -= 72000; // 73001 - 72000 = 1001 ºÎÅÍ Çì¾î ¹øÈ£ ½ÃÀÛ
			break;

		case JOB_ASSASSIN :
			hair -= 71250;
			break;

		case JOB_SURA :
			hair -= 70500;
			break;

		case JOB_SHAMAN :
			hair -= 69750;
			break;

		default :
			return false;
			break;
	}

	if (hair == GetPart(PART_HAIR))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µ¿ÀÏÇÑ ¸Ó¸® ½ºÅ¸ÀÏ·Î´Â ±³Ã¼ÇÒ ¼ö ¾ø½À´Ï´Ù."));
		return true;
	}

	item->SetCount(item->GetCount() - 1);

	SetPart(PART_HAIR, hair);
	UpdatePacket();

	return true;
}
// END_NEW_HAIR_STYLE_ADD

bool CHARACTER::ItemProcess_Polymorph(LPITEM item)
{
	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("ÀÌ¹Ì µÐ°©ÁßÀÎ »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	if (true == IsRiding())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°©ÇÒ ¼ö ¾ø´Â »óÅÂÀÔ´Ï´Ù."));
		return false;
	}

	DWORD dwVnum = item->GetSocket(0);

	if (dwVnum == 0)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àß¸øµÈ µÐ°© ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	const CMob* pMob = CMobManager::instance().Get(dwVnum);

	if (pMob == NULL)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Àß¸øµÈ µÐ°© ¾ÆÀÌÅÛÀÔ´Ï´Ù."));
		item->SetCount(item->GetCount()-1);
		return false;
	}

	switch (item->GetVnum())
	{
		case 70104 :
		case 70105 :
		case 70106 :
		case 70107 :
		case 71093 :
			{
				// µÐ°©±¸ Ã³¸®
				sys_log(0, "USE_POLYMORPH_BALL PID(%d) vnum(%d)", GetPlayerID(), dwVnum);

				// ·¹º§ Á¦ÇÑ Ã¼Å©
				int iPolymorphLevelLimit = MAX(0, 20 - GetLevel() * 3 / 10);
				if (pMob->m_table.bLevel >= GetLevel() + iPolymorphLevelLimit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("³ªº¸´Ù ³Ê¹« ³ôÀº ·¹º§ÀÇ ¸ó½ºÅÍ·Î´Â º¯½Å ÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}

				int iDuration = GetSkillLevel(POLYMORPH_SKILL_ID) == 0 ? 5 : (5 + (5 + GetSkillLevel(POLYMORPH_SKILL_ID)/40 * 25));
				iDuration *= 60;

				DWORD dwBonus = 0;
				
				if (true == LC_IsYMIR() || true == LC_IsKorea())
				{
					dwBonus = GetSkillLevel(POLYMORPH_SKILL_ID) + 60;
				}
				else
				{
					dwBonus = (2 + GetSkillLevel(POLYMORPH_SKILL_ID)/40) * 100;
				}

				AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, dwVnum, AFF_POLYMORPH, iDuration, 0, true);
				AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonus, AFF_POLYMORPH, iDuration, 0, false);
				
				item->SetCount(item->GetCount()-1);
			}
			break;

		case 50322:
			{
				// º¸·ù

				// µÐ°©¼­ Ã³¸®
				// ¼ÒÄÏ0                ¼ÒÄÏ1           ¼ÒÄÏ2   
				// µÐ°©ÇÒ ¸ó½ºÅÍ ¹øÈ£   ¼ö·ÃÁ¤µµ        µÐ°©¼­ ·¹º§
				sys_log(0, "USE_POLYMORPH_BOOK: %s(%u) vnum(%u)", GetName(), GetPlayerID(), dwVnum);

				if (CPolymorphUtils::instance().PolymorphCharacter(this, item, pMob) == true)
				{
					CPolymorphUtils::instance().UpdateBookPracticeGrade(this, item);
				}
				else
				{
				}
			}
			break;

		default :
			sys_err("POLYMORPH invalid item passed PID(%d) vnum(%d)", GetPlayerID(), item->GetOriginalVnum());
			return false;
	}

	return true;
}

bool CHARACTER::CanDoCube() const
{
	if (m_bIsObserver)	return false;
	if (GetShop())		return false;
	if (GetMyShop())	return false;
	if (m_bUnderRefine)	return false;
	if (IsWarping())	return false;

	return true;
}

bool CHARACTER::UnEquipSpecialRideUniqueItem()
{
	LPITEM Unique1 = GetWear(WEAR_UNIQUE1);
	LPITEM Unique2 = GetWear(WEAR_UNIQUE2);

	if( NULL != Unique1 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique1->GetSpecialGroup() )
		{
			return UnequipItem(Unique1);
		}
	}

	if( NULL != Unique2 )
	{
		if( UNIQUE_GROUP_SPECIAL_RIDE == Unique2->GetSpecialGroup() )
		{
			return UnequipItem(Unique2);
		}
	}

	return true;
}

void CHARACTER::AutoRecoveryItemProcess(const EAffectTypes type)
{
	if (true == IsDead() || true == IsStun())
		return;

	if (false == IsPC())
		return;

	if (AFFECT_AUTO_HP_RECOVERY != type && AFFECT_AUTO_SP_RECOVERY != type)
		return;

	if (NULL != FindAffect(AFFECT_STUN))
		return;

	{
		const DWORD stunSkills[] = { SKILL_TANHWAN, SKILL_GEOMPUNG, SKILL_BYEURAK, SKILL_GIGUNG };

		for (size_t i=0 ; i < sizeof(stunSkills)/sizeof(DWORD) ; ++i)
		{
			const CAffect* p = FindAffect(stunSkills[i]);

			if (NULL != p && AFF_STUN == p->dwFlag)
				return;
		}
	}

	const CAffect* pAffect = FindAffect(type);
	const size_t idx_of_amount_of_used = 1;
	const size_t idx_of_amount_of_full = 2;

	if (NULL != pAffect)
	{
		LPITEM pItem = FindItemByID(pAffect->dwFlag);

		if (NULL != pItem && true == pItem->GetSocket(0))
		{
			if (false == CArenaManager::instance().IsArenaMap(GetMapIndex()))
			{
				const long amount_of_used = pItem->GetSocket(idx_of_amount_of_used);
				const long amount_of_full = pItem->GetSocket(idx_of_amount_of_full);
				
				const int32_t avail = amount_of_full - amount_of_used;

				int32_t amount = 0;

				if (AFFECT_AUTO_HP_RECOVERY == type)
				{
					amount = GetMaxHP() - (GetHP() + GetPoint(POINT_HP_RECOVERY));
				}
				else if (AFFECT_AUTO_SP_RECOVERY == type)
				{
					amount = GetMaxSP() - (GetSP() + GetPoint(POINT_SP_RECOVERY));
				}

				if (amount > 0)
				{
					if (avail > amount)
					{
						const int pct_of_used = amount_of_used * 100 / amount_of_full;
						const int pct_of_will_used = (amount_of_used + amount) * 100 / amount_of_full;

						bool bLog = false;
						// »ç¿ë·®ÀÇ 10% ´ÜÀ§·Î ·Î±×¸¦ ³²±è
						// (»ç¿ë·®ÀÇ %¿¡¼­, ½ÊÀÇ ÀÚ¸®°¡ ¹Ù²ð ¶§¸¶´Ù ·Î±×¸¦ ³²±è.)
						if ((pct_of_will_used / 10) - (pct_of_used / 10) >= 1)
							bLog = true;
						pItem->SetSocket(idx_of_amount_of_used, amount_of_used + amount, bLog);
					}
					else
					{
						amount = avail;

						ITEM_MANAGER::instance().RemoveItem( pItem );
					}

					if (AFFECT_AUTO_HP_RECOVERY == type)
					{
						PointChange( POINT_HP_RECOVERY, amount );
						EffectPacket( SE_AUTO_HPUP );
					}
					else if (AFFECT_AUTO_SP_RECOVERY == type)
					{
						PointChange( POINT_SP_RECOVERY, amount );
						EffectPacket( SE_AUTO_SPUP );
					}
				}
			}
			else
			{
				pItem->Lock(false);
				pItem->SetSocket(0, false);
				RemoveAffect( const_cast<CAffect*>(pAffect) );
			}
		}
		else
		{
			RemoveAffect( const_cast<CAffect*>(pAffect) );
		}
	}
}

bool CHARACTER::IsValidItemPosition(TItemPos Pos) const
{
	BYTE window_type = Pos.window_type;
	WORD cell = Pos.cell;
	
	switch (window_type)
	{
	case RESERVED_WINDOW:
		return false;

	case INVENTORY:
	case EQUIPMENT:
		return cell < (INVENTORY_AND_EQUIP_SLOT_MAX);

	case DRAGON_SOUL_INVENTORY:
		return cell < (DRAGON_SOUL_INVENTORY_MAX_NUM);

	case SAFEBOX:
		if (NULL != m_pkSafebox)
			return m_pkSafebox->IsValidPosition(cell);
		else
			return false;

	case MALL:
		if (NULL != m_pkMall)
			return m_pkMall->IsValidPosition(cell);
		else
			return false;
	default:
		return false;
	}
}


// ±ÍÂú¾Æ¼­ ¸¸µç ¸ÅÅ©·Î.. exp°¡ true¸é msg¸¦ Ãâ·ÂÇÏ°í return false ÇÏ´Â ¸ÅÅ©·Î (ÀÏ¹ÝÀûÀÎ verify ¿ëµµ¶ûÀº return ¶§¹®¿¡ ¾à°£ ¹Ý´ë¶ó ÀÌ¸§¶§¹®¿¡ Çò°¥¸± ¼öµµ ÀÖ°Ú´Ù..)
#define VERIFY_MSG(exp, msg)  \
	if (true == (exp)) { \
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT(msg)); \
			return false; \
	}

		
/// ÇöÀç Ä³¸¯ÅÍÀÇ »óÅÂ¸¦ ¹ÙÅÁÀ¸·Î ÁÖ¾îÁø itemÀ» Âø¿ëÇÒ ¼ö ÀÖ´Â Áö È®ÀÎÇÏ°í, ºÒ°¡´É ÇÏ´Ù¸é Ä³¸¯ÅÍ¿¡°Ô ÀÌÀ¯¸¦ ¾Ë·ÁÁÖ´Â ÇÔ¼ö
bool CHARACTER::CanEquipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{
	const TItemTable* itemTable = item->GetProto();
	BYTE itemType = item->GetType();
	BYTE itemSubType = item->GetSubType();

	switch (GetJob())
	{
		case JOB_WARRIOR:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_WARRIOR)
				return false;
			break;

		case JOB_ASSASSIN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_ASSASSIN)
				return false;
			break;

		case JOB_SHAMAN:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SHAMAN)
				return false;
			break;

		case JOB_SURA:
			if (item->GetAntiFlag() & ITEM_ANTIFLAG_SURA)
				return false;
			break;
	}

	for (int i = 0; i < ITEM_LIMIT_MAX_NUM; ++i)
	{
		long limit = itemTable->aLimits[i].lValue;
		switch (itemTable->aLimits[i].bType)
		{
			case LIMIT_LEVEL:
				if (GetLevel() < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("·¹º§ÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_STR:
				if (GetPoint(POINT_ST) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("±Ù·ÂÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_INT:
				if (GetPoint(POINT_IQ) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Áö´ÉÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_DEX:
				if (GetPoint(POINT_DX) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("¹ÎÃ¸ÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;

			case LIMIT_CON:
				if (GetPoint(POINT_HT) < limit)
				{
					ChatPacket(CHAT_TYPE_INFO, LC_TEXT("Ã¼·ÂÀÌ ³·¾Æ Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
					return false;
				}
				break;
		}
	}

	if (item->GetWearFlag() & WEARABLE_UNIQUE)
	{
		if ((GetWear(WEAR_UNIQUE1) && GetWear(WEAR_UNIQUE1)->IsSameSpecialGroup(item)) ||
			(GetWear(WEAR_UNIQUE2) && GetWear(WEAR_UNIQUE2)->IsSameSpecialGroup(item)))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°°Àº Á¾·ùÀÇ À¯´ÏÅ© ¾ÆÀÌÅÛ µÎ °³¸¦ µ¿½Ã¿¡ ÀåÂøÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

		if (marriage::CManager::instance().IsMarriageUniqueItem(item->GetVnum()) && 
			!marriage::CManager::instance().IsMarried(GetPlayerID()))
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("°áÈ¥ÇÏÁö ¾ÊÀº »óÅÂ¿¡¼­ ¿¹¹°À» Âø¿ëÇÒ ¼ö ¾ø½À´Ï´Ù."));
			return false;
		}

	}

	return true;
}

/// ÇöÀç Ä³¸¯ÅÍÀÇ »óÅÂ¸¦ ¹ÙÅÁÀ¸·Î Âø¿ë ÁßÀÎ itemÀ» ¹þÀ» ¼ö ÀÖ´Â Áö È®ÀÎÇÏ°í, ºÒ°¡´É ÇÏ´Ù¸é Ä³¸¯ÅÍ¿¡°Ô ÀÌÀ¯¸¦ ¾Ë·ÁÁÖ´Â ÇÔ¼ö
bool CHARACTER::CanUnequipNow(const LPITEM item, const TItemPos& srcCell, const TItemPos& destCell) /*const*/
{	

	if (ITEM_BELT == item->GetType())
		VERIFY_MSG(CBeltInventoryHelper::IsExistItemInBeltInventory(this), "º§Æ® ÀÎº¥Åä¸®¿¡ ¾ÆÀÌÅÛÀÌ Á¸ÀçÇÏ¸é ÇØÁ¦ÇÒ ¼ö ¾ø½À´Ï´Ù.");

	// ¿µ¿øÈ÷ ÇØÁ¦ÇÒ ¼ö ¾ø´Â ¾ÆÀÌÅÛ
	if (IS_SET(item->GetFlag(), ITEM_FLAG_IRREMOVABLE))
		return false;

	// ¾ÆÀÌÅÛ unequip½Ã ÀÎº¥Åä¸®·Î ¿Å±æ ¶§ ºó ÀÚ¸®°¡ ÀÖ´Â Áö È®ÀÎ
	{
		int pos = -1;

		if (item->IsDragonSoul())
			pos = GetEmptyDragonSoulInventory(item);
		else
			pos = GetEmptyInventory(item->GetSize());

		VERIFY_MSG( -1 == pos, "¼ÒÁöÇ°¿¡ ºó °ø°£ÀÌ ¾ø½À´Ï´Ù." );
	}


	return true;
}
