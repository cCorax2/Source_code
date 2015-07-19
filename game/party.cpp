#include "stdafx.h"
#include "utils.h"
#include "char.h"
#include "party.h"
#include "char_manager.h"
#include "config.h"
#include "p2p.h"
#include "desc_client.h"
#include "dungeon.h"
#include "unique_item.h"

CPartyManager::CPartyManager()
{
	Initialize();
}

CPartyManager::~CPartyManager()
{
}

void CPartyManager::Initialize()
{
	m_bEnablePCParty = false;
}

void CPartyManager::DeleteAllParty()
{
	TPCPartySet::iterator it = m_set_pkPCParty.begin();

	while (it != m_set_pkPCParty.end())
	{
		DeleteParty(*it);
		it = m_set_pkPCParty.begin();
	}
}

bool CPartyManager::SetParty(LPCHARACTER ch)	// PC만 사용해야 한다!!
{
	TPartyMap::iterator it = m_map_pkParty.find(ch->GetPlayerID());

	if (it == m_map_pkParty.end())
		return false;

	LPPARTY pParty = it->second;
	pParty->Link(ch);
	return true;
}

void CPartyManager::P2PLogin(DWORD pid, const char* name)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);

	if (it == m_map_pkParty.end())
		return;

	it->second->UpdateOnlineState(pid, name);
}
void CPartyManager::P2PLogout(DWORD pid)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);

	if (it == m_map_pkParty.end())
		return;

	it->second->UpdateOfflineState(pid);
}

void CPartyManager::P2PJoinParty(DWORD leader, DWORD pid, BYTE role)
{
	TPartyMap::iterator it = m_map_pkParty.find(leader);

	if (it != m_map_pkParty.end())
	{
		it->second->P2PJoin(pid);

		if (role >= PARTY_ROLE_MAX_NUM)
			role = PARTY_ROLE_NORMAL;

		it->second->SetRole(pid, role, true);
	}
	else
	{
		sys_err("No such party with leader [%d]", leader);
	}
}

void CPartyManager::P2PQuitParty(DWORD pid)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);

	if (it != m_map_pkParty.end())
	{
		it->second->P2PQuit(pid);
	}
	else
	{
		sys_err("No such party with member [%d]", pid);
	}
}

LPPARTY CPartyManager::P2PCreateParty(DWORD pid)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);
	if (it != m_map_pkParty.end())
		return it->second;

	LPPARTY pParty = M2_NEW CParty;

	m_set_pkPCParty.insert(pParty);

	SetPartyMember(pid, pParty);
	pParty->SetPCParty(true);
	pParty->P2PJoin(pid);

	return pParty;
}

void CPartyManager::P2PDeleteParty(DWORD pid)
{
	TPartyMap::iterator it = m_map_pkParty.find(pid);

	if (it != m_map_pkParty.end())
	{
		m_set_pkPCParty.erase(it->second);
		M2_DELETE(it->second);
	}
	else
		sys_err("PARTY P2PDeleteParty Cannot find party [%u]", pid);
}

LPPARTY CPartyManager::CreateParty(LPCHARACTER pLeader)
{
	if (pLeader->GetParty())
		return pLeader->GetParty();

	LPPARTY pParty = M2_NEW CParty;

	if (pLeader->IsPC())
	{
		//TPacketGGParty p;
		//p.header	= HEADER_GG_PARTY;
		//p.subheader	= PARTY_SUBHEADER_GG_CREATE;
		//p.pid		= pLeader->GetPlayerID();
		//P2P_MANAGER::instance().Send(&p, sizeof(p));
		TPacketPartyCreate p;
		p.dwLeaderPID = pLeader->GetPlayerID();

		db_clientdesc->DBPacket(HEADER_GD_PARTY_CREATE, 0, &p, sizeof(TPacketPartyCreate));

		sys_log(0, "PARTY: Create %s pid %u", pLeader->GetName(), pLeader->GetPlayerID());
		pParty->SetPCParty(true);
		pParty->Join(pLeader->GetPlayerID());

		m_set_pkPCParty.insert(pParty);
	}
	else
	{
		pParty->SetPCParty(false);
		pParty->Join(pLeader->GetVID());
	}

	pParty->Link(pLeader);
	return (pParty);
}

void CPartyManager::DeleteParty(LPPARTY pParty)
{
	//TPacketGGParty p;
	//p.header = HEADER_GG_PARTY;
	//p.subheader = PARTY_SUBHEADER_GG_DESTROY;
	//p.pid = pParty->GetLeaderPID();
	//P2P_MANAGER::instance().Send(&p, sizeof(p));
	TPacketPartyDelete p;
	p.dwLeaderPID = pParty->GetLeaderPID();

	db_clientdesc->DBPacket(HEADER_GD_PARTY_DELETE, 0, &p, sizeof(TPacketPartyDelete));

	m_set_pkPCParty.erase(pParty);
	M2_DELETE(pParty);
}

void CPartyManager::SetPartyMember(DWORD dwPID, LPPARTY pParty)
{
	TPartyMap::iterator it = m_map_pkParty.find(dwPID);

	if (pParty == NULL)
	{
		if (it != m_map_pkParty.end())
			m_map_pkParty.erase(it);
	}
	else
	{
		if (it != m_map_pkParty.end())
		{
			if (it->second != pParty)
			{
				it->second->Quit(dwPID);
				it->second = pParty;
			}
		}
		else
			m_map_pkParty.insert(TPartyMap::value_type(dwPID, pParty));
	}
}

EVENTINFO(party_update_event_info)
{
	DWORD pid;

	party_update_event_info()
	: pid( 0 )
	{
	}
};

