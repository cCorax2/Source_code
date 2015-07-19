#ifndef __INC_AUCTION_TABLES_H__
#define __INC_AUCTION_TABLES_H__

#include "tables.h"

typedef struct _base_auction
{
public:
	DWORD item_num;
	TItemTable* item_proto;
	int offer_price;
	int price;
	DWORD offer_id;
	char shown_name[CHARACTER_NAME_MAX_LEN + 1];
	BYTE empire;
	time_t expired_time;

	DWORD get_item_num () { return item_num; }
	DWORD get_offer_id () { return offer_id; }
	BYTE get_empire () { return empire; }
	time_t get_expired_time () { return expired_time; }
	bool is_expired () 
	{
		return (time(NULL) > expired_time);
	}
	int get_price () { return offer_price; }
} TAuctionSimpleItemInfo;

// 각 auction 정보들.
// primary key (item_id)
typedef struct _auction : public _base_auction
{
public:
	DWORD item_id;
	DWORD bidder_id;

	_auction (){}
	_auction (DWORD _item_num, int _offer_price, int _price, DWORD _offer_id, 
		char* _shown_name, time_t _expired_time, DWORD _item_id, DWORD _bidder_id, BYTE _empire)
	{
		item_num = _item_num;
		offer_price= _offer_price;
		price = _price;
		offer_id = _offer_id;
		thecore_memcpy(shown_name, _shown_name, strlen(_shown_name) + 1);
		expired_time = _expired_time;
		item_id = _item_id;
		bidder_id = _bidder_id;
		empire = _empire;
	}

	// 이 메소드들은 어떤 변수가 auction에서 어떤 역할을 하는지 까먹을 까봐
	// 만들어놓았다.
	// by rtsummit
	DWORD get_item_id () { return item_id; }
	DWORD get_bidder_id () { return bidder_id; }
	int get_bid_price () { return offer_price; }
	void set_bid_price (int new_price) 
	{
		offer_price = new_price;
	}
	int get_impur_price () { return price; }
	
	const char* get_bidder_name () { return shown_name; }
	void set_bidder_name (const char* new_bidder_name)
	{
		thecore_memcpy(shown_name, new_bidder_name, strlen(new_bidder_name) + 1);
	}
} TAuctionItemInfo;

// primary key (item_id)
typedef struct _sale : public _base_auction
{
	_sale (){}

	_sale (DWORD _item_num, int _offer_price, DWORD _offer_id,
		char* _shown_name, DWORD _item_id, DWORD _wisher_id)
	{
		item_num = _item_num;
		offer_price= _offer_price;
		offer_id = _offer_id;
		thecore_memcpy(shown_name, _shown_name, strlen(_shown_name) + 1);
		item_id = _item_id;
		wisher_id = _wisher_id;
	}

	DWORD item_id;
	DWORD wisher_id;

} TSaleItemInfo;

// wish는 실제하는 아이템은 없다.
// primary key (item_num, wisher_id)
typedef struct _wish : public _base_auction
{
	_wish (){}
	
	_wish (DWORD _item_num, int _offer_price, DWORD _offer_id, 
		char* _shown_name, time_t _expired_time, BYTE _empire)
	{
		item_num = _item_num;
		offer_price= _offer_price;
		offer_id = _offer_id;
		thecore_memcpy(shown_name, _shown_name, strlen(_shown_name) + 1);
		expired_time = _expired_time;
		empire = _empire;
	}
} TWishItemInfo;

enum AuctionType {_AUCTION, _WISH_AUCTION, _MY_AUCTION, _MY_WISH_AUCTION, _AUCTION_MAX};

enum AuctionCmd {OPEN_AUCTION, OPEN_WISH_AUCTION, OPEN_MY_AUCTION, OPEN_MY_WISH_AUCTION, 
	AUCTION_BID, AUCTION_IMME_PUR, AUCTION_ENR_AUC, AUCTION_ENR_WISH, AUCTION_ENR_SALE, 
	AUCTION_GET_AUC, AUCTION_BUY_SOLD, 
	AUCTION_CANCEL_AUC, AUCTION_CANCEL_WISH, AUCTION_CANCEL_SALE,
	AUCTION_DELETE_AUCTION_ITEM, AUCTION_DELETE_SALE_ITEM,
	AUCTION_CHANGING_MONEY,
	AUCTION_REBID, AUCTION_BID_CANCEL,
};

// 반드시 FAIL 앞에, 실패 류 들이 와야한다.
// 왜냐, <= AUCTION_FAIL 이런 CHECK을 할 거거든
// 반대로 SUCCESS 뒤에, 성공 류 들이 와야한다. 근데 성공류가 있긴 하려나...

enum AuctionResult { AUCTION_EXPIRED, AUCTION_NOT_EXPIRED, AUCTION_NOT_ENOUGH_MONEY,
	AUCTION_SOLD, AUCTION_CANCEL, AUCTION_ALREADY_IN, AUCTION_NOT_IN, AUCTION_FAIL, AUCTION_SUCCESS };

