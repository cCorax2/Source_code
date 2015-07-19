#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "config.h"
#include "log.h"
#include "char.h"
#include "db.h"
#include "lzo_manager.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "packet.h"
#include "war_map.h"
#include "questmanager.h"
#include "locale_service.h"
#include "guild_manager.h"
#include "MarkManager.h"

namespace
{

	struct FGuildNameSender
	{
		FGuildNameSender(DWORD id, const char* guild_name) : id(id), name(guild_name)
		{
			p.header = HEADER_GC_GUILD;
			p.subheader = GUILD_SUBHEADER_GC_GUILD_NAME;
			p.size = sizeof(p) + GUILD_NAME_MAX_LEN + sizeof(DWORD);
		}

		void operator()(LPCHARACTER ch)
		{
			LPDESC d = ch->GetDesc();

			if (d)
			{
				d->BufferedPacket(&p, sizeof(p));
				d->BufferedPacket(&id, sizeof(id));
				d->Packet(name, GUILD_NAME_MAX_LEN);
			}
		}

		DWORD		id;
		const char *	name;
		TPacketGCGuild	p;
	};
}

CGuildManager::CGuildManager()
{
}

CGuildManager::~CGuildManager()
{
	for( TGuildMap::const_iterator iter = m_mapGuild.begin() ; iter != m_mapGuild.end() ; ++iter )
	{
		M2_DELETE(iter->second);
	}

	m_mapGuild.clear();
}

int CGuildManager::GetDisbandDelay()
{
	return quest::CQuestManager::instance().GetEventFlag("guild_disband_delay") * (test_server ? 60 : 86400);
}

int CGuildManager::GetWithdrawDelay()
{
	return quest::CQuestManager::instance().GetEventFlag("guild_withdraw_delay") * (test_server ? 60 : 86400);
}

DWORD CGuildManager::CreateGuild(TGuildCreateParameter& gcp)
{
	if (!gcp.master)
		return 0;

	if (!check_name(gcp.name))
	{
		gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드 이름이 적합하지 않습니다."));
		return 0;
	}

	std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery("SELECT COUNT(*) FROM guild%s WHERE name = '%s'",
				get_table_postfix(), gcp.name));

	if (pmsg->Get()->uiNumRows > 0)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

		if (!(row[0] && row[0][0] == '0'))
		{
			gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 이미 같은 이름의 길드가 있습니다."));
			return 0;
		}
	}
	else
	{
		gcp.master->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드를 생성할 수 없습니다."));
		return 0;
	}

	// new CGuild(gcp) queries guild tables and tell dbcache to notice other game servers.
	// other game server calls CGuildManager::LoadGuild to load guild.
	CGuild * pg = M2_NEW CGuild(gcp);
	m_mapGuild.insert(std::make_pair(pg->GetID(), pg));
	return pg->GetID();
}

void CGuildManager::Unlink(DWORD pid)
{
	m_map_pkGuildByPID.erase(pid);
}

CGuild * CGuildManager::GetLinkedGuild(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it == m_map_pkGuildByPID.end())
		return NULL;

	return it->second; 
}

void CGuildManager::Link(DWORD pid, CGuild* guild)
{
	if (m_map_pkGuildByPID.find(pid) == m_map_pkGuildByPID.end())
		m_map_pkGuildByPID.insert(std::make_pair(pid,guild));
}

void CGuildManager::P2PLogoutMember(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->P2PLogoutMember(pid);
	}
}

void CGuildManager::P2PLoginMember(DWORD pid)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(pid);

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->P2PLoginMember(pid);
	}
}

void CGuildManager::LoginMember(LPCHARACTER ch)
{
	TGuildMap::iterator it = m_map_pkGuildByPID.find(ch->GetPlayerID());

	if (it != m_map_pkGuildByPID.end())
	{
		it->second->LoginMember(ch);
	}
}


CGuild* CGuildManager::TouchGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
	{
		m_mapGuild.insert(std::make_pair(guild_id, M2_NEW CGuild(guild_id)));
		it = m_mapGuild.find(guild_id);

		CHARACTER_MANAGER::instance().for_each_pc(FGuildNameSender(guild_id, it->second->GetName()));
	}

	return it->second;
}

CGuild* CGuildManager::FindGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);
	if (it == m_mapGuild.end())
	{
		return NULL;
	}
	return it->second;
}