/////////////////////////////////////////////////////////////////////////////
//
// CParty begin!
//
/////////////////////////////////////////////////////////////////////////////
EVENTFUNC(party_update_event)
{
	party_update_event_info* info = dynamic_cast<party_update_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "party_update_event> <Factor> Null pointer" );
		return 0;
	}

	DWORD pid = info->pid;
	LPCHARACTER leader = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (leader && leader->GetDesc())
	{
		LPPARTY pParty = leader->GetParty();

		if (pParty)
			pParty->Update();
	}

	return PASSES_PER_SEC(3);
}

CParty::CParty()
{
	Initialize();
}

CParty::~CParty()
{
	Destroy();
}

void CParty::Initialize()
{
	sys_log(2, "Party::Initialize");

	m_iExpDistributionMode = PARTY_EXP_DISTRIBUTION_NON_PARITY;
	m_pkChrExpCentralize = NULL;

	m_dwLeaderPID = 0;

	m_eventUpdate = NULL;

	memset(&m_anRoleCount, 0, sizeof(m_anRoleCount));
	memset(&m_anMaxRole, 0, sizeof(m_anMaxRole));
	m_anMaxRole[PARTY_ROLE_LEADER] = 1;
	m_anMaxRole[PARTY_ROLE_NORMAL] = 32;

	m_dwPartyStartTime = get_dword_time();
	m_iLongTimeExpBonus = 0;

	m_dwPartyHealTime = get_dword_time();
	m_bPartyHealReady = false;
	m_bCanUsePartyHeal = false;

	m_iLeadership = 0;
	m_iExpBonus = 0;
	m_iAttBonus = 0;
	m_iDefBonus = 0;

	m_itNextOwner = m_memberMap.begin();

	m_iCountNearPartyMember = 0;

	m_pkChrLeader = NULL;
	m_bPCParty = false;
	m_pkDungeon = NULL;
	m_pkDungeon_for_Only_party = NULL;
}


void CParty::Destroy()
{
	sys_log(2, "Party::Destroy");

	// PC가 만든 파티면 파티매니저에 맵에서 PID를 삭제해야 한다.
	if (m_bPCParty)
	{
		for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
			CPartyManager::instance().SetPartyMember(it->first, NULL);
	}

	event_cancel(&m_eventUpdate); 

	RemoveBonus();

	TMemberMap::iterator it = m_memberMap.begin();

	DWORD dwTime = get_dword_time();

	while (it != m_memberMap.end())
	{
		TMember & rMember = it->second;
		++it;

		if (rMember.pCharacter)
		{
			if (rMember.pCharacter->GetDesc())
			{
				TPacketGCPartyRemove p;
				p.header = HEADER_GC_PARTY_REMOVE;
				p.pid = rMember.pCharacter->GetPlayerID();
				rMember.pCharacter->GetDesc()->Packet(&p, sizeof(p));
				rMember.pCharacter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티가 해산 되었습니다."));
			}
			else
			{
				// NPC일 경우 일정 시간 후 전투 중이 아닐 때 사라지게 하는 이벤트를 시작시킨다.
				rMember.pCharacter->SetLastAttacked(dwTime);
				rMember.pCharacter->StartDestroyWhenIdleEvent();
			}

			rMember.pCharacter->SetParty(NULL);
		}
	}

	m_memberMap.clear();
	m_itNextOwner = m_memberMap.begin();
	
	if (m_pkDungeon_for_Only_party != NULL)
	{
		m_pkDungeon_for_Only_party->SetPartyNull();
		m_pkDungeon_for_Only_party = NULL;
	}
}

void CParty::ChatPacketToAllMember(BYTE type, const char* format, ...)
{
	char chatbuf[CHAT_MAX_LEN + 1];
	va_list args;

	va_start(args, format);
	vsnprintf(chatbuf, sizeof(chatbuf), format, args);
	va_end(args);

	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		TMember & rMember = it->second;

		if (rMember.pCharacter)
		{
			if (rMember.pCharacter->GetDesc())
			{
				rMember.pCharacter->ChatPacket(type, "%s", chatbuf);
			}
		}
	}
}

DWORD CParty::GetLeaderPID()
{
	return m_dwLeaderPID;
}

DWORD CParty::GetMemberCount()
{
	return m_memberMap.size();
}

void CParty::P2PJoin(DWORD dwPID)
{
	TMemberMap::iterator it = m_memberMap.find(dwPID);

	if (it == m_memberMap.end())
	{
		TMember Member;

		Member.pCharacter	= NULL;
		Member.bNear		= false;

		if (m_memberMap.empty())
		{
			Member.bRole = PARTY_ROLE_LEADER;
			m_dwLeaderPID = dwPID;
		}
		else
			Member.bRole = PARTY_ROLE_NORMAL;

		if (m_bPCParty)
		{
			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

			if (ch)
			{
				sys_log(0, "PARTY: Join %s pid %u leader %u", ch->GetName(), dwPID, m_dwLeaderPID);
				Member.strName = ch->GetName();

				if (Member.bRole == PARTY_ROLE_LEADER)
					m_iLeadership = ch->GetLeadershipSkillLevel();
			}
			else
			{
				CCI * pcci = P2P_MANAGER::instance().FindByPID(dwPID);

				if (!pcci);
				else if (pcci->bChannel == g_bChannel)
					Member.strName = pcci->szName;
				else
					sys_err("member is not in same channel PID: %u channel %d, this channel %d", dwPID, pcci->bChannel, g_bChannel);
			}
		}

		sys_log(2, "PARTY[%d] MemberCountChange %d -> %d", GetLeaderPID(), GetMemberCount(), GetMemberCount()+1);

		m_memberMap.insert(TMemberMap::value_type(dwPID, Member));

		if (m_memberMap.size() == 1)
			m_itNextOwner = m_memberMap.begin();

		if (m_bPCParty)
		{
			CPartyManager::instance().SetPartyMember(dwPID, this);
			SendPartyJoinOneToAll(dwPID);

			LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(dwPID);

			if (ch)
				SendParameter(ch);
		}
	}

	if (m_pkDungeon)
	{
		m_pkDungeon->QuitParty(this);
	}
}

