#include "stdafx.h"
#include "../../libgame/include/grid.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "shop.h"
#include "desc.h"
#include "desc_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "log.h"
#include "db.h"
#include "questmanager.h"
#include "monarch.h"
#include "mob_manager.h"
#include "locale_service.h"
#include "desc_client.h"
#include "shop_manager.h"
#include "group_text_parse_tree.h"
#include "shopEx.h"
#include <boost/algorithm/string/predicate.hpp>
#include "shop_manager.h"
#include <cctype>

CShopManager::CShopManager()
{
}

CShopManager::~CShopManager()
{
	Destroy();
}

bool CShopManager::Initialize(TShopTable * table, int size)
{
	if (!m_map_pkShop.empty())
		return false;

	int i; 

	for (i = 0; i < size; ++i, ++table)
	{
		LPSHOP shop = M2_NEW CShop;

		if (!shop->Create(table->dwVnum, table->dwNPCVnum, table->items))
		{
			M2_DELETE(shop);
			continue;
		}

		m_map_pkShop.insert(TShopMap::value_type(table->dwVnum, shop));
		m_map_pkShopByNPCVnum.insert(TShopMap::value_type(table->dwNPCVnum, shop));
	}
	char szShopTableExFileName[256];

	snprintf(szShopTableExFileName, sizeof(szShopTableExFileName),
		"%s/shop_table_ex.txt", LocaleService_GetBasePath().c_str());

	return ReadShopTableEx(szShopTableExFileName);
}

void CShopManager::Destroy()
{
	TShopMap::iterator it = m_map_pkShop.begin();

	while (it != m_map_pkShop.end())
	{
		M2_DELETE(it->second);
		++it;
	}

	m_map_pkShop.clear();
}

LPSHOP CShopManager::Get(DWORD dwVnum)
{
	TShopMap::const_iterator it = m_map_pkShop.find(dwVnum);

	if (it == m_map_pkShop.end())
		return NULL;

	return (it->second);
}

LPSHOP CShopManager::GetByNPCVnum(DWORD dwVnum)
{
	TShopMap::const_iterator it = m_map_pkShopByNPCVnum.find(dwVnum);

	if (it == m_map_pkShopByNPCVnum.end())
		return NULL;

	return (it->second);
}

/*
 * 인터페이스 함수들
 */

// 상점 거래를 시작
bool CShopManager::StartShopping(LPCHARACTER pkChr, LPCHARACTER pkChrShopKeeper, int iShopVnum)
{
	if (pkChr->GetShopOwner() == pkChrShopKeeper)
		return false;
	// this method is only for NPC
	if (pkChrShopKeeper->IsPC())
		return false;

	//PREVENT_TRADE_WINDOW
	if (pkChr->IsOpenSafebox() || pkChr->GetExchange() || pkChr->GetMyShop() || pkChr->IsCubeOpen())
	{
		pkChr->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("다른 거래창이 열린상태에서는 상점거래를 할수 가 없습니다."));
		return false;
	}
	//END_PREVENT_TRADE_WINDOW

	long distance = DISTANCE_APPROX(pkChr->GetX() - pkChrShopKeeper->GetX(), pkChr->GetY() - pkChrShopKeeper->GetY());

	if (distance >= SHOP_MAX_DISTANCE)
	{
		sys_log(1, "SHOP: TOO_FAR: %s distance %d", pkChr->GetName(), distance);
		return false;
	}

	LPSHOP pkShop;

	if (iShopVnum)
		pkShop = Get(iShopVnum);
	else
		pkShop = GetByNPCVnum(pkChrShopKeeper->GetRaceNum());

	if (!pkShop)
	{
		sys_log(1, "SHOP: NO SHOP");
		return false;
	}

	bool bOtherEmpire = false;

	if (pkChr->GetEmpire() != pkChrShopKeeper->GetEmpire())
		bOtherEmpire = true;

	pkShop->AddGuest(pkChr, pkChrShopKeeper->GetVID(), bOtherEmpire);
	pkChr->SetShopOwner(pkChrShopKeeper);
	sys_log(0, "SHOP: START: %s", pkChr->GetName());
	return true;
}

LPSHOP CShopManager::FindPCShop(DWORD dwVID)
{
	TShopMap::iterator it = m_map_pkShopByPC.find(dwVID);

	if (it == m_map_pkShopByPC.end())
		return NULL;

	return it->second;
}

LPSHOP CShopManager::CreatePCShop(LPCHARACTER ch, TShopItemTable * pTable, BYTE bItemCount)
{
	if (FindPCShop(ch->GetVID()))
		return NULL;

	LPSHOP pkShop = M2_NEW CShop;
	pkShop->SetPCShop(ch);
	pkShop->SetShopItems(pTable, bItemCount);

	m_map_pkShopByPC.insert(TShopMap::value_type(ch->GetVID(), pkShop));
	return pkShop;
}

