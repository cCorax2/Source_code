#ifndef __INC_AUCTION_MANAGER_H
#define __INC_AUCTION_MANAGER_H

#include "../../libsql/AsyncSQL.h"
#include "../../common/auction_table.h"
#include <boost/unordered_map.hpp>
#include <algorithm>

#define GRADE_LOW 30
#define GRADE_MID 60
#define GRADE_HIGH 90

template<>
class hash<std::pair <DWORD, DWORD> >
{	// hash functor
public:
	typedef std::pair <DWORD, DWORD> _Kty;

	size_t operator()(const _Kty& _Keyval) const
	{	// hash _Keyval to size_t value by pseudorandomizing transform
		ldiv_t _Qrem = ldiv((size_t)_Keyval.first + (size_t)_Keyval.second, 127773);

		_Qrem.rem = 16807 * _Qrem.rem - 2836 * _Qrem.quot;
		if (_Qrem.rem < 0)
			_Qrem.rem += 2147483647;
		return ((size_t)_Qrem.rem);
	}
};

bool CompareItemInfoByItemNameAC (TAuctionItemInfo* i, TAuctionItemInfo* j);
bool CompareItemInfoByItemNameDC (TAuctionItemInfo* i, TAuctionItemInfo* j);

bool CompareItemInfoByCategoryAC (TAuctionItemInfo* i, TAuctionItemInfo* j);
bool CompareItemInfoByCategoryDC (TAuctionItemInfo* i, TAuctionItemInfo* j);

bool CompareItemInfoByTimeAC (TAuctionItemInfo* i, TAuctionItemInfo* j);
bool CompareItemInfoByTimeDC (TAuctionItemInfo* i, TAuctionItemInfo* j);

bool CompareItemInfoByCharNameAC (TAuctionItemInfo* i, TAuctionItemInfo* j);
bool CompareItemInfoByCharNameDC (TAuctionItemInfo* i, TAuctionItemInfo* j);

bool CompareItemInfoByPriceAC (TAuctionItemInfo* i, TAuctionItemInfo* j);
bool CompareItemInfoByPriceDC (TAuctionItemInfo* i, TAuctionItemInfo* j);

class AuctionBoard
{
public:
	AuctionBoard() {}
	~AuctionBoard() {}

	TAuctionItemInfo* GetItemInfo (DWORD key);
	bool DeleteItemInfo (DWORD key);
	bool InsertItemInfo (TAuctionItemInfo* item_info);
	bool UpdateItemInfo (TAuctionItemInfo* item_info);


private:
	typedef boost::unordered_map <DWORD, TAuctionItemInfo*> TItemInfoMap;
	TItemInfoMap item_map;

	typedef std::map <DWORD, TAuctionItemInfo*> TItemMap;
	typedef boost::unordered_map <DWORD, TItemMap*> TPCMap;

	TPCMap offer_map;

	// sorting을 위한 members
public:
	typedef std::vector <TAuctionItemInfo*> TItemInfoVec;
private:
	typedef std::map <std::string, TItemInfoVec*> SortByItemName;

	SortByItemName item_name_map;

	void Sort(TItemInfoVec& vec, BYTE order);

public:
	void SortedItemInfos (TItemInfoVec& vec, BYTE grade, BYTE category, int start_idx, BYTE size, BYTE order[5]);

	// 나의 경매장을 위한 함수.
	void YourItemInfoList (TItemInfoVec& vec, DWORD player_id, int start_idx, BYTE size);

};
class SaleBoard
{
private:
	typedef boost::unordered_map <DWORD, TSaleItemInfo*> TItemInfoMap;
	TItemInfoMap item_map;
	
	typedef std::map <DWORD, TSaleItemInfo*> TItemMap;
	typedef boost::unordered_map <DWORD, TItemMap*> TPCMap;

	TPCMap wisher_map;
	TPCMap seller_map;
	
	bool DeleteFromPCMap (TPCMap& pc_map, DWORD player_id, DWORD item_id);
	bool InsertInPCMap (TPCMap& pc_map, DWORD player_id, TSaleItemInfo* item_info);

public:
	SaleBoard() {}
	~SaleBoard() {}

	typedef std::vector <TSaleItemInfo*> TItemInfoVec;
	void WisherItemInfoList (TItemInfoVec& vec, DWORD wisher_id, int start_idx, BYTE size);

	TSaleItemInfo* GetItemInfo (DWORD key);
	bool DeleteItemInfo (DWORD key);
	bool InsertItemInfo (TSaleItemInfo* item_info);
};