void CParty::Join(DWORD dwPID)
{
	P2PJoin(dwPID);

	if (m_bPCParty)
	{
		TPacketPartyAdd p;
		p.dwLeaderPID = GetLeaderPID();
		p.dwPID = dwPID;
		p.bState = PARTY_ROLE_NORMAL; // #0000790: [M2EU] CZ 크래쉬 증가: 초기화 중요! 
		db_clientdesc->DBPacket(HEADER_GD_PARTY_ADD, 0, &p, sizeof(p));
	}
}

void CParty::P2PQuit(DWORD dwPID)
{
	TMemberMap::iterator it = m_memberMap.find(dwPID);

	if (it == m_memberMap.end())
		return;

	if (m_bPCParty)
		SendPartyRemoveOneToAll(dwPID);

	if (it == m_itNextOwner)
		IncreaseOwnership();

	if (m_bPCParty)
		RemoveBonusForOne(dwPID);

	LPCHARACTER ch = it->second.pCharacter;
	BYTE bRole = it->second.bRole;

	m_memberMap.erase(it);

	sys_log(2, "PARTY[%d] MemberCountChange %d -> %d", GetLeaderPID(), GetMemberCount(), GetMemberCount() - 1);

	if (bRole < PARTY_ROLE_MAX_NUM)
	{
		--m_anRoleCount[bRole];
	}
	else
	{
		sys_err("ROLE_COUNT_QUIT_ERROR: INDEX(%d) > MAX(%d)", bRole, PARTY_ROLE_MAX_NUM);
	}

	if (ch)
	{
		ch->SetParty(NULL);
		ComputeRolePoint(ch, bRole, false);
	}

	if (m_bPCParty)
		CPartyManager::instance().SetPartyMember(dwPID, NULL);

	// 리더가 나가면 파티는 해산되어야 한다.
	if (bRole == PARTY_ROLE_LEADER)
		CPartyManager::instance().DeleteParty(this);

	// 이 아래는 코드를 추가하지 말 것!!! 위 DeleteParty 하면 this는 없다.
}

void CParty::Quit(DWORD dwPID)
{
	// Always PC
	P2PQuit(dwPID);

	if (m_bPCParty && dwPID != GetLeaderPID())
	{
		//TPacketGGParty p;
		//p.header = HEADER_GG_PARTY;
		//p.subheader = PARTY_SUBHEADER_GG_QUIT;
		//p.pid = dwPID;
		//p.leaderpid = GetLeaderPID();
		//P2P_MANAGER::instance().Send(&p, sizeof(p));
		TPacketPartyRemove p;
		p.dwPID = dwPID;
		p.dwLeaderPID = GetLeaderPID();
		db_clientdesc->DBPacket(HEADER_GD_PARTY_REMOVE, 0, &p, sizeof(p));
	}
}

void CParty::Link(LPCHARACTER pkChr)
{
	TMemberMap::iterator it;

	if (pkChr->IsPC())
		it = m_memberMap.find(pkChr->GetPlayerID());
	else
		it = m_memberMap.find(pkChr->GetVID());

	if (it == m_memberMap.end())
	{
		sys_err("%s is not member of this party", pkChr->GetName());
		return;
	}

	// 플레이어 파티일 경우 업데이트 이벤트 생성
	if (m_bPCParty && !m_eventUpdate)
	{
		party_update_event_info* info = AllocEventInfo<party_update_event_info>();
		info->pid = m_dwLeaderPID;
		m_eventUpdate = event_create(party_update_event, info, PASSES_PER_SEC(3));
	}

	if (it->second.bRole == PARTY_ROLE_LEADER)
		m_pkChrLeader = pkChr;

	sys_log(2, "PARTY[%d] %s linked to party", GetLeaderPID(), pkChr->GetName());

	it->second.pCharacter = pkChr;
	pkChr->SetParty(this);

	if (pkChr->IsPC())
	{
		if (it->second.strName.empty())
		{
			it->second.strName = pkChr->GetName();
		}

		SendPartyJoinOneToAll(pkChr->GetPlayerID());

		SendPartyJoinAllToOne(pkChr);
		SendPartyLinkOneToAll(pkChr);
		SendPartyLinkAllToOne(pkChr);
		SendPartyInfoAllToOne(pkChr);
		SendPartyInfoOneToAll(pkChr);

		SendParameter(pkChr);

		//sys_log(0, "PARTY-DUNGEON connect %p %p", this, GetDungeon());
		if (GetDungeon() && GetDungeon()->GetMapIndex() == pkChr->GetMapIndex())
		{
			pkChr->SetDungeon(GetDungeon());
		}

		RequestSetMemberLevel(pkChr->GetPlayerID(), pkChr->GetLevel());

	}
}

void CParty::RequestSetMemberLevel(DWORD pid, int level)
{
	TPacketPartySetMemberLevel p;
	p.dwLeaderPID = GetLeaderPID();
	p.dwPID = pid;
	p.bLevel = level;
	db_clientdesc->DBPacket(HEADER_GD_PARTY_SET_MEMBER_LEVEL, 0, &p, sizeof(TPacketPartySetMemberLevel));
}

void CParty::P2PSetMemberLevel(DWORD pid, int level)
{
	if (!m_bPCParty)
		return;

	TMemberMap::iterator it;

	sys_log(0, "PARTY P2PSetMemberLevel leader %d pid %d level %d", GetLeaderPID(), pid, level);

	it = m_memberMap.find(pid);
	if (it != m_memberMap.end())
	{
		it->second.bLevel = level;
	}
}

namespace 
{
	struct FExitDungeon
	{
		void operator()(LPCHARACTER ch)
		{
			ch->ExitToSavedLocation();
		}
	};
}

