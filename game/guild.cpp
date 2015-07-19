#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "packet.h"
#include "desc_client.h"
#include "buffer_manager.h"
#include "char_manager.h"
#include "db.h"
#include "guild.h"
#include "guild_manager.h"
#include "affect.h"
#include "p2p.h"
#include "questmanager.h"
#include "building.h"
#include "locale_service.h"
#include "log.h"
#include "questmanager.h"

	SGuildMember::SGuildMember(LPCHARACTER ch, BYTE grade, DWORD offer_exp)
: pid(ch->GetPlayerID()), grade(grade), is_general(0), job(ch->GetJob()), level(ch->GetLevel()), offer_exp(offer_exp), name(ch->GetName())
{}
	SGuildMember::SGuildMember(DWORD pid, BYTE grade, BYTE is_general, BYTE job, int level, DWORD offer_exp, char* name)
: pid(pid), grade(grade), is_general(is_general), job(job), level(level), offer_exp(offer_exp), name(name)
{}

namespace 
{
	struct FGuildNameSender
	{
		FGuildNameSender(DWORD id, const char* guild_name) : id(id), name(guild_name)
		{
			p.header = HEADER_GC_GUILD;
			p.subheader = GUILD_SUBHEADER_GC_GUILD_NAME;
			p.size = sizeof(p) + sizeof(DWORD) + GUILD_NAME_MAX_LEN;
		}

		void operator() (LPCHARACTER ch)
		{
			LPDESC d = ch->GetDesc();

			if (d)
			{
				d->BufferedPacket(&p, sizeof(p));
				d->BufferedPacket(&id, sizeof(id));
				d->Packet(name, GUILD_NAME_MAX_LEN);
			}
		}

		DWORD id;
		const char * name;
		TPacketGCGuild p;
	};
}

CGuild::CGuild(TGuildCreateParameter & cp)
{
	Initialize();

	m_general_count = 0;

	m_iMemberCountBonus = 0;

	strlcpy(m_data.name, cp.name, sizeof(m_data.name));
	m_data.master_pid = cp.master->GetPlayerID();
	strlcpy(m_data.grade_array[0].grade_name, LC_TEXT("길드장"), sizeof(m_data.grade_array[0].grade_name));
	m_data.grade_array[0].auth_flag = GUILD_AUTH_ADD_MEMBER | GUILD_AUTH_REMOVE_MEMBER | GUILD_AUTH_NOTICE | GUILD_AUTH_USE_SKILL;

	for (int i = 1; i < GUILD_GRADE_COUNT; ++i)
	{
		strlcpy(m_data.grade_array[i].grade_name, LC_TEXT("길드원"), sizeof(m_data.grade_array[i].grade_name));
		m_data.grade_array[i].auth_flag = 0;
	}

	std::auto_ptr<SQLMsg> pmsg (DBManager::instance().DirectQuery(
				"INSERT INTO guild%s(name, master, sp, level, exp, skill_point, skill) "
				"VALUES('%s', %u, 1000, 1, 0, 0, '\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0\\0')", 
				get_table_postfix(), m_data.name, m_data.master_pid));

	// TODO if error occur?
	m_data.guild_id = pmsg->Get()->uiInsertID;

	for (int i = 0; i < GUILD_GRADE_COUNT; ++i)
	{
		DBManager::instance().Query("INSERT INTO guild_grade%s VALUES(%u, %d, '%s', %d)", 
				get_table_postfix(),
				m_data.guild_id, 
				i + 1, 
				m_data.grade_array[i].grade_name, 
				m_data.grade_array[i].auth_flag);
	}

	ComputeGuildPoints();
	m_data.power	= m_data.max_power;
	m_data.ladder_point	= 0;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_CREATE, 0, &m_data.guild_id, sizeof(DWORD));

	TPacketGuildSkillUpdate guild_skill;
	guild_skill.guild_id = m_data.guild_id;
	guild_skill.amount = 0;
	guild_skill.skill_point = 0;
	memset(guild_skill.skill_levels, 0, GUILD_SKILL_COUNT);

	db_clientdesc->DBPacket(HEADER_GD_GUILD_SKILL_UPDATE, 0, &guild_skill, sizeof(guild_skill));

	// TODO GUILD_NAME
	CHARACTER_MANAGER::instance().for_each_pc(FGuildNameSender(GetID(), GetName()));
	/*
	   TPacketDGGuildMember p;
	   memset(&p, 0, sizeof(p));
	   p.dwPID = cp.master->GetPlayerID();
	   p.bGrade = 15;
	   AddMember(&p);
	 */
	RequestAddMember(cp.master, GUILD_LEADER_GRADE);
}

void CGuild::Initialize()
{
	memset(&m_data, 0, sizeof(m_data));
	m_data.level = 1;

	for (int i = 0; i < GUILD_SKILL_COUNT; ++i)
		abSkillUsable[i] = true;

	m_iMemberCountBonus = 0;
}

CGuild::~CGuild()
{
}

void CGuild::RequestAddMember(LPCHARACTER ch, int grade)
{
	if (ch->GetGuild())
		return;

	TPacketGDGuildAddMember gd;

	if (m_member.find(ch->GetPlayerID()) != m_member.end())
	{
		sys_err("Already a member in guild %s[%d]", ch->GetName(), ch->GetPlayerID());
		return;
	}

	gd.dwPID = ch->GetPlayerID();
	gd.dwGuild = GetID();
	gd.bGrade = grade;

	db_clientdesc->DBPacket(HEADER_GD_GUILD_ADD_MEMBER, 0, &gd, sizeof(TPacketGDGuildAddMember));
}

void CGuild::AddMember(TPacketDGGuildMember * p)
{
	TGuildMemberContainer::iterator it;

	if ((it = m_member.find(p->dwPID)) == m_member.end())
		m_member.insert(std::make_pair(p->dwPID, TGuildMember(p->dwPID, p->bGrade, p->isGeneral, p->bJob, p->bLevel, p->dwOffer, p->szName)));
	else
	{
		TGuildMember & r_gm = it->second;
		r_gm.pid = p->dwPID;
		r_gm.grade = p->bGrade;
		r_gm.job = p->bJob;
		r_gm.offer_exp = p->dwOffer;
		r_gm.is_general = p->isGeneral;
	}

	CGuildManager::instance().Link(p->dwPID, this);

	SendListOneToAll(p->dwPID);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(p->dwPID);

	sys_log(0, "GUILD: AddMember PID %u, grade %u, job %u, level %d, offer %u, name %s ptr %p",
			p->dwPID, p->bGrade, p->bJob, p->bLevel, p->dwOffer, p->szName, get_pointer(ch));

	if (ch)
		LoginMember(ch);
	else
		P2PLoginMember(p->dwPID);
}

bool CGuild::RequestRemoveMember(DWORD pid)
{
	TGuildMemberContainer::iterator it;

	if ((it = m_member.find(pid)) == m_member.end())
		return false;

	if (it->second.grade == GUILD_LEADER_GRADE)
		return false;

	TPacketGuild gd_guild;

	gd_guild.dwGuild = GetID();
	gd_guild.dwInfo = pid;

	db_clientdesc->DBPacket(HEADER_GD_GUILD_REMOVE_MEMBER, 0, &gd_guild, sizeof(TPacketGuild));
	return true;
}

bool CGuild::RemoveMember(DWORD pid)
{
	sys_log(0, "Receive Guild P2P RemoveMember");
	TGuildMemberContainer::iterator it;

	if ((it = m_member.find(pid)) == m_member.end())
		return false;

	if (it->second.grade == GUILD_LEADER_GRADE)
		return false;

	if (it->second.is_general)
		m_general_count--;

	m_member.erase(it);
	SendOnlineRemoveOnePacket(pid);

	CGuildManager::instance().Unlink(pid);

	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(pid);

	if (ch)
	{
		//GuildRemoveAffect(ch);
		m_memberOnline.erase(ch);
		ch->SetGuild(NULL);
	}

	if ( LC_IsBrazil() == true )
	{
		DBManager::instance().Query("REPLACE INTO guild_invite_limit VALUES(%d, %d)", GetID(), get_global_time());
	}

	return true;
}

void CGuild::P2PLoginMember(DWORD pid)
{
	if (m_member.find(pid) == m_member.end())
	{
		sys_err("GUILD [%d] is not a memeber of guild.", pid);
		return;
	}

	m_memberP2POnline.insert(pid);

	// Login event occur + Send List
	TGuildMemberOnlineContainer::iterator it;

	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
		SendLoginPacket(*it, pid);
}

void CGuild::LoginMember(LPCHARACTER ch)
{
	if (m_member.find(ch->GetPlayerID()) == m_member.end())
	{
		sys_err("GUILD %s[%d] is not a memeber of guild.", ch->GetName(), ch->GetPlayerID());
		return;
	}

	ch->SetGuild(this);

	// Login event occur + Send List
	TGuildMemberOnlineContainer::iterator it;

	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
		SendLoginPacket(*it, ch);

	m_memberOnline.insert(ch);

	SendAllGradePacket(ch);
	SendGuildInfoPacket(ch);
	SendListPacket(ch);
	SendSkillInfoPacket(ch);
	SendEnemyGuild(ch);

	//GuildUpdateAffect(ch);
}

