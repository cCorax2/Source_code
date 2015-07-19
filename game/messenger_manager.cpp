#include "stdafx.h"
#include "constants.h"
#include "gm.h"
#include "messenger_manager.h"
#include "buffer_manager.h"
#include "desc_client.h"
#include "log.h"
#include "config.h"
#include "p2p.h"
#include "crc32.h"
#include "char.h"
#include "char_manager.h"
#include "questmanager.h"

MessengerManager::MessengerManager()
{
}

MessengerManager::~MessengerManager()
{
}

void MessengerManager::Initialize()
{
}

void MessengerManager::Destroy()
{
}

void MessengerManager::P2PLogin(MessengerManager::keyA account)
{
	Login(account);
}

void MessengerManager::P2PLogout(MessengerManager::keyA account)
{
	Logout(account);
}

void MessengerManager::Login(MessengerManager::keyA account)
{
	if (m_set_loginAccount.find(account) != m_set_loginAccount.end())
		return;

	DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&MessengerManager::LoadList), this),
			"SELECT account, companion FROM messenger_list%s WHERE account='%s'", get_table_postfix(), account.c_str());

	m_set_loginAccount.insert(account);
}

void MessengerManager::LoadList(SQLMsg * msg)
{
	if (NULL == msg)
		return;

	if (NULL == msg->Get())
		return;

	if (msg->Get()->uiNumRows == 0)
		return;

	std::string account;

	sys_log(1, "Messenger::LoadList");

	for (uint i = 0; i < msg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(msg->Get()->pSQLResult);

		if (row[0] && row[1])
		{
			if (account.length() == 0)
				account = row[0];

			m_Relation[row[0]].insert(row[1]);
			m_InverseRelation[row[1]].insert(row[0]);
		}
	}

	SendList(account);

	std::set<MessengerManager::keyT>::iterator it;

	for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
		SendLogin(*it, account);
}

void MessengerManager::Logout(MessengerManager::keyA account)
{
	if (m_set_loginAccount.find(account) == m_set_loginAccount.end())
		return;

	m_set_loginAccount.erase(account);

	std::set<MessengerManager::keyT>::iterator it;

	for (it = m_InverseRelation[account].begin(); it != m_InverseRelation[account].end(); ++it)
	{
		SendLogout(*it, account);
	}

	std::map<keyT, std::set<keyT> >::iterator it2 = m_Relation.begin();

	while (it2 != m_Relation.end())
	{
		it2->second.erase(account);
		++it2;
	}

	m_Relation.erase(account);
	//m_map_stMobile.erase(account);
}

void MessengerManager::RequestToAdd(LPCHARACTER ch, LPCHARACTER target)
{
	if (!ch->IsPC() || !target->IsPC())
		return;
	
	if (quest::CQuestManager::instance().GetPCForce(ch->GetPlayerID())->IsRunning() == true)
	{
	    ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("상대방이 친구 추가를 받을 수 없는 상태입니다."));
	    return;
	}

	if (quest::CQuestManager::instance().GetPCForce(target->GetPlayerID())->IsRunning() == true)
		return;

	DWORD dw1 = GetCRC32(ch->GetName(), strlen(ch->GetName()));
	DWORD dw2 = GetCRC32(target->GetName(), strlen(target->GetName()));

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

	m_set_requestToAdd.insert(dwComplex);

	target->ChatPacket(CHAT_TYPE_COMMAND, "messenger_auth %s", ch->GetName());
}

void MessengerManager::AuthToAdd(MessengerManager::keyA account, MessengerManager::keyA companion, bool bDeny)
{
	DWORD dw1 = GetCRC32(companion.c_str(), companion.length());
	DWORD dw2 = GetCRC32(account.c_str(), account.length());

	char buf[64];
	snprintf(buf, sizeof(buf), "%u:%u", dw1, dw2);
	DWORD dwComplex = GetCRC32(buf, strlen(buf));

	if (m_set_requestToAdd.find(dwComplex) == m_set_requestToAdd.end())
	{
		sys_log(0, "MessengerManager::AuthToAdd : request not exist %s -> %s", companion.c_str(), account.c_str());
		return;
	}

	m_set_requestToAdd.erase(dwComplex);

	if (!bDeny)
	{
		AddToList(companion, account);
		AddToList(account, companion);
	}
}

void MessengerManager::__AddToList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	m_Relation[account].insert(companion);
	m_InverseRelation[companion].insert(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (d)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<메신져> %s 님을 친구로 추가하였습니다."), companion.c_str());
	}

	LPCHARACTER tch = CHARACTER_MANAGER::instance().FindPC(companion.c_str());

	if (tch)
		SendLogin(account, companion);
	else
		SendLogout(account, companion);
}