void CParty::Unlink(LPCHARACTER pkChr)
{
	TMemberMap::iterator it;

	if (pkChr->IsPC())
		it = m_memberMap.find(pkChr->GetPlayerID());
	else
		it = m_memberMap.find(pkChr->GetVID());

	if (it == m_memberMap.end())
	{
		sys_err("%s is not member of this party", pkChr->GetName());
		return;
	}

	if (pkChr->IsPC())
	{
		SendPartyUnlinkOneToAll(pkChr);
		//SendPartyUnlinkAllToOne(pkChr); // 끊기는 것이므로 구지 Unlink 패킷을 보낼 필요 없다.

		if (it->second.bRole == PARTY_ROLE_LEADER)
		{
			RemoveBonus();

			if (it->second.pCharacter->GetDungeon())
			{
				// TODO: 던젼에 있으면 나머지도 나간다
				FExitDungeon f;
				ForEachNearMember(f);
			}
		}
	}

	if (it->second.bRole == PARTY_ROLE_LEADER)
		m_pkChrLeader = NULL;

	it->second.pCharacter = NULL;
	pkChr->SetParty(NULL);
}

void CParty::SendPartyRemoveOneToAll(DWORD pid)
{
	TMemberMap::iterator it;

	TPacketGCPartyRemove p;
	p.header = HEADER_GC_PARTY_REMOVE;
	p.pid = pid;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
			it->second.pCharacter->GetDesc()->Packet(&p, sizeof(p));
	}
}

void CParty::SendPartyJoinOneToAll(DWORD pid)
{
	const TMember& r = m_memberMap[pid];

	TPacketGCPartyAdd p;

	p.header = HEADER_GC_PARTY_ADD;
	p.pid = pid;
	strlcpy(p.name, r.strName.c_str(), sizeof(p.name));

	for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
			it->second.pCharacter->GetDesc()->Packet(&p, sizeof(p));
	}
}

void CParty::SendPartyJoinAllToOne(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TPacketGCPartyAdd p;

	p.header = HEADER_GC_PARTY_ADD;
	p.name[CHARACTER_NAME_MAX_LEN] = '\0';

	for (TMemberMap::iterator it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		p.pid = it->first;
		strlcpy(p.name, it->second.strName.c_str(), sizeof(p.name));
		ch->GetDesc()->Packet(&p, sizeof(p));
	}
}

void CParty::SendPartyUnlinkOneToAll(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TMemberMap::iterator it;

	TPacketGCPartyLink p;
	p.header = HEADER_GC_PARTY_UNLINK;
	p.pid = ch->GetPlayerID();
	p.vid = (DWORD)ch->GetVID();

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
		{
			it->second.pCharacter->GetDesc()->Packet(&p, sizeof(p));
		}
	}
}

void CParty::SendPartyLinkOneToAll(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TMemberMap::iterator it;

	TPacketGCPartyLink p;
	p.header = HEADER_GC_PARTY_LINK;
	p.vid = ch->GetVID();
	p.pid = ch->GetPlayerID();

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
		{
			it->second.pCharacter->GetDesc()->Packet(&p, sizeof(p));
		}
	}
}

void CParty::SendPartyLinkAllToOne(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TMemberMap::iterator it;

	TPacketGCPartyLink p;
	p.header = HEADER_GC_PARTY_LINK;

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter)
		{
			p.vid = it->second.pCharacter->GetVID();
			p.pid = it->second.pCharacter->GetPlayerID();
			ch->GetDesc()->Packet(&p, sizeof(p));
		}
	}
}

void CParty::SendPartyInfoOneToAll(DWORD pid)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return;

	if (it->second.pCharacter)
	{
		SendPartyInfoOneToAll(it->second.pCharacter);
		return;
	}

	// Data Building
	TPacketGCPartyUpdate p;
	memset(&p, 0, sizeof(p));
	p.header = HEADER_GC_PARTY_UPDATE;
	p.pid = pid;
	p.percent_hp = 255;
	p.role = it->second.bRole;

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if ((it->second.pCharacter) && (it->second.pCharacter->GetDesc()))
		{
			//sys_log(2, "PARTY send info %s[%d] to %s[%d]", ch->GetName(), (DWORD)ch->GetVID(), it->second.pCharacter->GetName(), (DWORD)it->second.pCharacter->GetVID());
			it->second.pCharacter->GetDesc()->Packet(&p, sizeof(p));
		}
	}
}

void CParty::SendPartyInfoOneToAll(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TMemberMap::iterator it;

	// Data Building
	TPacketGCPartyUpdate p;
	ch->BuildUpdatePartyPacket(p);

	for (it = m_memberMap.begin();it!= m_memberMap.end(); ++it)
	{
		if ((it->second.pCharacter) && (it->second.pCharacter->GetDesc()))
		{
			sys_log(2, "PARTY send info %s[%d] to %s[%d]", ch->GetName(), (DWORD)ch->GetVID(), it->second.pCharacter->GetName(), (DWORD)it->second.pCharacter->GetVID());
			it->second.pCharacter->GetDesc()->Packet(&p, sizeof(p));
		}
	}
}

void CParty::SendPartyInfoAllToOne(LPCHARACTER ch)
{
	TMemberMap::iterator it;

	TPacketGCPartyUpdate p;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (!it->second.pCharacter)
		{
			DWORD pid = it->first;
			memset(&p, 0, sizeof(p));
			p.header = HEADER_GC_PARTY_UPDATE;
			p.pid = pid;
			p.percent_hp = 255;
			p.role = it->second.bRole;
			ch->GetDesc()->Packet(&p, sizeof(p));
			continue;
		}

		it->second.pCharacter->BuildUpdatePartyPacket(p);
		sys_log(2, "PARTY send info %s[%d] to %s[%d]", it->second.pCharacter->GetName(), (DWORD)it->second.pCharacter->GetVID(), ch->GetName(), (DWORD)ch->GetVID());
		ch->GetDesc()->Packet(&p, sizeof(p));
	}
}

