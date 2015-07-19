#include "stdafx.h"
#ifdef __AUCTION__

#include "desc_client.h"
#include "desc_manager.h"
#include "buffer_manager.h"
#include "packet.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "log.h"
#include "db.h"
#include <boost/bind.hpp>
#include "item.h"
#include "desc_client.h"
#include "../../common/tables.h"
#include "protocol.h"
#include "auction_manager.h"
extern int auction_server;

const char* auction_table_name [_AUCTION_MAX] = {"auction", "wish_auction", "my_auction", "my_wish_auction"};

bool CompareItemInfoByItemNameAC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (strcmp (i->item_proto->szLocaleName, j->item_proto->szLocaleName) < 0);
}

bool CompareItemInfoByItemNameDC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (strcmp (i->item_proto->szLocaleName, j->item_proto->szLocaleName) > 0);
}

bool CompareItemInfoByCategoryAC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (i->item_proto->bType < j->item_proto->bType);
}

bool CompareItemInfoByCategoryDC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (i->item_proto->bType > j->item_proto->bType);
}

bool CompareItemInfoByTimeAC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (i->expired_time > j->expired_time);
}

bool CompareItemInfoByTimeDC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (i->expired_time < j->expired_time);
}

bool CompareItemInfoByCharNameAC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (strcmp (i->shown_name, j->shown_name) < 0);
}

bool CompareItemInfoByCharNameDC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (strcmp (i->shown_name, j->shown_name) > 0);
}

bool CompareItemInfoByPriceAC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (i->get_price() < j->get_price());
}

bool CompareItemInfoByPriceDC (TAuctionItemInfo* i, TAuctionItemInfo* j)
{
	return (i->get_price() > j->get_price());
}

void AuctionBoard::Sort(TItemInfoVec& vec, BYTE order)
{
	switch (order)
	{
	case AUCTION_ITEM_NAME_AC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByItemNameAC);
		break;
	case AUCTION_ITEM_NAME_DC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByItemNameDC);
		break;
	case AUCTION_CATEGORY_AC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByCategoryAC);
		break;
	case AUCTION_CATEGORY_DC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByCategoryDC);
		break;
	case AUCTION_TIME_AC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByTimeAC);
		break;
	case AUCTION_TIME_DC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByTimeDC);
		break;
	case AUCTION_CHAR_NAME_AC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByCharNameAC);
		break;
	case AUCTION_CHAR_NAME_DC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByCharNameDC);
		break;
	case AUCTION_PRICE_AC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByPriceAC);
		break;
	case AUCTION_PRICE_DC:
		std::stable_sort (vec.begin(), vec.end(), CompareItemInfoByPriceDC);
		break;
	}
}

bool AuctionBoard::InsertItemInfo (TAuctionItemInfo* item_info)
{
	TAuctionItemInfo* c = GetItemInfo (item_info->item_id);
	if (c != NULL)
	{
		return false;
	}

	c = new TAuctionItemInfo();
	thecore_memcpy (c, item_info, sizeof(TAuctionItemInfo));

	c->item_proto = ITEM_MANAGER::instance().GetTable(c->item_num);
	item_map.insert(TItemInfoMap::value_type(item_info->item_id, c));

	DWORD offer_id = item_info->get_offer_id();

	TPCMap::iterator pc_it = offer_map.find (offer_id);
	TItemMap* item_map;
	if (pc_it == offer_map.end())
	{
		item_map = new TItemMap();
		offer_map.insert(TPCMap::value_type (offer_id, item_map));
	}
	else
	{
		item_map = pc_it->second;
	}
	DWORD item_id = item_info->item_id;
	TItemMap::iterator item_it = item_map->find (item_id);
	if (item_it == item_map->end())
	{
		item_map->insert (TItemMap::value_type (item_id, c));
		return true;
	}

	return false;
#ifdef FAST_SORT
	SortByItemName::iterator it = item_name_map.find (std::string (c->item_proto->szName));
	TItemInfoVec* vec;

	if (it == item_name_map.end())
	{
		vec = new TItemInfoVec();
		item_name_map.insert (SortByItemName::value_type (std::string (c->item_proto->szName), vec));
	}
	else
	{
		vec = it->second;
	}
	vec.push_back (c);

	return true;
#endif
}

bool AuctionBoard::UpdateItemInfo (TAuctionItemInfo* item_info)
{
	TAuctionItemInfo* c = GetItemInfo (item_info->item_id);
	if (c == NULL)
	{
		return false;
	}

	thecore_memcpy (c, item_info, sizeof(TAuctionItemInfo));
	return true;
}

bool AuctionBoard::DeleteItemInfo (DWORD key)
{
	TItemInfoMap::iterator it = item_map.find (key);
	if (it == item_map.end())
		return false;
	else
	{
		TAuctionItemInfo* item_info = it->second;
		delete item_info;
		item_map.erase(it);
		return true;
	}
}

TAuctionItemInfo* AuctionBoard::GetItemInfo (DWORD key)
{
	TItemInfoMap::iterator it = item_map.find (key);
	if (it == item_map.end())
		return NULL;
	else
		return it->second;
}

