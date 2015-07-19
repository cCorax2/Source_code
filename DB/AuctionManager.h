#ifdef __AUCTION__

#ifndef __INC_AUCTION_MANAGER_H__
#define __INC_AUCTION_MANAGER_H__

#include <boost/unordered_map.hpp>
#include "Cache.h"
#include "../../common/auction_table.h"

class CItemCache;
class CAuctionItemInfoCache;
class CSaleItemInfoCache;
class CWishItemInfoCache;

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


class AuctionBoard
{
public:
	typedef DWORD Key;
	typedef CAuctionItemInfoCache ItemInfoCache;
	typedef TAuctionItemInfo ItemInfo;
	

public:
	AuctionBoard() {}
	~AuctionBoard() {}

	void Boot(CPeer* peer)
	{
		peer->EncodeWORD(sizeof(ItemInfo));
		peer->EncodeWORD(item_cache_map.size());

		TItemInfoCacheMap::iterator it = item_cache_map.begin();

		while (it != item_cache_map.end())
			peer->Encode((it++)->second->Get(), sizeof(ItemInfo));
	}
	
	size_t Size()
	{
		return item_cache_map.size();
	}

	ItemInfoCache* GetItemInfoCache (Key key)
	{
		TItemInfoCacheMap::iterator it = item_cache_map.find (key);
		if (it == item_cache_map.end())
			return NULL;
		else
			return it->second;
	}

	bool DeleteItemInfoCache (Key key)
	{
		TItemInfoCacheMap::iterator it = item_cache_map.find (key);
		if (it == item_cache_map.end())
			return false;
		else
		{
			it->second->Delete();
			item_cache_map.erase(it);
			return true;
		}
	}

	bool InsertItemInfo (ItemInfo *pNew, bool bSkipQuery = false)
	{
		ItemInfoCache* c = GetItemInfoCache (Key (pNew->item_id));
		if (c != NULL)
		{
			return false;
		}

		c = new ItemInfoCache();

		c->Put(pNew, bSkipQuery);

		item_cache_map.insert(TItemInfoCacheMap::value_type(pNew->item_id, c));
		c->Flush();

		return true;
	}
private:
	typedef boost::unordered_map <Key, ItemInfoCache *> TItemInfoCacheMap;
	TItemInfoCacheMap item_cache_map;
};

class SaleBoard 
{
public:
	typedef DWORD Key;
	typedef CSaleItemInfoCache ItemInfoCache;
	typedef TSaleItemInfo ItemInfo;
	
	SaleBoard() {}
	~SaleBoard() {}

	void Boot(CPeer* peer)
	{
		peer->EncodeWORD(sizeof(ItemInfo));
		peer->EncodeWORD(item_cache_map.size());

		TItemInfoCacheMap::iterator it = item_cache_map.begin();

		while (it != item_cache_map.end())
			peer->Encode((it++)->second->Get(), sizeof(ItemInfo));
	}
	
	size_t Size()
	{
		return item_cache_map.size();
	}

	ItemInfoCache* GetItemInfoCache (Key key)
	{
		TItemInfoCacheMap::iterator it = item_cache_map.find (key);
		if (it == item_cache_map.end())
			return NULL;
		else
			return it->second;
	}

	bool DeleteItemInfoCache (Key key)
	{
		TItemInfoCacheMap::iterator it = item_cache_map.find (key);
		if (it == item_cache_map.end())
			return false;
		else
		{
			it->second->Delete();
			item_cache_map.erase(it);
			return true;
		}
	}

	bool InsertItemInfo (ItemInfo *pNew, bool bSkipQuery = false)
	{
		ItemInfoCache* c = GetItemInfoCache (Key (pNew->item_id));
		if (c != NULL)
		{
			return false;
		}

		c = new ItemInfoCache();

		c->Put(pNew, bSkipQuery);

		item_cache_map.insert(TItemInfoCacheMap::value_type(pNew->item_id, c));
		c->Flush();

		return true;
	}

private:
	typedef boost::unordered_map <Key, ItemInfoCache *> TItemInfoCacheMap;
	TItemInfoCacheMap item_cache_map;
};

class WishBoard 
{
public:
	typedef std::pair <DWORD, DWORD> Key;
	typedef CWishItemInfoCache ItemInfoCache;
	typedef TWishItemInfo ItemInfo;
	
	WishBoard() {}
	virtual ~WishBoard() {}

	void Boot(CPeer* peer)
	{
		peer->EncodeWORD(sizeof(ItemInfo));
		peer->EncodeWORD(item_cache_map.size());

		TItemInfoCacheMap::iterator it = item_cache_map.begin();

		while (it != item_cache_map.end())
			peer->Encode((it++)->second->Get(), sizeof(ItemInfo));
	}
	
	size_t Size()
	{
		return item_cache_map.size();
	}

	ItemInfoCache* GetItemInfoCache (Key key)
	{
		TItemInfoCacheMap::iterator it = item_cache_map.find (key);
		if (it == item_cache_map.end())
			return NULL;
		else
			return it->second;
	}

	bool DeleteItemInfoCache (Key key)
	{
		TItemInfoCacheMap::iterator it = item_cache_map.find (key);
		if (it == item_cache_map.end())
			return false;
		else
		{
			it->second->Delete();
			item_cache_map.erase(it);
			return true;
		}
	}

