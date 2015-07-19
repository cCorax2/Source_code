
#include "stdafx.h"
#include "Cache.h"

#include "QID.h"
#include "ClientManager.h"
#ifdef __AUCTION__
#include "AuctionManager.h"
#endif
#include "Main.h"

extern CPacketInfo g_item_info;
extern int g_iPlayerCacheFlushSeconds;
extern int g_iItemCacheFlushSeconds;
extern int g_test_server;
// MYSHOP_PRICE_LIST
extern int g_iItemPriceListTableCacheFlushSeconds;
// END_OF_MYSHOP_PRICE_LIST
//
extern int g_item_count;
const int auctionMinFlushSec = 1800;

CItemCache::CItemCache()
{
	m_expireTime = MIN(1800, g_iItemCacheFlushSeconds);
}

CItemCache::~CItemCache()
{
}

// 이거 이상한데...
// Delete를 했으면, Cache도 해제해야 하는것 아닌가???
// 근데 Cache를 해제하는 부분이 없어.
// 못 찾은 건가?
// 이렇게 해놓으면, 계속 시간이 될 때마다 아이템을 계속 지워...
// 이미 사라진 아이템인데... 확인사살??????
// fixme
// by rtsummit
void CItemCache::Delete()
{
	if (m_data.vnum == 0)
		return;

	//char szQuery[QUERY_MAX_LEN];
	//szQuery[QUERY_MAX_LEN] = '\0';
	if (g_test_server)
		sys_log(0, "ItemCache::Delete : DELETE %u", m_data.id);

	m_data.vnum = 0;
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();
	
	//m_bNeedQuery = false;
	//m_lastUpdateTime = time(0) - m_expireTime; // 바로 타임아웃 되도록 하자.
}

void CItemCache::OnFlush()
{
	if (m_data.vnum == 0) // vnum이 0이면 삭제하라고 표시된 것이다.
	{
		char szQuery[QUERY_MAX_LEN];
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM item%s WHERE id=%u", GetTablePostfix(), m_data.id);
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_DESTROY, 0, NULL);

		if (g_test_server)
			sys_log(0, "ItemCache::Flush : DELETE %u %s", m_data.id, szQuery);
	}
	else
	{
		long alSockets[ITEM_SOCKET_MAX_NUM];
		TPlayerItemAttribute aAttr[ITEM_ATTRIBUTE_MAX_NUM];
		bool isSocket = false, isAttr = false;

		memset(&alSockets, 0, sizeof(long) * ITEM_SOCKET_MAX_NUM);
		memset(&aAttr, 0, sizeof(TPlayerItemAttribute) * ITEM_ATTRIBUTE_MAX_NUM);

		TPlayerItem * p = &m_data;

		if (memcmp(alSockets, p->alSockets, sizeof(long) * ITEM_SOCKET_MAX_NUM))
			isSocket = true;

		if (memcmp(aAttr, p->aAttr, sizeof(TPlayerItemAttribute) * ITEM_ATTRIBUTE_MAX_NUM))
			isAttr = true;

		char szColumns[QUERY_MAX_LEN];
		char szValues[QUERY_MAX_LEN];
		char szUpdate[QUERY_MAX_LEN];

		int iLen = snprintf(szColumns, sizeof(szColumns), "id, owner_id, window, pos, count, vnum");

		int iValueLen = snprintf(szValues, sizeof(szValues), "%u, %u, %d, %d, %u, %u",
				p->id, p->owner, p->window, p->pos, p->count, p->vnum);

		int iUpdateLen = snprintf(szUpdate, sizeof(szUpdate), "owner_id=%u, window=%d, pos=%d, count=%u, vnum=%u",
				p->owner, p->window, p->pos, p->count, p->vnum);

		if (isSocket)
		{
			iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, ", socket0, socket1, socket2");
			iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,
					", %lu, %lu, %lu", p->alSockets[0], p->alSockets[1], p->alSockets[2]);
			iUpdateLen += snprintf(szUpdate + iUpdateLen, sizeof(szUpdate) - iUpdateLen,
					", socket0=%lu, socket1=%lu, socket2=%lu", p->alSockets[0], p->alSockets[1], p->alSockets[2]);
		}

		if (isAttr)
		{
			iLen += snprintf(szColumns + iLen, sizeof(szColumns) - iLen, 
					", attrtype0, attrvalue0, attrtype1, attrvalue1, attrtype2, attrvalue2, attrtype3, attrvalue3"
					", attrtype4, attrvalue4, attrtype5, attrvalue5, attrtype6, attrvalue6");

			iValueLen += snprintf(szValues + iValueLen, sizeof(szValues) - iValueLen,
					", %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d",
					p->aAttr[0].bType, p->aAttr[0].sValue,
					p->aAttr[1].bType, p->aAttr[1].sValue,
					p->aAttr[2].bType, p->aAttr[2].sValue,
					p->aAttr[3].bType, p->aAttr[3].sValue,
					p->aAttr[4].bType, p->aAttr[4].sValue,
					p->aAttr[5].bType, p->aAttr[5].sValue,
					p->aAttr[6].bType, p->aAttr[6].sValue);

			iUpdateLen += snprintf(szUpdate + iUpdateLen, sizeof(szUpdate) - iUpdateLen,
					", attrtype0=%d, attrvalue0=%d"
					", attrtype1=%d, attrvalue1=%d"
					", attrtype2=%d, attrvalue2=%d"
					", attrtype3=%d, attrvalue3=%d"
					", attrtype4=%d, attrvalue4=%d"
					", attrtype5=%d, attrvalue5=%d"
					", attrtype6=%d, attrvalue6=%d",
					p->aAttr[0].bType, p->aAttr[0].sValue,
					p->aAttr[1].bType, p->aAttr[1].sValue,
					p->aAttr[2].bType, p->aAttr[2].sValue,
					p->aAttr[3].bType, p->aAttr[3].sValue,
					p->aAttr[4].bType, p->aAttr[4].sValue,
					p->aAttr[5].bType, p->aAttr[5].sValue,
					p->aAttr[6].bType, p->aAttr[6].sValue);
		}

		char szItemQuery[QUERY_MAX_LEN + QUERY_MAX_LEN];
		snprintf(szItemQuery, sizeof(szItemQuery), "REPLACE INTO item%s (%s) VALUES(%s)", GetTablePostfix(), szColumns, szValues);

		if (g_test_server)	
			sys_log(0, "ItemCache::Flush :REPLACE  (%s)", szItemQuery);

		CDBManager::instance().ReturnQuery(szItemQuery, QID_ITEM_SAVE, 0, NULL);

		//g_item_info.Add(p->vnum);
		++g_item_count;
	}

	m_bNeedQuery = false;
}