void AuctionBoard::YourItemInfoList (TItemInfoVec& vec, DWORD player_id, int start_idx, BYTE size)
{
	int pass = 0;
	vec.clear();
	TPCMap::iterator offer_it = offer_map.find (player_id);
	if (offer_it == offer_map.end())
		return;
	TItemMap* item_map = offer_it->second;
	for (TItemMap::iterator item_it = item_map->begin(); item_it != item_map->end() && vec.size() < size; item_it++)
	{
		if (pass >= start_idx)
		{
			vec.push_back (item_it->second);
		}
	}
}

// 0~1, 2~3, 4~5, 6~7, 8~9
// 짝수면 descending, 홀수면 accending.
struct FCheckGradeSatisfied
{
	BYTE grade;

	bool operator() (int item_level)
	{
		switch (grade)
		{
		case 0:
			return true;
			break;
		case 1:
			if (item_level <= GRADE_LOW)
			{
				return true;
			}
			break;
		case 2:
			if (GRADE_LOW < item_level && item_level <= GRADE_MID)
			{
				return true;
			}
			break;
		case 3:
			if (GRADE_MID < item_level && item_level <= GRADE_HIGH)
			{
				return true;
			}
			break;
		default:
			return true;
		}
		return false;
	}
};

void AuctionBoard::SortedItemInfos (TItemInfoVec& vec, BYTE grade, BYTE category, int start_idx, BYTE size, BYTE order[5])
{
	FCheckGradeSatisfied f_grade = { grade };
	vec.clear();
	int pass = 0;
	switch (order[4])
	{
#ifdef FAST_SORT
	case AUCTION_ITEM_NAME_AC:
		for (SortByItemName::iterator it = item_name_map.begin(); it != item_name_map.end() && vec.size() < size; it++)
		{
			TItemInfoVec& temp = *(it->second);

			pass += temp.size();
			if (pass < start_idx)
			{
				continue;
			}
			for (uint i = 0; i < temp.size(); i++)
			{
				bool check_grade = (grade == 0);
				for (int j = 0; j < ITEM_LIMIT_MAX_NUM; j++)
				{
					if (temp[i]->item_proto->aLimits[j].bType == LIMIT_LEVEL)
					{
						check_grade = f_grade (temp[i]->item_proto->aLimits[j].lValue);
						break;
					}
				}

				if (check_grade)
				{
					vec.push_back (temp[i]);
				}
			}
		}
		break;
#endif
	default :
		for (TItemInfoMap::iterator it = item_map.begin(); it != item_map.end(); it++)
		{
			vec.push_back (it->second);
		}
		break;
	}
	for (int i = 0; i < 5; i++)
	{
		Sort (vec, order[i]);
	}
}

TSaleItemInfo* SaleBoard::GetItemInfo (DWORD item_id)
{
	TItemInfoMap::iterator it = item_map.find (item_id);
	if (it == item_map.end())
		return NULL;

	return it->second;
}

bool SaleBoard::DeleteItemInfo (DWORD item_id)
{
	TItemInfoMap::iterator it = item_map.find (item_id);
	if (it == item_map.end())
		return false;

	TSaleItemInfo* item_info = it->second;
	DeleteFromPCMap (wisher_map, item_info->wisher_id, item_info->item_id);
	DeleteFromPCMap (seller_map, item_info->offer_id, item_info->item_id);

	delete item_info;
	item_map.erase(it);
	return true;
}

bool SaleBoard::DeleteFromPCMap (TPCMap& pc_map, DWORD player_id, DWORD item_id)
{
	TPCMap::iterator pc_it = pc_map.find (player_id);
	if (pc_it == pc_map.end())
	{
		return false;
	}
	TItemMap* item_map = pc_it->second;
	TItemMap::iterator item_it = item_map->find (item_id);
	if (item_it == item_map->end())
	{
		return false;
	}
	item_map->erase (item_it);
	return true;
}

bool SaleBoard::InsertItemInfo (TSaleItemInfo* item_info)
{
	TSaleItemInfo* c = GetItemInfo (DWORD (item_info->item_id));
	if (c != NULL)
	{
		return false;
	}
	c = new TSaleItemInfo ();
	thecore_memcpy (c, item_info, sizeof(TSaleItemInfo));

	c->item_proto = ITEM_MANAGER::instance().GetTable(c->item_num);

	InsertInPCMap (wisher_map, item_info->wisher_id, item_info);
	InsertInPCMap (seller_map, item_info->offer_id, item_info);

	return true;
}

bool SaleBoard::InsertInPCMap (TPCMap& pc_map, DWORD player_id, TSaleItemInfo* item_info)
{
	TPCMap::iterator pc_it = pc_map.find (player_id);
	TItemMap* item_map;
	if (pc_it == pc_map.end())
	{
		item_map = new TItemMap();
		pc_map.insert(TPCMap::value_type (player_id, item_map));
	}
	else
	{
		item_map = pc_it->second;
	}
	DWORD item_id = item_info->item_id;
	TItemMap::iterator item_it = item_map->find (item_id);
	if (item_it == item_map->end())
	{
		item_map->insert (TItemMap::value_type (item_id, item_info));
		return true;
	}
	return false;
}