	bool InsertItemInfo (ItemInfo *pNew, bool bSkipQuery = false)
	{
		ItemInfoCache* c = GetItemInfoCache (Key (pNew->item_num, pNew->offer_id));
		if (c != NULL)
		{
			return false;
		}

		c = new ItemInfoCache();

		c->Put(pNew, bSkipQuery);

		item_cache_map.insert(TItemInfoCacheMap::value_type(Key (pNew->item_num, pNew->offer_id), c));
		c->Flush();

		return true;
	}
private:
	typedef boost::unordered_map <Key, ItemInfoCache *> TItemInfoCacheMap;
	TItemInfoCacheMap item_cache_map;
};

// pc가 입찰에 참여했던 경매를 관리.
class MyBidBoard
{
public:
	MyBidBoard() {}
	~MyBidBoard() {}

	void Boot(CPeer* peer);
	size_t Size();
	
	int GetMoney (DWORD player_id, DWORD item_id);
	bool Delete (DWORD player_id, DWORD item_id);
	// 이미 있으면 덮어 씌운다.
	void Insert (DWORD player_id, DWORD item_id, int money);

private:
	typedef std::map <DWORD, int> TItemMap;
	typedef boost::unordered_map <DWORD, TItemMap*> TMyBidBoard;
	TMyBidBoard pc_map;
};

class AuctionManager : public singleton <AuctionManager>
{
private:
	// auction에 등록된 아이템들.
	typedef boost::unordered_map<DWORD, CItemCache *> TItemCacheMap;
	TItemCacheMap auction_item_cache_map;

	// auction에 등록된 정보 중 가격, 등등 아이템 테이블에 포함되지 않는 정보들을 관리하는 것들
	AuctionBoard Auction;
	SaleBoard Sale;
	WishBoard Wish;
	MyBidBoard MyBid;

public:
	AuctionManager();
	~AuctionManager();

	void Initialize ();
	void LoadAuctionItem ();

	void LoadAuctionInfo ();
	void LoadSaleInfo ();
	void LoadWishInfo ();
	void LoadMyBidInfo ();

	void Boot(CPeer* peer);

	bool InsertItemCache (CItemCache *item_cache, bool bSkipQuery = false);
	bool InsertItemCache (TPlayerItem * pNew, bool bSkipQuery = false);
	bool DeleteItemCache (DWORD item_id);
	CItemCache* GetItemCache (DWORD item_id);

	size_t GetAuctionItemSize()
	{
		return auction_item_cache_map.size();
	}
	size_t GetAuctionSize()
	{
		return Auction.Size();
	}
	size_t GetSaleSize()
	{
		return Sale.Size();
	}
	size_t GetWishSize()
	{
		return Wish.Size();
	}
	size_t GetMyBidSize()
	{
		return MyBid.Size();
	}

	void InsertAuctionItemInfoCache (TAuctionItemInfo *pNew, bool bSkipQuery = false)
	{
		Auction.InsertItemInfo (pNew, bSkipQuery);
	}
	CAuctionItemInfoCache* GetAuctionItemInfoCache (DWORD item_id)
	{
		return Auction.GetItemInfoCache(item_id);
	}

	void InsertSaleItemInfoCache (TSaleItemInfo *pNew, bool bSkipQuery = false)
	{
		Sale.InsertItemInfo (pNew, bSkipQuery);
	}
	CSaleItemInfoCache* GetSaleItemInfoCache (DWORD item_id)
	{
		return Sale.GetItemInfoCache(item_id);
	}

	void InsertWishItemInfoCache (TWishItemInfo *pNew, bool bSkipQuery = false)
	{
		Wish.InsertItemInfo (pNew, bSkipQuery);
	}
	CWishItemInfoCache* GetWishItemInfoCache (DWORD item_id, DWORD wisher_id)
	{
		return Wish.GetItemInfoCache(WishBoard::Key (item_id, wisher_id));
	}

	void InsertMyBid (DWORD player_id, DWORD item_id, DWORD money)
	{
		MyBid.Insert (player_id, item_id, money);
	}

	AuctionResult EnrollInAuction(CItemCache* item_cache, TAuctionItemInfo &item_info);
	AuctionResult EnrollInSale(CItemCache* item_cache, TSaleItemInfo &item_info);
	AuctionResult EnrollInWish(TWishItemInfo &item_info);
	AuctionResult Bid(DWORD bidder_id,  const char* bidder_name, DWORD item_id, DWORD bid_price);
	AuctionResult Impur(DWORD purchaser_id, const char* purchaser_name, DWORD item_id);
	AuctionResult GetAuctionedItem (DWORD actor_id, DWORD item_id, TPlayerItem& item);
	AuctionResult BuySoldItem (DWORD actor_id, DWORD item_id, TPlayerItem& item);
	AuctionResult CancelAuction (DWORD actor_id, DWORD item_id, TPlayerItem& item);
	AuctionResult CancelWish (DWORD actor_id, DWORD item_num);
	AuctionResult CancelSale (DWORD actor_id, DWORD item_id, TPlayerItem& item);
	AuctionResult DeleteAuctionItem (DWORD actor_id, DWORD item_id);
	AuctionResult DeleteSaleItem (DWORD actor_id, DWORD item_id);
	AuctionResult ReBid(DWORD bidder_id,  const char* bidder_name, DWORD item_id, DWORD bid_price);
	AuctionResult BidCancel (DWORD bidder_id, DWORD item_id);
};

#endif

#endif