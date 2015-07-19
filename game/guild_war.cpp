#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "log.h"
#include "char.h"
#include "packet.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "db.h"
#include "affect.h"
#include "p2p.h"
#include "war_map.h"
#include "questmanager.h"
#include "sectree_manager.h"
#include "locale_service.h"
#include "guild_manager.h"

enum
{
	GUILD_WAR_WAIT_START_DURATION = 10*60
};

// 
// Packet
//
void CGuild::GuildWarPacket(DWORD dwOppGID, BYTE bWarType, BYTE bWarState)
{
	TPacketGCGuild pack;
	TPacketGCGuildWar pack2;

	pack.header		= HEADER_GC_GUILD;
	pack.subheader	= GUILD_SUBHEADER_GC_WAR;
	pack.size		= sizeof(pack) + sizeof(pack2);
	pack2.dwGuildSelf	= GetID();
	pack2.dwGuildOpp	= dwOppGID;
	pack2.bWarState	= bWarState;
	pack2.bType		= bWarType;

	for (itertype(m_memberOnline) it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;

		if (bWarState == GUILD_WAR_ON_WAR)
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전중에는 사냥에 따른 이익이 없습니다."));

		LPDESC d = ch->GetDesc();

		if (d)
		{
			ch->SendGuildName( dwOppGID );

			d->BufferedPacket(&pack, sizeof(pack));
			d->Packet(&pack2, sizeof(pack2));
		}
	}
}

void CGuild::SendEnemyGuild(LPCHARACTER ch)
{
	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	TPacketGCGuild pack;
	TPacketGCGuildWar pack2;
	pack.header = HEADER_GC_GUILD;
	pack.subheader = GUILD_SUBHEADER_GC_WAR;
	pack.size = sizeof(pack) + sizeof(pack2);
	pack2.dwGuildSelf = GetID();

	TPacketGCGuild p;
	p.header = HEADER_GC_GUILD;
	p.subheader = GUILD_SUBHEADER_GC_WAR_SCORE;
	p.size = sizeof(p) + sizeof(DWORD) + sizeof(DWORD) + sizeof(long);

	for (itertype(m_EnemyGuild) it = m_EnemyGuild.begin(); it != m_EnemyGuild.end(); ++it)
	{
		ch->SendGuildName( it->first );

		pack2.dwGuildOpp = it->first;
		pack2.bType = it->second.type;
		pack2.bWarState = it->second.state;

		d->BufferedPacket(&pack, sizeof(pack));
		d->Packet(&pack2, sizeof(pack2));

		if (it->second.state == GUILD_WAR_ON_WAR)
		{
			long lScore;

			lScore = GetWarScoreAgainstTo(pack2.dwGuildOpp);

			d->BufferedPacket(&p, sizeof(p));
			d->BufferedPacket(&pack2.dwGuildSelf, sizeof(DWORD));
			d->BufferedPacket(&pack2.dwGuildOpp, sizeof(DWORD));
			d->Packet(&lScore, sizeof(long));

			lScore = CGuildManager::instance().TouchGuild(pack2.dwGuildOpp)->GetWarScoreAgainstTo(pack2.dwGuildSelf);

			d->BufferedPacket(&p, sizeof(p));
			d->BufferedPacket(&pack2.dwGuildOpp, sizeof(DWORD));
			d->BufferedPacket(&pack2.dwGuildSelf, sizeof(DWORD));
			d->Packet(&lScore, sizeof(long));
		}
	}
}

//
// War Login
//
int CGuild::GetGuildWarState(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return GUILD_WAR_NONE;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);
	return (it != m_EnemyGuild.end()) ? (it->second.state) : GUILD_WAR_NONE;
} 

int CGuild::GetGuildWarType(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return GUILD_WAR_TYPE_FIELD;

	return git->second.type;
}

DWORD CGuild::GetGuildWarMapIndex(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return 0;

	return git->second.map_index;
}