void SaleBoard::WisherItemInfoList (TItemInfoVec& vec, DWORD wisher_id, int start_idx, BYTE size)
{
	int pass = 0;
	vec.clear();
	TPCMap::iterator wish_it = wisher_map.find (wisher_id);
	if (wish_it == wisher_map.end())
		return;
	TItemMap* item_map = wish_it->second;
	for (TItemMap::iterator item_it = item_map->begin(); item_it != item_map->end() && vec.size() < size; item_it++)
	{
		if (pass >= start_idx)
		{
			vec.push_back (item_it->second);
		}
	}
}

TWishItemInfo* WishBoard::GetItemInfo (DWORD wisher_id, DWORD item_num)
{
	TPCMap::iterator wish_it = wisher_map.find (wisher_id);
	if (wish_it == wisher_map.end())
		return NULL;

	TItemMap* item_map = wish_it->second;
	TItemMap::iterator item_it = item_map->find (item_num);
	if (item_it == item_map->end())
		return NULL;
	else
	{
		return item_it->second;
	}
}

bool WishBoard::DeleteItemInfo (DWORD wisher_id, DWORD item_num)
{
	TPCMap::iterator wish_it = wisher_map.find (wisher_id);
	if (wish_it == wisher_map.end())
		return false;

	TItemMap* item_map = wish_it->second;
	TItemMap::iterator item_it = item_map->find (item_num);
	if (item_it == item_map->end())
		return false;
	else
	{
		delete item_it->second;
		item_map->erase (item_it);
		return true;
	}
}

bool WishBoard::InsertItemInfo (TWishItemInfo* item_info)
{
	DWORD wisher_id = item_info->offer_id;
	DWORD item_num = item_info->item_num;
	
	TPCMap::iterator wish_it = wisher_map.find (wisher_id);
	TItemMap* item_map;
	if (wish_it == wisher_map.end())
	{
		item_map = new TItemMap();
		wisher_map.insert (TPCMap::value_type (wisher_id, item_map));
	}
	else
	{
		item_map = wish_it->second;
	}
	
	TItemMap::iterator item_it = item_map->find (item_num);
	if (item_it == item_map->end())
	{
		TWishItemInfo* c = new TWishItemInfo();
		thecore_memcpy (c, item_info, sizeof(TWishItemInfo));

		c->item_proto = ITEM_MANAGER::instance().GetTable(c->item_num);
		item_map->insert (TItemMap::value_type (item_num, c));
		return true;
	}
	else
	{
		return false;
	}
}

std::pair <int, bool> MyBidBoard::GetMoney (DWORD player_id, DWORD item_id)
{
	TMyBidBoard::iterator pc_it = pc_map.find (player_id);
	if (pc_it == pc_map.end())
	{
		return BidInfo (-1, false);
	}
	TItemMap* item_map = pc_it->second;
	TItemMap::iterator it = item_map->find (item_id);
	if (it == item_map->end())
		return BidInfo (-1, false);
	else
		return it->second;
}

bool MyBidBoard::Delete (DWORD player_id, DWORD item_id)
{
	TMyBidBoard::iterator pc_it = pc_map.find (player_id);
	if (pc_it == pc_map.end())
	{
		return false;
	}
	TItemMap* item_map = pc_it->second;
	TItemMap::iterator it = item_map->find (item_id);
	if (it == item_map->end())
		return false;
	else
	{
		item_map->erase(it);
		return true;
	}
}

void MyBidBoard::Insert (DWORD player_id, DWORD item_id, int money)
{
	TMyBidBoard::iterator pc_it = pc_map.find (player_id);
	TItemMap* item_map;
	if (pc_it == pc_map.end())
	{
		item_map = new TItemMap();
		pc_map.insert (TMyBidBoard::value_type (player_id, item_map));
	}
	else
		item_map = pc_it->second;

	TItemMap::iterator it = item_map->find (item_id);
	if (it == item_map->end())
	{
		item_map->insert (TItemMap::value_type (item_id, std::pair <int, bool> (money, false)));
	}
	else
	{
		it->second = BidInfo (money, false);
	}
}

void MyBidBoard::Lock (DWORD player_id, DWORD item_id)
{
	TMyBidBoard::iterator pc_it = pc_map.find (player_id);
	if (pc_it == pc_map.end())
	{
		return;
	}
	TItemMap* item_map = pc_it->second;
	TItemMap::iterator it = item_map->find (item_id);
	if (it == item_map->end())
		return;
	else
		it->second.second = true;
}

void MyBidBoard::UnLock (DWORD player_id, DWORD item_id)
{
	TMyBidBoard::iterator pc_it = pc_map.find (player_id);
	if (pc_it == pc_map.end())
	{
		return;
	}
	TItemMap* item_map = pc_it->second;
	TItemMap::iterator it = item_map->find (item_id);
	if (it == item_map->end())
		return;
	else
		it->second.second = false;
}