void MessengerManager::AddToList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	if (m_Relation[account].find(companion) != m_Relation[account].end())
		return;

	sys_log(0, "Messenger Add %s %s", account.c_str(), companion.c_str());
	DBManager::instance().Query("INSERT INTO messenger_list%s VALUES ('%s', '%s')", 
			get_table_postfix(), account.c_str(), companion.c_str());

	__AddToList(account, companion);

	TPacketGGMessenger p2ppck;

	p2ppck.bHeader = HEADER_GG_MESSENGER_ADD;
	strlcpy(p2ppck.szAccount, account.c_str(), sizeof(p2ppck.szAccount));
	strlcpy(p2ppck.szCompanion, companion.c_str(), sizeof(p2ppck.szCompanion));
	P2P_MANAGER::instance().Send(&p2ppck, sizeof(TPacketGGMessenger));
}

void MessengerManager::__RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	m_Relation[account].erase(companion);
	m_InverseRelation[companion].erase(account);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (d)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<메신져> %s 님을 메신저에서 삭제하였습니다."), companion.c_str());
}

void MessengerManager::RemoveFromList(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (companion.size() == 0)
		return;

	sys_log(1, "Messenger Remove %s %s", account.c_str(), companion.c_str());
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account='%s' AND companion = '%s'",
			get_table_postfix(), account.c_str(), companion.c_str());

	__RemoveFromList(account, companion);

	TPacketGGMessenger p2ppck;

	p2ppck.bHeader = HEADER_GG_MESSENGER_REMOVE;
	strlcpy(p2ppck.szAccount, account.c_str(), sizeof(p2ppck.szAccount));
	strlcpy(p2ppck.szCompanion, companion.c_str(), sizeof(p2ppck.szCompanion));
	P2P_MANAGER::instance().Send(&p2ppck, sizeof(TPacketGGMessenger));
}

void MessengerManager::RemoveAllList(keyA account)
{
	std::set<keyT>	company(m_Relation[account]);

	/* SQL Data 삭제 */
	DBManager::instance().Query("DELETE FROM messenger_list%s WHERE account='%s' OR companion='%s'",
			get_table_postfix(), account.c_str(), account.c_str());

	/* 내가 가지고있는 리스트 삭제 */
	for (std::set<keyT>::iterator iter = company.begin();
			iter != company.end();
			iter++ )
	{
		this->RemoveFromList(account, *iter);
	}

	/* 복사한 데이타 삭제 */
	for (std::set<keyT>::iterator iter = company.begin();
			iter != company.end();
			)
	{
		company.erase(iter++);
	}

	company.clear();
}


void MessengerManager::SendList(MessengerManager::keyA account)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());

	if (!ch)
		return;

	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	if (m_Relation.find(account) == m_Relation.end())
		return;

	if (m_Relation[account].empty())
		return;

	TPacketGCMessenger pack;

	pack.header		= HEADER_GC_MESSENGER;
	pack.subheader	= MESSENGER_SUBHEADER_GC_LIST;
	pack.size		= sizeof(TPacketGCMessenger);

	TPacketGCMessengerListOffline pack_offline;
	TPacketGCMessengerListOnline pack_online;

	TEMP_BUFFER buf(128 * 1024); // 128k

	itertype(m_Relation[account]) it = m_Relation[account].begin(), eit = m_Relation[account].end();

	while (it != eit)
	{
		if (m_set_loginAccount.find(*it) != m_set_loginAccount.end())
		{
			pack_online.connected = 1;

			// Online
			pack_online.length = it->size();

			buf.write(&pack_online, sizeof(TPacketGCMessengerListOnline));
			buf.write(it->c_str(), it->size());
		}
		else
		{
			pack_offline.connected = 0;

			// Offline
			pack_offline.length = it->size();

			buf.write(&pack_offline, sizeof(TPacketGCMessengerListOffline));
			buf.write(it->c_str(), it->size());
		}

		++it;
	}

	pack.size += buf.size();

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->Packet(buf.read_peek(), buf.size());
}

void MessengerManager::SendLogin(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	if (!d->GetCharacter())
		return;

	if (ch->GetGMLevel() == GM_PLAYER && gm_get_level(companion.c_str()) != GM_PLAYER)
		return;

	BYTE bLen = companion.size();

	TPacketGCMessenger pack;

	pack.header			= HEADER_GC_MESSENGER;
	pack.subheader		= MESSENGER_SUBHEADER_GC_LOGIN;
	pack.size			= sizeof(TPacketGCMessenger) + sizeof(BYTE) + bLen;

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->BufferedPacket(&bLen, sizeof(BYTE));
	d->Packet(companion.c_str(), companion.size());
}

void MessengerManager::SendLogout(MessengerManager::keyA account, MessengerManager::keyA companion)
{
	if (!companion.size())
		return;

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindPC(account.c_str());
	LPDESC d = ch ? ch->GetDesc() : NULL;

	if (!d)
		return;

	BYTE bLen = companion.size();

	TPacketGCMessenger pack;

	pack.header		= HEADER_GC_MESSENGER;
	pack.subheader	= MESSENGER_SUBHEADER_GC_LOGOUT;
	pack.size		= sizeof(TPacketGCMessenger) + sizeof(BYTE) + bLen;

	d->BufferedPacket(&pack, sizeof(TPacketGCMessenger));
	d->BufferedPacket(&bLen, sizeof(BYTE));
	d->Packet(companion.c_str(), companion.size());
}

