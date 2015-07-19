#include "stdafx.h"
#include "char.h"
#include "char_manager.h"
#include "item_manager.h"
#include "mob_manager.h"
#include "regen.h"
#include "sectree.h"
#include "text_file_loader.h"
#include "questmanager.h"
#include "locale_service.h"

CMob::CMob()
{
	memset( &m_table, 0, sizeof(m_table) );

	for( size_t i=0 ; i < MOB_SKILL_MAX_NUM ; ++i )
	{
		m_mobSkillInfo[i].dwSkillVnum = 0;
		m_mobSkillInfo[i].bSkillLevel = 0;
		m_mobSkillInfo[i].vecSplashAttack.clear();
	}
}

CMob::~CMob()
{
}

void CMob::AddSkillSplash(int iIndex, DWORD dwTiming, DWORD dwHitDistance)
{
	if (iIndex >= MOB_SKILL_MAX_NUM || iIndex < 0)
		return;

	sys_log(0, "MOB_SPLASH %s idx %d timing %u hit_distance %u",
			m_table.szLocaleName, iIndex, dwTiming, dwHitDistance);

	m_mobSkillInfo[iIndex].vecSplashAttack.push_back(TMobSplashAttackInfo(dwTiming, dwHitDistance));
}

CMobInstance::CMobInstance()
	: m_IsBerserk(false), m_IsGodSpeed(false), m_IsRevive(false)
{
	m_dwLastAttackedTime = get_dword_time();
	m_dwLastWarpTime = get_dword_time();

	memset( &m_posLastAttacked, 0, sizeof(m_posLastAttacked) );
}

CMobManager::CMobManager()
{
}

CMobManager::~CMobManager()
{
}

bool CMobManager::Initialize(TMobTable * pTable, int iSize)
{
	m_map_pkMobByVnum.clear();
	m_map_pkMobByName.clear();

	TMobTable * t = pTable;


	for (int i = 0; i < iSize; ++i, ++t)
	{
		CMob * pkMob = M2_NEW CMob;

		thecore_memcpy(&pkMob->m_table, t, sizeof(TMobTable));

		m_map_pkMobByVnum.insert(std::map<DWORD, CMob *>::value_type(t->dwVnum, pkMob));
		m_map_pkMobByName.insert(std::map<std::string, CMob *>::value_type(t->szLocaleName, pkMob));

		int SkillCount = 0;

		for (int j = 0; j < MOB_SKILL_MAX_NUM; ++j)
			if (pkMob->m_table.Skills[j].dwVnum)
				++SkillCount;

		sys_log(0, "MOB: #%-5d %-30s LEVEL %u HP %u DEF %u EXP %u DROP_ITEM_VNUM %u SKILL_COUNT %d", 
				t->dwVnum, t->szLocaleName, t->bLevel, t->dwMaxHP, t->wDef, t->dwExp, t->dwDropItemVnum, SkillCount);

		if (t->bType == CHAR_TYPE_NPC || t->bType == CHAR_TYPE_WARP || t->bType == CHAR_TYPE_GOTO)
			CHARACTER_MANAGER::instance().RegisterRaceNum(t->dwVnum);

		quest::CQuestManager::instance().RegisterNPCVnum(t->dwVnum);
	}


	// LOCALE_SERVICE
	const int FILE_NAME_LEN = 256;
	char szGroupFileName[FILE_NAME_LEN];
	char szGroupGroupFileName[FILE_NAME_LEN];

	snprintf(szGroupFileName, sizeof(szGroupGroupFileName),
			"%s/group.txt", LocaleService_GetBasePath().c_str());
	snprintf(szGroupGroupFileName, sizeof(szGroupGroupFileName),
			"%s/group_group.txt", LocaleService_GetBasePath().c_str());
	
	if (!LoadGroup(szGroupFileName))
	{
		sys_err("cannot load %s", szGroupFileName);
		thecore_shutdown();
	}
	if (!LoadGroupGroup(szGroupGroupFileName))
	{
		sys_err("cannot load %s", szGroupGroupFileName);
		thecore_shutdown();
	}
	// END_OF_LOCALE_SERVICE

	//exit(1);
	CHARACTER_MANAGER::instance().for_each_pc(std::bind1st(std::mem_fun(&CMobManager::RebindMobProto),this));
	return true;
}

void CMobManager::RebindMobProto(LPCHARACTER ch)
{
	if (ch->IsPC())
		return;

	const CMob * pMob = Get(ch->GetRaceNum());

	if (pMob)
		ch->SetProto(pMob);
}

const CMob * CMobManager::Get(DWORD dwVnum)
{
	std::map<DWORD, CMob *>::iterator it = m_map_pkMobByVnum.find(dwVnum);

	if (it == m_map_pkMobByVnum.end())
		return NULL;

	return it->second;
}

const CMob * CMobManager::Get(const char * c_pszName, bool bIsAbbrev)
{
	std::map<std::string, CMob *>::iterator it;

	if (!bIsAbbrev)
	{
		it = m_map_pkMobByName.find(c_pszName);

		if (it == m_map_pkMobByName.end())
			return NULL;

		return it->second;
	}

	int len = strlen(c_pszName);
	it = m_map_pkMobByName.begin();

	while (it != m_map_pkMobByName.end())
	{
		if (!strncmp(it->first.c_str(), c_pszName, len))
			return it->second;

		++it;
	}

	return NULL;
}