void CShopManager::DestroyPCShop(LPCHARACTER ch)
{
	LPSHOP pkShop = FindPCShop(ch->GetVID());

	if (!pkShop)
		return;

	//PREVENT_ITEM_COPY;
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY
	
	m_map_pkShopByPC.erase(ch->GetVID());
	M2_DELETE(pkShop);
}

// 상점 거래를 종료
void CShopManager::StopShopping(LPCHARACTER ch)
{
	LPSHOP shop;

	if (!(shop = ch->GetShop()))
		return;

	//PREVENT_ITEM_COPY;
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY
	
	shop->RemoveGuest(ch);
	sys_log(0, "SHOP: END: %s", ch->GetName());
}

// 아이템 구입
void CShopManager::Buy(LPCHARACTER ch, BYTE pos)
{
	if (!ch->GetShop())
		return;

	if (!ch->GetShopOwner())
		return;

	if (DISTANCE_APPROX(ch->GetX() - ch->GetShopOwner()->GetX(), ch->GetY() - ch->GetShopOwner()->GetY()) > 2000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상점과의 거리가 너무 멀어 물건을 살 수 없습니다."));
		return;
	}

	CShop* pkShop = ch->GetShop();

	if (!pkShop->IsPCShop())
	{
		//if (pkShop->GetVnum() == 0)
		//	return;
		//const CMob* pkMob = CMobManager::instance().Get(pkShop->GetNPCVnum());
		//if (!pkMob)
		//	return;

		//if (pkMob->m_table.bType != CHAR_TYPE_NPC)
		//{
		//	return;
		//}
	}
	else
	{
	}

	//PREVENT_ITEM_COPY
	ch->SetMyShopTime();
	//END_PREVENT_ITEM_COPY

	int ret = pkShop->Buy(ch, pos);

	if (SHOP_SUBHEADER_GC_OK != ret) // 문제가 있었으면 보낸다.
	{
		TPacketGCShop pack;

		pack.header	= HEADER_GC_SHOP;
		pack.subheader	= ret;
		pack.size	= sizeof(TPacketGCShop);

		ch->GetDesc()->Packet(&pack, sizeof(pack));
	}
}

void CShopManager::Sell(LPCHARACTER ch, BYTE bCell, BYTE bCount)
{
	if (!ch->GetShop())
		return;

	if (!ch->GetShopOwner())
		return;

	if (!ch->CanHandleItem())
		return;

	if (ch->GetShop()->IsPCShop())
		return;

	if (DISTANCE_APPROX(ch->GetX()-ch->GetShopOwner()->GetX(), ch->GetY()-ch->GetShopOwner()->GetY())>2000)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상점과의 거리가 너무 멀어 물건을 팔 수 없습니다."));
		return;
	}
	
	LPITEM item = ch->GetInventoryItem(bCell);

	if (!item)
		return;

	if (item->IsEquipped() == true)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("착용 중인 아이템은 판매할 수 없습니다."));
		return;
	}

	if (true == item->isLocked())
	{
		return;
	}

	if (IS_SET(item->GetAntiFlag(), ITEM_ANTIFLAG_SELL))
		return;

	DWORD dwPrice;

	if (bCount == 0 || bCount > item->GetCount())
		bCount = item->GetCount();

	dwPrice = item->GetShopBuyPrice();

	if (IS_SET(item->GetFlag(), ITEM_FLAG_COUNT_PER_1GOLD))
	{
		if (dwPrice == 0)
			dwPrice = bCount;
		else
			dwPrice = bCount / dwPrice;
	}
	else
		dwPrice *= bCount;

	dwPrice /= 5;
	
	//세금 계산
	DWORD dwTax = 0;
	int iVal = 0;
	
	if (LC_IsYMIR() ||  LC_IsKorea())
	{
		dwTax = dwPrice * iVal / 100;
		dwPrice -= dwTax;
	}
	else
	{
		dwTax = dwPrice * iVal/100;
		dwPrice -= dwTax;
	}

	if (test_server)
		sys_log(0, "Sell Item price id %d %s itemid %d", ch->GetPlayerID(), ch->GetName(), item->GetID());

	const int64_t nTotalMoney = static_cast<int64_t>(ch->GetGold()) + static_cast<int64_t>(dwPrice);

	if (GOLD_MAX <= nTotalMoney)
	{
		sys_err("[OVERFLOW_GOLD] id %u name %s gold %u", ch->GetPlayerID(), ch->GetName(), ch->GetGold());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("20억냥이 초과하여 물품을 팔수 없습니다."));
		return;
	}

	// 20050802.myevan.상점 판매 로그에 아이템 ID 추가
	sys_log(0, "SHOP: SELL: %s item name: %s(x%d):%u price: %u", ch->GetName(), item->GetName(), bCount, item->GetID(), dwPrice);

	if (iVal > 0)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("판매금액의 %d %% 가 세금으로 나가게됩니다"), iVal);

	DBManager::instance().SendMoneyLog(MONEY_LOG_SHOP, item->GetVnum(), dwPrice);

	if (bCount == item->GetCount())
	{
		// 한국에는 아이템을 버리고 복구해달라는 진상유저들이 많아서
		// 상점 판매시 속성로그를 남긴다.
		if (LC_IsYMIR())
			item->AttrLog();

		ITEM_MANAGER::instance().RemoveItem(item, "SELL");
	}
	else
		item->SetCount(item->GetCount() - bCount);

	//군주 시스템 : 세금 징수
	CMonarch::instance().SendtoDBAddMoney(dwTax, ch->GetEmpire(), ch);

	ch->PointChange(POINT_GOLD, dwPrice, false);
}