void MyBidBoard::YourBidInfo (TItemVec& vec, DWORD bidder_id, int start_idx, int size)
{
	vec.clear();
	TMyBidBoard::iterator pc_it = pc_map.find (bidder_id);
	if (pc_it == pc_map.end())
		return;
	TItemMap* item_map = pc_it->second;
	int pass = 0;
	for (TItemMap::iterator it = item_map->begin(); it != item_map->end() && vec.size() < size; it++, pass++)
	{
		if (pass >= start_idx)
		{
			vec.push_back (it->first);
		}
	}
}

void AuctionManager::Boot (const char* &data, WORD size)
{
	if (decode_2bytes(data) != sizeof(TPlayerItem))
	{
		sys_err("TPlayerItem table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	TPlayerItem* item = (TPlayerItem*) data;
	data += size * sizeof(TPlayerItem);
	if (auction_server)
	{
		for (WORD i = 0; i < size; ++i, ++item)
		{
			InsertItem (item);
		}

	}
	if (decode_2bytes(data) != sizeof(TAuctionItemInfo))
	{
		sys_err("TAuctionItemInfo table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	TAuctionItemInfo* auction_item_info = (TAuctionItemInfo*) data;
	data += size * sizeof(TAuctionItemInfo);
	if (auction_server)
	{
		for (WORD i = 0; i < size; ++i, ++auction_item_info)
		{
			Auction.InsertItemInfo (auction_item_info);
		}
	}
	if (decode_2bytes(data) != sizeof(TSaleItemInfo))
	{
		sys_err("TSaleItemInfo table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	TSaleItemInfo* sale_item_info = (TSaleItemInfo*) data;
	data += size * sizeof(TSaleItemInfo);
	if (auction_server)
	{
		for (WORD i = 0; i < size; ++i, ++sale_item_info)
		{
			Sale.InsertItemInfo (sale_item_info);
		}
	}

	if (decode_2bytes(data) != sizeof(TWishItemInfo))
	{
		sys_err("TWishItemInfo table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	TWishItemInfo* wish_item_info = (TWishItemInfo*) data;
	data += size * sizeof(TWishItemInfo);
	if (auction_server)
	{
		for (WORD i = 0; i < size; ++i, ++wish_item_info)
		{
			Wish.InsertItemInfo (wish_item_info);
		}
	}

	if (decode_2bytes(data) != (sizeof(DWORD) + sizeof(DWORD) + sizeof(int)))
	{
		sys_err("my_bid table size error");
		thecore_shutdown();
		return;
	}
	data += 2;

	size = decode_2bytes(data);
	data += 2;

	if (auction_server)
	{
		for (WORD i = 0; i < size; i++)
		{
			DWORD player_id = *((DWORD*) data);
			data += sizeof(DWORD);
			DWORD item_id = *((DWORD*) data);
			data += sizeof(DWORD);
			int money = *((int*) data);
			data += sizeof(int);
			MyBid.Insert (player_id, item_id, money);
		}
	}
	else
		data += size * (sizeof(DWORD) + sizeof(DWORD) + sizeof(int));
}

bool AuctionManager::InsertItem (LPITEM item)
{
	TItemMap::iterator it = auction_item_map.find (item->GetID());
	if (it != auction_item_map.end())
		return false;
	auction_item_map.insert(TItemMap::value_type (item->GetID(), item));
	return true;
}

bool AuctionManager::InsertItem (TPlayerItem* player_item)
{
	if (player_item->window != AUCTION)
		return false;
	LPITEM item = ITEM_MANAGER::instance().CreateItem(player_item->vnum, player_item->count, player_item->id, false, -1, true);

	if (!item)
	{
		sys_err("cannot create item vnum %d id %u",player_item->vnum, player_item->id);
		return false;
	}

	item->SetSkipSave(true);

	if (!InsertItem(item))
	{
		M2_DESTROY_ITEM(item);
		return false;
	}
	item->SetSockets(player_item->alSockets);
	item->SetAttributes(player_item->aAttr);
	item->SetWindow(AUCTION);

	return true;
}

LPITEM AuctionManager::GetInventoryItem (DWORD item_id)
{
	TItemMap::iterator it = auction_item_map.find (item_id);
	if (it == auction_item_map.end())
		return NULL;
	else
		return it->second;
}

bool AuctionManager::DeleteItem (DWORD item_id)
{
	TItemMap::iterator it = auction_item_map.find (item_id);
	if (it == auction_item_map.end())
		return false;
	else
	{
		auction_item_map.erase (it);
		return true;
	}
}
//
//bool AuctionManager::InsertAuctionMoney (DWORD player_id, int money)
//{
//	TAuctionMoneyMap::iterator it = auction_item_map.find (player_id);
//	if (it == auction_item_map.end())
//		return false;
//	auction_item_map.insert (TAuctionMoneyMap::value_type (player_id, money));
//}
//
//bool AuctionManager::ChangeAuctionMoney (DWORD player_id, int changing_amount)
//{
//	TAuctionMoneyMap::iterator it = auction_item_map.find (player_id);
//	if (it == auction_item_map.end())
//		return false;
//	if (it->second + changing_amount < 0)
//	{
//		sys_err ("Cannot have money under 0.");
//		return false;
//	}
//	
//	TPacketGDCommnadAuction pack_cm;
//	pack_cm.changing_money (it->second);
//
//	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, player_id, &pack_cm, sizeof(TPacketGDCommnadAuction));
//}


bool AuctionManager::InsertAuctionItemInfo (TAuctionItemInfo* item_info)
{
	return Auction.InsertItemInfo (item_info);
}

bool AuctionManager::InsertSaleItemInfo (TSaleItemInfo* item_info)
{
	return Sale.InsertItemInfo (item_info);
}

bool AuctionManager::InsertWishItemInfo (TWishItemInfo* item_info)
{
	return Wish.InsertItemInfo (item_info);
}

void AuctionManager::YourBidItemInfoList (AuctionBoard::TItemInfoVec& vec, DWORD bidder_id, int start_idx, int size)
{
	vec.clear();
	MyBidBoard::TItemVec item_id_vec;
	MyBid.YourBidInfo (item_id_vec, bidder_id, 0, 6);
	for (int i = 0; i < item_id_vec.size(); i++)
	{
		TAuctionItemInfo* item_info = Auction.GetItemInfo(item_id_vec[i]);
		if (item_info != NULL)
		{
			vec.push_back (item_info);
		}
		else
		{
			// expired 만들고 여기서 넣어야한다.
		}
	}
}

void AuctionManager::get_auction_list (LPCHARACTER ch, int start_idx, int size, int cond)
{
	BYTE order[5] = {0,};
	AuctionBoard::TItemInfoVec vec;
	switch (cond)
	{
	case 0:
		order[4] = AUCTION_ITEM_NAME_AC;
		order[3] = AUCTION_PRICE_DC;
		Auction.SortedItemInfos (vec, 0, 0, 0, 12, order);

		//		SortedItemInfos
		break;
	case 1:
		order[4] = AUCTION_PRICE_DC;
		order[3] = AUCTION_ITEM_NAME_DC;
		Auction.SortedItemInfos (vec, 0, 0, 5, 12, order);

		break;
	case 2:
		order[4] = AUCTION_CHAR_NAME_DC;
		order[3] = AUCTION_ITEM_NAME_DC;
		order[2] = AUCTION_TIME_AC;
		Auction.SortedItemInfos (vec, 0, 0, 0, 12, order);

		break;

	}
	for (uint i = 0; i < vec.size(); i++)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s item id : %d price : %d", vec[i]->item_proto->szName, vec[i]->get_item_id(), vec[i]->get_price());
	}
}

void AuctionManager::get_my_auction_list (LPCHARACTER ch, int start_idx, int size)
{
	AuctionBoard::TItemInfoVec auction_vec;
	Auction.YourItemInfoList (auction_vec, ch->GetPlayerID(), 0, 6);

	for (uint i = 0; i < auction_vec.size(); i++)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s item id : %d price : %d", auction_vec[i]->item_proto->szName, auction_vec[i]->get_item_id(), auction_vec[i]->get_price());
	}
	SaleBoard::TItemInfoVec wish_vec;
	Sale.WisherItemInfoList (wish_vec, ch->GetPlayerID(), 0, 6);
	for (uint i = 0; i < auction_vec.size(); i++)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s item id : %d price : %d", wish_vec[i]->item_proto->szName, wish_vec[i]->item_id, 
			wish_vec[i]->get_price());
	}
}

void AuctionManager::get_my_purchase_list (LPCHARACTER ch, int start_idx, int size)
{
	AuctionBoard::TItemInfoVec auction_vec;
	YourBidItemInfoList (auction_vec, ch->GetPlayerID(), 0, 6);

	for (uint i = 0; i < auction_vec.size(); i++)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "%s item id : %d price : %d", auction_vec[i]->item_proto->szName, auction_vec[i]->get_item_id(), auction_vec[i]->get_price());
	}

	//WishBoard::TItemInfoVec wish_vec;
	//Wish.WisherItemInfoList (wish_vec, ch->GetPlayerID(), 0, 6);
	//for (uint i = 0; i < auction_vec.size(); i++)
	//{
	//	ch->ChatPacket(CHAT_TYPE_INFO, "%s item id : %d price : %d", wish_vec[i]->item_proto->szName, wish_vec[i]->item_id, 
	//		wish_vec[i]->get_price());
	//}
}

void AuctionManager::enroll_auction (LPCHARACTER ch, LPITEM item, BYTE empire, int bidPrice, int immidiatePurchasePrice)
{
	if (ch != item->GetOwner())
	{
		sys_err ("Item %d's owner is %s, not %s",ch->GetName(), item->GetOwner()->GetName());
		return;
	}
	if (item->IsEquipped())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "장착한 건 등록할 수 없어.");
		return;
	}

	if (GetAuctionItemInfo (item->GetID()))
	{
		sys_err ("Item %d is already in auction.", item->GetID());
		ch->ChatPacket(CHAT_TYPE_INFO, "이미 등록한 거야. 도대체 뭐지?");
		return;
	}

	if (item->GetWindow() == AUCTION)
	{
		sys_err ("Item %d is already in auction.", item->GetID());
		ch->ChatPacket(CHAT_TYPE_INFO, "얜 또 뭐냐..");
		return;
	}

	TPacketGDCommnadAuction pack_ea;
	pack_ea.enroll_product (item->GetID(), empire, bidPrice, immidiatePurchasePrice);

	item->SetSkipSave(true);
	item->RemoveFromCharacter();
	M2_DESTROY_ITEM (item);

	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_ea, sizeof(TPacketGDCommnadAuction));
}

