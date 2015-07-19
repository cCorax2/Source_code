#include "stdafx.h"
#ifdef __AUCTION__

#include "DBManager.h"
#include "Peer.h"
#include "AuctionManager.h"

void MyBidBoard::Boot (CPeer* peer)
{
	peer->EncodeWORD(sizeof(DWORD) + sizeof(DWORD) + sizeof(int));
	peer->EncodeWORD(Size());

	for (TMyBidBoard::iterator pc_it = pc_map.begin(); pc_it != pc_map.end(); pc_it++)
	{
		TItemMap* item_map = pc_it->second;
		for (TItemMap::iterator it = item_map->begin(); it != item_map->end(); it++)
		{
			peer->Encode(&(pc_it->first), sizeof(DWORD));
			peer->Encode(&(it->first), sizeof(DWORD));
			peer->Encode(&(it->second), sizeof(int));
		}
	}
}

size_t MyBidBoard::Size ()
{
	size_t size = 0;
	for (TMyBidBoard::iterator it = pc_map.begin(); it != pc_map.end(); it++)
	{
		size += it->second->size();
	}
	return size;
}

int MyBidBoard::GetMoney (DWORD player_id, DWORD item_id)
{
	TMyBidBoard::iterator pc_it = pc_map.find (player_id);
	if (pc_it == pc_map.end())
	{
		return -1;
	}
	TItemMap* item_map = pc_it->second;
	TItemMap::iterator it = item_map->find (item_id);
	if (it == item_map->end())
		return -1;
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
	}
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "DELETE FROM my_bid WHERE player_id = %d and item_id = %d", player_id, item_id);
	CDBManager::instance().AsyncQuery(szQuery);
	return true;
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
		item_map->insert (TItemMap::value_type (item_id, money));
	}
	else
	{
		it->second = money;
	}
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO my_bid VALUES (%d, %d, %d)", player_id, item_id, money);
	CDBManager::instance().AsyncQuery(szQuery);
}

AuctionManager::AuctionManager()
{
}

AuctionManager::~AuctionManager()
{
}

void AuctionManager::Initialize()
{	auction_item_cache_map.clear();
	LoadAuctionItem();
	LoadAuctionInfo();
	LoadSaleInfo();
	LoadWishInfo();
	LoadMyBidInfo();
}

void AuctionManager::LoadAuctionItem()
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), 
		"SELECT id,	owner_id, count, vnum, socket0, socket1, socket2, "
		"attrtype0, attrvalue0, "
		"attrtype1, attrvalue1, "
		"attrtype2, attrvalue2, "
		"attrtype3, attrvalue3, "
		"attrtype4, attrvalue4, "
		"attrtype5, attrvalue5, "
		"attrtype6, attrvalue6  "
		"FROM item WHERE window = 'AUCTION'");

	SQLMsg *msg = CDBManager::instance().DirectQuery(szQuery);

	MYSQL_RES *res = msg->Get()->pSQLResult;
	
	if (!res)
	{
		return;
	}
	int rows;

	if ((rows = mysql_num_rows(res)) <= 0)	// 데이터 없음
	{
		return;
	}

	for (int i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		TPlayerItem item;

		int cur = 0;

		str_to_number(item.id, row[cur++]);
		str_to_number(item.owner, row[cur++]);
		item.window = AUCTION;
		str_to_number(item.count, row[cur++]);
		str_to_number(item.vnum, row[cur++]);
		str_to_number(item.alSockets[0], row[cur++]);
		str_to_number(item.alSockets[1], row[cur++]);
		str_to_number(item.alSockets[2], row[cur++]);

		for (int j = 0; j < ITEM_ATTRIBUTE_MAX_NUM; j++)
		{
			str_to_number(item.aAttr[j].bType, row[cur++]);
			str_to_number(item.aAttr[j].sValue, row[cur++]);
		}
		InsertItemCache(&item, true);
	}
	return;
}