void CMobManager::IncRegenCount(BYTE bRegenType, DWORD dwVnum, int iCount, int iTime)
{
	switch (bRegenType)
	{
		case REGEN_TYPE_MOB:
			m_mapRegenCount[dwVnum] += iCount * 86400. / iTime;
			break;

		case REGEN_TYPE_GROUP:
			{
				CMobGroup * pkGroup = CMobManager::Instance().GetGroup(dwVnum);
				if (!pkGroup)
					return;
				const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

				for (DWORD i=0; i<c_rdwMembers.size(); i++)
					m_mapRegenCount[c_rdwMembers[i]] += iCount * 86400. / iTime;
			}
			break;

		case REGEN_TYPE_GROUP_GROUP:
			{
				std::map<DWORD, CMobGroupGroup *>::iterator it = m_map_pkMobGroupGroup.find(dwVnum);

				if (it == m_map_pkMobGroupGroup.end())
					return;

				std::vector<DWORD>& v = it->second->m_vec_dwMemberVnum;
				for (DWORD i=0; i<v.size(); i++)
				{
					//m_mapRegenCount[v[i]] += iCount * 86400. / iTime / v.size();
					CMobGroup * pkGroup = CMobManager::Instance().GetGroup(v[i]);
					if (!pkGroup)
						return;
					const std::vector<DWORD> & c_rdwMembers = pkGroup->GetMemberVector();

					for (DWORD i=0; i<c_rdwMembers.size(); i++)
						m_mapRegenCount[c_rdwMembers[i]] += iCount * 86400. / iTime / v.size();
				}
			}
			break;
	}
}

void CMobManager::DumpRegenCount(const char* c_szFilename)
{
	FILE* fp = fopen(c_szFilename, "w");

	if (fp)
	{
		std::map<DWORD, double>::iterator it;

		fprintf(fp,"MOB_VNUM\tCOUNT\n");

		for (it = m_mapRegenCount.begin(); it != m_mapRegenCount.end(); ++it)
		{
			fprintf(fp,"%u\t%g\n", it->first, it->second);
		}

		fclose(fp);
	}
}

DWORD CMobManager::GetGroupFromGroupGroup(DWORD dwVnum)
{
	std::map<DWORD, CMobGroupGroup *>::iterator it = m_map_pkMobGroupGroup.find(dwVnum);

	if (it == m_map_pkMobGroupGroup.end())
		return 0;

	return it->second->GetMember();
}

CMobGroup * CMobManager::GetGroup(DWORD dwVnum)
{
	std::map<DWORD, CMobGroup *>::iterator it = m_map_pkMobGroup.find(dwVnum);

	if (it == m_map_pkMobGroup.end())
		return NULL;

	return it->second;
}

bool CMobManager::LoadGroupGroup(const char * c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			sys_err("LoadGroupGroup : Syntax error %s : no vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		TTokenVector * pTok;

		CMobGroupGroup * pkGroup = M2_NEW CMobGroupGroup(iVnum);

		for (int k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				DWORD dwMobVnum = 0;
				str_to_number(dwMobVnum, pTok->at(0).c_str());

				// ADD_MOB_GROUP_GROUP_PROB
				int prob = 1;
				if (pTok->size() > 1)
					str_to_number(prob, pTok->at(1).c_str());
				// END_OF_ADD_MOB_GROUP_GROUP_PROB

				if (dwMobVnum)
					pkGroup->AddMember(dwMobVnum);

				continue;
			}

			break;
		}

		loader.SetParentNode();

		m_map_pkMobGroupGroup.insert(std::make_pair((DWORD)iVnum, pkGroup));
	}

	return true;
}

bool CMobManager::LoadGroup(const char * c_pszFileName)
{
	CTextFileLoader loader;

	if (!loader.Load(c_pszFileName))
		return false;

	std::string stName;

	for (DWORD i = 0; i < loader.GetChildNodeCount(); ++i)
	{
		loader.SetChildNode(i);

		loader.GetCurrentNodeName(&stName);

		int iVnum;

		if (!loader.GetTokenInteger("vnum", &iVnum))
		{
			sys_err("LoadGroup : Syntax error %s : no vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		TTokenVector * pTok;

		if (!loader.GetTokenVector("leader", &pTok))
		{
			sys_err("LoadGroup : Syntax error %s : no leader, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		if (pTok->size() < 2)
		{
			sys_err("LoadGroup : Syntax error %s : no leader vnum, node %s", c_pszFileName, stName.c_str());
			loader.SetParentNode();
			continue;
		}

		CMobGroup * pkGroup = M2_NEW CMobGroup;

		pkGroup->Create(iVnum, stName);
		DWORD vnum = 0;
		str_to_number(vnum, pTok->at(1).c_str());
		pkGroup->AddMember(vnum);

		sys_log(0, "GROUP: %-5d %s", iVnum, stName.c_str());
		sys_log(0, "               %s %s", pTok->at(0).c_str(), pTok->at(1).c_str());

		for (int k = 1; k < 256; ++k)
		{
			char buf[4];
			snprintf(buf, sizeof(buf), "%d", k);

			if (loader.GetTokenVector(buf, &pTok))
			{
				sys_log(0, "               %s %s", pTok->at(0).c_str(), pTok->at(1).c_str());
				DWORD vnum = 0;
				str_to_number(vnum, pTok->at(1).c_str());
				pkGroup->AddMember(vnum);
				continue;
			}

			break;
		}

		loader.SetParentNode();
		m_map_pkMobGroup.insert(std::map<DWORD, CMobGroup *>::value_type(iVnum, pkGroup));
	}

	return true;
}