void AuctionManager::enroll_sale (LPCHARACTER ch, LPITEM item, DWORD wisher_id, int salePrice)
{
	if (ch != item->GetOwner())
	{
		sys_err ("Item %d's owner is %s, not %s",ch->GetName(), item->GetOwner()->GetName());
		return;
	}
	if (item->IsEquipped())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "장착한 건 등록할 수 없어.");
		return;
	}

	if (GetSaleItemInfo (item->GetID()))
	{
		sys_err ("Item %d is already in auction.", item->GetID());
		ch->ChatPacket(CHAT_TYPE_INFO, "이미 등록한 거야. 도대체 뭐지?");
		return;
	}

	if (item->GetWindow() == AUCTION)
	{
		sys_err ("Item %d is already in auction.", item->GetID());
		ch->ChatPacket(CHAT_TYPE_INFO, "얜 또 뭐냐..");
		return;
	}

	TPacketGDCommnadAuction pack_es;
	pack_es.enroll_sale (item->GetID(), wisher_id, salePrice);

	item->SetSkipSave(true);
	item->RemoveFromCharacter();
	M2_DESTROY_ITEM (item);

	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_es, sizeof(TPacketGDCommnadAuction));
}

void AuctionManager::enroll_wish (LPCHARACTER ch, DWORD item_num, BYTE empire, int wishPrice)
{
	TPacketGDCommnadAuction pack_ew;
	pack_ew.enroll_wish (item_num, empire, wishPrice);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_ew, sizeof(TPacketGDCommnadAuction));
}