CGuild*	CGuildManager::FindGuildByName(const std::string guild_name)
{
	itertype(m_mapGuild) it;
	for (it = m_mapGuild.begin(); it!=m_mapGuild.end(); ++it)
	{
		if (it->second->GetName()==guild_name)
			return it->second;
	}
	return NULL;
}

void CGuildManager::Initialize()
{
	sys_log(0, "Initializing Guild");

	if (g_bAuthServer)
	{
		sys_log(0, "	No need for auth server");
		return;
	}

	std::auto_ptr<SQLMsg> pmsg(DBManager::instance().DirectQuery("SELECT id FROM guild%s", get_table_postfix()));

	std::vector<DWORD> vecGuildID;
	vecGuildID.reserve(pmsg->Get()->uiNumRows);

	for (uint i = 0; i < pmsg->Get()->uiNumRows; i++)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		DWORD guild_id = strtoul(row[0], (char**) NULL, 10);
		LoadGuild(guild_id);

		vecGuildID.push_back(guild_id);
	}

	CGuildMarkManager & rkMarkMgr = CGuildMarkManager::instance();

	rkMarkMgr.SetMarkPathPrefix("mark");

	extern bool GuildMarkConvert(const std::vector<DWORD> & vecGuildID);
	GuildMarkConvert(vecGuildID);

	rkMarkMgr.LoadMarkIndex();
	rkMarkMgr.LoadMarkImages();
	rkMarkMgr.LoadSymbol(GUILD_SYMBOL_FILENAME);
}

void CGuildManager::LoadGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
	{
		m_mapGuild.insert(std::make_pair(guild_id, M2_NEW CGuild(guild_id)));
	}
	else
	{
		//it->second->Load(guild_id);
	}
}

void CGuildManager::DisbandGuild(DWORD guild_id)
{
	TGuildMap::iterator it = m_mapGuild.find(guild_id);

	if (it == m_mapGuild.end())
		return;

	it->second->Disband();

	M2_DELETE(it->second);
	m_mapGuild.erase(it);

	CGuildMarkManager::instance().DeleteMark(guild_id);
}

void CGuildManager::SkillRecharge()
{
	for (TGuildMap::iterator it = m_mapGuild.begin(); it!=m_mapGuild.end();++it)
	{
		it->second->SkillRecharge();
	}
}

int CGuildManager::GetRank(CGuild* g)
{
	int point = g->GetLadderPoint();
	int rank = 1;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end();++it)
	{
		CGuild * pg = it->second;

		if (pg->GetLadderPoint() > point)
			rank++;
	}

	return rank;
}

struct FGuildCompare : public std::binary_function<CGuild*, CGuild*, bool>
{
	bool operator () (CGuild* g1, CGuild* g2) const
	{
		if (g1->GetLadderPoint() < g2->GetLadderPoint())
			return true;
		if (g1->GetLadderPoint() > g2->GetLadderPoint())
			return false;
		if (g1->GetGuildWarWinCount() < g2->GetGuildWarWinCount())
			return true;
		if (g1->GetGuildWarWinCount() > g2->GetGuildWarWinCount())
			return false;
		if (g1->GetGuildWarLossCount() < g2->GetGuildWarLossCount())
			return true;
		if (g1->GetGuildWarLossCount() > g2->GetGuildWarLossCount())
			return false;
		int c = strcmp(g1->GetName(), g2->GetName());
		if (c>0) 
			return true;
		return false;
	}
};