void CParty::SendMessage(LPCHARACTER ch, BYTE bMsg, DWORD dwArg1, DWORD dwArg2)
{
	if (ch->GetParty() != this)
	{
		sys_err("%s is not member of this party %p", ch->GetName(), this);
		return;
	}

	switch (bMsg)
	{
		case PM_ATTACK:
			break;

		case PM_RETURN:
			{
				TMemberMap::iterator it = m_memberMap.begin();

				while (it != m_memberMap.end())
				{
					TMember & rMember = it->second;
					++it;

					LPCHARACTER pkChr;

					if ((pkChr = rMember.pCharacter) && ch != pkChr)
					{
						DWORD x = dwArg1 + number(-500, 500);
						DWORD y = dwArg2 + number(-500, 500);

						pkChr->SetVictim(NULL);
						pkChr->SetRotationToXY(x, y);

						if (pkChr->Goto(x, y))
						{
							LPCHARACTER victim = pkChr->GetVictim();
							sys_log(0, "%s %p RETURN victim %p", pkChr->GetName(), get_pointer(pkChr), get_pointer(victim));
							pkChr->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
						}
					}
				}
			}
			break;

		case PM_ATTACKED_BY:	// 공격 받았음, 리더에게 도움을 요청
			{
				// 리더가 없을 때
				LPCHARACTER pkChrVictim = ch->GetVictim();

				if (!pkChrVictim)
					return;

				TMemberMap::iterator it = m_memberMap.begin();

				while (it != m_memberMap.end())
				{
					TMember & rMember = it->second;
					++it;

					LPCHARACTER pkChr;

					if ((pkChr = rMember.pCharacter) && ch != pkChr)
					{
						if (pkChr->CanBeginFight())
							pkChr->BeginFight(pkChrVictim);
					}
				}
			}
			break;

		case PM_AGGRO_INCREASE:
			{
				LPCHARACTER victim = CHARACTER_MANAGER::instance().Find(dwArg2);

				if (!victim)
					return;

				TMemberMap::iterator it = m_memberMap.begin();

				while (it != m_memberMap.end())
				{
					TMember & rMember = it->second;
					++it;

					LPCHARACTER pkChr;

					if ((pkChr = rMember.pCharacter) && ch != pkChr)
					{
						pkChr->UpdateAggrPoint(victim, DAMAGE_TYPE_SPECIAL, dwArg1);
					}
				}
			}
			break;
	}
}

LPCHARACTER CParty::GetLeaderCharacter()
{
	return m_memberMap[GetLeaderPID()].pCharacter;
}

bool CParty::SetRole(DWORD dwPID, BYTE bRole, bool bSet)
{
	TMemberMap::iterator it = m_memberMap.find(dwPID);

	if (it == m_memberMap.end())
	{
		return false;
	}

	LPCHARACTER ch = it->second.pCharacter;

	if (bSet)
	{
		if (m_anRoleCount[bRole] >= m_anMaxRole[bRole])
			return false;

		if (it->second.bRole != PARTY_ROLE_NORMAL)
			return false;

		it->second.bRole = bRole;

		if (ch && GetLeader())
			ComputeRolePoint(ch, bRole, true);

		if (bRole < PARTY_ROLE_MAX_NUM)
		{
			++m_anRoleCount[bRole];
		}
		else
		{
			sys_err("ROLE_COUNT_INC_ERROR: INDEX(%d) > MAX(%d)", bRole, PARTY_ROLE_MAX_NUM);
		}
	}
	else
	{
		if (it->second.bRole == PARTY_ROLE_LEADER)
			return false;

		if (it->second.bRole == PARTY_ROLE_NORMAL)
			return false;

		it->second.bRole = PARTY_ROLE_NORMAL;

		if (ch && GetLeader())
			ComputeRolePoint(ch, PARTY_ROLE_NORMAL, false);

		if (bRole < PARTY_ROLE_MAX_NUM)
		{
			--m_anRoleCount[bRole];
		}
		else
		{
			sys_err("ROLE_COUNT_DEC_ERROR: INDEX(%d) > MAX(%d)", bRole, PARTY_ROLE_MAX_NUM);
		}
	}

	SendPartyInfoOneToAll(dwPID);
	return true;
}

BYTE CParty::GetRole(DWORD pid)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return PARTY_ROLE_NORMAL;
	else
		return it->second.bRole;
}

bool CParty::IsRole(DWORD pid, BYTE bRole)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return false;

	return it->second.bRole == bRole;
}

void CParty::RemoveBonus()
{
	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch;

		if ((ch = it->second.pCharacter))
		{
			ComputeRolePoint(ch, it->second.bRole, false);
		}

		it->second.bNear = false;
	}
}

void CParty::RemoveBonusForOne(DWORD pid)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return;

	LPCHARACTER ch;

	if ((ch = it->second.pCharacter))
		ComputeRolePoint(ch, it->second.bRole, false);
}

void CParty::HealParty()
{
	// XXX DELETEME 클라이언트 완료될때까지
	{
		return;
	}
	if (!m_bPartyHealReady)
		return;

	TMemberMap::iterator it;
	LPCHARACTER l = GetLeaderCharacter();

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (!it->second.pCharacter)
			continue;

		LPCHARACTER ch = it->second.pCharacter;

		if (DISTANCE_APPROX(l->GetX()-ch->GetX(), l->GetY()-ch->GetY()) < PARTY_DEFAULT_RANGE)
		{
			ch->PointChange(POINT_HP, ch->GetMaxHP()-ch->GetHP());
			ch->PointChange(POINT_SP, ch->GetMaxSP()-ch->GetSP());
		}
	}

	m_bPartyHealReady = false;
	m_dwPartyHealTime = get_dword_time();
}