void CGuild::P2PLogoutMember(DWORD pid)
{
	if (m_member.find(pid)==m_member.end())
	{
		sys_err("GUILD [%d] is not a memeber of guild.", pid);
		return;
	}

	m_memberP2POnline.erase(pid);

	// Logout event occur
	TGuildMemberOnlineContainer::iterator it;
	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
	{
		SendLogoutPacket(*it, pid);
	}
}

void CGuild::LogoutMember(LPCHARACTER ch)
{
	if (m_member.find(ch->GetPlayerID())==m_member.end())
	{
		sys_err("GUILD %s[%d] is not a memeber of guild.", ch->GetName(), ch->GetPlayerID());
		return;
	}

	//GuildRemoveAffect(ch);

	//ch->SetGuild(NULL);
	m_memberOnline.erase(ch);

	// Logout event occur
	TGuildMemberOnlineContainer::iterator it;
	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
	{
		SendLogoutPacket(*it, ch);
	}
}

void CGuild::SendOnlineRemoveOnePacket(DWORD pid)
{
	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack)+4;
	pack.subheader = GUILD_SUBHEADER_GC_REMOVE;

	TEMP_BUFFER buf;
	buf.write(&pack,sizeof(pack));
	buf.write(&pid, sizeof(pid));

	TGuildMemberOnlineContainer::iterator it;

	for (it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(buf.read_peek(), buf.size());
	}
}

void CGuild::SendAllGradePacket(LPCHARACTER ch)
{
	LPDESC d = ch->GetDesc();
	if (!d)
		return;

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack)+1+GUILD_GRADE_COUNT*(sizeof(TGuildGrade)+1);
	pack.subheader = GUILD_SUBHEADER_GC_GRADE;

	TEMP_BUFFER buf;

	buf.write(&pack, sizeof(pack));
	BYTE n = 15;
	buf.write(&n, 1);

	for (int i=0;i<GUILD_GRADE_COUNT;i++)
	{
		BYTE j = i+1;
		buf.write(&j, 1);
		buf.write(&m_data.grade_array[i], sizeof(TGuildGrade));
	}

	d->Packet(buf.read_peek(), buf.size());
}

void CGuild::SendListOneToAll(LPCHARACTER ch)
{
	SendListOneToAll(ch->GetPlayerID());
}

void CGuild::SendListOneToAll(DWORD pid)
{

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(TPacketGCGuild);
	pack.subheader = GUILD_SUBHEADER_GC_LIST;

	pack.size += sizeof(TGuildMemberPacketData);

	TGuildMemberPacketData mbData;

	char c[CHARACTER_NAME_MAX_LEN+1];
	memset(c, 0, sizeof(c));

	TGuildMemberContainer::iterator cit = m_member.find(pid);
	if (cit == m_member.end())
		return;

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it!= m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();
		if (!d) 
			continue;

		TEMP_BUFFER buf;

		buf.write(&pack, sizeof(pack));

		cit->second._dummy = 1;

		mbData.byNameFlag = 1;
		mbData.byGrade = cit->second.grade;
		mbData.byIsGeneral = cit->second.is_general;
		mbData.byJob = cit->second.job;
		mbData.byLevel = cit->second.level;
		mbData.dwOffer = cit->second.offer_exp;
		mbData.pid = cit->second.pid;
		//buf.write(&(cit->second), sizeof(DWORD) * 3 +1);
		buf.write(&mbData, sizeof(TGuildMemberPacketData));
		buf.write(cit->second.name.c_str(), cit->second.name.length());
		buf.write(c, CHARACTER_NAME_MAX_LEN + 1 - cit->second.name.length());
		d->Packet(buf.read_peek(), buf.size());
	}
}

void CGuild::SendListPacket(LPCHARACTER ch)
{
	/*
	   List Packet

	   Header
	   Count (byte)
	   [
	   ...
	   name_flag 1 - 이름을 보내느냐 안보내느냐
	   name CHARACTER_NAME_MAX_LEN+1
	   ] * Count

	 */
	LPDESC d;
	if (!(d=ch->GetDesc()))
		return;

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(TPacketGCGuild);
	pack.subheader = GUILD_SUBHEADER_GC_LIST;

	pack.size += sizeof(TGuildMemberPacketData) * m_member.size();

	TEMP_BUFFER buf;

	TGuildMemberPacketData mbData;

	buf.write(&pack,sizeof(pack));

	char c[CHARACTER_NAME_MAX_LEN+1];

	for (TGuildMemberContainer::iterator it = m_member.begin(); it != m_member.end(); ++it)
	{
		it->second._dummy = 1;

		mbData.byNameFlag = 1;
		mbData.byGrade = it->second.grade;
		mbData.byIsGeneral = it->second.is_general;
		mbData.byJob = it->second.job;
		mbData.byLevel = it->second.level;
		mbData.dwOffer = it->second.offer_exp;
		mbData.pid = it->second.pid;

		buf.write(&mbData, sizeof(TGuildMemberPacketData));

		strlcpy(c, it->second.name.c_str(), MIN(sizeof(c), it->second.name.length() + 1));

		buf.write(c, CHARACTER_NAME_MAX_LEN+1 );

		if ( test_server )
			sys_log(0 ,"name %s job %d  ", it->second.name.c_str(), it->second.job );
	}

	d->Packet(buf.read_peek(), buf.size());

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		SendLoginPacket(ch, *it);
	}

	for (TGuildMemberP2POnlineContainer::iterator it = m_memberP2POnline.begin(); it != m_memberP2POnline.end(); ++it)
	{
		SendLoginPacket(ch, *it);
	}

}

void CGuild::SendLoginPacket(LPCHARACTER ch, LPCHARACTER chLogin)
{
	SendLoginPacket(ch, chLogin->GetPlayerID());
}

void CGuild::SendLoginPacket(LPCHARACTER ch, DWORD pid)
{
	/*
	   Login Packet
	   header 4
	   pid 4
	 */
	if (!ch->GetDesc())
		return;

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack)+4;
	pack.subheader = GUILD_SUBHEADER_GC_LOGIN;

	TEMP_BUFFER buf;

	buf.write(&pack, sizeof(pack));

	buf.write(&pid, 4);

	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CGuild::SendLogoutPacket(LPCHARACTER ch, LPCHARACTER chLogout)
{
	SendLogoutPacket(ch, chLogout->GetPlayerID());
}

void CGuild::SendLogoutPacket(LPCHARACTER ch, DWORD pid)
{
	/*
	   Logout Packet
	   header 4
	   pid 4
	 */
	if (!ch->GetDesc())
		return;

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack)+4;
	pack.subheader = GUILD_SUBHEADER_GC_LOGOUT;

	TEMP_BUFFER buf;

	buf.write(&pack, sizeof(pack));
	buf.write(&pid, 4);

	ch->GetDesc()->Packet(buf.read_peek(), buf.size());
}

void CGuild::LoadGuildMemberData(SQLMsg* pmsg)
{
	if (pmsg->Get()->uiNumRows == 0)
		return;

	m_general_count = 0;

	m_member.clear();

	for (uint i = 0; i < pmsg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);

		DWORD pid = strtoul(row[0], (char**) NULL, 10);
		BYTE grade = (BYTE) strtoul(row[1], (char**) NULL, 10);
		BYTE is_general = 0;

		if (row[2] && *row[2] == '1')
			is_general = 1;

		DWORD offer = strtoul(row[3], (char**) NULL, 10);
		int level = (int)strtoul(row[4], (char**) NULL, 10);
		BYTE job = (BYTE)strtoul(row[5], (char**) NULL, 10);
		char * name = row[6];

		if (is_general)
			m_general_count++;

		m_member.insert(std::make_pair(pid, TGuildMember(pid, grade, is_general, job, level, offer, name)));
		CGuildManager::instance().Link(pid, this);
	}
}

void CGuild::LoadGuildGradeData(SQLMsg* pmsg)
{
	/*
    // 15개 아닐 가능성 존재
	if (pmsg->Get()->iNumRows != 15)
	{
		sys_err("Query failed: getting guild grade data. GuildID(%d)", GetID());
		return;
	}
	*/
	for (uint i = 0; i < pmsg->Get()->uiNumRows; ++i)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		BYTE grade = 0;
		str_to_number(grade, row[0]);
		char * name = row[1];
		DWORD auth = strtoul(row[2], NULL, 10);

		if (grade >= 1 && grade <= 15)
		{
			//sys_log(0, "GuildGradeLoad %s", name);
			strlcpy(m_data.grade_array[grade-1].grade_name, name, sizeof(m_data.grade_array[grade-1].grade_name));
			m_data.grade_array[grade-1].auth_flag = auth;
		}
	}
}
void CGuild::LoadGuildData(SQLMsg* pmsg)
{
	if (pmsg->Get()->uiNumRows == 0)
	{
		sys_err("Query failed: getting guild data %s", pmsg->stQuery.c_str());
		return;
	}

	MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
	m_data.master_pid = strtoul(row[0], (char **)NULL, 10);
	m_data.level = (BYTE)strtoul(row[1], (char **)NULL, 10);
	m_data.exp = strtoul(row[2], (char **)NULL, 10);
	strlcpy(m_data.name, row[3], sizeof(m_data.name));

	m_data.skill_point = (BYTE) strtoul(row[4], (char **) NULL, 10);
	if (row[5])
		thecore_memcpy(m_data.abySkill, row[5], sizeof(BYTE) * GUILD_SKILL_COUNT);
	else
		memset(m_data.abySkill, 0, sizeof(BYTE) * GUILD_SKILL_COUNT);

	m_data.power = MAX(0, strtoul(row[6], (char **) NULL, 10));

	str_to_number(m_data.ladder_point, row[7]);

	if (m_data.ladder_point < 0)
		m_data.ladder_point = 0;

	str_to_number(m_data.win, row[8]);
	str_to_number(m_data.draw, row[9]);
	str_to_number(m_data.loss, row[10]);
	str_to_number(m_data.gold, row[11]);

	ComputeGuildPoints();
}