class WishBoard
{
private:
	typedef std::map <DWORD, TWishItemInfo*> TItemMap;
	typedef boost::unordered_map <DWORD, TItemMap*> TPCMap;
	TPCMap wisher_map;

public:
	typedef TWishItemInfo ItemInfo;

	WishBoard() {}
	~WishBoard() {}

	TWishItemInfo* GetItemInfo (DWORD wisher_id, DWORD item_num);
	bool DeleteItemInfo (DWORD wisher_id, DWORD item_num);
	bool InsertItemInfo (TWishItemInfo* item_info);
};

class MyBidBoard
{
private:
	typedef std::pair <int, bool> BidInfo;
	typedef std::map <DWORD, BidInfo > TItemMap;
	typedef boost::unordered_map <DWORD, TItemMap*> TMyBidBoard;
	// bidder_id가 key
	TMyBidBoard pc_map;

public:
	MyBidBoard() {}
	~MyBidBoard() {}

	typedef std::vector <DWORD> TItemVec;

	void YourBidInfo (TItemVec& vec, DWORD bidder_id, int start_idx, int size);

	BidInfo GetMoney (DWORD player_id, DWORD item_id);
	bool Delete (DWORD player_id, DWORD item_id);
	// 이미 있으면 덮어 씌운다.
	void Insert (DWORD player_id, DWORD item_id, int money);
	void Lock (DWORD player_id, DWORD item_id);
	void UnLock (DWORD player_id, DWORD item_id);
};

class AuctionManager : public singleton <AuctionManager> 
{
private :
	typedef boost::unordered_map<DWORD, LPITEM> TItemMap;
	TItemMap auction_item_map;

	// auction에 등록된 정보 중 가격, 등등 아이템 테이블에 포함되지 않는 정보들을 관리하는 것들
	AuctionBoard Auction;
	SaleBoard Sale;
	WishBoard Wish;
	MyBidBoard MyBid;

public:
	bool InsertItem (LPITEM item);
	bool InsertItem (TPlayerItem* player_item);
	LPITEM GetInventoryItem (DWORD item_id);
	bool DeleteItem (DWORD item_id);

	bool InsertAuctionItemInfo (TAuctionItemInfo* item_info);
	TAuctionItemInfo* GetAuctionItemInfo (DWORD item_id)
	{
		return Auction.GetItemInfo (item_id);
	}

	bool InsertSaleItemInfo (TSaleItemInfo* item_info);
	TSaleItemInfo* GetSaleItemInfo (DWORD item_id)
	{
		return Sale.GetItemInfo (item_id);
	}

	bool InsertWishItemInfo (TWishItemInfo* item_info);
	TWishItemInfo* GetWishItemInfo (DWORD wisher_id, DWORD item_id)
	{
		return Wish.GetItemInfo (wisher_id, item_id);
	}

	void YourBidItemInfoList (AuctionBoard::TItemInfoVec& vec, DWORD bidder_id, int start_idx, int size);

	void Boot (const char* &pdata, WORD size);

	void get_auction_list (LPCHARACTER ch, int start_idx, int size, int cond);
	void get_my_auction_list (LPCHARACTER ch, int start_idx, int size);
	void get_my_purchase_list (LPCHARACTER ch, int start_idx, int size);
	void enroll_auction (LPCHARACTER ch, LPITEM item, BYTE empire, int bidPrice, int immidiatePurchasePrice);

	void recv_result_auction (DWORD commander_id, TPacketDGResultAuction* cmd_result);

	void bid (LPCHARACTER ch, DWORD item_id, int price);
	void immediate_purchase (LPCHARACTER ch, DWORD item_id);

	void enroll_sale (LPCHARACTER ch, LPITEM item, DWORD wisher_id, int salePrice);
	void enroll_wish (LPCHARACTER ch, DWORD item_num, BYTE empire, int wishPrice);

	void get_auctioned_item (LPCHARACTER ch, DWORD item_id, DWORD item_num);
	void buy_sold_item (LPCHARACTER ch, DWORD item_id);
	void cancel_auction (LPCHARACTER ch, DWORD item_id);
	void cancel_wish (LPCHARACTER ch, DWORD item_num);
	void cancel_sale (LPCHARACTER ch, DWORD item_id);
	
	void rebid (LPCHARACTER ch, DWORD item_id, int price);
	void bid_cancel (LPCHARACTER ch, DWORD item_id);
/*	
	void close_auction (LPCHARACTER ch);*/
};

#endif
