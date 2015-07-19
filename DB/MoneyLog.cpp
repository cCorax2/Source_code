#include "stdafx.h"
#include "MoneyLog.h"
#include "ClientManager.h"
#include "Peer.h"

CMoneyLog::CMoneyLog()
{
}

CMoneyLog::~CMoneyLog()
{
}

void CMoneyLog::Save()
{
	CPeer* peer = CClientManager::instance().GetAnyPeer();
	if (!peer)
		return;
	for (BYTE bType = 0; bType < MONEY_LOG_TYPE_MAX_NUM; bType ++)
	{
		typeof(m_MoneyLogContainer[bType].begin()) it;
		for (it = m_MoneyLogContainer[bType].begin(); it != m_MoneyLogContainer[bType].end(); ++it)
		{
			//bType;
			TPacketMoneyLog p;
			p.type = bType;
			p.vnum = it->first;
			p.gold = it->second;
			peer->EncodeHeader(HEADER_DG_MONEY_LOG, 0, sizeof(p));
			peer->Encode(&p, sizeof(p));
		}
		m_MoneyLogContainer[bType].clear();
	}
	/*
	   CPeer* peer = GetPeer();

	   peer->

	   for (BYTE bType = 0; bType < MONEY_LOG_TYPE_MAX_NUM; bType++)
	   {
	//"INSERT INTO money_log%s VALUES('%s', %d, %d, %d)", CClientManager::instance().GetTablePostfix(), 
	typeof(m_MoneyLogContainer[bType].begin()) it;
	for (it = m_MoneyLogContainer[bType].begin(); it != m_MoneyLogContainer[bType].end(); ++it)
	{
	typeof(it->second.begin()) 
	}
	}

	for (BYTE bType = 0; bType < MONEY_LOG_TYPE_MAX_NUM; bType++)
	m_MoneyLogContainer[bType].clear()
	*/
}

void CMoneyLog::AddLog(BYTE bType, DWORD dwVnum, int iGold)
{
	m_MoneyLogContainer[bType][dwVnum] += iGold;
}