void CGuildManager::GetHighRankString(DWORD dwMyGuild, char * buffer, size_t buflen)
{
	using namespace std;
	vector<CGuild*> v;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
	{
		if (it->second)
			v.push_back(it->second);
	}

	std::sort(v.begin(), v.end(), FGuildCompare());
	int n = v.size();
	int len = 0, len2;
	*buffer = '\0';

	for (int i = 0; i < 8; ++i)
	{
		if (n - i - 1 < 0)
			break;

		CGuild * g = v[n - i - 1];

		if (!g)
			continue;

		if (g->GetLadderPoint() <= 0)
			break;

		if (dwMyGuild == g->GetID())
		{
			len2 = snprintf(buffer + len, buflen - len, "[COLOR r;255|g;255|b;0]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		if (i)
		{
			len2 = snprintf(buffer + len, buflen - len, "[ENTER]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		len2 = snprintf(buffer + len, buflen - len, "%3d | %-12s | %-8d | %4d | %4d | %4d", 
				GetRank(g),
				g->GetName(),
				g->GetLadderPoint(),
				g->GetGuildWarWinCount(),
				g->GetGuildWarDrawCount(),
				g->GetGuildWarLossCount());

		if (len2 < 0 || len2 >= (int) buflen - len)
			len += (buflen - len) - 1;
		else
			len += len2;

		if (g->GetID() == dwMyGuild)
		{
			len2 = snprintf(buffer + len, buflen - len, "[/COLOR]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}
	}
}

void CGuildManager::GetAroundRankString(DWORD dwMyGuild, char * buffer, size_t buflen)
{
	using namespace std;
	vector<CGuild*> v;

	for (itertype(m_mapGuild) it = m_mapGuild.begin(); it != m_mapGuild.end(); ++it)
	{
		if (it->second)
			v.push_back(it->second);
	}

	std::sort(v.begin(), v.end(), FGuildCompare());
	int n = v.size();
	int idx;

	for (idx = 0; idx < (int) v.size(); ++idx)
		if (v[idx]->GetID() == dwMyGuild)
			break;

	int len = 0, len2;
	int count = 0;
	*buffer = '\0';

	for (int i = -3; i <= 3; ++i)
	{
		if (idx - i < 0)
			continue;

		if (idx - i >= n)
			continue;

		CGuild * g = v[idx - i];

		if (!g)
			continue;

		if (dwMyGuild == g->GetID())
		{
			len2 = snprintf(buffer + len, buflen - len, "[COLOR r;255|g;255|b;0]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		if (count)
		{
			len2 = snprintf(buffer + len, buflen - len, "[ENTER]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}

		len2 = snprintf(buffer + len, buflen - len, "%3d | %-12s | %-8d | %4d | %4d | %4d", 
				GetRank(g),
				g->GetName(),
				g->GetLadderPoint(),
				g->GetGuildWarWinCount(),
				g->GetGuildWarDrawCount(),
				g->GetGuildWarLossCount());

		if (len2 < 0 || len2 >= (int) buflen - len)
			len += (buflen - len) - 1;
		else
			len += len2;

		++count;

		if (g->GetID() == dwMyGuild)
		{
			len2 = snprintf(buffer + len, buflen - len, "[/COLOR]");

			if (len2 < 0 || len2 >= (int) buflen - len)
				len += (buflen - len) - 1;
			else
				len += len2;
		}
	}
}

/////////////////////////////////////////////////////////////////////
// Guild War
/////////////////////////////////////////////////////////////////////
void CGuildManager::RequestCancelWar(DWORD guild_id1, DWORD guild_id2)
{
	sys_log(0, "RequestCancelWar %d %d", guild_id1, guild_id2);

	TPacketGuildWar p;
	p.bWar = GUILD_WAR_CANCEL;
	p.dwGuildFrom = guild_id1;
	p.dwGuildTo = guild_id2;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
}

void CGuildManager::RequestEndWar(DWORD guild_id1, DWORD guild_id2)
{
	sys_log(0, "RequestEndWar %d %d", guild_id1, guild_id2);

	TPacketGuildWar p;
	p.bWar = GUILD_WAR_END;
	p.dwGuildFrom = guild_id1;
	p.dwGuildTo = guild_id2;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
}

void CGuildManager::RequestWarOver(DWORD dwGuild1, DWORD dwGuild2, DWORD dwGuildWinner, long lReward)
{
	CGuild * g1 = TouchGuild(dwGuild1);
	CGuild * g2 = TouchGuild(dwGuild2);

	if (g1->GetGuildWarState(g2->GetID()) != GUILD_WAR_ON_WAR)
	{
		sys_log(0, "RequestWarOver : both guild was not in war %u %u", dwGuild1, dwGuild2);
		RequestEndWar(dwGuild1, dwGuild2);
		return;
	}

	TPacketGuildWar p;

	p.bWar = GUILD_WAR_OVER;
	// 길드전이 끝나도 보상은 없다.
	//p.lWarPrice = lReward;
	p.lWarPrice = 0;
	p.bType = dwGuildWinner == 0 ? 1 : 0; // bType == 1 means draw for this packet.

	if (dwGuildWinner == 0)
	{
		p.dwGuildFrom = dwGuild1;
		p.dwGuildTo = dwGuild2;
	}
	else
	{
		p.dwGuildFrom = dwGuildWinner;
		p.dwGuildTo = dwGuildWinner == dwGuild1 ? dwGuild2 : dwGuild1;
	}

	db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR, 0, &p, sizeof(p));
	sys_log(0, "RequestWarOver : winner %u loser %u draw %u betprice %d", p.dwGuildFrom, p.dwGuildTo, p.bType, p.lWarPrice);
}

void CGuildManager::DeclareWar(DWORD guild_id1, DWORD guild_id2, BYTE bType)
{
	if (guild_id1 == guild_id2)
		return;

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
		return;

	if (g1->DeclareWar(guild_id2, bType, GUILD_WAR_SEND_DECLARE) &&
		g2->DeclareWar(guild_id1, bType, GUILD_WAR_RECV_DECLARE))
	{
		if (false == LC_IsGermany())
		{
			char buf[256];
			snprintf(buf, sizeof(buf), LC_TEXT("%s 길드가 %s 길드에 선전포고를 하였습니다!"), TouchGuild(guild_id1)->GetName(), TouchGuild(guild_id2)->GetName());
			SendNotice(buf);
		}
	}
}

void CGuildManager::RefuseWar(DWORD guild_id1, DWORD guild_id2)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (g1 && g2)
	{
		if (g2->GetMasterCharacter())
			g2->GetMasterCharacter()->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> %s 길드가 길드전을 거부하였습니다."), g1->GetName());
	}

	if ( g1 != NULL )
		g1->RefuseWar(guild_id2);

	if ( g2 != NULL && g1 != NULL )
		g2->RefuseWar(g1->GetID());
}

void CGuildManager::WaitStartWar(DWORD guild_id1, DWORD guild_id2)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
	{
		sys_log(0, "GuildWar: CGuildManager::WaitStartWar(%d,%d) - something wrong in arg. there is no guild like that.", guild_id1, guild_id2);
		return;
	}

	if (g1->WaitStartWar(guild_id2) || g2->WaitStartWar(guild_id1) )
	{
		char buf[256];
		snprintf(buf, sizeof(buf), LC_TEXT("%s 길드와 %s 길드가 잠시 후 전쟁을 시작합니다!"), g1->GetName(), g2->GetName());
		SendNotice(buf);
	}
}

struct FSendWarList
{
	FSendWarList(BYTE subheader, DWORD guild_id1, DWORD guild_id2)
	{
		gid1 = guild_id1;
		gid2 = guild_id2;

		p.header	= HEADER_GC_GUILD;
		p.size		= sizeof(p) + sizeof(DWORD) * 2;
		p.subheader	= subheader;
	}

	void operator() (LPCHARACTER ch)
	{
		LPDESC d = ch->GetDesc();

		if (d)
		{
			d->BufferedPacket(&p, sizeof(p));
			d->BufferedPacket(&gid1, sizeof(DWORD));
			d->Packet(&gid2, sizeof(DWORD));
		}
	}

	DWORD gid1, gid2;
	TPacketGCGuild p;
};

void CGuildManager::StartWar(DWORD guild_id1, DWORD guild_id2)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (!g1 || !g2)
		return;

	if (!g1->CheckStartWar(guild_id2) || !g2->CheckStartWar(guild_id1))
		return;

	g1->StartWar(guild_id2);
	g2->StartWar(guild_id1);

	char buf[256];
	snprintf(buf, sizeof(buf), LC_TEXT("%s 길드와 %s 길드가 전쟁을 시작하였습니다!"), g1->GetName(), g2->GetName());
	SendNotice(buf);

	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);

	CHARACTER_MANAGER::instance().for_each_pc(FSendWarList(GUILD_SUBHEADER_GC_GUILD_WAR_LIST, guild_id1, guild_id2));
	m_GuildWar.insert(std::make_pair(guild_id1, guild_id2));
}

void SendGuildWarOverNotice(CGuild* g1, CGuild* g2, bool bDraw)
{
	if (g1 && g2)
	{
		char buf[256];

		if (bDraw)
		{
			snprintf(buf, sizeof(buf), LC_TEXT("%s 길드와 %s 길드 사이의 전쟁이 무승부로 끝났습니다."), g1->GetName(), g2->GetName());
		}
		else
		{
			if ( g1->GetWarScoreAgainstTo( g2->GetID() ) > g2->GetWarScoreAgainstTo( g1->GetID() ) )
			{
				snprintf(buf, sizeof(buf), LC_TEXT("%s 길드가 %s 길드와의 전쟁에서 승리 했습니다."), g1->GetName(), g2->GetName());
			}
			else
			{
				snprintf(buf, sizeof(buf), LC_TEXT("%s 길드가 %s 길드와의 전쟁에서 승리 했습니다."), g2->GetName(), g1->GetName());
			}
		}

		SendNotice(buf);
	}
}

bool CGuildManager::EndWar(DWORD guild_id1, DWORD guild_id2)
{
	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	std::pair<DWORD, DWORD> k = std::make_pair(guild_id1, guild_id2);

	TGuildWarContainer::iterator it = m_GuildWar.find(k);

	if (m_GuildWar.end() == it)
	{
		sys_log(0, "EndWar(%d,%d) - EndWar request but guild is not in list", guild_id1, guild_id2);
		return false;
	}

	if ( g1 && g2 )
	{
	    if (g1->GetGuildWarType(guild_id2) == GUILD_WAR_TYPE_FIELD)
		{
			SendGuildWarOverNotice(g1, g2, g1->GetWarScoreAgainstTo(guild_id2) == g2->GetWarScoreAgainstTo(guild_id1));
		}
	}
	else
	{
	    return false;
	}

	if (g1)
		g1->EndWar(guild_id2);

	if (g2)
		g2->EndWar(guild_id1);

	m_GuildWarEndTime[k] = get_global_time();
	CHARACTER_MANAGER::instance().for_each_pc(FSendWarList(GUILD_SUBHEADER_GC_GUILD_WAR_END_LIST, guild_id1, guild_id2));
	m_GuildWar.erase(it);

	return true;
}

void CGuildManager::WarOver(DWORD guild_id1, DWORD guild_id2, bool bDraw)
{
	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (guild_id1 > guild_id2)
		std::swap(guild_id1, guild_id2);

	std::pair<DWORD, DWORD> k = std::make_pair(guild_id1, guild_id2);

	TGuildWarContainer::iterator it = m_GuildWar.find(k);

	if (m_GuildWar.end() == it)
		return;

	SendGuildWarOverNotice(g1, g2, bDraw);

	EndWar(guild_id1, guild_id2);
}

void CGuildManager::CancelWar(DWORD guild_id1, DWORD guild_id2)
{
	if (!EndWar(guild_id1, guild_id2))
		return;

	CGuild * g1 = FindGuild(guild_id1);
	CGuild * g2 = FindGuild(guild_id2);

	if (g1)
	{
		LPCHARACTER master1 = g1->GetMasterCharacter();

		if (master1)
			master1->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전이 취소 되었습니다."));
	}

	if (g2)
	{
		LPCHARACTER master2 = g2->GetMasterCharacter();

		if (master2)
			master2->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드전이 취소 되었습니다."));
	}

	if (g1 && g2)
	{
		char buf[256+1];
		snprintf(buf, sizeof(buf), LC_TEXT("%s 길드와 %s 길드 사이의 전쟁이 취소되었습니다."), g1->GetName(), g2->GetName());
		SendNotice(buf);
	}
}

void CGuildManager::ReserveWar(DWORD dwGuild1, DWORD dwGuild2, BYTE bType) // from DB
{
	sys_log(0, "GuildManager::ReserveWar %u %u", dwGuild1, dwGuild2);

	CGuild * g1 = FindGuild(dwGuild1);
	CGuild * g2 = FindGuild(dwGuild2);

	if (!g1 || !g2)
		return;

	g1->ReserveWar(dwGuild2, bType);
	g2->ReserveWar(dwGuild1, bType);
}

void CGuildManager::ShowGuildWarList(LPCHARACTER ch)
{
	for (itertype(m_GuildWar) it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		CGuild * A = TouchGuild(it->first);
		CGuild * B = TouchGuild(it->second);

		if (A && B)
		{
			ch->ChatPacket(CHAT_TYPE_NOTICE, "%s[%d] vs %s[%d] time %u sec.",
					A->GetName(), A->GetID(),
					B->GetName(), B->GetID(),
					get_global_time() - A->GetWarStartTime(B->GetID()));
		}
	}
}

void CGuildManager::SendGuildWar(LPCHARACTER ch)
{
	if (!ch->GetDesc())
		return;

	TEMP_BUFFER buf;
	TPacketGCGuild p;
	p.header= HEADER_GC_GUILD;
	p.subheader = GUILD_SUBHEADER_GC_GUILD_WAR_LIST;
	p.size = sizeof(p) + (sizeof(DWORD) * 2) * m_GuildWar.size();
	buf.write(&p, sizeof(p));

	itertype(m_GuildWar) it;

	for (it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		buf.write(&it->first, sizeof(DWORD));
		buf.write(&it->second, sizeof(DWORD));
	}

	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void SendGuildWarScore(DWORD dwGuild, DWORD dwGuildOpp, int iDelta, int iBetScoreDelta)
{
	TPacketGuildWarScore p;

	p.dwGuildGainPoint = dwGuild;
	p.dwGuildOpponent = dwGuildOpp;
	p.lScore = iDelta;
	p.lBetScore = iBetScoreDelta;

	db_clientdesc->DBPacket(HEADER_GD_GUILD_WAR_SCORE, 0, &p, sizeof(TPacketGuildWarScore));
	sys_log(0, "SendGuildWarScore %u %u %d", dwGuild, dwGuildOpp, iDelta);
}

void CGuildManager::Kill(LPCHARACTER killer, LPCHARACTER victim)
{
	if (!killer->IsPC())
		return;

	if (!victim->IsPC())
		return;

	if (killer->GetWarMap())
	{
		killer->GetWarMap()->OnKill(killer, victim);
		return;
	}

	CGuild * gAttack = killer->GetGuild();
	CGuild * gDefend = victim->GetGuild();

	if (!gAttack || !gDefend)
		return;

	if (gAttack->GetGuildWarType(gDefend->GetID()) != GUILD_WAR_TYPE_FIELD)
		return;

	if (!gAttack->UnderWar(gDefend->GetID()))
		return;

	SendGuildWarScore(gAttack->GetID(), gDefend->GetID(), victim->GetLevel());
}

void CGuildManager::StopAllGuildWar()
{
	for (itertype(m_GuildWar) it = m_GuildWar.begin(); it != m_GuildWar.end(); ++it)
	{
		CGuild * g = CGuildManager::instance().TouchGuild(it->first);
		CGuild * pg = CGuildManager::instance().TouchGuild(it->second);
		g->EndWar(it->second);
		pg->EndWar(it->first);
	}

	m_GuildWar.clear();
}

void CGuildManager::ReserveWarAdd(TGuildWarReserve * p)
{
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(p->dwID);

	CGuildWarReserveForGame * pkReserve;

	if (it != m_map_kReserveWar.end())
		pkReserve = it->second;
	else
	{
		pkReserve = M2_NEW CGuildWarReserveForGame;

		m_map_kReserveWar.insert(std::make_pair(p->dwID, pkReserve));
		m_vec_kReserveWar.push_back(pkReserve);
	}

	thecore_memcpy(&pkReserve->data, p, sizeof(TGuildWarReserve));

	sys_log(0, "ReserveWarAdd %u gid1 %u power %d gid2 %u power %d handicap %d",
			pkReserve->data.dwID, p->dwGuildFrom, p->lPowerFrom, p->dwGuildTo, p->lPowerTo, p->lHandicap);
}

void CGuildManager::ReserveWarBet(TPacketGDGuildWarBet * p)
{
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(p->dwWarID);

	if (it == m_map_kReserveWar.end())
		return;

	it->second->mapBet.insert(std::make_pair(p->szLogin, std::make_pair(p->dwGuild, p->dwGold)));
}

bool CGuildManager::IsBet(DWORD dwID, const char * c_pszLogin)
{
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(dwID);

	if (it == m_map_kReserveWar.end())
		return true;

	return it->second->mapBet.end() != it->second->mapBet.find(c_pszLogin);
}

void CGuildManager::ReserveWarDelete(DWORD dwID)
{
	sys_log(0, "ReserveWarDelete %u", dwID);
	itertype(m_map_kReserveWar) it = m_map_kReserveWar.find(dwID);

	if (it == m_map_kReserveWar.end())
		return;

	itertype(m_vec_kReserveWar) it_vec = m_vec_kReserveWar.begin();

	while (it_vec != m_vec_kReserveWar.end())
	{
		if (*it_vec == it->second)
		{
			it_vec = m_vec_kReserveWar.erase(it_vec);
			break;
		}
		else
			++it_vec;
	}

	M2_DELETE(it->second);
	m_map_kReserveWar.erase(it);
}

std::vector<CGuildWarReserveForGame *> & CGuildManager::GetReserveWarRef()
{
	return m_vec_kReserveWar;
}

//
// End of Guild War
//

void CGuildManager::ChangeMaster(DWORD dwGID)
{
	TGuildMap::iterator iter = m_mapGuild.find(dwGID);

	if ( iter != m_mapGuild.end() )
	{
		iter->second->Load(dwGID);
	}

	// 업데이트된 정보 보내주기
	DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::SendGuildDataUpdateToAllMember), iter->second), 
			"SELECT 1");

}