bool CGuild::CanStartWar(BYTE bGuildWarType) // 타입에 따라 다른 조건이 생길 수도 있음
{
	if (bGuildWarType >= GUILD_WAR_TYPE_MAX_NUM)
		return false;

	// 테스트시에는 인원수를 확인하지 않는다.
	if (test_server || quest::CQuestManager::instance().GetEventFlag("guild_war_test") != 0)
		return GetLadderPoint() > 0;

	return GetLadderPoint() > 0 && GetMemberCount() >= GUILD_WAR_MIN_MEMBER_COUNT;
}

bool CGuild::UnderWar(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return false;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);
	return (it != m_EnemyGuild.end()) && (it->second.IsWarBegin());
} 

DWORD CGuild::UnderAnyWar(BYTE bType)
{
	for (itertype(m_EnemyGuild) it = m_EnemyGuild.begin(); it != m_EnemyGuild.end(); ++it)
	{
		if (bType < GUILD_WAR_TYPE_MAX_NUM)
			if (it->second.type != bType)
				continue;

		if (it->second.IsWarBegin())
			return it->first;
	}

	return 0;
}

void CGuild::SetWarScoreAgainstTo(DWORD dwOppGID, int iScore)
{
	DWORD dwSelfGID = GetID();

	sys_log(0, "GuildWarScore Set %u from %u %d", dwSelfGID, dwOppGID, iScore);
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		it->second.score = iScore;

		if (it->second.type != GUILD_WAR_TYPE_FIELD)
		{
			CGuild * gOpp = CGuildManager::instance().TouchGuild(dwOppGID);
			CWarMap * pMap = CWarMapManager::instance().Find(it->second.map_index);

			if (pMap)
				pMap->UpdateScore(dwSelfGID, iScore, dwOppGID, gOpp->GetWarScoreAgainstTo(dwSelfGID));
		}
		else
		{
			TPacketGCGuild p;

			p.header = HEADER_GC_GUILD;
			p.subheader = GUILD_SUBHEADER_GC_WAR_SCORE;
			p.size = sizeof(p) + sizeof(DWORD) + sizeof(DWORD) + sizeof(long);

			TEMP_BUFFER buf;
			buf.write(&p, sizeof(p));

			buf.write(&dwSelfGID, sizeof(DWORD));
			buf.write(&dwOppGID, sizeof(DWORD));
			buf.write(&iScore, sizeof(long));

			Packet(buf.read_peek(), buf.size());

			CGuild * gOpp = CGuildManager::instance().TouchGuild(dwOppGID);

			if (gOpp)
				gOpp->Packet(buf.read_peek(), buf.size());
		}
	}
}

int CGuild::GetWarScoreAgainstTo(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		sys_log(0, "GuildWarScore Get %u from %u %d", GetID(), dwOppGID, it->second.score);
		return it->second.score;
	}

	sys_log(0, "GuildWarScore Get %u from %u No data", GetID(), dwOppGID);
	return 0;
}

DWORD CGuild::GetWarStartTime(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return 0;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return 0;

	return it->second.war_start_time;
}

const TGuildWarInfo& GuildWar_GetTypeInfo(unsigned type)
{
	return KOR_aGuildWarInfo[type];
}

unsigned GuildWar_GetTypeMapIndex(unsigned type)
{
	return GuildWar_GetTypeInfo(type).lMapIndex;
}

bool GuildWar_IsWarMap(unsigned type)
{
	if (type == GUILD_WAR_TYPE_FIELD)
		return true;

	unsigned mapIndex = GuildWar_GetTypeMapIndex(type);

	if (SECTREE_MANAGER::instance().GetMapRegion(mapIndex))
		return true;

	return false;
}

void CGuild::NotifyGuildMaster(const char* msg)
{
	LPCHARACTER ch = GetMasterCharacter();
	if (ch)
		ch->ChatPacket(CHAT_TYPE_INFO, msg);
}

extern void map_allow_log();