void CGuild::Load(DWORD guild_id)
{
	Initialize();

	m_data.guild_id = guild_id;

	DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::LoadGuildData), this), 
			"SELECT master, level, exp, name, skill_point, skill, sp, ladder_point, win, draw, loss, gold FROM guild%s WHERE id = %u", get_table_postfix(), m_data.guild_id);

	sys_log(0, "GUILD: loading guild id %12s %u", m_data.name, guild_id);

	DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::LoadGuildGradeData), this), 
			"SELECT grade, name, auth+0 FROM guild_grade%s WHERE guild_id = %u", get_table_postfix(), m_data.guild_id);

	DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::LoadGuildMemberData), this), 
			"SELECT pid, grade, is_general, offer, level, job, name FROM guild_member%s, player%s WHERE guild_id = %u and pid = id", get_table_postfix(), get_table_postfix(), guild_id);
}

void CGuild::SaveLevel()
{
	DBManager::instance().Query("UPDATE guild%s SET level=%d, exp=%u, skill_point=%d WHERE id = %u", get_table_postfix(), m_data.level,m_data.exp, m_data.skill_point,m_data.guild_id);
}

void CGuild::SendDBSkillUpdate(int amount)
{
	TPacketGuildSkillUpdate guild_skill;
	guild_skill.guild_id = m_data.guild_id;
	guild_skill.amount = amount;
	guild_skill.skill_point = m_data.skill_point;
	thecore_memcpy(guild_skill.skill_levels, m_data.abySkill, sizeof(BYTE) * GUILD_SKILL_COUNT);

	db_clientdesc->DBPacket(HEADER_GD_GUILD_SKILL_UPDATE, 0, &guild_skill, sizeof(guild_skill));
}

void CGuild::SaveSkill()
{
	char text[GUILD_SKILL_COUNT * 2 + 1];

	DBManager::instance().EscapeString(text, sizeof(text), (const char *) m_data.abySkill, sizeof(m_data.abySkill));
	DBManager::instance().Query("UPDATE guild%s SET sp = %d, skill_point=%d, skill='%s' WHERE id = %u",
			get_table_postfix(), m_data.power, m_data.skill_point, text, m_data.guild_id);
}

TGuildMember* CGuild::GetMember(DWORD pid)
{
	TGuildMemberContainer::iterator it = m_member.find(pid);
	if (it==m_member.end())
		return NULL;

	return &it->second;
}

DWORD CGuild::GetMemberPID(const std::string& strName)
{
	for ( TGuildMemberContainer::iterator iter = m_member.begin();
			iter != m_member.end(); iter++ )
	{
		if ( iter->second.name == strName ) return iter->first;
	}

	return 0;
}

void CGuild::__P2PUpdateGrade(SQLMsg* pmsg)
{
	if (pmsg->Get()->uiNumRows)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		
		int grade = 0;
		const char* name = row[1];
		int auth = 0;

		str_to_number(grade, row[0]);
		str_to_number(auth, row[2]);

		if (grade <= 0)
			return;

		grade--;

		// 등급 명칭이 현재와 다르다면 업데이트
		if (0 != strcmp(m_data.grade_array[grade].grade_name, name))
		{
			strlcpy(m_data.grade_array[grade].grade_name, name, sizeof(m_data.grade_array[grade].grade_name));

			TPacketGCGuild pack;
			
			pack.header = HEADER_GC_GUILD;
			pack.size = sizeof(pack);
			pack.subheader = GUILD_SUBHEADER_GC_GRADE_NAME;

			TOneGradeNamePacket pack2;

			pack.size += sizeof(pack2);
			pack2.grade = grade + 1;
			strlcpy(pack2.grade_name, name, sizeof(pack2.grade_name));

			TEMP_BUFFER buf;

			buf.write(&pack,sizeof(pack));
			buf.write(&pack2,sizeof(pack2));

			for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it!=m_memberOnline.end(); ++it)
			{
				LPDESC d = (*it)->GetDesc();

				if (d)
					d->Packet(buf.read_peek(), buf.size());
			}
		}

		if (m_data.grade_array[grade].auth_flag != auth)
		{
			m_data.grade_array[grade].auth_flag = auth;

			TPacketGCGuild pack;
			pack.header = HEADER_GC_GUILD;
			pack.size = sizeof(pack);
			pack.subheader = GUILD_SUBHEADER_GC_GRADE_AUTH;

			TOneGradeAuthPacket pack2;
			pack.size+=sizeof(pack2);
			pack2.grade = grade+1;
			pack2.auth = auth;

			TEMP_BUFFER buf;
			buf.write(&pack,sizeof(pack));
			buf.write(&pack2,sizeof(pack2));

			for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it!=m_memberOnline.end(); ++it)
			{
				LPDESC d = (*it)->GetDesc();
				if (d)
				{
					d->Packet(buf.read_peek(), buf.size());
				}
			}
		}
	}
}

void CGuild::P2PChangeGrade(BYTE grade)
{
	DBManager::instance().FuncQuery(std::bind1st(std::mem_fun(&CGuild::__P2PUpdateGrade),this),
			"SELECT grade, name, auth+0 FROM guild_grade%s WHERE guild_id = %u and grade = %d", get_table_postfix(), m_data.guild_id, grade);
}

namespace 
{
	struct FSendChangeGrade
	{
		BYTE grade;
		TPacketGuild p;

		FSendChangeGrade(DWORD guild_id, BYTE grade) : grade(grade)
		{
			p.dwGuild = guild_id;
			p.dwInfo = grade;
		}

		void operator()()
		{
			db_clientdesc->DBPacket(HEADER_GD_GUILD_CHANGE_GRADE, 0, &p, sizeof(p));
		}
	};
}

void CGuild::ChangeGradeName(BYTE grade, const char* grade_name)
{
	if (grade == 1)
		return;

	if (grade < 1 || grade > 15)
	{
		sys_err("Wrong guild grade value %d", grade);
		return;
	}

	if (strlen(grade_name) > GUILD_NAME_MAX_LEN)
		return;

	if (!*grade_name)
		return;

	char text[GUILD_NAME_MAX_LEN * 2 + 1];

	DBManager::instance().EscapeString(text, sizeof(text), grade_name, strlen(grade_name));
	DBManager::instance().FuncAfterQuery(FSendChangeGrade(GetID(), grade), "UPDATE guild_grade%s SET name = '%s' where guild_id = %u and grade = %d", get_table_postfix(), text, m_data.guild_id, grade);

	grade--;
	strlcpy(m_data.grade_array[grade].grade_name, grade_name, sizeof(m_data.grade_array[grade].grade_name));

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack);
	pack.subheader = GUILD_SUBHEADER_GC_GRADE_NAME;

	TOneGradeNamePacket pack2;
	pack.size+=sizeof(pack2);
	pack2.grade = grade+1;
	strlcpy(pack2.grade_name,grade_name, sizeof(pack2.grade_name));

	TEMP_BUFFER buf;
	buf.write(&pack,sizeof(pack));
	buf.write(&pack2,sizeof(pack2));

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it!=m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(buf.read_peek(), buf.size());
	}
}

void CGuild::ChangeGradeAuth(BYTE grade, BYTE auth)
{
	if (grade == 1)
		return;

	if (grade < 1 || grade > 15)
	{
		sys_err("Wrong guild grade value %d", grade);
		return;
	}

	DBManager::instance().FuncAfterQuery(FSendChangeGrade(GetID(),grade), "UPDATE guild_grade%s SET auth = %d where guild_id = %u and grade = %d", get_table_postfix(), auth, m_data.guild_id, grade);

	grade--;

	m_data.grade_array[grade].auth_flag=auth;

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack);
	pack.subheader = GUILD_SUBHEADER_GC_GRADE_AUTH;

	TOneGradeAuthPacket pack2;
	pack.size += sizeof(pack2);
	pack2.grade = grade + 1;
	pack2.auth = auth;

	TEMP_BUFFER buf;
	buf.write(&pack, sizeof(pack));
	buf.write(&pack2, sizeof(pack2));

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(buf.read_peek(), buf.size());
	}
}

