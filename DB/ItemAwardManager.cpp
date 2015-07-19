#include "stdafx.h"
#include "QID.h"
#include "DBManager.h"
#include "ItemAwardManager.h"
#include "Peer.h"

#include "ClientManager.h"



DWORD g_dwLastCachedItemAwardID = 0;
ItemAwardManager::ItemAwardManager()
{
}

ItemAwardManager::~ItemAwardManager()
{
}

void ItemAwardManager::RequestLoad()
{
	char szQuery[QUERY_MAX_LEN];
	snprintf(szQuery, sizeof(szQuery), "SELECT id,login,vnum,count,socket0,socket1,socket2,mall,why FROM item_award WHERE taken_time IS NULL and id > %d", g_dwLastCachedItemAwardID);
	CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_AWARD_LOAD, 0, NULL);
}

void ItemAwardManager::Load(SQLMsg * pMsg)
{
	MYSQL_RES * pRes = pMsg->Get()->pSQLResult;

	for (uint i = 0; i < pMsg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(pRes);
		int col = 0;

		DWORD dwID = 0;
		str_to_number(dwID, row[col++]);

		if (m_map_award.find(dwID) != m_map_award.end())
			continue;

		TItemAward * kData = new TItemAward;
		memset(kData, 0, sizeof(TItemAward));

		kData->dwID	= dwID;
		trim_and_lower(row[col++], kData->szLogin, sizeof(kData->szLogin));
		str_to_number(kData->dwVnum, row[col++]);
		str_to_number(kData->dwCount, row[col++]);
		str_to_number(kData->dwSocket0, row[col++]);
		str_to_number(kData->dwSocket1, row[col++]);
		str_to_number(kData->dwSocket2, row[col++]);
		str_to_number(kData->bMall, row[col++]);

		if (row[col])
		{
			strlcpy(kData->szWhy, row[col], sizeof(kData->szWhy));
			//게임 중에 why콜룸에 변동이 생기면				
			char* whyStr = kData->szWhy;	//why 콜룸 읽기
			char cmdStr[100] = "";	//why콜룸에서 읽은 값을 임시 문자열에 복사해둠
			strcpy(cmdStr,whyStr);	//명령어 얻는 과정에서 토큰쓰면 원본도 토큰화 되기 때문
			char command[20] = "";
			strcpy(command,CClientManager::instance().GetCommand(cmdStr));	// command 얻기
			//sys_err("%d,  %s",pItemAward->dwID,command);
			if( !(strcmp(command,"GIFT") ))	// command 가 GIFT이면
			{
				TPacketItemAwardInfromer giftData;
				strcpy(giftData.login, kData->szLogin);	//로그인 아이디 복사
				strcpy(giftData.command, command);					//명령어 복사
				giftData.vnum = kData->dwVnum;				//아이템 vnum도 복사
				CClientManager::instance().ForwardPacket(HEADER_DG_ITEMAWARD_INFORMER,&giftData,sizeof(TPacketItemAwardInfromer));
			}
		}

		m_map_award.insert(std::make_pair(dwID, kData));

		printf("ITEM_AWARD load id %u bMall %d \n", kData->dwID, kData->bMall);
		sys_log(0, "ITEM_AWARD: load id %lu login %s vnum %lu count %u socket %lu", kData->dwID, kData->szLogin, kData->dwVnum, kData->dwCount, kData->dwSocket0);
		std::set<TItemAward *> & kSet = m_map_kSetAwardByLogin[kData->szLogin];
		kSet.insert(kData);

		if (dwID > g_dwLastCachedItemAwardID)
			g_dwLastCachedItemAwardID = dwID;
	}
}

std::set<TItemAward *> * ItemAwardManager::GetByLogin(const char * c_pszLogin)
{
	itertype(m_map_kSetAwardByLogin) it = m_map_kSetAwardByLogin.find(c_pszLogin);

	if (it == m_map_kSetAwardByLogin.end())
		return NULL;

	return &it->second;
}

void ItemAwardManager::Taken(DWORD dwAwardID, DWORD dwItemID)
{
	itertype(m_map_award) it = m_map_award.find(dwAwardID);

	if (it == m_map_award.end())
	{
		sys_log(0, "ITEM_AWARD: Taken ID not exist %lu", dwAwardID);
		return;
	}

	TItemAward * k = it->second;
	k->bTaken = true;

	//
	// Update taken_time in database to prevent not to give him again.
	// 
	char szQuery[QUERY_MAX_LEN];

	snprintf(szQuery, sizeof(szQuery), 
			"UPDATE item_award SET taken_time=NOW(),item_id=%u WHERE id=%u AND taken_time IS NULL", 
			dwItemID, dwAwardID);

	CDBManager::instance().ReturnQuery(szQuery, QID_ITEM_AWARD_TAKEN, 0, NULL);
}

std::map<DWORD, TItemAward *>& ItemAwardManager::GetMapAward()
{
	return m_map_award;
}

std::map<std::string, std::set<TItemAward *> >& ItemAwardManager::GetMapkSetAwardByLogin()
{
	return m_map_kSetAwardByLogin;
}