//
// War State Relative
//
//
// A Declare -> B Refuse
//           -> B Declare -> StartWar -> EndWar
//
// A Declare -> B Refuse
//           -> B Declare -> WaitStart -> Fail
//                                     -> StartWar -> EndWar
//
void CGuild::RequestDeclareWar(DWORD dwOppGID, BYTE type)
{
	if (dwOppGID == GetID())
	{
		sys_log(0, "GuildWar.DeclareWar.DECLARE_WAR_SELF id(%d -> %d), type(%d)", GetID(), dwOppGID, type);
		return;
	}

	if (type >= GUILD_WAR_TYPE_MAX_NUM)
	{
		sys_log(0, "GuildWar.DeclareWar.UNKNOWN_WAR_TYPE id(%d -> %d), type(%d)", GetID(), dwOppGID, type);
		return;
	}

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);
	if (it == m_EnemyGuild.end())
	{
		if (!GuildWar_IsWarMap(type))
		{
			sys_err("GuildWar.DeclareWar.NOT_EXIST_MAP id(%d -> %d), type(%d), map(%d)", 
					GetID(), dwOppGID, type, GuildWar_GetTypeMapIndex(type));

			map_allow_log();
			NotifyGuildMaster(LC_TEXT("전쟁 서버가 열려있지 않아 길드전을 시작할 수 없습니다."));
			return;
		}

		// 패킷 보내기 to another server
		TPacketGuildWar p;
		p.bType = type;
		p.bWar = GUILD_WAR_SEND_DECLARE;
		p.dwGuildFrom = GetID();
		p.dwGuildTo = dwOppGID;
		db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
		sys_log(0, "GuildWar.DeclareWar id(%d -> %d), type(%d)", GetID(), dwOppGID, type);
		return;
	}

	switch (it->second.state)
	{	
		case GUILD_WAR_RECV_DECLARE:
			{
				const unsigned saved_type = it->second.type;

				if (saved_type == GUILD_WAR_TYPE_FIELD)
				{
					// 선전포고 한것을 받아들였다.
					TPacketGuildWar p;
					p.bType = saved_type;
					p.bWar = GUILD_WAR_ON_WAR;
					p.dwGuildFrom = GetID();
					p.dwGuildTo = dwOppGID;
					db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
					sys_log(0, "GuildWar.AcceptWar id(%d -> %d), type(%d)", GetID(), dwOppGID, saved_type);
					return;
				}

				if (!GuildWar_IsWarMap(saved_type))
				{
					sys_err("GuildWar.AcceptWar.NOT_EXIST_MAP id(%d -> %d), type(%d), map(%d)", 
							GetID(), dwOppGID, type, GuildWar_GetTypeMapIndex(type));

					map_allow_log();
					NotifyGuildMaster(LC_TEXT("전쟁 서버가 열려있지 않아 길드전을 시작할 수 없습니다."));
					return;
				}

				const TGuildWarInfo& guildWarInfo = GuildWar_GetTypeInfo(saved_type);

				TPacketGuildWar p;
				p.bType = saved_type;
				p.bWar = GUILD_WAR_WAIT_START;
				p.dwGuildFrom = GetID();
				p.dwGuildTo = dwOppGID;
				p.lWarPrice = guildWarInfo.iWarPrice;
				p.lInitialScore = guildWarInfo.iInitialScore;

				if (test_server) 
					p.lInitialScore /= 10;

				db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));

				sys_log(0, "GuildWar.WaitStartSendToDB id(%d vs %d), type(%d), bet(%d), map_index(%d)", 
						GetID(), dwOppGID, saved_type, guildWarInfo.iWarPrice, guildWarInfo.lMapIndex);

			}
			break;
		case GUILD_WAR_SEND_DECLARE:
			{
				NotifyGuildMaster(LC_TEXT("이미 선전포고 중인 길드입니다."));
			}
			break;
		default:
			sys_err("GuildWar.DeclareWar.UNKNOWN_STATE[%d]: id(%d vs %d), type(%d), guild(%s:%u)", 
					it->second.state, GetID(), dwOppGID, type, GetName(), GetID());
			break;
	}
}