void CGuild::SendGuildInfoPacket(LPCHARACTER ch)
{
	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(TPacketGCGuild) + sizeof(TPacketGCGuildInfo);
	pack.subheader = GUILD_SUBHEADER_GC_INFO;

	TPacketGCGuildInfo pack_sub;

	memset(&pack_sub, 0, sizeof(TPacketGCGuildInfo));
	pack_sub.member_count = GetMemberCount(); 
	pack_sub.max_member_count = GetMaxMemberCount();
	pack_sub.guild_id = m_data.guild_id;
	pack_sub.master_pid = m_data.master_pid;
	pack_sub.exp	= m_data.exp;
	pack_sub.level	= m_data.level;
	strlcpy(pack_sub.name, m_data.name, sizeof(pack_sub.name));
	pack_sub.gold	= m_data.gold;
	pack_sub.has_land	= HasLand();

	sys_log(0, "GMC guild_name %s", m_data.name);
	sys_log(0, "GMC master %d", m_data.master_pid);

	d->BufferedPacket(&pack, sizeof(TPacketGCGuild));
	d->Packet(&pack_sub, sizeof(TPacketGCGuildInfo));
}

bool CGuild::OfferExp(LPCHARACTER ch, int amount)
{
	TGuildMemberContainer::iterator cit = m_member.find(ch->GetPlayerID());

	if (cit == m_member.end())
		return false;

	if (m_data.exp+amount < m_data.exp)
		return false;

	if (amount < 0)
		return false;

	if (ch->GetExp() < (DWORD) amount)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 제공하고자 하는 경험치가 남은 경험치보다 많습니다."));
		return false;
	}

	if (ch->GetExp() - (DWORD) amount > ch->GetExp())
	{
		sys_err("Wrong guild offer amount %d by %s[%u]", amount, ch->GetName(), ch->GetPlayerID());
		return false;
	}

	ch->PointChange(POINT_EXP, -amount);

	TPacketGuildExpUpdate guild_exp;
	guild_exp.guild_id = GetID();
	guild_exp.amount = amount / 100;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_EXP_UPDATE, 0, &guild_exp, sizeof(guild_exp));
	GuildPointChange(POINT_EXP, amount / 100, true);

	cit->second.offer_exp += amount / 100;
	cit->second._dummy = 0;

	TPacketGCGuild pack;
	TGuildMemberPacketData mbData;
	pack.header = HEADER_GC_GUILD;
	pack.subheader = GUILD_SUBHEADER_GC_LIST;
	pack.size = sizeof(TPacketGCGuild);
	pack.size += sizeof(TGuildMemberPacketData);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();
		if (d)
		{

			TEMP_BUFFER buf;

			buf.write(&pack, sizeof(pack));

			mbData.byNameFlag = 0;
			mbData.byGrade = cit->second.grade;
			mbData.byIsGeneral = cit->second.is_general;
			mbData.byJob = cit->second.job;
			mbData.byLevel = cit->second.level;
			mbData.dwOffer = cit->second.offer_exp;
			mbData.pid = cit->second.pid;
			buf.write(&mbData, sizeof(TGuildMemberPacketData));

			d->Packet(buf.read_peek(), buf.size());
		}
	}

	SaveMember(ch->GetPlayerID());

	TPacketGuildChangeMemberData gd_guild;

	gd_guild.guild_id = GetID();
	gd_guild.pid = ch->GetPlayerID();
	gd_guild.offer = cit->second.offer_exp;
	gd_guild.level = ch->GetLevel();
	gd_guild.grade = cit->second.grade;

	db_clientdesc->DBPacket(HEADER_GD_GUILD_CHANGE_MEMBER_DATA, 0, &gd_guild, sizeof(gd_guild));
	return true;
}

void CGuild::Disband()
{
	sys_log(0, "GUILD: Disband %s:%u", GetName(), GetID());

	//building::CLand* pLand = building::CManager::instance().FindLandByGuild(GetID());
	//if (pLand)
	//pLand->SetOwner(0);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;
		ch->SetGuild(NULL);
		SendOnlineRemoveOnePacket(ch->GetPlayerID());
		ch->SetQuestFlag("guild_manage.new_withdraw_time", get_global_time());
	}

	for (TGuildMemberContainer::iterator it = m_member.begin(); it != m_member.end(); ++it)
	{
		CGuildManager::instance().Unlink(it->first);
	}

}

void CGuild::RequestDisband(DWORD pid)
{
	if (m_data.master_pid != pid)
		return;

	TPacketGuild gd_guild;
	gd_guild.dwGuild = GetID();
	gd_guild.dwInfo = 0;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_DISBAND, 0, &gd_guild, sizeof(TPacketGuild));

	// LAND_CLEAR
	building::CManager::instance().ClearLandByGuildID(GetID());
	// END_LAND_CLEAR
}

void CGuild::AddComment(LPCHARACTER ch, const std::string& str)
{
	if (str.length() > GUILD_COMMENT_MAX_LEN)
		return;

	char text[GUILD_COMMENT_MAX_LEN * 2 + 1];
	DBManager::instance().EscapeString(text, sizeof(text), str.c_str(), str.length());

	DBManager::instance().FuncAfterQuery(void_bind(std::bind1st(std::mem_fun(&CGuild::RefreshCommentForce),this),ch->GetPlayerID()),
			"INSERT INTO guild_comment%s(guild_id, name, notice, content, time) VALUES(%u, '%s', %d, '%s', NOW())", 
			get_table_postfix(), m_data.guild_id, ch->GetName(), (str[0] == '!') ? 1 : 0, text);
}

void CGuild::DeleteComment(LPCHARACTER ch, DWORD comment_id)
{
	SQLMsg * pmsg;

	if (GetMember(ch->GetPlayerID())->grade == GUILD_LEADER_GRADE)
		pmsg = DBManager::instance().DirectQuery("DELETE FROM guild_comment%s WHERE id = %u AND guild_id = %u",get_table_postfix(), comment_id, m_data.guild_id);
	else
		pmsg = DBManager::instance().DirectQuery("DELETE FROM guild_comment%s WHERE id = %u AND guild_id = %u AND name = '%s'",get_table_postfix(), comment_id, m_data.guild_id, ch->GetName());

	if (pmsg->Get()->uiAffectedRows == 0 || pmsg->Get()->uiAffectedRows == (uint32_t)-1)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 삭제할 수 없는 글입니다."));
	else
		RefreshCommentForce(ch->GetPlayerID());

	M2_DELETE(pmsg);
}

void CGuild::RefreshComment(LPCHARACTER ch)
{
	RefreshCommentForce(ch->GetPlayerID());
}

void CGuild::RefreshCommentForce(DWORD player_id)
{
	LPCHARACTER ch = CHARACTER_MANAGER::instance().FindByPID(player_id);
	if (ch == NULL) {
		return;
	}

	std::auto_ptr<SQLMsg> pmsg (DBManager::instance().DirectQuery("SELECT id, name, content FROM guild_comment%s WHERE guild_id = %u ORDER BY notice DESC, id DESC LIMIT %d", get_table_postfix(), m_data.guild_id, GUILD_COMMENT_MAX_COUNT));

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack)+1;
	pack.subheader = GUILD_SUBHEADER_GC_COMMENTS;

	BYTE count = pmsg->Get()->uiNumRows;

	LPDESC d = ch->GetDesc();

	if (!d) 
		return;

	pack.size += (sizeof(DWORD)+CHARACTER_NAME_MAX_LEN+1+GUILD_COMMENT_MAX_LEN+1)*(WORD)count;
	d->BufferedPacket(&pack,sizeof(pack));
	d->BufferedPacket(&count, 1);
	char szName[CHARACTER_NAME_MAX_LEN + 1];
	char szContent[GUILD_COMMENT_MAX_LEN + 1];
	memset(szName, 0, sizeof(szName));
	memset(szContent, 0, sizeof(szContent));

	for (uint i = 0; i < pmsg->Get()->uiNumRows; i++)
	{
		MYSQL_ROW row = mysql_fetch_row(pmsg->Get()->pSQLResult);
		DWORD id = strtoul(row[0], NULL, 10);

		strlcpy(szName, row[1], sizeof(szName));
		strlcpy(szContent, row[2], sizeof(szContent));

		d->BufferedPacket(&id, sizeof(id));
		d->BufferedPacket(szName, sizeof(szName));

		if (i == pmsg->Get()->uiNumRows - 1)
			d->Packet(szContent, sizeof(szContent)); // 마지막 줄이면 보내기
		else
			d->BufferedPacket(szContent, sizeof(szContent));
	}
}

bool CGuild::ChangeMemberGeneral(DWORD pid, BYTE is_general)
{
	if (is_general && GetGeneralCount() >= GetMaxGeneralCount())
		return false;

	TGuildMemberContainer::iterator it = m_member.find(pid);
	if (it == m_member.end())
	{
		return true;
	}

	is_general = is_general?1:0;

	if (it->second.is_general == is_general)
		return true;

	if (is_general)
		++m_general_count;
	else
		--m_general_count;

	it->second.is_general = is_general;

	TGuildMemberOnlineContainer::iterator itOnline = m_memberOnline.begin();

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack)+5;
	pack.subheader = GUILD_SUBHEADER_GC_CHANGE_MEMBER_GENERAL;

	while (itOnline != m_memberOnline.end())
	{
		LPDESC d = (*(itOnline++))->GetDesc();

		if (!d)
			continue;

		d->BufferedPacket(&pack, sizeof(pack));
		d->BufferedPacket(&pid, sizeof(pid));
		d->Packet(&is_general, sizeof(is_general));
	}

	SaveMember(pid);
	return true;
}