void AuctionManager::LoadAuctionInfo()
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), 
		"select * from auction");

	SQLMsg *msg = CDBManager::instance().DirectQuery(szQuery);

	MYSQL_RES *res = msg->Get()->pSQLResult;
	
	if (!res)
	{
		return;
	}
	int rows;

	if ((rows = mysql_num_rows(res)) <= 0)	// 데이터 없음
	{
		return;
	}

	for (int i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		TAuctionItemInfo auctionItemInfo;

		int cur = 0;

		str_to_number(auctionItemInfo.item_num, row[cur++]);
		str_to_number(auctionItemInfo.offer_price, row[cur++]);
		str_to_number(auctionItemInfo.price, row[cur++]);
		str_to_number(auctionItemInfo.offer_id, row[cur++]);
		thecore_memcpy (auctionItemInfo.shown_name, (char*)row[cur], strlen((char*)row[cur]) +1);
		cur++;
		str_to_number(auctionItemInfo.empire, row[cur++]);
		str_to_number(auctionItemInfo.expired_time, row[cur++]);
		str_to_number(auctionItemInfo.item_id, row[cur++]);
		str_to_number(auctionItemInfo.bidder_id, row[cur++]);

		InsertAuctionItemInfoCache(&auctionItemInfo, true);
	}
	return;
}

void AuctionManager::LoadSaleInfo()
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), 
		"select * from sale");

	SQLMsg *msg = CDBManager::instance().DirectQuery(szQuery);

	MYSQL_RES *res = msg->Get()->pSQLResult;
	
	if (!res)
	{
		return;
	}
	int rows;

	if ((rows = mysql_num_rows(res)) <= 0)	// 데이터 없음
	{
		return;
	}

	for (int i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		TSaleItemInfo saleItemInfo;

		int cur = 0;

		str_to_number(saleItemInfo.item_num, row[cur++]);
		str_to_number(saleItemInfo.offer_price, row[cur++]);
		str_to_number(saleItemInfo.price, row[cur++]);
		str_to_number(saleItemInfo.offer_id, row[cur++]);
		thecore_memcpy (saleItemInfo.shown_name, (char*)row[cur], strlen((char*)row[cur]) +1);
		cur++;
		str_to_number(saleItemInfo.empire, row[cur++]);
		str_to_number(saleItemInfo.expired_time, row[cur++]);
		str_to_number(saleItemInfo.item_id, row[cur++]);
		str_to_number(saleItemInfo.wisher_id, row[cur++]);

		InsertSaleItemInfoCache(&saleItemInfo, true);
	}
	return;
}
void AuctionManager::LoadWishInfo()
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), 
		"select * from wish");

	SQLMsg *msg = CDBManager::instance().DirectQuery(szQuery);

	MYSQL_RES *res = msg->Get()->pSQLResult;
	
	if (!res)
	{
		return;
	}
	int rows;

	if ((rows = mysql_num_rows(res)) <= 0)	// 데이터 없음
	{
		return;
	}

	for (int i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);
		TWishItemInfo wishItemInfo;

		int cur = 0;

		str_to_number(wishItemInfo.item_num, row[cur++]);
		str_to_number(wishItemInfo.offer_price, row[cur++]);
		str_to_number(wishItemInfo.price, row[cur++]);
		str_to_number(wishItemInfo.offer_id, row[cur++]);
		thecore_memcpy (wishItemInfo.shown_name, (char*)row[cur], strlen((char*)row[cur]) +1);
		cur++;
		str_to_number(wishItemInfo.empire, row[cur++]);
		str_to_number(wishItemInfo.expired_time, row[cur++]);

		InsertWishItemInfoCache(&wishItemInfo, true);
	}
	return;
}

void AuctionManager::LoadMyBidInfo ()
{
	char szQuery[512];
	snprintf(szQuery, sizeof(szQuery), 
		"select * from my_bid");

	SQLMsg *msg = CDBManager::instance().DirectQuery(szQuery);

	MYSQL_RES *res = msg->Get()->pSQLResult;
	
	if (!res)
	{
		return;
	}
	int rows;

	if ((rows = mysql_num_rows(res)) <= 0)	// 데이터 없음
	{
		return;
	}

	for (int i = 0; i < rows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(res);

		int cur = 0;
		DWORD player_id;
		DWORD item_id;
		int money;

		str_to_number(player_id, row[cur++]);
		str_to_number(item_id, row[cur++]);
		str_to_number(money, row[cur++]);

		InsertMyBid (player_id, item_id, money);
	}
	return;
}

inline CItemCache* AuctionManager::GetItemCache(DWORD item_id)
{
	TItemCacheMap::iterator it = auction_item_cache_map.find (item_id);
	if (it == auction_item_cache_map.end())
		return NULL;
	else
		return it->second;
}

void AuctionManager::Boot(CPeer* peer)
{
	peer->EncodeWORD(sizeof(TPlayerItem));
	peer->EncodeWORD(auction_item_cache_map.size());

	itertype(auction_item_cache_map) auction_item_cache_map_it = auction_item_cache_map.begin();

	while (auction_item_cache_map_it != auction_item_cache_map.end())
		peer->Encode((auction_item_cache_map_it++)->second->Get(), sizeof(TPlayerItem));
	
	Auction.Boot(peer);
	Sale.Boot(peer);
	Wish.Boot(peer);
	MyBid.Boot(peer);
}