bool CGuild::DeclareWar(DWORD dwOppGID, BYTE type, BYTE state)
{
	if (m_EnemyGuild.find(dwOppGID) != m_EnemyGuild.end())
		return false;

	TGuildWar gw(type);
	gw.state = state;

	m_EnemyGuild.insert(std::make_pair(dwOppGID, gw));

	GuildWarPacket(dwOppGID, type, state);
	return true;
}

bool CGuild::CheckStartWar(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return false;

	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_ON_WAR)
		return false;

	return true;
}

void CGuild::StartWar(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return;

	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_ON_WAR)
		return;

	gw.state = GUILD_WAR_ON_WAR;
	gw.war_start_time = get_global_time();

	GuildWarPacket(dwOppGID, gw.type, GUILD_WAR_ON_WAR);

	if (gw.type != GUILD_WAR_TYPE_FIELD)
		GuildWarEntryAsk(dwOppGID);
}

bool CGuild::WaitStartWar(DWORD dwOppGID)
{
	//자기자신이면 
	if (dwOppGID == GetID())
	{
		sys_log(0 ,"GuildWar.WaitStartWar.DECLARE_WAR_SELF id(%u -> %u)", GetID(), dwOppGID);
		return false;
	}

	//상대방 길드 TGuildWar 를 얻어온다.
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);
	if (it == m_EnemyGuild.end())
	{
		sys_log(0 ,"GuildWar.WaitStartWar.UNKNOWN_ENEMY id(%u -> %u)", GetID(), dwOppGID);
		return false;
	}

	//레퍼런스에 등록하고
	TGuildWar & gw(it->second);

	if (gw.state == GUILD_WAR_WAIT_START)
	{
		sys_log(0 ,"GuildWar.WaitStartWar.UNKNOWN_STATE id(%u -> %u), state(%d)", GetID(), dwOppGID, gw.state);
		return false;
	}

	//상태를 저장한다.
	gw.state = GUILD_WAR_WAIT_START;

	//상대편의 길드 클래스 포인터를 얻어오고
	CGuild* g = CGuildManager::instance().FindGuild(dwOppGID);
	if (!g)
	{
		sys_log(0 ,"GuildWar.WaitStartWar.NOT_EXIST_GUILD id(%u -> %u)", GetID(), dwOppGID);
		return false;
	}

	// GUILDWAR_INFO
	const TGuildWarInfo& rkGuildWarInfo = GuildWar_GetTypeInfo(gw.type);
	// END_OF_GUILDWAR_INFO


	// 필드형이면 맵생성 안함
	if (gw.type == GUILD_WAR_TYPE_FIELD)
	{
		sys_log(0 ,"GuildWar.WaitStartWar.FIELD_TYPE id(%u -> %u)", GetID(), dwOppGID);
		return true;
	}		

	// 전쟁 서버 인지 확인
	sys_log(0 ,"GuildWar.WaitStartWar.CheckWarServer id(%u -> %u), type(%d), map(%d)", 
			GetID(), dwOppGID, gw.type, rkGuildWarInfo.lMapIndex);

	if (!map_allow_find(rkGuildWarInfo.lMapIndex))
	{
		sys_log(0 ,"GuildWar.WaitStartWar.SKIP_WAR_MAP id(%u -> %u)", GetID(), dwOppGID);
		return true;
	}


	DWORD id1 = GetID();
	DWORD id2 = dwOppGID;

	if (id1 > id2)
		std::swap(id1, id2);

	//워프 맵을 생성
	DWORD lMapIndex = CWarMapManager::instance().CreateWarMap(rkGuildWarInfo, id1, id2);
	if (!lMapIndex) 
	{
		sys_err("GuildWar.WaitStartWar.CREATE_WARMAP_ERROR id(%u vs %u), type(%u), map(%d)", id1, id2, gw.type, rkGuildWarInfo.lMapIndex);
		CGuildManager::instance().RequestEndWar(GetID(), dwOppGID);
		return false;
	}

	sys_log(0, "GuildWar.WaitStartWar.CreateMap id(%u vs %u), type(%u), map(%d) -> map_inst(%u)", id1, id2, gw.type, rkGuildWarInfo.lMapIndex, lMapIndex);

	//길드전 정보에 맵인덱스를 세팅
	gw.map_index = lMapIndex;

	//양쪽에 등록(?)
	SetGuildWarMapIndex(dwOppGID, lMapIndex);
	g->SetGuildWarMapIndex(GetID(), lMapIndex);

	///////////////////////////////////////////////////////
	TPacketGGGuildWarMapIndex p;

	p.bHeader	= HEADER_GG_GUILD_WAR_ZONE_MAP_INDEX;
	p.dwGuildID1	= id1;
	p.dwGuildID2 	= id2;
	p.lMapIndex	= lMapIndex;

	P2P_MANAGER::instance().Send(&p, sizeof(p));
	///////////////////////////////////////////////////////

	return true;
}