void CGuild::ChangeMemberGrade(DWORD pid, BYTE grade)
{
	if (grade == 1)
		return;

	TGuildMemberContainer::iterator it = m_member.find(pid);

	if (it == m_member.end())
		return;

	it->second.grade = grade;

	TGuildMemberOnlineContainer::iterator itOnline = m_memberOnline.begin();

	TPacketGCGuild pack;
	pack.header = HEADER_GC_GUILD;
	pack.size = sizeof(pack)+5;
	pack.subheader = GUILD_SUBHEADER_GC_CHANGE_MEMBER_GRADE;

	while (itOnline != m_memberOnline.end())
	{
		LPDESC d = (*(itOnline++))->GetDesc();

		if (!d)
			continue;

		d->BufferedPacket(&pack, sizeof(pack));
		d->BufferedPacket(&pid, sizeof(pid));
		d->Packet(&grade, sizeof(grade));
	}

	SaveMember(pid);

	TPacketGuildChangeMemberData gd_guild;

	gd_guild.guild_id = GetID();
	gd_guild.pid = pid;
	gd_guild.offer = it->second.offer_exp;
	gd_guild.level = it->second.level;
	gd_guild.grade = grade;

	db_clientdesc->DBPacket(HEADER_GD_GUILD_CHANGE_MEMBER_DATA, 0, &gd_guild, sizeof(gd_guild));
}

void CGuild::SkillLevelUp(DWORD dwVnum)
{
	DWORD dwRealVnum = dwVnum - GUILD_SKILL_START;

	if (dwRealVnum >= GUILD_SKILL_COUNT)
		return;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such guild skill by number %u", dwVnum);
		return;
	}

	if (m_data.abySkill[dwRealVnum] >= pkSk->bMaxLevel)
		return;

	if (m_data.skill_point <= 0)
		return;
	m_data.skill_point --;

	m_data.abySkill[dwRealVnum] ++;

	ComputeGuildPoints();
	SaveSkill();
	SendDBSkillUpdate();

	/*switch (dwVnum)
	  {
	  case GUILD_SKILL_GAHO:
	  {
	  TGuildMemberOnlineContainer::iterator it;

	  for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	  (*it)->PointChange(POINT_DEF_GRADE, 1);
	  }
	  break;
	  case GUILD_SKILL_HIM:
	  {
	  TGuildMemberOnlineContainer::iterator it;

	  for (it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	  (*it)->PointChange(POINT_ATT_GRADE, 1);
	  }
	  break;
	  }*/

	for_each(m_memberOnline.begin(), m_memberOnline.end(), std::bind1st(std::mem_fun_ref(&CGuild::SendSkillInfoPacket),*this));

	sys_log(0, "Guild SkillUp: %s %d level %d type %u", GetName(), pkSk->dwVnum, m_data.abySkill[dwRealVnum], pkSk->dwType);
}

void CGuild::UseSkill(DWORD dwVnum, LPCHARACTER ch, DWORD pid)
{
	LPCHARACTER victim = NULL;

	if (!GetMember(ch->GetPlayerID()) || !HasGradeAuth(GetMember(ch->GetPlayerID())->grade, GUILD_AUTH_USE_SKILL))
		return;

	sys_log(0,"GUILD_USE_SKILL : cname(%s), skill(%d)", ch ? ch->GetName() : "", dwVnum);

	DWORD dwRealVnum = dwVnum - GUILD_SKILL_START;

	if (!ch->CanMove())
		return;

	if (dwRealVnum >= GUILD_SKILL_COUNT)
		return;

	CSkillProto* pkSk = CSkillManager::instance().Get(dwVnum);

	if (!pkSk)
	{
		sys_err("There is no such guild skill by number %u", dwVnum);
		return;
	}

	if (m_data.abySkill[dwRealVnum] == 0)
		return;

	if ((pkSk->dwFlag & SKILL_FLAG_SELFONLY))
	{
		// 이미 걸려 있으므로 사용하지 않음.
		if (ch->FindAffect(pkSk->dwVnum))
			return;

		victim = ch;
	}

	if (ch->IsAffectFlag(AFF_REVIVE_INVISIBLE))
		ch->RemoveAffect(AFFECT_REVIVE_INVISIBLE);

	if (ch->IsAffectFlag(AFF_EUNHYUNG))
		ch->RemoveAffect(SKILL_EUNHYUNG);

	double k =1.0*m_data.abySkill[dwRealVnum]/pkSk->bMaxLevel;
	pkSk->kSPCostPoly.SetVar("k", k);
	int iNeededSP = (int) pkSk->kSPCostPoly.Eval();

	if (GetSP() < iNeededSP)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 용신력이 부족합니다. (%d, %d)"), GetSP(), iNeededSP);
		return;
	}

	pkSk->kCooldownPoly.SetVar("k", k);
	int iCooltime = (int) pkSk->kCooldownPoly.Eval();

	if (!abSkillUsable[dwRealVnum])
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 쿨타임이 끝나지 않아 길드 스킬을 사용할 수 없습니다."));
		return;
	}

	{
		TPacketGuildUseSkill p;
		p.dwGuild = GetID();
		p.dwSkillVnum = pkSk->dwVnum;
		p.dwCooltime = iCooltime;
		db_clientdesc->DBPacket(HEADER_GD_GUILD_USE_SKILL, 0, &p, sizeof(p));
	}
	abSkillUsable[dwRealVnum] = false;
	//abSkillUsed[dwRealVnum] = true;
	//adwSkillNextUseTime[dwRealVnum] = get_dword_time() + iCooltime * 1000;

	//PointChange(POINT_SP, -iNeededSP);
	//GuildPointChange(POINT_SP, -iNeededSP);

	if (test_server)
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> %d 스킬을 사용함 (%d, %d) to %u"), dwVnum, GetSP(), iNeededSP, pid);

	switch (dwVnum)
	{
		case GUILD_SKILL_TELEPORT:
			// 현재 서버에 있는 사람을 먼저 시도.
			SendDBSkillUpdate(-iNeededSP);
			if ((victim = (CHARACTER_MANAGER::instance().FindByPID(pid))))
				ch->WarpSet(victim->GetX(), victim->GetY());
			else
			{
				if (m_memberP2POnline.find(pid) != m_memberP2POnline.end())
				{
					// 다른 서버에 로그인된 사람이 있음 -> 메시지 보내 좌표를 받아오자
					// 1. A.pid, B.pid 를 뿌림
					// 2. B.pid를 가진 서버가 뿌린서버에게 A.pid, 좌표 를 보냄
					// 3. 워프
					CCI * pcci = P2P_MANAGER::instance().FindByPID(pid);

					if (pcci->bChannel != g_bChannel)
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대가 %d 채널에 있습니다. (현재 채널 %d)"), pcci->bChannel, g_bChannel);
					}
					else
					{
						TPacketGGFindPosition p;
						p.header = HEADER_GG_FIND_POSITION;
						p.dwFromPID = ch->GetPlayerID();
						p.dwTargetPID = pid;
						pcci->pkDesc->Packet(&p, sizeof(TPacketGGFindPosition));
						if (test_server) ch->ChatPacket(CHAT_TYPE_PARTY, "sent find position packet for guild teleport");
					}
				}
				else
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대가 온라인 상태가 아닙니다."));
			}
			break;

			/*case GUILD_SKILL_ACCEL:
			  ch->RemoveAffect(dwVnum);
			  ch->AddAffect(dwVnum, POINT_MOV_SPEED, m_data.abySkill[dwRealVnum]*3, pkSk->dwAffectFlag, (int)pkSk->kDurationPoly.Eval(), 0, false);
			  ch->AddAffect(dwVnum, POINT_ATT_SPEED, m_data.abySkill[dwRealVnum]*3, pkSk->dwAffectFlag, (int)pkSk->kDurationPoly.Eval(), 0, false);
			  break;*/

		default:
			{
				/*if (ch->GetPlayerID() != GetMasterPID())
				  {
				  ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드장만 길드 스킬을 사용할 수 있습니다."));
				  return;
				  }*/

				if (!UnderAnyWar())
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드 스킬은 길드전 중에만 사용할 수 있습니다."));
					return;
				}

				SendDBSkillUpdate(-iNeededSP);

				for (itertype(m_memberOnline) it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
				{
					LPCHARACTER victim = *it;
					victim->RemoveAffect(dwVnum);
					ch->ComputeSkill(dwVnum, victim, m_data.abySkill[dwRealVnum]);
				}
			}
			break;
			/*if (!victim)
			  return;

			  ch->ComputeSkill(dwVnum, victim, m_data.abySkill[dwRealVnum]);*/
	}
}

void CGuild::SendSkillInfoPacket(LPCHARACTER ch) const
{
	LPDESC d = ch->GetDesc();

	if (!d)
		return;

	TPacketGCGuild pack;

	pack.header		= HEADER_GC_GUILD;
	pack.size		= sizeof(pack) + 6 + GUILD_SKILL_COUNT;
	pack.subheader	= GUILD_SUBHEADER_GC_SKILL_INFO;

	d->BufferedPacket(&pack, sizeof(pack));
	d->BufferedPacket(&m_data.skill_point,	1);
	d->BufferedPacket(&m_data.abySkill,		GUILD_SKILL_COUNT);
	d->BufferedPacket(&m_data.power,		2);
	d->Packet(&m_data.max_power,	2);
}