// fixme
void AuctionManager::bid (LPCHARACTER ch, DWORD item_id, int bid_price)
{
	std::pair <int, bool> mb = MyBid.GetMoney(ch->GetPlayerID(), item_id);
	if (mb.first != -1)
	{
		ch->ChatPacket (CHAT_TYPE_INFO, "재입찰을 하란 말이다.");
	}
	if (ch->GetGold() < bid_price)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "돈이 부족해");
		return;
	}

	ch->PointChange(POINT_GOLD, -bid_price, false);

	TPacketGDCommnadAuction pack_bid;
	pack_bid.bid (item_id, bid_price);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_bid, sizeof(TPacketGDCommnadAuction));
}

// fixme
// 반드시 돈!!!
void AuctionManager::immediate_purchase (LPCHARACTER ch, DWORD item_id)
{
	TAuctionItemInfo* item_info = GetAuctionItemInfo (item_id);

	if (item_info == NULL)
	{
		sys_err ("Invild item id : %d", item_id);
		return;
	}

	if (item_info->get_impur_price() == 0)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "즉구 할 수 엄서");
		return;
	}

	if (ch->GetGold() < item_info->get_impur_price())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "돈이 부족해");
		return;
	}

	ch->PointChange(POINT_GOLD, -item_info->get_impur_price(), false);

	TPacketGDCommnadAuction pack_impur;
	pack_impur.impur (item_id);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_impur, sizeof(TPacketGDCommnadAuction));
}

// 시작
void AuctionManager::get_auctioned_item (LPCHARACTER ch, DWORD item_id, DWORD item_num)
{
	TItemTable* proto = ITEM_MANAGER::instance().GetTable(item_num);
	int pos = ch->GetEmptyInventory(proto->bSize);

	if (pos == -1)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "자리가 엄서");
		return;
	}

	//TAuctionItemInfo* 
	//	if (Auction.GetItemInfo (item_id))
	//	{

	//	}

	TPacketGDCommnadAuction pack_gai;
	pack_gai.get_auctioned_item (item_id);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_gai, sizeof(TPacketGDCommnadAuction));
}

void AuctionManager::buy_sold_item (LPCHARACTER ch, DWORD item_id)
{
	TPacketGDCommnadAuction pack_bsi;
	pack_bsi.buy_sold_item (item_id);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_bsi, sizeof(TPacketGDCommnadAuction));
}

void AuctionManager::cancel_auction (LPCHARACTER ch, DWORD item_id)
{
	TPacketGDCommnadAuction pack_ca;
	pack_ca.cancel_auction (item_id);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_ca, sizeof(TPacketGDCommnadAuction));
}

void AuctionManager::cancel_wish (LPCHARACTER ch, DWORD item_num)
{
	TPacketGDCommnadAuction pack_cw;
	pack_cw.cancel_wish (item_num);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_cw, sizeof(TPacketGDCommnadAuction));
}

