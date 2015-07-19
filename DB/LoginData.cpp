#include "stdafx.h"
#include "LoginData.h"
#include "ClientManager.h"

CLoginData::CLoginData()
{
	m_dwKey = 0;
	memset(m_adwClientKey, 0, sizeof(m_adwClientKey));
	m_dwConnectedPeerHandle = 0;
	m_dwLogonTime = 0;
	memset(m_szIP, 0, sizeof(m_szIP));
	m_bPlay = false;
	m_bDeleted = false;
	m_bBillType = 0;
	m_dwBillID = 0;
	m_lastPlayTime = 0;
	m_dwLastPlayerID = 0;

	memset(&m_data, 0, sizeof(TAccountTable));
}

TAccountTable & CLoginData::GetAccountRef()
{
	return m_data;
}

void CLoginData::SetClientKey(const DWORD * c_pdwClientKey)
{
	thecore_memcpy(&m_adwClientKey, c_pdwClientKey, sizeof(DWORD) * 4);
}

const DWORD * CLoginData::GetClientKey()
{
	return &m_adwClientKey[0];
}

void CLoginData::SetKey(DWORD dwKey)
{
	m_dwKey = dwKey;
}

DWORD CLoginData::GetKey()
{
	return m_dwKey;
}

void CLoginData::SetConnectedPeerHandle(DWORD dwHandle)
{
	m_dwConnectedPeerHandle = dwHandle;
}

DWORD CLoginData::GetConnectedPeerHandle()
{
	return m_dwConnectedPeerHandle;
}

void CLoginData::SetLogonTime()
{
	m_dwLogonTime = get_dword_time();
}

DWORD CLoginData::GetLogonTime()
{
	return m_dwLogonTime;
}

void CLoginData::SetIP(const char * c_pszIP)
{
	strlcpy(m_szIP, c_pszIP, sizeof(m_szIP));
}

const char * CLoginData::GetIP()
{
	return m_szIP;
}

void CLoginData::SetPlay(bool bOn)
{
	if (bOn)
	{
		sys_log(0, "SetPlay on %lu %s", GetKey(), m_data.login);
		SetLogonTime();
	}
	else
		sys_log(0, "SetPlay off %lu %s", GetKey(), m_data.login);

	m_bPlay = bOn;
	m_lastPlayTime = CClientManager::instance().GetCurrentTime();
}

bool CLoginData::IsPlay()
{
	return m_bPlay;
}

void CLoginData::SetDeleted(bool bSet)
{
	m_bDeleted = bSet;
}

bool CLoginData::IsDeleted()
{
	return m_bDeleted;
}

void CLoginData::SetPremium(int * paiPremiumTimes)
{
	thecore_memcpy(m_aiPremiumTimes, paiPremiumTimes, sizeof(m_aiPremiumTimes));
}

int CLoginData::GetPremium(BYTE type)
{
	if (type >= PREMIUM_MAX_NUM)
		return 0;

	return m_aiPremiumTimes[type];
}

int * CLoginData::GetPremiumPtr()
{
	return &m_aiPremiumTimes[0];
}