void CGuild::ComputeGuildPoints()
{
	m_data.max_power = GUILD_BASE_POWER + (m_data.level-1) * GUILD_POWER_PER_LEVEL;

	m_data.power = MINMAX(0, m_data.power, m_data.max_power);
}

int CGuild::GetSkillLevel(DWORD vnum)
{
	DWORD dwRealVnum = vnum - GUILD_SKILL_START;

	if (dwRealVnum >= GUILD_SKILL_COUNT)
		return 0;

	return m_data.abySkill[dwRealVnum];
}

/*void CGuild::GuildUpdateAffect(LPCHARACTER ch)
  {
  if (GetSkillLevel(GUILD_SKILL_GAHO))
  ch->PointChange(POINT_DEF_GRADE, GetSkillLevel(GUILD_SKILL_GAHO));

  if (GetSkillLevel(GUILD_SKILL_HIM))
  ch->PointChange(POINT_ATT_GRADE, GetSkillLevel(GUILD_SKILL_HIM));
  }*/

/*void CGuild::GuildRemoveAffect(LPCHARACTER ch)
  {
  if (GetSkillLevel(GUILD_SKILL_GAHO))
  ch->PointChange(POINT_DEF_GRADE, -(int) GetSkillLevel(GUILD_SKILL_GAHO));

  if (GetSkillLevel(GUILD_SKILL_HIM))
  ch->PointChange(POINT_ATT_GRADE, -(int) GetSkillLevel(GUILD_SKILL_HIM));
  }*/

void CGuild::UpdateSkill(BYTE skill_point, BYTE* skill_levels)
{
	//int iDefMoreBonus = 0;
	//int iAttMoreBonus = 0;

	m_data.skill_point = skill_point;
	/*if (skill_levels[GUILD_SKILL_GAHO - GUILD_SKILL_START]!=GetSkillLevel(GUILD_SKILL_GAHO))
	  {
	  iDefMoreBonus = skill_levels[GUILD_SKILL_GAHO - GUILD_SKILL_START]-GetSkillLevel(GUILD_SKILL_GAHO);
	  }
	  if (skill_levels[GUILD_SKILL_HIM - GUILD_SKILL_START]!=GetSkillLevel(GUILD_SKILL_HIM))
	  {
	  iAttMoreBonus = skill_levels[GUILD_SKILL_HIM  - GUILD_SKILL_START]-GetSkillLevel(GUILD_SKILL_HIM);
	  }

	  if (iDefMoreBonus || iAttMoreBonus)
	  {
	  for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	  {
	  (*it)->PointChange(POINT_ATT_GRADE, iAttMoreBonus);
	  (*it)->PointChange(POINT_DEF_GRADE, iDefMoreBonus);
	  }
	  }*/

	thecore_memcpy(m_data.abySkill, skill_levels, sizeof(BYTE) * GUILD_SKILL_COUNT);
	ComputeGuildPoints();
}

static DWORD __guild_levelup_exp(int level)
{
	if (LC_IsYMIR())
	{
		return guild_exp_table[level];
	}
	else
	{
		return guild_exp_table2[level];
	}
}

void CGuild::GuildPointChange(BYTE type, int amount, bool save)
{
	switch (type)
	{
		case POINT_SP:
			m_data.power += amount;

			m_data.power = MINMAX(0, m_data.power, m_data.max_power);

			if (save)
			{
				SaveSkill();
			}

			for_each(m_memberOnline.begin(), m_memberOnline.end(), std::bind1st(std::mem_fun_ref(&CGuild::SendSkillInfoPacket),*this));
			break;

		case POINT_EXP:
			if (amount < 0 && m_data.exp < (DWORD) - amount)
			{
				m_data.exp = 0;
			}
			else
			{
				m_data.exp += amount;

				while (m_data.exp >= __guild_levelup_exp(m_data.level))
				{

					if (m_data.level < GUILD_MAX_LEVEL)
					{
						m_data.exp -= __guild_levelup_exp(m_data.level);
						++m_data.level;
						++m_data.skill_point;

						if (m_data.level > GUILD_MAX_LEVEL)
							m_data.level = GUILD_MAX_LEVEL;

						ComputeGuildPoints();
						GuildPointChange(POINT_SP, m_data.max_power-m_data.power);

						if (save)
							ChangeLadderPoint(GUILD_LADDER_POINT_PER_LEVEL);

						// NOTIFY_GUILD_EXP_CHANGE
						for_each(m_memberOnline.begin(), m_memberOnline.end(), std::bind1st(std::mem_fun(&CGuild::SendGuildInfoPacket), this));
						// END_OF_NOTIFY_GUILD_EXP_CHANGE
					}

					if (m_data.level == GUILD_MAX_LEVEL)
					{
						m_data.exp = 0;
					}
				}
			}

			TPacketGCGuild pack;
			pack.header = HEADER_GC_GUILD;
			pack.size = sizeof(pack)+5;
			pack.subheader = GUILD_SUBHEADER_GC_CHANGE_EXP;

			TEMP_BUFFER buf;
			buf.write(&pack,sizeof(pack));
			buf.write(&m_data.level,1);
			buf.write(&m_data.exp,4);

			for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
			{
				LPDESC d = (*it)->GetDesc();

				if (d)
					d->Packet(buf.read_peek(), buf.size());
			}

			if (save)
				SaveLevel();

			break;
	}
}

void CGuild::SkillRecharge()
{
	//GuildPointChange(POINT_SP, m_data.max_power / 2);
	//GuildPointChange(POINT_SP, 10);
}

void CGuild::SaveMember(DWORD pid)
{
	TGuildMemberContainer::iterator it = m_member.find(pid);

	if (it == m_member.end())
		return;

	DBManager::instance().Query(
			"UPDATE guild_member%s SET grade = %d, offer = %u, is_general = %d WHERE pid = %u and guild_id = %u",
			get_table_postfix(), it->second.grade, it->second.offer_exp, it->second.is_general, pid, m_data.guild_id);
}

void CGuild::LevelChange(DWORD pid, int level)
{
	TGuildMemberContainer::iterator cit = m_member.find(pid);

	if (cit == m_member.end())
		return;

	cit->second.level = level;

	TPacketGuildChangeMemberData gd_guild;

	gd_guild.guild_id = GetID();
	gd_guild.pid = pid;
	gd_guild.offer = cit->second.offer_exp;
	gd_guild.grade = cit->second.grade;
	gd_guild.level = level;

	db_clientdesc->DBPacket(HEADER_GD_GUILD_CHANGE_MEMBER_DATA, 0, &gd_guild, sizeof(gd_guild));

	TPacketGCGuild pack;
	TGuildMemberPacketData mbData;
	pack.header = HEADER_GC_GUILD;
	pack.subheader = GUILD_SUBHEADER_GC_LIST;
	pack.size = sizeof(TPacketGCGuild);
	pack.size += sizeof(TGuildMemberPacketData);
	cit->second._dummy = 0;

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
		{
			TEMP_BUFFER buf;

			buf.write(&pack, sizeof(pack));

			mbData.byNameFlag = 0;
			mbData.byGrade = cit->second.grade;
			mbData.byIsGeneral = cit->second.is_general;
			mbData.byJob = cit->second.job;
			mbData.byLevel = cit->second.level;
			mbData.dwOffer = cit->second.offer_exp;
			mbData.pid = cit->second.pid;
			buf.write(&mbData, sizeof(TGuildMemberPacketData));

			d->Packet(buf.read_peek(), buf.size());
		}
	}
}

void CGuild::ChangeMemberData(DWORD pid, DWORD offer, int level, BYTE grade)
{
	TGuildMemberContainer::iterator cit = m_member.find(pid);

	if (cit == m_member.end())
		return;

	cit->second.offer_exp = offer;
	cit->second.level = level;
	cit->second.grade = grade;
	cit->second._dummy = 0;

	TPacketGCGuild pack;
	TGuildMemberPacketData mbData;
	pack.header = HEADER_GC_GUILD;
	pack.subheader = GUILD_SUBHEADER_GC_LIST;
	pack.size = sizeof(TPacketGCGuild);
	pack.size += sizeof(TGuildMemberPacketData);

	for (TGuildMemberOnlineContainer::iterator it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPDESC d = (*it)->GetDesc();
		if (d)
		{
			TEMP_BUFFER buf;

			buf.write(&pack, sizeof(pack));

			mbData.byNameFlag = 0;
			mbData.byGrade = cit->second.grade;
			mbData.byIsGeneral = cit->second.is_general;
			mbData.byJob = cit->second.job;
			mbData.byLevel = cit->second.level;
			mbData.dwOffer = cit->second.offer_exp;
			mbData.pid = cit->second.pid;
			buf.write(&mbData, sizeof(TGuildMemberPacketData));

			d->Packet(buf.read_peek(), buf.size());
		}
	}
}

namespace
{
	struct FGuildChat
	{
		const char* c_pszText;