void AuctionManager::cancel_sale (LPCHARACTER ch, DWORD item_id)
{
	TPacketGDCommnadAuction pack_cs;
	pack_cs.cancel_sale (item_id);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_cs, sizeof(TPacketGDCommnadAuction));
}

void AuctionManager::rebid (LPCHARACTER ch, DWORD item_id, int bid_price)
{
	std::pair <int, bool> mb = MyBid.GetMoney (ch->GetPlayerID(), item_id);
	int money = mb.first;
	bool lock = mb.second;
	if (money == -1)
	{
		sys_err ("Do bid first. How can you rebid? pid %d, item_id %d",ch->GetPlayerID(), item_id);
		return;
	}
	
	if (lock)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "입찰 중이야.");
		return;
	}

	if (ch->GetGold() + money < bid_price)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "돈이 부족해");
		return;
	}

	MyBid.Lock (ch->GetPlayerID(), item_id);

	ch->PointChange(POINT_GOLD, -(bid_price - money), false);
	
	TPacketGDCommnadAuction pack_rebid;
	pack_rebid.rebid (item_id, (bid_price - money));
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_rebid, sizeof(TPacketGDCommnadAuction));
}

void AuctionManager::bid_cancel (LPCHARACTER ch, DWORD item_id)
{
	std::pair <int, bool> mb = MyBid.GetMoney (ch->GetPlayerID(), item_id);
	int money = mb.first;
	bool lock = mb.second;
	if (money == -1)
	{
		sys_err ("Do bid first. How can you bid cancel? pid %d, item_id %d",ch->GetPlayerID(), item_id);
		return;
	}
	
	if (lock)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "입찰 중이야.");
		return;
	}

	TAuctionItemInfo* item_info = GetAuctionItemInfo(item_id);
	if (item_info->get_bidder_id() == ch->GetPlayerID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, "니가 최고 입찰자야. 취소 못해.");
		return;
	}

	MyBid.Delete (ch->GetPlayerID(), item_id);
	ch->PointChange(POINT_GOLD, money, true);
	
	TPacketGDCommnadAuction pack_bc;
	pack_bc.bid_cancel (item_id);
	db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_bc, sizeof(TPacketGDCommnadAuction));
}