enum AuctionSort { AUCTION_NO_ORDER, 
		AUCTION_ITEM_NAME_AC, AUCTION_ITEM_NAME_DC,
		AUCTION_CATEGORY_AC, AUCTION_CATEGORY_DC, 
		AUCTION_TIME_AC, AUCTION_TIME_DC, 
		AUCTION_CHAR_NAME_AC, AUCTION_CHAR_NAME_DC,
		AUCTION_PRICE_AC, AUCTION_PRICE_DC,
};

typedef struct command_get_auction_list
{
	AuctionCmd	cmd;
	DWORD		start_idx;
	BYTE		size;
} TPacketGDGetAuctionList;

typedef struct command_auction
{
	void enroll_product (DWORD _item_id, BYTE _empire, int _bidPrice, int _impurPrice)
	{
		cmd = AUCTION_ENR_AUC;
		item = _item_id;
		empire = _empire;
		price1 = _bidPrice;
		price2 = _impurPrice;
	}
	void enroll_sale (DWORD _item_id, DWORD _wisher_id, int _salePrice)
	{
		cmd = AUCTION_ENR_SALE;
		item = _item_id;
		price1 = _salePrice;
		player_id = _wisher_id;
	}
	void enroll_wish (DWORD _item_num, BYTE _empire, int _wishPrice)
	{
		cmd = AUCTION_ENR_WISH;
		item = _item_num;
		empire = _empire;
		price1 = _wishPrice;
	}
	void bid (DWORD _item_id, int _bidPrice)
	{
		cmd = AUCTION_BID;
		item = _item_id;
		price1 = _bidPrice;
	}
	void impur (DWORD _item_id)
	{
		cmd = AUCTION_IMME_PUR;
		item = _item_id;
	}
	void get_auctioned_item (DWORD _item_id)
	{
		cmd = AUCTION_GET_AUC;
		item = _item_id;
	}
	void buy_sold_item (DWORD _item_id)
	{
		cmd = AUCTION_BUY_SOLD;
		item = _item_id;
	}
	void cancel_auction (DWORD _item_id)
	{
		cmd = AUCTION_CANCEL_AUC;
		item = _item_id;
	}
	void cancel_wish (DWORD _item_num)
	{
		cmd = AUCTION_CANCEL_WISH;
		item = _item_num;
	}
	void cancel_sale (DWORD _item_id)
	{
		cmd = AUCTION_CANCEL_SALE;
		item = _item_id;
	}

	void delete_auction_item (DWORD _item_id)
	{
		cmd = AUCTION_DELETE_AUCTION_ITEM;
		item = _item_id;
	}

	void delete_sale_item (DWORD _item_id)
	{
		cmd = AUCTION_DELETE_SALE_ITEM;
		item = _item_id;
	}

	void changing_money (int _money)
	{
		cmd = AUCTION_CHANGING_MONEY;
		price1 = _money;
	}
	// bid랑 cmd만 다르다.
	void rebid (DWORD _item_id, int _bidPrice)
	{
		cmd = AUCTION_REBID;
		item = _item_id;
		price1 = _bidPrice;
	}
	void bid_cancel (DWORD _item_id)
	{
		cmd = AUCTION_BID_CANCEL;
		item = _item_id;
	}
	DWORD get_item () { return item; }

protected:
	AuctionCmd	cmd;
	DWORD		player_id;
	DWORD		item;
	BYTE		empire;
	int			price1;
	int			price2;

public:
	AuctionCmd get_cmd() { return cmd; }
	BYTE get_empire () { return empire; }
} TPacketGDCommnadAuction;

typedef struct result_auction
{
	AuctionCmd cmd;
	BYTE result;
	DWORD target;
} TPacketDGResultAuction;

// wrapper struct
typedef struct auction_enroll_product : public command_auction
{
	DWORD get_item_id() { return item; }
	int get_bid_price() { return price1; }
	int get_impur_price() { return price2; }
} AuctionEnrollProductInfo;

typedef struct auction_enroll_sale : public command_auction
{
	DWORD get_item_id() { return item; }
	DWORD get_wisher_id() { return player_id; }
	int get_sale_price() { return price1; }
} AuctionEnrollSaleInfo;

typedef struct auction_enroll_wish : public command_auction
{
	DWORD get_item_num() { return item; }
	int get_wish_price() { return price1; }
} AuctionEnrollWishInfo;

typedef struct auction_bid : public command_auction
{
	DWORD get_item_id() { return item; }
	int get_bid_price() { return price1; }
} AuctionBidInfo;

typedef struct auction_impur : public command_auction
{
	DWORD get_item_id() { return item; }
} AuctionImpurInfo;


//typedef struct get_auction_list
//
//bid
//{
//	item_id;
//	bidder_id;
//	price;
//}
//impur
//{
//	item_id;
//	purchaser_id;
//}
//enroll_wish
//{
//	item_num;
//	wisher_id;
//	wish_price;
//}
//enroll_sale
//{
//	item_id;
//	seller_id;
//	sale_price;
//}
//
//return_packet
//{
//	isSuccess;
//}
//
//
//get_auction_simple_item_info_list
//{
//	auction_type;
//	start_idx;
//	size;
//	conditions; 정렬은 승철님께 조언을 구해보자.ㅇㅇ
//}
//
//get_auction_detail_item_info
//{
//	item_id;
//}


#endif