void CParty::SummonToLeader(DWORD pid)
{
	int xy[12][2] = 
	{
		{	250,	0		},
		{	216,	125		},
		{	125,	216		},
		{	0,		250		},
		{	-125,	216		},
		{	-216,	125		},
		{	-250,	0		},
		{	-216,	-125	},
		{	-125,	-216	},
		{	0,		-250	},
		{	125,	-216	},
		{	216,	-125	},
	};

	int n = 0;
	int x[12], y[12];

	SECTREE_MANAGER & s = SECTREE_MANAGER::instance();
	LPCHARACTER l = GetLeaderCharacter();

	if (m_memberMap.find(pid) == m_memberMap.end())
	{
		l->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 소환하려는 대상을 찾을 수 없습니다."));
		return;
	}

	LPCHARACTER ch = m_memberMap[pid].pCharacter;

	if (!ch)
	{
		l->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 소환하려는 대상을 찾을 수 없습니다."));
		return;
	}

	if (!ch->CanSummon(m_iLeadership))
	{
		l->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 대상을 소환할 수 없습니다."));
		return;
	}

	for (int i = 0; i < 12; ++i)
	{
		PIXEL_POSITION p;

		if (s.GetMovablePosition(l->GetMapIndex(), l->GetX() + xy [i][0], l->GetY() + xy[i][1], p))
		{
			x[n] = p.x;
			y[n] = p.y;
			n++;
		}
	}

	if (n == 0)
		l->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<파티> 파티원을 현재 위치로 소환할 수 없습니다."));
	else
	{
		int i = number(0, n - 1);
		ch->Show(l->GetMapIndex(), x[i], y[i]);
		ch->Stop();
	}
}

void CParty::IncreaseOwnership()
{
	if (m_memberMap.empty())
	{
		m_itNextOwner = m_memberMap.begin();
		return;
	}

	if (m_itNextOwner == m_memberMap.end())
		m_itNextOwner = m_memberMap.begin();
	else
	{
		m_itNextOwner++;

		if (m_itNextOwner == m_memberMap.end())
			m_itNextOwner = m_memberMap.begin();
	}
}

LPCHARACTER CParty::GetNextOwnership(LPCHARACTER ch, long x, long y)
{
	if (m_itNextOwner == m_memberMap.end())
		return ch;

	int size = m_memberMap.size();

	while (size-- > 0)
	{
		LPCHARACTER pkMember = m_itNextOwner->second.pCharacter;

		if (pkMember && DISTANCE_APPROX(pkMember->GetX() - x, pkMember->GetY() - y) < 3000)
		{
			IncreaseOwnership();
			return pkMember;
		}

		IncreaseOwnership();
	}

	return ch;
}

void CParty::ComputeRolePoint(LPCHARACTER ch, BYTE bRole, bool bAdd)
{
	if (!bAdd)
	{
		ch->PointChange(POINT_PARTY_ATTACKER_BONUS, -ch->GetPoint(POINT_PARTY_ATTACKER_BONUS));
		ch->PointChange(POINT_PARTY_TANKER_BONUS, -ch->GetPoint(POINT_PARTY_TANKER_BONUS));
		ch->PointChange(POINT_PARTY_BUFFER_BONUS, -ch->GetPoint(POINT_PARTY_BUFFER_BONUS));
		ch->PointChange(POINT_PARTY_SKILL_MASTER_BONUS, -ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
		ch->PointChange(POINT_PARTY_DEFENDER_BONUS, -ch->GetPoint(POINT_PARTY_DEFENDER_BONUS));
		ch->PointChange(POINT_PARTY_HASTE_BONUS, -ch->GetPoint(POINT_PARTY_HASTE_BONUS));
		ch->ComputeBattlePoints();
		return;
	}

	//SKILL_POWER_BY_LEVEL
	float k = (float) ch->GetSkillPowerByLevel( MIN(SKILL_MAX_LEVEL, m_iLeadership ) )/ 100.0f;
	//float k = (float) aiSkillPowerByLevel[MIN(SKILL_MAX_LEVEL, m_iLeadership)] / 100.0f;
	//
	//sys_log(0,"ComputeRolePoint %fi %d, %d ", k, SKILL_MAX_LEVEL, m_iLeadership ); 
	//END_SKILL_POWER_BY_LEVEL

	switch (bRole)
	{
		case PARTY_ROLE_ATTACKER:
			{
				//int iBonus = (int) (10 + 90 * k);
				int iBonus = (int) (10 + 60 * k);

				if (ch->GetPoint(POINT_PARTY_ATTACKER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_ATTACKER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_ATTACKER_BONUS));
					ch->ComputePoints();
				}
			}
			break;

		case PARTY_ROLE_TANKER:
			{
				int iBonus = (int) (50 + 1450 * k);

				if (ch->GetPoint(POINT_PARTY_TANKER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_TANKER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_TANKER_BONUS));
					ch->ComputePoints();
				}
			}
			break;

		case PARTY_ROLE_BUFFER:
			{
				int iBonus = (int) (5 + 45 * k);

				if (ch->GetPoint(POINT_PARTY_BUFFER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_BUFFER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_BUFFER_BONUS));
				}
			}
			break;

		case PARTY_ROLE_SKILL_MASTER:
			{
				int iBonus = (int) (25 + 600 * k);

				if (ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_SKILL_MASTER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_SKILL_MASTER_BONUS));
					ch->ComputePoints();
				}
			}
			break;
		case PARTY_ROLE_HASTE:
			{
				int iBonus = (int) (1+5*k);
				if (ch->GetPoint(POINT_PARTY_HASTE_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_HASTE_BONUS, iBonus - ch->GetPoint(POINT_PARTY_HASTE_BONUS));
					ch->ComputePoints();
				}
			}
			break;
		case PARTY_ROLE_DEFENDER:
			{
				int iBonus = (int) (5+30*k);
				if (ch->GetPoint(POINT_PARTY_DEFENDER_BONUS) != iBonus)
				{
					ch->PointChange(POINT_PARTY_DEFENDER_BONUS, iBonus - ch->GetPoint(POINT_PARTY_DEFENDER_BONUS));
					ch->ComputePoints();
				}
			}
			break;
	}
}