void CGuild::RequestRefuseWar(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end() && it->second.state == GUILD_WAR_RECV_DECLARE)
	{
		// 선전포고를 거절했다.
		TPacketGuildWar p;
		p.bWar = GUILD_WAR_REFUSE;
		p.dwGuildFrom = GetID();
		p.dwGuildTo = dwOppGID;

		db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
	}
}

void CGuild::RefuseWar(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end() && (it->second.state == GUILD_WAR_SEND_DECLARE || it->second.state == GUILD_WAR_RECV_DECLARE))
	{
		BYTE type = it->second.type;
		m_EnemyGuild.erase(dwOppGID);

		GuildWarPacket(dwOppGID, type, GUILD_WAR_END);
	}
}

void CGuild::ReserveWar(DWORD dwOppGID, BYTE type)
{
	if (dwOppGID == GetID())
		return;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
	{
		TGuildWar gw(type);
		gw.state = GUILD_WAR_RESERVE;
		m_EnemyGuild.insert(std::make_pair(dwOppGID, gw));
	}
	else
		it->second.state = GUILD_WAR_RESERVE;

	sys_log(0, "Guild::ReserveWar %u", dwOppGID);
}

void CGuild::EndWar(DWORD dwOppGID)
{
	if (dwOppGID == GetID())
		return;

	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it != m_EnemyGuild.end())
	{
		CWarMap * pMap = CWarMapManager::instance().Find(it->second.map_index);

		if (pMap)
			pMap->SetEnded();

		GuildWarPacket(dwOppGID, it->second.type, GUILD_WAR_END);
		m_EnemyGuild.erase(it);

		if (!UnderAnyWar())
		{
			for (itertype(m_memberOnline) it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
			{
				LPCHARACTER ch = *it;
				ch->RemoveAffect(GUILD_SKILL_BLOOD);
				ch->RemoveAffect(GUILD_SKILL_BLESS);
				ch->RemoveAffect(GUILD_SKILL_SEONGHWI);
				ch->RemoveAffect(GUILD_SKILL_ACCEL);
				ch->RemoveAffect(GUILD_SKILL_BUNNO);
				ch->RemoveAffect(GUILD_SKILL_JUMUN);

				ch->RemoveBadAffect();
			}
		}
	}
}

void CGuild::SetGuildWarMapIndex(DWORD dwOppGID, long lMapIndex)
{
	itertype(m_EnemyGuild) it = m_EnemyGuild.find(dwOppGID);

	if (it == m_EnemyGuild.end())
		return;

	it->second.map_index = lMapIndex;
	sys_log(0, "GuildWar.SetGuildWarMapIndex id(%d -> %d), map(%d)", GetID(), dwOppGID, lMapIndex);
}

