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
#include "shopEx.h"
#include "group_text_parse_tree.h"

bool CShopEx::Create(DWORD dwVnum, DWORD dwNPCVnum)
{
	m_dwVnum = dwVnum;
	m_dwNPCVnum = dwNPCVnum;
	return true;
}

bool CShopEx::AddShopTable(TShopTableEx& shopTable)
{
	for (itertype(m_vec_shopTabs) it = m_vec_shopTabs.begin(); it != m_vec_shopTabs.end(); it++)
	{
		const TShopTableEx& _shopTable = *it;
		if (0 != _shopTable.dwVnum && _shopTable.dwVnum == shopTable.dwVnum)
			return false;
		if (0 != _shopTable.dwNPCVnum && _shopTable.dwNPCVnum == shopTable.dwVnum)
			return false;
	}
	m_vec_shopTabs.push_back(shopTable);
	return true;
}

bool CShopEx::AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire)
{
	if (!ch)
		return false;

	if (ch->GetExchange())
		return false;

	if (ch->GetShop())
		return false;

	ch->SetShop(this);

	m_map_guest.insert(GuestMapType::value_type(ch, bOtherEmpire));

	TPacketGCShop pack;

	pack.header		= HEADER_GC_SHOP;
	pack.subheader	= SHOP_SUBHEADER_GC_START_EX;

	TPacketGCShopStartEx pack2;

	memset(&pack2, 0, sizeof(pack2));
	
	pack2.owner_vid = owner_vid;
	pack2.shop_tab_count = m_vec_shopTabs.size();
	char temp[8096]; // √÷¥Î 1728 * 3
	char* buf = &temp[0];
	size_t size = 0;
	for (itertype(m_vec_shopTabs) it = m_vec_shopTabs.begin(); it != m_vec_shopTabs.end(); it++)
	{
		const TShopTableEx& shop_tab = *it;
		TPacketGCShopStartEx::TSubPacketShopTab pack_tab;
		pack_tab.coin_type = shop_tab.coinType;
		memcpy(pack_tab.name, shop_tab.name.c_str(), SHOP_TAB_NAME_MAX);
		
		for (BYTE i = 0; i < SHOP_HOST_ITEM_MAX_NUM; i++)
		{
			pack_tab.items[i].vnum = shop_tab.items[i].vnum;
			pack_tab.items[i].count = shop_tab.items[i].count;
			switch(shop_tab.coinType)
			{
			case SHOP_COIN_TYPE_GOLD:
				if (bOtherEmpire) // no empire price penalty for pc shop
					pack_tab.items[i].price = shop_tab.items[i].price * 3;
				else
					pack_tab.items[i].price = shop_tab.items[i].price;
				break;
			case SHOP_COIN_TYPE_SECONDARY_COIN:
				pack_tab.items[i].price = shop_tab.items[i].price;
				break;
			}
			memset(pack_tab.items[i].aAttr, 0, sizeof(pack_tab.items[i].aAttr));
			memset(pack_tab.items[i].alSockets, 0, sizeof(pack_tab.items[i].alSockets));
		}

		memcpy(buf, &pack_tab, sizeof(pack_tab));
		buf += sizeof(pack_tab);
		size += sizeof(pack_tab);
	}

	pack.size = sizeof(pack) + sizeof(pack2) + size;

	ch->GetDesc()->BufferedPacket(&pack, sizeof(TPacketGCShop));
	ch->GetDesc()->BufferedPacket(&pack2, sizeof(TPacketGCShopStartEx));
	ch->GetDesc()->Packet(temp, size);

	return true;
}

