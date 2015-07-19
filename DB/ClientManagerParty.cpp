// vim:ts=4 sw=4
#include "stdafx.h"
#include "ClientManager.h"
#include "Config.h"
#include "DBManager.h"
#include "QID.h"

void CClientManager::QUERY_PARTY_CREATE(CPeer* peer, TPacketPartyCreate* p)
{
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];

	if (pm.find(p->dwLeaderPID) == pm.end())
	{
		pm.insert(make_pair(p->dwLeaderPID, TPartyMember()));
		ForwardPacket(HEADER_DG_PARTY_CREATE, p, sizeof(TPacketPartyCreate), peer->GetChannel(), peer);
		sys_log(0, "PARTY Create [%lu]", p->dwLeaderPID);
	}
	else
	{
		sys_err("PARTY Create - Already exists [%lu]", p->dwLeaderPID);
	}
}

void CClientManager::QUERY_PARTY_DELETE(CPeer* peer, TPacketPartyDelete* p)
{
	TPartyMap& pm = m_map_pkChannelParty[peer->GetChannel()];
	itertype(pm) it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		sys_err("PARTY Delete - Non exists [%lu]", p->dwLeaderPID);
		return;
	}

	pm.erase(it);
	ForwardPacket(HEADER_DG_PARTY_DELETE, p, sizeof(TPacketPartyDelete), peer->GetChannel(), peer);
	sys_log(0, "PARTY Delete [%lu]", p->dwLeaderPID);
}

void CClientManager::QUERY_PARTY_ADD(CPeer* peer, TPacketPartyAdd* p)
{
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
	itertype(pm) it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		sys_err("PARTY Add - Non exists [%lu]", p->dwLeaderPID);
		return;
	}

	if (it->second.find(p->dwPID) == it->second.end())
	{
		it->second.insert(std::make_pair(p->dwPID, TPartyInfo()));
		ForwardPacket(HEADER_DG_PARTY_ADD, p, sizeof(TPacketPartyAdd), peer->GetChannel(), peer);
		sys_log(0, "PARTY Add [%lu] to [%lu]", p->dwPID, p->dwLeaderPID);
	}
	else
		sys_err("PARTY Add - Already [%lu] in party [%lu]", p->dwPID, p->dwLeaderPID);
}

void CClientManager::QUERY_PARTY_REMOVE(CPeer* peer, TPacketPartyRemove* p)
{
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
	itertype(pm) it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		sys_err("PARTY Remove - Non exists [%lu] cannot remove [%lu]",p->dwLeaderPID, p->dwPID);
		return;
	}

	itertype(it->second) pit = it->second.find(p->dwPID);

	if (pit != it->second.end())
	{
		it->second.erase(pit);
		ForwardPacket(HEADER_DG_PARTY_REMOVE, p, sizeof(TPacketPartyRemove), peer->GetChannel(), peer);
		sys_log(0, "PARTY Remove [%lu] to [%lu]", p->dwPID, p->dwLeaderPID);
	}
	else
		sys_err("PARTY Remove - Cannot find [%lu] in party [%lu]", p->dwPID, p->dwLeaderPID);
}

void CClientManager::QUERY_PARTY_STATE_CHANGE(CPeer* peer, TPacketPartyStateChange* p)
{
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
	itertype(pm) it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		sys_err("PARTY StateChange - Non exists [%lu] cannot state change [%lu]",p->dwLeaderPID, p->dwPID);
		return;
	}

	itertype(it->second) pit = it->second.find(p->dwPID);

	if (pit == it->second.end())
	{
		sys_err("PARTY StateChange - Cannot find [%lu] in party [%lu]", p->dwPID, p->dwLeaderPID);
		return;
	}

	if (p->bFlag)
		pit->second.bRole = p->bRole;
	else 
		pit->second.bRole = 0;

	ForwardPacket(HEADER_DG_PARTY_STATE_CHANGE, p, sizeof(TPacketPartyStateChange), peer->GetChannel(), peer);
	sys_log(0, "PARTY StateChange [%lu] at [%lu] from %d %d",p->dwPID, p->dwLeaderPID, p->bRole, p->bFlag);
}

void CClientManager::QUERY_PARTY_SET_MEMBER_LEVEL(CPeer* peer, TPacketPartySetMemberLevel* p)
{
	TPartyMap & pm = m_map_pkChannelParty[peer->GetChannel()];
	itertype(pm) it = pm.find(p->dwLeaderPID);

	if (it == pm.end())
	{
		sys_err("PARTY SetMemberLevel - Non exists [%lu] cannot level change [%lu]",p->dwLeaderPID, p->dwPID);
		return;
	}

	itertype(it->second) pit = it->second.find(p->dwPID);

	if (pit == it->second.end())
	{
		sys_err("PARTY SetMemberLevel - Cannot find [%lu] in party [%lu]", p->dwPID, p->dwLeaderPID);
		return;
	}

	pit->second.bLevel = p->bLevel;

	ForwardPacket(HEADER_DG_PARTY_SET_MEMBER_LEVEL, p, sizeof(TPacketPartySetMemberLevel), peer->GetChannel());
	sys_log(0, "PARTY SetMemberLevel pid [%lu] level %d",p->dwPID, p->bLevel);
}