void CGuild::GuildWarEntryAccept(DWORD dwOppGID, LPCHARACTER ch)
{
	itertype(m_EnemyGuild) git = m_EnemyGuild.find(dwOppGID);

	if (git == m_EnemyGuild.end())
		return;

	TGuildWar & gw(git->second);

	if (gw.type == GUILD_WAR_TYPE_FIELD)
		return;

	if (gw.state != GUILD_WAR_ON_WAR)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이미 전쟁이 끝났습니다."));
		return;
	}

	if (!gw.map_index)
		return;

	PIXEL_POSITION pos;

	if (!CWarMapManager::instance().GetStartPosition(gw.map_index, GetID() < dwOppGID ? 0 : 1, pos))
		return;

	quest::PC * pPC = quest::CQuestManager::instance().GetPC(ch->GetPlayerID());
	pPC->SetFlag("war.is_war_member", 1);

	ch->SaveExitLocation();
	ch->WarpSet(pos.x, pos.y, gw.map_index);
}

void CGuild::GuildWarEntryAsk(DWORD dwOppGID)
{
	itertype(m_EnemyGuild) git = m_EnemyGuild.find(dwOppGID);
	if (git == m_EnemyGuild.end())
	{
		sys_err("GuildWar.GuildWarEntryAsk.UNKNOWN_ENEMY(%d)", dwOppGID);
		return;
	}

	TGuildWar & gw(git->second);

	sys_log(0, "GuildWar.GuildWarEntryAsk id(%d vs %d), map(%d)", GetID(), dwOppGID, gw.map_index);
	if (!gw.map_index)
	{
		sys_err("GuildWar.GuildWarEntryAsk.NOT_EXIST_MAP id(%d vs %d)", GetID(), dwOppGID);
		return;
	}

	PIXEL_POSITION pos;
	if (!CWarMapManager::instance().GetStartPosition(gw.map_index, GetID() < dwOppGID ? 0 : 1, pos))
	{
		sys_err("GuildWar.GuildWarEntryAsk.START_POSITION_ERROR id(%d vs %d), pos(%d, %d)", GetID(), dwOppGID, pos.x, pos.y);
		return;
	}

	sys_log(0, "GuildWar.GuildWarEntryAsk.OnlineMemberCount(%d)", m_memberOnline.size());

	itertype(m_memberOnline) it = m_memberOnline.begin();

	while (it != m_memberOnline.end())
	{
		LPCHARACTER ch = *it++;

		using namespace quest;
		unsigned int questIndex=CQuestManager::instance().GetQuestIndexByName("guild_war_join");
		if (questIndex)
		{
			sys_log(0, "GuildWar.GuildWarEntryAsk.SendLetterToMember pid(%d), qid(%d)", ch->GetPlayerID(), questIndex);
			CQuestManager::instance().Letter(ch->GetPlayerID(), questIndex, 0);
		}
		else
		{
			sys_err("GuildWar.GuildWarEntryAsk.SendLetterToMember.QUEST_ERROR pid(%d), quest_name('guild_war_join.quest')", 
					ch->GetPlayerID(), questIndex);
			break;
		}
	}
}

//
// LADDER POINT
//
void CGuild::SetLadderPoint(int point) 
{ 
	if (m_data.ladder_point != point)
	{
		char buf[256];
		snprintf(buf, sizeof(buf), LC_TEXT("<길드> 래더 점수가 %d 점이 되었습니다"), point);
		for (itertype(m_memberOnline) it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
		{
			LPCHARACTER ch = (*it);
			ch->ChatPacket(CHAT_TYPE_INFO, buf);
		}
	}
	m_data.ladder_point = point; 
}

void CGuild::ChangeLadderPoint(int iChange)
{
	TPacketGuildLadderPoint p;
	p.dwGuild = GetID();
	p.lChange = iChange;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_CHANGE_LADDER_POINT, 0, &p, sizeof(p));
}