int CShopEx::Buy(LPCHARACTER ch, BYTE pos)
{
	BYTE tabIdx = pos / SHOP_HOST_ITEM_MAX_NUM;
	BYTE slotPos = pos % SHOP_HOST_ITEM_MAX_NUM;
	if (tabIdx >= GetTabCount())
	{
		sys_log(0, "ShopEx::Buy : invalid position %d : %s", pos, ch->GetName());
		return SHOP_SUBHEADER_GC_INVALID_POS;
	}

	sys_log(0, "ShopEx::Buy : name %s pos %d", ch->GetName(), pos);

	GuestMapType::iterator it = m_map_guest.find(ch);

	if (it == m_map_guest.end())
		return SHOP_SUBHEADER_GC_END;

	TShopTableEx& shopTab = m_vec_shopTabs[tabIdx];
	TShopItemTable& r_item = shopTab.items[slotPos];

	if (r_item.price <= 0)
	{
		LogManager::instance().HackLog("SHOP_BUY_GOLD_OVERFLOW", ch);
		return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
	}

	DWORD dwPrice = r_item.price;

	switch (shopTab.coinType)
	{
	case SHOP_COIN_TYPE_GOLD:
		if (it->second)	// if other empire, price is triple
			dwPrice *= 3;

		if (ch->GetGold() < (int) dwPrice)
		{
			sys_log(1, "ShopEx::Buy : Not enough money : %s has %d, price %d", ch->GetName(), ch->GetGold(), dwPrice);
			return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY;
		}
		break;
	case SHOP_COIN_TYPE_SECONDARY_COIN:
		{
			int count = ch->CountSpecifyTypeItem(ITEM_SECONDARY_COIN);
			if (count < dwPrice)
			{
				sys_log(1, "ShopEx::Buy : Not enough myeongdojun : %s has %d, price %d", ch->GetName(), count, dwPrice);
				return SHOP_SUBHEADER_GC_NOT_ENOUGH_MONEY_EX;
			}
		}
		break;
	}
	
	LPITEM item;

	item = ITEM_MANAGER::instance().CreateItem(r_item.vnum, r_item.count);

	if (!item)
		return SHOP_SUBHEADER_GC_SOLD_OUT;

	int iEmptyPos;
	if (item->IsDragonSoul())
	{
		iEmptyPos = ch->GetEmptyDragonSoulInventory(item);
	}
	else
	{
		iEmptyPos = ch->GetEmptyInventory(item->GetSize());
	}

	if (iEmptyPos < 0)
	{
		sys_log(1, "ShopEx::Buy : Inventory full : %s size %d", ch->GetName(), item->GetSize());
		M2_DESTROY_ITEM(item);
		return SHOP_SUBHEADER_GC_INVENTORY_FULL;
	}

	switch (shopTab.coinType)
	{
	case SHOP_COIN_TYPE_GOLD:
		ch->PointChange(POINT_GOLD, -dwPrice, false);
		break;
	case SHOP_COIN_TYPE_SECONDARY_COIN:
		ch->RemoveSpecifyTypeItem(ITEM_SECONDARY_COIN, dwPrice);
		break;
	}


	if (item->IsDragonSoul())
		item->AddToCharacter(ch, TItemPos(DRAGON_SOUL_INVENTORY, iEmptyPos));
	else
		item->AddToCharacter(ch, TItemPos(INVENTORY, iEmptyPos));

	ITEM_MANAGER::instance().FlushDelayedSave(item);
	LogManager::instance().ItemLog(ch, item, "BUY", item->GetName());

	if (item->GetVnum() >= 80003 && item->GetVnum() <= 80007)
	{
		LogManager::instance().GoldBarLog(ch->GetPlayerID(), item->GetID(), PERSONAL_SHOP_BUY, "");
	}

	DBManager::instance().SendMoneyLog(MONEY_LOG_SHOP, item->GetVnum(), -dwPrice);

	if (item)
		sys_log(0, "ShopEx: BUY: name %s %s(x %d):%u price %u", ch->GetName(), item->GetName(), item->GetCount(), item->GetID(), dwPrice);

	if (LC_IsBrazil())
	{
		ch->SaveReal();
		db_clientdesc->DBPacketHeader(HEADER_GD_FLUSH_CACHE, 0, sizeof(DWORD));
		DWORD pid = ch->GetPlayerID();
		db_clientdesc->Packet(&pid, sizeof(DWORD));
	}
	else
	{
		ch->Save();
	}

    return (SHOP_SUBHEADER_GC_OK);
}