		FGuildChat(const char* c_pszText)
			: c_pszText(c_pszText)
			{}

		void operator()(LPCHARACTER ch)
		{
			ch->ChatPacket(CHAT_TYPE_GUILD, "%s", c_pszText);
		}
	};
}

void CGuild::P2PChat(const char* c_pszText)
{
	std::for_each(m_memberOnline.begin(), m_memberOnline.end(), FGuildChat(c_pszText));
}

void CGuild::Chat(const char* c_pszText)
{
	std::for_each(m_memberOnline.begin(), m_memberOnline.end(), FGuildChat(c_pszText));

	TPacketGGGuild p1;
	TPacketGGGuildChat p2;

	p1.bHeader = HEADER_GG_GUILD;
	p1.bSubHeader = GUILD_SUBHEADER_GG_CHAT;
	p1.dwGuild = GetID();
	strlcpy(p2.szText, c_pszText, sizeof(p2.szText));

	P2P_MANAGER::instance().Send(&p1, sizeof(TPacketGGGuild));
	P2P_MANAGER::instance().Send(&p2, sizeof(TPacketGGGuildChat));
}

LPCHARACTER CGuild::GetMasterCharacter()
{ 
	return CHARACTER_MANAGER::instance().FindByPID(GetMasterPID()); 
}

void CGuild::Packet(const void* buf, int size)
{
	for (itertype(m_memberOnline) it = m_memberOnline.begin(); it!=m_memberOnline.end();++it)
	{
		LPDESC d = (*it)->GetDesc();

		if (d)
			d->Packet(buf, size);
	}
}

int CGuild::GetTotalLevel() const
{
	int total = 0;

	for (itertype(m_member) it = m_member.begin(); it != m_member.end(); ++it)
	{
		total += it->second.level;
	}

	return total;
}

bool CGuild::ChargeSP(LPCHARACTER ch, int iSP)
{
	int gold = iSP * 100;

	if (gold < iSP || ch->GetGold() < gold)
		return false;

	int iRemainSP = m_data.max_power - m_data.power;

	if (iSP > iRemainSP)
	{
		iSP = iRemainSP;
		gold = iSP * 100;
	}

	ch->PointChange(POINT_GOLD, -gold);
	DBManager::instance().SendMoneyLog(MONEY_LOG_GUILD, 1, -gold);

	SendDBSkillUpdate(iSP);
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> %u의 용신력을 회복하였습니다."), iSP);
	}
	return true;
}

void CGuild::SkillUsableChange(DWORD dwSkillVnum, bool bUsable)
{
	DWORD dwRealVnum = dwSkillVnum - GUILD_SKILL_START;

	if (dwRealVnum >= GUILD_SKILL_COUNT)
		return; 

	abSkillUsable[dwRealVnum] = bUsable;

	// GUILD_SKILL_COOLTIME_BUG_FIX
	sys_log(0, "CGuild::SkillUsableChange(guild=%s, skill=%d, usable=%d)", GetName(), dwSkillVnum, bUsable);
	// END_OF_GUILD_SKILL_COOLTIME_BUG_FIX
}

// GUILD_MEMBER_COUNT_BONUS
void CGuild::SetMemberCountBonus(int iBonus)
{
	m_iMemberCountBonus = iBonus;
	sys_log(0, "GUILD_IS_FULL_BUG : Bonus set to %d(val:%d)", iBonus, m_iMemberCountBonus);
}

void CGuild::BroadcastMemberCountBonus()
{
	TPacketGGGuild p1;

	p1.bHeader = HEADER_GG_GUILD;
	p1.bSubHeader = GUILD_SUBHEADER_GG_SET_MEMBER_COUNT_BONUS;
	p1.dwGuild = GetID();

	P2P_MANAGER::instance().Send(&p1, sizeof(TPacketGGGuild));
	P2P_MANAGER::instance().Send(&m_iMemberCountBonus, sizeof(int));
}

int CGuild::GetMaxMemberCount()
{
	// GUILD_IS_FULL_BUG_FIX
	if ( m_iMemberCountBonus < 0 || m_iMemberCountBonus > 18 )
		m_iMemberCountBonus = 0;
	// END_GUILD_IS_FULL_BUG_FIX

	if ( LC_IsHongKong() == true )
	{
		quest::PC* pPC = quest::CQuestManager::instance().GetPC(GetMasterPID());

		if ( pPC != NULL )
		{
			if ( pPC->GetFlag("guild.is_unlimit_member") == 1 )
			{
				return INT_MAX;
			}
		}
	}

	return 32 + 2 * (m_data.level-1) + m_iMemberCountBonus;
}
// END_OF_GUILD_MEMBER_COUNT_BONUS

void CGuild::AdvanceLevel(int iLevel)
{
	if (m_data.level == iLevel)
		return;

	m_data.level = MIN(GUILD_MAX_LEVEL, iLevel);
}

void CGuild::RequestDepositMoney(LPCHARACTER ch, int iGold)
{
	if (false==ch->CanDeposit())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 잠시후에 이용해주십시오"));
		return;
	}

	if (ch->GetGold() < iGold)
		return;


	ch->PointChange(POINT_GOLD, -iGold);

	TPacketGDGuildMoney p;
	p.dwGuild = GetID();
	p.iGold = iGold;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_DEPOSIT_MONEY, 0, &p, sizeof(p));

	char buf[64+1];
	snprintf(buf, sizeof(buf), "%u %s", GetID(), GetName());
	LogManager::instance().CharLog(ch, iGold, "GUILD_DEPOSIT", buf);

	ch->UpdateDepositPulse();
	sys_log(0, "GUILD: DEPOSIT %s:%u player %s[%u] gold %d", GetName(), GetID(), ch->GetName(), ch->GetPlayerID(), iGold);
}

void CGuild::RequestWithdrawMoney(LPCHARACTER ch, int iGold)
{
	if (false==ch->CanDeposit())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 잠시후에 이용해주십시오"));
		return;
	}

	if (ch->GetPlayerID() != GetMasterPID())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 길드 금고에선 길드장만 출금할 수 있습니다."));
		return;
	}

	if (m_data.gold < iGold)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 가지고 있는 돈이 부족합니다."));
		return;
	}

	TPacketGDGuildMoney p;
	p.dwGuild = GetID();
	p.iGold = iGold;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_WITHDRAW_MONEY, 0, &p, sizeof(p));

	ch->UpdateDepositPulse();
}

void CGuild::RecvMoneyChange(int iGold)
{
	m_data.gold = iGold;

	TPacketGCGuild p;
	p.header = HEADER_GC_GUILD;
	p.size = sizeof(p) + sizeof(int);
	p.subheader = GUILD_SUBHEADER_GC_MONEY_CHANGE;

	for (itertype(m_memberOnline) it = m_memberOnline.begin(); it != m_memberOnline.end(); ++it)
	{
		LPCHARACTER ch = *it;
		LPDESC d = ch->GetDesc();
		d->BufferedPacket(&p, sizeof(p));
		d->Packet(&iGold, sizeof(int));
	}
}

void CGuild::RecvWithdrawMoneyGive(int iChangeGold)
{
	LPCHARACTER ch = GetMasterCharacter();

	if (ch)
	{
		ch->PointChange(POINT_GOLD, iChangeGold);
		sys_log(0, "GUILD: WITHDRAW %s:%u player %s[%u] gold %d", GetName(), GetID(), ch->GetName(), ch->GetPlayerID(), iChangeGold);
	}

	TPacketGDGuildMoneyWithdrawGiveReply p;
	p.dwGuild = GetID();
	p.iChangeGold = iChangeGold;
	p.bGiveSuccess = ch ? 1 : 0;
	db_clientdesc->DBPacket(HEADER_GD_GUILD_WITHDRAW_MONEY_GIVE_REPLY, 0, &p, sizeof(p));
}

bool CGuild::HasLand()
{
	return building::CManager::instance().FindLandByGuild(GetID()) != NULL;
}

// GUILD_JOIN_BUG_FIX
/// 길드 초대 event 정보
EVENTINFO(TInviteGuildEventInfo)
{
	DWORD	dwInviteePID;		///< 초대받은 character 의 PID
	DWORD	dwGuildID;		///< 초대한 Guild 의 ID

	TInviteGuildEventInfo()
	: dwInviteePID( 0 )
	, dwGuildID( 0 )
	{
	}
};

/**
 * 길드 초대 event callback 함수.
 * event 가 발동하면 초대 거절로 처리한다.
 */
EVENTFUNC( GuildInviteEvent )
{
	TInviteGuildEventInfo *pInfo = dynamic_cast<TInviteGuildEventInfo*>( event->info );

	if ( pInfo == NULL )
	{
		sys_err( "GuildInviteEvent> <Factor> Null pointer" );
		return 0;
	}

	CGuild* pGuild = CGuildManager::instance().FindGuild( pInfo->dwGuildID );

	if ( pGuild ) 
	{
		sys_log( 0, "GuildInviteEvent %s", pGuild->GetName() );
		pGuild->InviteDeny( pInfo->dwInviteePID );
	}

	return 0;
}