bool AuctionManager::InsertItemCache(CItemCache *item_cache, bool bSkipQuery)
{
	CItemCache* c = GetItemCache (item_cache->Get(false)->id);
	if (c != NULL)
	{
		return false;
	}
	auction_item_cache_map.insert(TItemCacheMap::value_type(item_cache->Get(true)->id, item_cache));
	item_cache->OnFlush();
	return true;
}

bool AuctionManager::InsertItemCache(TPlayerItem * pNew, bool bSkipQuery)
{
	CItemCache* c = GetItemCache (pNew->id);
	if (c != NULL)
	{
		return false;
	}
	
	c = new CItemCache();

	c->Put(pNew, bSkipQuery);

	auction_item_cache_map.insert(TItemCacheMap::value_type(pNew->id, c));
	c->Flush();
	return true;
}

bool AuctionManager::DeleteItemCache(DWORD item_id)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		return false;
	}

	c->Delete();

	return true;
}

AuctionResult AuctionManager::EnrollInAuction(CItemCache* item_cache, TAuctionItemInfo &item_info)
{
	CItemCache* c = GetItemCache (item_info.item_id);
	if (c != NULL)
	{
		sys_err ("item id : %d is already in AuctionManager", item_info.item_id);
		return AUCTION_FAIL;
	}

	if (!Auction.InsertItemInfo (&item_info))
	{
		sys_err ("item id : %d is already in AuctionBoard", item_info.item_id);
		return AUCTION_FAIL;
	}

	item_cache->Get()->window = AUCTION;
	item_cache->Get()->pos = 9999999;

	InsertItemCache (item_cache);
	
	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::EnrollInSale(CItemCache* item_cache, TSaleItemInfo &item_info)
{
	CItemCache* c = GetItemCache (item_info.item_id);
	if (c != NULL)
	{
		sys_err ("item id : %d is already in AuctionManager", item_info.item_id);
		return AUCTION_FAIL;
	}

	if (!Wish.GetItemInfoCache (WishBoard::Key (item_info.item_num, item_info.wisher_id)))
	{
		sys_err ("item_num : %d, wisher_id : %d is not in wish auction.", item_info.item_num, item_info.wisher_id);
		return AUCTION_FAIL;
	}
	
	if (!Sale.InsertItemInfo (&item_info))
	{
		sys_err ("item id : %d is already in SaleBoard", item_info.item_id);
		return AUCTION_FAIL;
	}
	
	item_cache->Get()->window = AUCTION;
	item_cache->Get()->pos = 999999;

	InsertItemCache (item_cache);
	
	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::EnrollInWish(TWishItemInfo &item_info)
{
	if (!Wish.InsertItemInfo (&item_info))
	{
		sys_err ("wisher_id : %d, item_num : %d is already in WishBoard", item_info.offer_id, item_info.item_num);
		return AUCTION_FAIL;
	}
		
	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::Bid(DWORD bidder_id, const char* bidder_name, DWORD item_id, DWORD bid_price)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	if (MyBid.GetMoney (bidder_id, item_id) != 0)
	{
		return AUCTION_ALREADY_IN;
	}

	CAuctionItemInfoCache* item_cache = Auction.GetItemInfoCache(item_id);
	TAuctionItemInfo* item_info = item_cache->Get(false);

	if (item_info->is_expired())
	{
		return AUCTION_EXPIRED;
	}

	if ((double)bid_price < (double)item_info->get_bid_price() * 1.05)
	{
		return AUCTION_NOT_ENOUGH_MONEY;
	}

	item_info->set_bid_price(bid_price);
	item_info->bidder_id = bidder_id;
	item_info->set_bidder_name (bidder_name);
	item_cache->OnFlush();

	InsertMyBid (bidder_id, item_id, bid_price);

	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::Impur(DWORD purchaser_id, const char* purchaser_name, DWORD item_id)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	CAuctionItemInfoCache* item_cache = Auction.GetItemInfoCache(item_id);
	TAuctionItemInfo* item_info = item_cache->Get(false);

	if (item_info->is_expired())
	{
		return AUCTION_EXPIRED;
	}

	// 즉구 해버렸으므로, 경매는 끝났다.
	item_info->expired_time = 0;
	item_info->bidder_id = purchaser_id;
	item_info->set_bidder_name (purchaser_name);
	item_info->set_bid_price (item_info->get_impur_price());
	item_cache->OnFlush();
	
	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::GetAuctionedItem (DWORD actor_id, DWORD item_id, TPlayerItem& item)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	CAuctionItemInfoCache* item_info_cache = Auction.GetItemInfoCache(item_id);
	if (item_info_cache == NULL)
	{
		sys_err ("how can this accident happen?");
		return AUCTION_FAIL;
	}

	TAuctionItemInfo* item_info = item_info_cache->Get(false);

	if (!item_info->is_expired())
	{
		return AUCTION_NOT_EXPIRED;
	}

	thecore_memcpy(&item, c->Get(), sizeof(TPlayerItem));

	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::BuySoldItem (DWORD actor_id, DWORD item_id, TPlayerItem& item)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	CSaleItemInfoCache* item_info_cache = Sale.GetItemInfoCache(item_id);
	if (item_info_cache == NULL)
	{
		sys_err ("how can this accident happen?");
		return AUCTION_FAIL;
	}

	TSaleItemInfo* item_info = item_info_cache->Get(false);

	thecore_memcpy(&item, c->Get(), sizeof(TPlayerItem));

	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::CancelAuction (DWORD actor_id, DWORD item_id, TPlayerItem& item)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	CAuctionItemInfoCache* item_info_cache = Auction.GetItemInfoCache(item_id);
	if (item_info_cache == NULL)
	{
		sys_err ("how can this accident happen?");
		return AUCTION_FAIL;
	}
	TAuctionItemInfo* item_info = item_info_cache->Get(false);

	thecore_memcpy(&item, c->Get(), sizeof(TPlayerItem));

	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::CancelWish (DWORD actor_id, DWORD item_num)
{
	if (!Wish.DeleteItemInfoCache (WishBoard::Key (actor_id, item_num)))
	{
		return AUCTION_FAIL;
	}
	else
	{
		return AUCTION_SUCCESS;
	}
}

AuctionResult AuctionManager::CancelSale (DWORD actor_id, DWORD item_id, TPlayerItem& item)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	CSaleItemInfoCache* item_info_cache = Sale.GetItemInfoCache(item_id);
	if (item_info_cache == NULL)
	{
		sys_err ("how can this accident happen?");
		return AUCTION_FAIL;
	}
	TSaleItemInfo* item_info = item_info_cache->Get(false);

	thecore_memcpy(&item, c->Get(), sizeof(TPlayerItem));

	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::DeleteAuctionItem (DWORD actor_id, DWORD item_id)
{
	if (DeleteItemCache (item_id) == false)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	if (Auction.DeleteItemInfoCache (item_id) == NULL)
	{
		sys_err ("how can this accident happen?");
		return AUCTION_FAIL;
	}
	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::DeleteSaleItem (DWORD actor_id, DWORD item_id)
{
	if (DeleteItemCache (item_id) == false)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	if (Sale.DeleteItemInfoCache (item_id) == NULL)
	{
		sys_err ("how can this accident happen?");
		return AUCTION_FAIL;
	}
	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::ReBid(DWORD bidder_id, const char* bidder_name, DWORD item_id, DWORD bid_price)
{
	CItemCache* c = GetItemCache (item_id);
	if (c == NULL)
	{
		sys_err ("item id : %d does not exist in auction.", item_id);
		return AUCTION_FAIL;
	}

	int money = MyBid.GetMoney (bidder_id, item_id);
	if (money == -1)
	{
		return AUCTION_NOT_IN;
	}

	CAuctionItemInfoCache* item_cache = Auction.GetItemInfoCache(item_id);
	TAuctionItemInfo* item_info = item_cache->Get(false);

	if (item_info->is_expired())
	{
		return AUCTION_EXPIRED;
	}

	if ((double)(bid_price + money) < (double)item_info->get_bid_price() * 1.05)
	{
		return AUCTION_NOT_ENOUGH_MONEY;
	}
	
	item_info->set_bid_price(bid_price + money);
	item_info->bidder_id = bidder_id;
	item_info->set_bidder_name (bidder_name);
	item_cache->OnFlush();

	InsertMyBid (bidder_id, item_id, money + bid_price);

	return AUCTION_SUCCESS;
}

AuctionResult AuctionManager::BidCancel (DWORD bidder_id, DWORD item_id)
{
	if (MyBid.Delete (bidder_id, item_id))
	{
		return AUCTION_SUCCESS;
	}
	else
	{
		return AUCTION_NOT_IN;
	}
}
#endif