bool CompareShopItemName(const SShopItemTable& lhs, const SShopItemTable& rhs)
{
	TItemTable* lItem = ITEM_MANAGER::instance().GetTable(lhs.vnum);
	TItemTable* rItem = ITEM_MANAGER::instance().GetTable(rhs.vnum);
	if (lItem && rItem)
		return strcmp(lItem->szLocaleName, rItem->szLocaleName) < 0;
	else
		return true;
}

bool ConvertToShopItemTable(IN CGroupNode* pNode, OUT TShopTableEx& shopTable)
{
	if (!pNode->GetValue("vnum", 0, shopTable.dwVnum))
	{
		sys_err("Group %s does not have vnum.", pNode->GetNodeName().c_str());
		return false;
	}

	if (!pNode->GetValue("name", 0, shopTable.name))
	{
		sys_err("Group %s does not have name.", pNode->GetNodeName().c_str());
		return false;
	}
	
	if (shopTable.name.length() >= SHOP_TAB_NAME_MAX)
	{
		sys_err("Shop name length must be less than %d. Error in Group %s, name %s", SHOP_TAB_NAME_MAX, pNode->GetNodeName().c_str(), shopTable.name.c_str());
		return false;
	}

	std::string stCoinType;
	if (!pNode->GetValue("cointype", 0, stCoinType))
	{
		stCoinType = "Gold";
	}
	
	if (boost::iequals(stCoinType, "Gold"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_GOLD;
	}
	else if (boost::iequals(stCoinType, "SecondaryCoin"))
	{
		shopTable.coinType = SHOP_COIN_TYPE_SECONDARY_COIN;
	}
	else
	{
		sys_err("Group %s has undefine cointype(%s).", pNode->GetNodeName().c_str(), stCoinType.c_str());
		return false;
	}

	CGroupNode* pItemGroup = pNode->GetChildNode("items");
	if (!pItemGroup)
	{
		sys_err("Group %s does not have 'group items'.", pNode->GetNodeName().c_str());
		return false;
	}

	int itemGroupSize = pItemGroup->GetRowCount();
	std::vector <TShopItemTable> shopItems(itemGroupSize);
	if (itemGroupSize >= SHOP_HOST_ITEM_MAX_NUM)
	{
		sys_err("count(%d) of rows of group items of group %s must be smaller than %d", itemGroupSize, pNode->GetNodeName().c_str(), SHOP_HOST_ITEM_MAX_NUM);
		return false;
	}

	for (int i = 0; i < itemGroupSize; i++)
	{
		if (!pItemGroup->GetValue(i, "vnum", shopItems[i].vnum))
		{
			sys_err("row(%d) of group items of group %s does not have vnum column", i, pNode->GetNodeName().c_str());
			return false;
		}
		
		if (!pItemGroup->GetValue(i, "count", shopItems[i].count))
		{
			sys_err("row(%d) of group items of group %s does not have count column", i, pNode->GetNodeName().c_str());
			return false;
		}
		if (!pItemGroup->GetValue(i, "price", shopItems[i].price))
		{
			sys_err("row(%d) of group items of group %s does not have price column", i, pNode->GetNodeName().c_str());
			return false;
		}
	}
	std::string stSort;
	if (!pNode->GetValue("sort", 0, stSort))
	{
		stSort = "None";
	}

	if (boost::iequals(stSort, "Asc"))
	{
		std::sort(shopItems.begin(), shopItems.end(), CompareShopItemName);
	}
	else if(boost::iequals(stSort, "Desc"))
	{
		std::sort(shopItems.rbegin(), shopItems.rend(), CompareShopItemName);
	}

	CGrid grid = CGrid(5, 9);
	int iPos;

	memset(&shopTable.items[0], 0, sizeof(shopTable.items));

	for (int i = 0; i < shopItems.size(); i++)
	{
		TItemTable * item_table = ITEM_MANAGER::instance().GetTable(shopItems[i].vnum);
		if (!item_table)
		{
			sys_err("vnum(%d) of group items of group %s does not exist", shopItems[i].vnum, pNode->GetNodeName().c_str());
			return false;
		}

		iPos = grid.FindBlank(1, item_table->bSize);

		grid.Put(iPos, 1, item_table->bSize);
		shopTable.items[iPos] = shopItems[i];
	}

	shopTable.byItemCount = shopItems.size();
	return true;
}

bool CShopManager::ReadShopTableEx(const char* stFileName)
{
	// file 유무 체크.
	// 없는 경우는 에러로 처리하지 않는다.
	FILE* fp = fopen(stFileName, "rb");
	if (NULL == fp)
		return true;
	fclose(fp);

	CGroupTextParseTreeLoader loader;
	if (!loader.Load(stFileName))
	{
		sys_err("%s Load fail.", stFileName);
		return false;
	}

	CGroupNode* pShopNPCGroup = loader.GetGroup("shopnpc");
	if (NULL == pShopNPCGroup)
	{
		sys_err("Group ShopNPC is not exist.");
		return false;
	}

	typedef std::multimap <DWORD, TShopTableEx> TMapNPCshop;
	TMapNPCshop map_npcShop;
	for (int i = 0; i < pShopNPCGroup->GetRowCount(); i++)
	{
		DWORD npcVnum;
		std::string shopName;
		if (!pShopNPCGroup->GetValue(i, "npc", npcVnum) || !pShopNPCGroup->GetValue(i, "group", shopName))
		{
			sys_err("Invalid row(%d). Group ShopNPC rows must have 'npc', 'group' columns", i);
			return false;
		}
		std::transform(shopName.begin(), shopName.end(), shopName.begin(), (int(*)(int))std::tolower);
		CGroupNode* pShopGroup = loader.GetGroup(shopName.c_str());
		if (!pShopGroup)
		{
			sys_err("Group %s is not exist.", shopName.c_str());
			return false;
		}
		TShopTableEx table;
		if (!ConvertToShopItemTable(pShopGroup, table))
		{
			sys_err("Cannot read Group %s.", shopName.c_str());
			return false;
		}
		if (m_map_pkShopByNPCVnum.find(npcVnum) != m_map_pkShopByNPCVnum.end())
		{
			sys_err("%d cannot have both original shop and extended shop", npcVnum);
			return false;
		}
		
		map_npcShop.insert(TMapNPCshop::value_type(npcVnum, table));	
	}

	for (TMapNPCshop::iterator it = map_npcShop.begin(); it != map_npcShop.end(); ++it)
	{
		DWORD npcVnum = it->first;
		TShopTableEx& table = it->second;
		if (m_map_pkShop.find(table.dwVnum) != m_map_pkShop.end())
		{
			sys_err("Shop vnum(%d) already exists", table.dwVnum);
			return false;
		}
		TShopMap::iterator shop_it = m_map_pkShopByNPCVnum.find(npcVnum);
		
		LPSHOPEX pkShopEx = NULL;
		if (m_map_pkShopByNPCVnum.end() == shop_it)
		{
			pkShopEx = M2_NEW CShopEx;
			pkShopEx->Create(0, npcVnum);
			m_map_pkShopByNPCVnum.insert(TShopMap::value_type(npcVnum, pkShopEx));
		}
		else
		{
			pkShopEx = dynamic_cast <CShopEx*> (shop_it->second);
			if (NULL == pkShopEx)
			{
				sys_err("WTF!!! It can't be happend. NPC(%d) Shop is not extended version.", shop_it->first);
				return false;
			}
		}

		if (pkShopEx->GetTabCount() >= SHOP_TAB_COUNT_MAX)
		{
			sys_err("ShopEx cannot have tab more than %d", SHOP_TAB_COUNT_MAX);
			return false;
		}

		if (pkShopEx->GetVnum() != 0 && m_map_pkShop.find(pkShopEx->GetVnum()) != m_map_pkShop.end())
		{
			sys_err("Shop vnum(%d) already exist.", pkShopEx->GetVnum());
			return false;
		}
		m_map_pkShop.insert(TShopMap::value_type (pkShopEx->GetVnum(), pkShopEx));
		pkShopEx->AddShopTable(table);
	}

	return true;
}