void CGuild::Invite( LPCHARACTER pchInviter, LPCHARACTER pchInvitee )
{
	if (quest::CQuestManager::instance().GetPCForce(pchInviter->GetPlayerID())->IsRunning() == true)
	{
	    pchInviter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방이 초대 신청을 받을 수 없는 상태입니다."));
	    return;
	}

	
	if (quest::CQuestManager::instance().GetPCForce(pchInvitee->GetPlayerID())->IsRunning() == true)
		return;

	if ( pchInvitee->IsBlockMode( BLOCK_GUILD_INVITE ) ) 
	{
		pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방이 길드 초대 거부 상태입니다.") );
		return;
	} 
	else if ( !HasGradeAuth( GetMember( pchInviter->GetPlayerID() )->grade, GUILD_AUTH_ADD_MEMBER ) ) 
	{
		pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("<길드> 길드원을 초대할 권한이 없습니다.") );
		return;
	} 
	else if ( pchInvitee->GetEmpire() != pchInviter->GetEmpire() ) 
	{
		pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("<길드> 다른 제국 사람을 길드에 초대할 수 없습니다.") );
		return;
	}

	GuildJoinErrCode errcode = VerifyGuildJoinableCondition( pchInvitee );
	switch ( errcode ) 
	{
		case GERR_NONE: break;
		case GERR_WITHDRAWPENALTY:
						pchInviter->ChatPacket( CHAT_TYPE_INFO, 
								LC_TEXT("<길드> 탈퇴한 후 %d일이 지나지 않은 사람은 길드에 초대할 수 없습니다."), 
								quest::CQuestManager::instance().GetEventFlag( "guild_withdraw_delay" ) );
						return;
		case GERR_COMMISSIONPENALTY:
						pchInviter->ChatPacket( CHAT_TYPE_INFO, 
								LC_TEXT("<길드> 길드를 해산한 지 %d일이 지나지 않은 사람은 길드에 초대할 수 없습니다."), 
								quest::CQuestManager::instance().GetEventFlag( "guild_disband_delay") );
						return;
		case GERR_ALREADYJOIN:	pchInviter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방이 이미 다른 길드에 속해있습니다.")); return;
		case GERR_GUILDISFULL:	pchInviter->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 최대 길드원 수를 초과했습니다.")); return;
		case GERR_GUILD_IS_IN_WAR : pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("<길드> 현재 길드가 전쟁 중 입니다.") ); return;
		case GERR_INVITE_LIMIT : pchInviter->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("<길드> 현재 신규 가입 제한 상태 입니다.") ); return;

		default: sys_err( "ignore guild join error(%d)", errcode ); return;
	}

	if ( m_GuildInviteEventMap.end() != m_GuildInviteEventMap.find( pchInvitee->GetPlayerID() ) )
		return;

	//
	// 이벤트 생성
	// 
	TInviteGuildEventInfo* pInfo = AllocEventInfo<TInviteGuildEventInfo>();
	pInfo->dwInviteePID = pchInvitee->GetPlayerID();
	pInfo->dwGuildID = GetID();

	m_GuildInviteEventMap.insert(EventMap::value_type(pchInvitee->GetPlayerID(), event_create(GuildInviteEvent, pInfo, PASSES_PER_SEC(10))));

	//
	// 초대 받는 character 에게 초대 패킷 전송
	// 

	DWORD gid = GetID();

	TPacketGCGuild p;
	p.header	= HEADER_GC_GUILD;
	p.size	= sizeof(p) + sizeof(DWORD) + GUILD_NAME_MAX_LEN + 1;
	p.subheader	= GUILD_SUBHEADER_GC_GUILD_INVITE;

	TEMP_BUFFER buf;
	buf.write( &p, sizeof(p) );
	buf.write( &gid, sizeof(DWORD) );
	buf.write( GetName(), GUILD_NAME_MAX_LEN + 1 );

	pchInvitee->GetDesc()->Packet( buf.read_peek(), buf.size() );
}

void CGuild::InviteAccept( LPCHARACTER pchInvitee )
{
	EventMap::iterator itFind = m_GuildInviteEventMap.find( pchInvitee->GetPlayerID() );
	if ( itFind == m_GuildInviteEventMap.end() ) 
	{
		sys_log( 0, "GuildInviteAccept from not invited character(invite guild: %s, invitee: %s)", GetName(), pchInvitee->GetName() );
		return;
	}

	event_cancel( &itFind->second );
	m_GuildInviteEventMap.erase( itFind );

	GuildJoinErrCode errcode = VerifyGuildJoinableCondition( pchInvitee );
	switch ( errcode ) 
	{
		case GERR_NONE: break;
		case GERR_WITHDRAWPENALTY:
						pchInvitee->ChatPacket( CHAT_TYPE_INFO, 
								LC_TEXT("<길드> 탈퇴한 후 %d일이 지나지 않은 사람은 길드에 초대할 수 없습니다."), 
								quest::CQuestManager::instance().GetEventFlag( "guild_withdraw_delay" ) );
						return;
		case GERR_COMMISSIONPENALTY:
						pchInvitee->ChatPacket( CHAT_TYPE_INFO, 
								LC_TEXT("<길드> 길드를 해산한 지 %d일이 지나지 않은 사람은 길드에 초대할 수 없습니다."), 
								quest::CQuestManager::instance().GetEventFlag( "guild_disband_delay") );
						return;
		case GERR_ALREADYJOIN:	pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 상대방이 이미 다른 길드에 속해있습니다.")); return;
		case GERR_GUILDISFULL:	pchInvitee->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("<길드> 최대 길드원 수를 초과했습니다.")); return;
		case GERR_GUILD_IS_IN_WAR : pchInvitee->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("<길드> 현재 길드가 전쟁 중 입니다.") ); return;
		case GERR_INVITE_LIMIT : pchInvitee->ChatPacket( CHAT_TYPE_INFO, LC_TEXT("<길드> 현재 신규 가입 제한 상태 입니다.") ); return;

		default: sys_err( "ignore guild join error(%d)", errcode ); return;
	}

	RequestAddMember( pchInvitee, 15 );
}

void CGuild::InviteDeny( DWORD dwPID )
{
	EventMap::iterator itFind = m_GuildInviteEventMap.find( dwPID );
	if ( itFind == m_GuildInviteEventMap.end() ) 
	{
		sys_log( 0, "GuildInviteDeny from not invited character(invite guild: %s, invitee PID: %d)", GetName(), dwPID );
		return;
	}

	event_cancel( &itFind->second );
	m_GuildInviteEventMap.erase( itFind );
}

CGuild::GuildJoinErrCode CGuild::VerifyGuildJoinableCondition( const LPCHARACTER pchInvitee )
{
	if ( get_global_time() - pchInvitee->GetQuestFlag( "guild_manage.new_withdraw_time" )
			< CGuildManager::instance().GetWithdrawDelay() )
		return GERR_WITHDRAWPENALTY;
	else if ( get_global_time() - pchInvitee->GetQuestFlag( "guild_manage.new_disband_time" )
			< CGuildManager::instance().GetDisbandDelay() )
		return GERR_COMMISSIONPENALTY;
	else if ( pchInvitee->GetGuild() )
		return GERR_ALREADYJOIN;
	else if ( GetMemberCount() >= GetMaxMemberCount() )
	{
		sys_log(1, "GuildName = %s, GetMemberCount() = %d, GetMaxMemberCount() = %d (32 + MAX(level(%d)-10, 0) * 2 + bonus(%d)", 
				GetName(), GetMemberCount(), GetMaxMemberCount(), m_data.level, m_iMemberCountBonus);
		return GERR_GUILDISFULL;
	}
	else if ( UnderAnyWar() != 0 )
	{
		return GERR_GUILD_IS_IN_WAR;
	}
	else if ( LC_IsBrazil() == true )
	{
		std::auto_ptr<SQLMsg> pMsg( DBManager::instance().DirectQuery("SELECT value FROM guild_invite_limit WHERE id=%d", GetID()) );

		if ( pMsg->Get()->uiNumRows > 0 )
		{
			MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			time_t limit_time=0;
			str_to_number( limit_time, row[0] );

			if ( test_server == true )
			{
				limit_time += quest::CQuestManager::instance().GetEventFlag("guild_invite_limit") * 60;
			}
			else
			{
				limit_time += quest::CQuestManager::instance().GetEventFlag("guild_invite_limit") * 24 * 60 * 60;
			}

			if ( get_global_time() < limit_time ) return GERR_INVITE_LIMIT;
		}
	}

	return GERR_NONE;
}
// END_OF_GUILD_JOIN_BUG_FIX

bool CGuild::ChangeMasterTo(DWORD dwPID)
{
	if ( GetMember(dwPID) == NULL ) return false;

	TPacketChangeGuildMaster p;
	p.dwGuildID = GetID();
	p.idFrom = GetMasterPID();
	p.idTo = dwPID;

	db_clientdesc->DBPacket(HEADER_GD_REQ_CHANGE_GUILD_MASTER, 0, &p, sizeof(p));

	return true;
}

void CGuild::SendGuildDataUpdateToAllMember(SQLMsg* pmsg)
{
	TGuildMemberOnlineContainer::iterator iter = m_memberOnline.begin();

	for (; iter != m_memberOnline.end(); iter++ )
	{
		SendGuildInfoPacket(*iter);
		SendAllGradePacket(*iter);
	}
}