// 끝
void AuctionManager::recv_result_auction (DWORD commander_id, TPacketDGResultAuction* cmd_result)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(commander_id);
	switch (cmd_result->cmd)
	{
	case AUCTION_ENR_AUC:
		switch (cmd_result->result)
		{
		case AUCTION_SUCCESS:
			{
				cmd_result++;
				TPlayerItem* player_item = (TPlayerItem*)cmd_result;

				InsertItem (player_item);
				player_item++;

				TAuctionItemInfo* item_info = (TAuctionItemInfo*)player_item;

				Auction.InsertItemInfo (item_info);
				if (ch != NULL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "경매장에 등록했어.");
				}
				break;
			}
		case AUCTION_FAIL:
			{
				if (ch != NULL)
				{
					cmd_result++;
					TPlayerItem* player_item = (TPlayerItem*)cmd_result;
					LPITEM item = ITEM_MANAGER::instance().CreateItem(player_item->vnum, player_item->count, player_item->id);

					LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID (player_item->owner);

					ch->AutoGiveItem (item, true);
					ch->ChatPacket(CHAT_TYPE_INFO, "경매장에 등록하지 못했어.");
				}
				break;
			}
		}
		break;
	case AUCTION_ENR_SALE:
		switch (cmd_result->result)
		{
		case AUCTION_SUCCESS:
			{
				cmd_result++;
				TPlayerItem* player_item = (TPlayerItem*)cmd_result;

				InsertItem (player_item);
				player_item++;

				TSaleItemInfo* item_info = (TSaleItemInfo*)player_item;

				Sale.InsertItemInfo (item_info);
				if (ch != NULL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "판매장에 등록했어.");
				}
				break;
			}
		case AUCTION_FAIL:
			{
				if (ch != NULL)
				{
					cmd_result++;
					TPlayerItem* player_item = (TPlayerItem*)cmd_result;
					LPITEM item = ITEM_MANAGER::instance().CreateItem(player_item->vnum, player_item->count, player_item->id);


					ch->AutoGiveItem (item, true);
					ch->ChatPacket(CHAT_TYPE_INFO, "판매장에 등록하지 못했어.");
				}
				break;
			}
		}
		break;
	case AUCTION_ENR_WISH:
		switch (cmd_result->result)
		{
		case AUCTION_SUCCESS:
			{
				cmd_result++;
				TWishItemInfo* item_info = (TWishItemInfo*)cmd_result;

				Wish.InsertItemInfo (item_info);
				if (ch != NULL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "삽니다에 등록했어.");
				}
				break;
			}
		case AUCTION_FAIL:
			{
				if (ch != NULL)
				{
					ch->ChatPacket(CHAT_TYPE_INFO, "삽니다에 등록하지 못했어.");
				}
				break;
			}
		}
		break;
	case AUCTION_BID:
		if (cmd_result->result <= AUCTION_FAIL)
		{
			
		}
		else
		{
			cmd_result++;
			TAuctionItemInfo* new_item_info = (TAuctionItemInfo*)cmd_result;
			TAuctionItemInfo* old_item_info = Auction.GetItemInfo (new_item_info->get_item_id());
			thecore_memcpy (old_item_info, new_item_info, sizeof(TAuctionItemInfo));
			MyBid.Insert(new_item_info->bidder_id, new_item_info->item_id, new_item_info->get_bid_price());
			if (ch != NULL)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "입찰했어.");
			}
		}
		break;
	case AUCTION_IMME_PUR:
		if (cmd_result->result <= AUCTION_FAIL)
		{
		}
		else
		{
			cmd_result++;
			TAuctionItemInfo* new_item_info = (TAuctionItemInfo*)cmd_result;
			TAuctionItemInfo* old_item_info = Auction.GetItemInfo (new_item_info->get_item_id());
			thecore_memcpy (old_item_info, new_item_info, sizeof(TAuctionItemInfo));
			if (ch != NULL)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "즉구 해버렸어.");
			}
		}
		break;
	case AUCTION_GET_AUC:
	case AUCTION_BUY_SOLD:
	case AUCTION_CANCEL_AUC:
	case AUCTION_CANCEL_SALE:
		if (cmd_result->result <= AUCTION_FAIL)
		{
		}
		else
		{
			BYTE cmd = cmd_result->cmd;
			cmd_result++;
			TPlayerItem* player_item = (TPlayerItem*)cmd_result;

			DWORD item_id = player_item->id;

			if (ch != NULL)
			{
				LPITEM item = ITEM_MANAGER::instance().CreateItem(player_item->vnum, player_item->count, item_id);
				ch->AutoGiveItem (item, true);
				ch->ChatPacket(CHAT_TYPE_INFO, "가져왔어.");
				if (cmd == AUCTION_GET_AUC || cmd == AUCTION_CANCEL_AUC)
				{
					TPacketGDCommnadAuction pack_dai;
					pack_dai.delete_auction_item (item_id);
					db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_dai, sizeof(TPacketGDCommnadAuction));
				}
				else if (cmd == AUCTION_BUY_SOLD || cmd == AUCTION_CANCEL_SALE)
				{
					TPacketGDCommnadAuction pack_dsi;
					pack_dsi.delete_sale_item (item_id);
					db_clientdesc->DBPacket(HEADER_GD_COMMAND_AUCTION, ch->GetPlayerID(), &pack_dsi, sizeof(TPacketGDCommnadAuction));
				}
			}
		}
	case AUCTION_DELETE_AUCTION_ITEM:
	case AUCTION_DELETE_SALE_ITEM:
		{
			AuctionCmd cmd = cmd_result->cmd;
			cmd_result++;
			TPlayerItem* player_item = (TPlayerItem*)cmd_result;

			DWORD item_id = player_item->id;

			if (!GetInventoryItem(player_item->id))
			{
				sys_err ("AUCTION_CMD %d : invalid item_id %d", cmd, item_id);
				break;
			}

			if (cmd == AUCTION_DELETE_AUCTION_ITEM)
			{
				if (!GetAuctionItemInfo(item_id))
				{
					DeleteItem (item_id);
					Auction.DeleteItemInfo (item_id);
				}
			}
			else if (cmd == AUCTION_DELETE_SALE_ITEM)
			{
				if (!GetSaleItemInfo(item_id))
				{
					DeleteItem (item_id);
					Sale.DeleteItemInfo (item_id);
				}
			}
		}
		break;
	case AUCTION_CANCEL_WISH:
		if (cmd_result->result <= AUCTION_FAIL)
		{
		}
		else
		{
			if (!Wish.DeleteItemInfo (commander_id, cmd_result->target))
			{
				sys_err ("Cannot cancel wish, invalid player_id : %d, item_num : %d", commander_id, cmd_result->target);
			}
			else if (ch != NULL)
			{
				ch->ChatPacket(CHAT_TYPE_INFO, "취소했어.");
			}
		}
		break;
	case AUCTION_REBID:
		if (cmd_result->result <= AUCTION_FAIL)
		{
			BYTE result = cmd_result->result;
			DWORD item_id = cmd_result->target;
			cmd_result++;
			int restore_money = *((int*)cmd_result);
			ch->PointChange(POINT_GOLD, restore_money, true);
			
			MyBid.UnLock (commander_id, item_id);
		}
		else
		{
			// insert하면 lock이 풀린다.
			DWORD item_id = cmd_result->target;
			cmd_result++;
			TAuctionItemInfo* auction_info = (TAuctionItemInfo*)cmd_result;
			ch->PointChange(POINT_GOLD, 0, true);

			MyBid.Insert (commander_id, item_id, auction_info->get_bid_price());
			Auction.UpdateItemInfo (auction_info);
		}
		break;
	case AUCTION_BID_CANCEL:
		if (cmd_result->result <= AUCTION_FAIL)
		{
		}
		else
		{
			MyBid.Delete (commander_id, cmd_result->target);
		}
	}
}

#endif