//
// CPlayerTableCache
//
CPlayerTableCache::CPlayerTableCache()
{
	m_expireTime = MIN(1800, g_iPlayerCacheFlushSeconds);
}

CPlayerTableCache::~CPlayerTableCache()
{
}

void CPlayerTableCache::OnFlush()
{
	if (g_test_server)
		sys_log(0, "PlayerTableCache::Flush : %s", m_data.name);

	char szQuery[QUERY_MAX_LEN];
	CreatePlayerSaveQuery(szQuery, sizeof(szQuery), &m_data);
	CDBManager::instance().ReturnQuery(szQuery, QID_PLAYER_SAVE, 0, NULL);
}

// MYSHOP_PRICE_LIST
//
// CItemPriceListTableCache class implementation
//

const int CItemPriceListTableCache::s_nMinFlushSec = 1800;

CItemPriceListTableCache::CItemPriceListTableCache()
{
	m_expireTime = MIN(s_nMinFlushSec, g_iItemPriceListTableCacheFlushSeconds);
}

void CItemPriceListTableCache::UpdateList(const TItemPriceListTable* pUpdateList)
{
	//
	// 이미 캐싱된 아이템과 중복된 아이템을 찾고 중복되지 않는 이전 정보는 tmpvec 에 넣는다.
	//

	std::vector<TItemPriceInfo> tmpvec;

	for (uint idx = 0; idx < m_data.byCount; ++idx)
	{
		const TItemPriceInfo* pos = pUpdateList->aPriceInfo;
		for (; pos != pUpdateList->aPriceInfo + pUpdateList->byCount && m_data.aPriceInfo[idx].dwVnum != pos->dwVnum; ++pos)
			;

		if (pos == pUpdateList->aPriceInfo + pUpdateList->byCount)
			tmpvec.push_back(m_data.aPriceInfo[idx]);
	}

	//
	// pUpdateList 를 m_data 에 복사하고 남은 공간을 tmpvec 의 앞에서 부터 남은 만큼 복사한다.
	// 

	if (pUpdateList->byCount > SHOP_PRICELIST_MAX_NUM)
	{
		sys_err("Count overflow!");
		return;
	}

	m_data.byCount = pUpdateList->byCount;

	thecore_memcpy(m_data.aPriceInfo, pUpdateList->aPriceInfo, sizeof(TItemPriceInfo) * pUpdateList->byCount);

	int nDeletedNum;	// 삭제된 가격정보의 갯수

	if (pUpdateList->byCount < SHOP_PRICELIST_MAX_NUM)
	{
		size_t sizeAddOldDataSize = SHOP_PRICELIST_MAX_NUM - pUpdateList->byCount;

		if (tmpvec.size() < sizeAddOldDataSize)
			sizeAddOldDataSize = tmpvec.size();

		thecore_memcpy(m_data.aPriceInfo + pUpdateList->byCount, &tmpvec[0], sizeof(TItemPriceInfo) * sizeAddOldDataSize);
		m_data.byCount += sizeAddOldDataSize;

		nDeletedNum = tmpvec.size() - sizeAddOldDataSize;
	}
	else
		nDeletedNum = tmpvec.size();

	m_bNeedQuery = true;

	sys_log(0, 
			"ItemPriceListTableCache::UpdateList : OwnerID[%u] Update [%u] Items, Delete [%u] Items, Total [%u] Items", 
			m_data.dwOwnerID, pUpdateList->byCount, nDeletedNum, m_data.byCount);
}

void CItemPriceListTableCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];

	//
	// 이 캐시의 소유자에 대한 기존에 DB 에 저장된 아이템 가격정보를 모두 삭제한다.
	//

	snprintf(szQuery, sizeof(szQuery), "DELETE FROM myshop_pricelist%s WHERE owner_id = %u", GetTablePostfix(), m_data.dwOwnerID);
	CDBManager::instance().ReturnQuery(szQuery, QID_ITEMPRICE_DESTROY, 0, NULL);

	//
	// 캐시의 내용을 모두 DB 에 쓴다.
	//

	for (int idx = 0; idx < m_data.byCount; ++idx)
	{
		snprintf(szQuery, sizeof(szQuery),
				"INSERT INTO myshop_pricelist%s(owner_id, item_vnum, price) VALUES(%u, %u, %u)", 
				GetTablePostfix(), m_data.dwOwnerID, m_data.aPriceInfo[idx].dwVnum, m_data.aPriceInfo[idx].dwPrice);
		CDBManager::instance().ReturnQuery(szQuery, QID_ITEMPRICE_SAVE, 0, NULL);
	}

	sys_log(0, "ItemPriceListTableCache::Flush : OwnerID[%u] Update [%u]Items", m_data.dwOwnerID, m_data.byCount);
	
	m_bNeedQuery = false;
}
// END_OF_MYSHOP_PRICE_LIST
#ifdef __AUCTION__
CAuctionItemInfoCache::CAuctionItemInfoCache()
{
	m_expireTime = MIN (auctionMinFlushSec, g_iItemCacheFlushSeconds);
}

CAuctionItemInfoCache::~CAuctionItemInfoCache()
{

}

void CAuctionItemInfoCache::Delete()
{
	if (m_data.item_num == 0)
		return;

	if (g_test_server)
		sys_log(0, "CAuctionItemInfoCache::Delete : DELETE %u", m_data.item_id);

	m_data.item_num = 0;
	m_bNeedQuery = true;
	m_lastUpdateTime = time(0);
	OnFlush();
	delete this;
}

void CAuctionItemInfoCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];
	
	if (m_data.item_num == 0)
	{
		snprintf(szQuery, sizeof(szQuery), "DELETE FROM auction where item_id = %d", m_data.item_id);
		CDBManager::instance().AsyncQuery(szQuery);
	}
	else
	{
		snprintf(szQuery, sizeof(szQuery), "REPLACE INTO auction VALUES (%u, %d, %d, %u, \"%s\", %u, %u, %u, %u)", 
			m_data.item_num, m_data.offer_price, m_data.price, m_data.offer_id, m_data.shown_name, (DWORD)m_data.empire, (DWORD)m_data.expired_time, 
			m_data.item_id, m_data.bidder_id);

		CDBManager::instance().AsyncQuery(szQuery);
	}
}

CSaleItemInfoCache::CSaleItemInfoCache()
{
	m_expireTime = MIN (auctionMinFlushSec, g_iItemCacheFlushSeconds);
}

CSaleItemInfoCache::~CSaleItemInfoCache()
{
}

void CSaleItemInfoCache::Delete()
{
}

void CSaleItemInfoCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];
	
	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO sale VALUES (%u, %d, %d, %u, \"%s\", %u, %u, %u, %u)", 
		m_data.item_num, m_data.offer_price, m_data.price, m_data.offer_id, m_data.shown_name, (DWORD)m_data.empire, (DWORD)m_data.expired_time,
		m_data.item_id, m_data.wisher_id);
	
	CDBManager::instance().AsyncQuery(szQuery);
}

CWishItemInfoCache::CWishItemInfoCache()
{
	m_expireTime = MIN (auctionMinFlushSec, g_iItemCacheFlushSeconds);
}

CWishItemInfoCache::~CWishItemInfoCache()
{
}

void CWishItemInfoCache::Delete()
{
}

void CWishItemInfoCache::OnFlush()
{
	char szQuery[QUERY_MAX_LEN];
	
	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO wish VALUES (%u, %d, %d, %u, \"%s\", %u, %d)", 
		m_data.item_num, m_data.offer_price, m_data.price, m_data.offer_id, m_data.shown_name, (DWORD)m_data.empire, (DWORD)m_data.expired_time);
	
	CDBManager::instance().AsyncQuery(szQuery);
}
#endif