void CParty::Update()
{
	sys_log(1, "PARTY::Update");

	LPCHARACTER l = GetLeaderCharacter();

	if (!l)
		return;

	TMemberMap::iterator it;

	int iNearMember = 0;
	bool bResendAll = false;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch = it->second.pCharacter;

		it->second.bNear = false;

		if (!ch)
			continue;

		if (l->GetDungeon())
			it->second.bNear = l->GetDungeon() == ch->GetDungeon();
		else
			it->second.bNear = (DISTANCE_APPROX(l->GetX()-ch->GetX(), l->GetY()-ch->GetY()) < PARTY_DEFAULT_RANGE);

		if (it->second.bNear)
		{
			++iNearMember;
			//sys_log(0,"NEAR %s", ch->GetName());
		}
	}

	if (iNearMember <= 1 && !l->GetDungeon())
	{
		for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
			it->second.bNear = false;

		iNearMember = 0;
	}

	if (iNearMember != m_iCountNearPartyMember)
	{
		m_iCountNearPartyMember = iNearMember;
		bResendAll = true;
	}

	m_iLeadership = l->GetLeadershipSkillLevel();
	int iNewExpBonus = ComputePartyBonusExpPercent();
	m_iAttBonus = ComputePartyBonusAttackGrade();
	m_iDefBonus = ComputePartyBonusDefenseGrade();

	if (m_iExpBonus != iNewExpBonus)
	{
		bResendAll = true;
		m_iExpBonus = iNewExpBonus;
	}

	bool bLongTimeExpBonusChanged = false;

	// 파티 결성 후 충분한 시간이 지나면 경험치 보너스를 받는다.
	if (!m_iLongTimeExpBonus && (get_dword_time() - m_dwPartyStartTime > PARTY_ENOUGH_MINUTE_FOR_EXP_BONUS * 60 * 1000 / (g_iUseLocale?1:2)))
	{
		bLongTimeExpBonusChanged = true;
		m_iLongTimeExpBonus = 5;
		bResendAll = true;
	}

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		LPCHARACTER ch = it->second.pCharacter;

		if (!ch)
			continue;

		if (bLongTimeExpBonusChanged && ch->GetDesc())
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("파티의 협동력이 높아져 지금부터 추가 경험치 보너스를 받습니다."));

		bool bNear = it->second.bNear;

		ComputeRolePoint(ch, it->second.bRole, bNear);

		if (bNear)
		{
			if (!bResendAll)
				SendPartyInfoOneToAll(ch);
		}
	}

	// PARTY_ROLE_LIMIT_LEVEL_BUG_FIX
	m_anMaxRole[PARTY_ROLE_ATTACKER]	 = m_iLeadership >= 10 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_HASTE]	 = m_iLeadership >= 20 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_TANKER]	 = m_iLeadership >= 20 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_BUFFER]	 = m_iLeadership >= 25 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_SKILL_MASTER] = m_iLeadership >= 35 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_DEFENDER] 	 = m_iLeadership >= 40 ? 1 : 0;
	m_anMaxRole[PARTY_ROLE_ATTACKER]	+= m_iLeadership >= 40 ? 1 : 0;
	// END_OF_PARTY_ROLE_LIMIT_LEVEL_BUG_FIX

	// Party Heal Update
	if (!m_bPartyHealReady)
	{
		if (!m_bCanUsePartyHeal && m_iLeadership >= 18)
			m_dwPartyHealTime = get_dword_time();

		m_bCanUsePartyHeal = m_iLeadership >= 18; // 통솔력 18 이상은 힐을 사용할 수 있음.

		// 통솔력 40이상은 파티 힐 쿨타임이 적다.
		DWORD PartyHealCoolTime = (m_iLeadership >= 40) ? PARTY_HEAL_COOLTIME_SHORT * 60 * 1000 : PARTY_HEAL_COOLTIME_LONG * 60 * 1000;

		if (m_bCanUsePartyHeal)
		{
			if (get_dword_time() > m_dwPartyHealTime + PartyHealCoolTime)
			{
				m_bPartyHealReady = true;

				// send heal ready
				if (0) // XXX  DELETEME 클라이언트 완료될때까지
					if (GetLeaderCharacter())
						GetLeaderCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "PartyHealReady");
			}
		}
	}

	if (bResendAll)
	{
		for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
			if (it->second.pCharacter)
				SendPartyInfoOneToAll(it->second.pCharacter);
	}
}

void CParty::UpdateOnlineState(DWORD dwPID, const char* name)
{
	TMember& r = m_memberMap[dwPID];

	TPacketGCPartyAdd p;

	p.header = HEADER_GC_PARTY_ADD;
	p.pid = dwPID;
	r.strName = name;
	strlcpy(p.name, name, sizeof(p.name));

	for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
			it->second.pCharacter->GetDesc()->Packet(&p, sizeof(p));
	}
}
void CParty::UpdateOfflineState(DWORD dwPID)
{
	//const TMember& r = m_memberMap[dwPID];

	TPacketGCPartyAdd p;
	p.header = HEADER_GC_PARTY_ADD;
	p.pid = dwPID;
	memset(p.name, 0, CHARACTER_NAME_MAX_LEN+1);

	for (TMemberMap::iterator it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (it->second.pCharacter && it->second.pCharacter->GetDesc())
			it->second.pCharacter->GetDesc()->Packet(&p, sizeof(p));
	}
}

int CParty::GetFlag(const std::string& name)
{
	TFlagMap::iterator it = m_map_iFlag.find(name);

	if (it != m_map_iFlag.end())
	{
		//sys_log(0,"PARTY GetFlag %s %d", name.c_str(), it->second);
		return it->second;
	}

	//sys_log(0,"PARTY GetFlag %s 0", name.c_str());
	return 0;
}

void CParty::SetFlag(const std::string& name, int value)
{
	TFlagMap::iterator it = m_map_iFlag.find(name);

	//sys_log(0,"PARTY SetFlag %s %d", name.c_str(), value);
	if (it == m_map_iFlag.end())
	{
		m_map_iFlag.insert(make_pair(name, value));
	}
	else if (it->second != value)
	{
		it->second = value;
	}
}

void CParty::SetDungeon(LPDUNGEON pDungeon)
{
	m_pkDungeon = pDungeon;
	m_map_iFlag.clear();
}

LPDUNGEON CParty::GetDungeon()
{
	return m_pkDungeon;
}

void CParty::SetDungeon_for_Only_party(LPDUNGEON pDungeon)
{
	m_pkDungeon_for_Only_party = pDungeon;
}

LPDUNGEON CParty::GetDungeon_for_Only_party()
{
	return m_pkDungeon_for_Only_party;
}


bool CParty::IsPositionNearLeader(LPCHARACTER ch)
{
	if (!m_pkChrLeader)
		return false;

	if (DISTANCE_APPROX(ch->GetX() - m_pkChrLeader->GetX(), ch->GetY() - m_pkChrLeader->GetY()) >= PARTY_DEFAULT_RANGE)
		return false;

	return true;
}


int CParty::GetExpBonusPercent()
{
	if (GetNearMemberCount() <= 1)
		return 0;

	return m_iExpBonus + m_iLongTimeExpBonus;
}

bool CParty::IsNearLeader(DWORD pid)
{
	TMemberMap::iterator it = m_memberMap.find(pid);

	if (it == m_memberMap.end())
		return false;    

	return it->second.bNear;
}

BYTE CParty::CountMemberByVnum(DWORD dwVnum)
{
	if (m_bPCParty)
		return 0;

	LPCHARACTER tch;
	BYTE bCount = 0;

	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
	{
		if (!(tch = it->second.pCharacter))
			continue;

		if (tch->IsPC())
			continue;

		if (tch->GetMobTable().dwVnum == dwVnum)
			++bCount;
	}

	return bCount;
}

void CParty::SendParameter(LPCHARACTER ch)
{
	TPacketGCPartyParameter p;

	p.bHeader = HEADER_GC_PARTY_PARAMETER;
	p.bDistributeMode = m_iExpDistributionMode;

	LPDESC d = ch->GetDesc();

	if (d)
	{
		d->Packet(&p, sizeof(TPacketGCPartyParameter));
	}
}

void CParty::SendParameterToAll()
{
	if (!m_bPCParty)
		return;

	TMemberMap::iterator it;

	for (it = m_memberMap.begin(); it != m_memberMap.end(); ++it)
		if (it->second.pCharacter)
			SendParameter(it->second.pCharacter);
}

void CParty::SetParameter(int iMode)
{
	if (iMode >= PARTY_EXP_DISTRIBUTION_MAX_NUM)
	{
		sys_err("Invalid exp distribution mode %d", iMode);
		return;
	}

	m_iExpDistributionMode = iMode;
	SendParameterToAll();
}

int CParty::GetExpDistributionMode()
{
	return m_iExpDistributionMode;
}

void CParty::SetExpCentralizeCharacter(DWORD dwPID)
{
	TMemberMap::iterator it = m_memberMap.find(dwPID);

	if (it == m_memberMap.end())
		return;

	m_pkChrExpCentralize = it->second.pCharacter;
}

LPCHARACTER CParty::GetExpCentralizeCharacter()
{
	return m_pkChrExpCentralize;
}

int CParty::GetMemberMaxLevel()
{
	int bMax = 0;

	itertype(m_memberMap) it = m_memberMap.begin();
	while (it!=m_memberMap.end())
	{
		if (!it->second.bLevel)
		{
			++it;
			continue;
		}

		if (!bMax)
			bMax = it->second.bLevel;
		else if (it->second.bLevel)
			bMax = MAX(bMax, it->second.bLevel);
		++it;
	}
	return bMax;
}

int CParty::GetMemberMinLevel()
{
	int bMin = PLAYER_MAX_LEVEL_CONST;

	itertype(m_memberMap) it = m_memberMap.begin();
	while (it!=m_memberMap.end())
	{
		if (!it->second.bLevel)
		{
			++it;
			continue;
		}

		if (!bMin)
			bMin = it->second.bLevel;
		else if (it->second.bLevel)
			bMin = MIN(bMin, it->second.bLevel);
		++it;
	}
	return bMin;
}

int CParty::ComputePartyBonusExpPercent()
{
	if (GetNearMemberCount() <= 1)
		return 0;

	LPCHARACTER leader = GetLeaderCharacter();

	int iBonusPartyExpFromItem = 0;

	// UPGRADE_PARTY_BONUS
	int iMemberCount=MIN(8, GetNearMemberCount());

	if (leader && (leader->IsEquipUniqueItem(UNIQUE_ITEM_PARTY_BONUS_EXP) || leader->IsEquipUniqueItem(UNIQUE_ITEM_PARTY_BONUS_EXP_MALL)
		|| leader->IsEquipUniqueItem(UNIQUE_ITEM_PARTY_BONUS_EXP_GIFT) || leader->IsEquipUniqueGroup(10010)))
	{
		// 중국측 육도 적용을 확인해야한다.
		if (g_iUseLocale)
		{
			iBonusPartyExpFromItem = 30;
		}
		else
		{
			iBonusPartyExpFromItem = KOR_aiUniqueItemPartyBonusExpPercentByMemberCount[iMemberCount];
		}
	}

	if (g_iUseLocale)
		return iBonusPartyExpFromItem + CHN_aiPartyBonusExpPercentByMemberCount[iMemberCount];
	else
		return iBonusPartyExpFromItem + KOR_aiPartyBonusExpPercentByMemberCount[iMemberCount];
	// END_OF_UPGRADE_PARTY_BONUS
}

