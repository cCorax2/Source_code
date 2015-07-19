#include "stdafx.h"
#include "../../common/stl.h"

#include "constants.h"
#include "skill.h"
#include "char.h"

void CSkillProto::SetPointVar(const std::string& strName, double dVar)
{
	kPointPoly.SetVar(strName, dVar);
	kPointPoly2.SetVar(strName, dVar);
	kPointPoly3.SetVar(strName, dVar);
	kMasterBonusPoly.SetVar(strName, dVar);
}

void CSkillProto::SetDurationVar(const std::string& strName, double dVar)
{
	kDurationPoly.SetVar(strName, dVar);
	kDurationPoly2.SetVar(strName, dVar);
	kDurationPoly3.SetVar(strName, dVar);
}

void CSkillProto::SetSPCostVar(const std::string& strName, double dVar)
{
	kSPCostPoly.SetVar(strName, dVar);
	kGrandMasterAddSPCostPoly.SetVar(strName, dVar);
}

CSkillManager::CSkillManager()
{
}

CSkillManager::~CSkillManager()
{
	itertype(m_map_pkSkillProto) it = m_map_pkSkillProto.begin();
	for ( ; it != m_map_pkSkillProto.end(); ++it) {
		M2_DELETE(it->second);
	}
}

struct SPointOnType
{
	const char * c_pszName;
	int		 iPointOn;
} kPointOnTypes[] = {
	{ "NONE",		POINT_NONE		},
	{ "MAX_HP",		POINT_MAX_HP		},
	{ "MAX_SP",		POINT_MAX_SP		},
	{ "HP_REGEN",	POINT_HP_REGEN		},
	{ "SP_REGEN",	POINT_SP_REGEN		},
	{ "BLOCK",		POINT_BLOCK		},
	{ "HP",		POINT_HP		},
	{ "SP",		POINT_SP		},
	{ "ATT_GRADE",	POINT_ATT_GRADE_BONUS	},
	{ "DEF_GRADE",	POINT_DEF_GRADE_BONUS	},
	{ "MAGIC_ATT_GRADE",POINT_MAGIC_ATT_GRADE_BONUS	},
	{ "MAGIC_DEF_GRADE",POINT_MAGIC_DEF_GRADE_BONUS	},
	{ "BOW_DISTANCE",	POINT_BOW_DISTANCE	},
	{ "MOV_SPEED",	POINT_MOV_SPEED		},
	{ "ATT_SPEED",	POINT_ATT_SPEED		},
	{ "POISON_PCT",	POINT_POISON_PCT	},
	{ "RESIST_RANGE",   POINT_RESIST_BOW	},
	//{ "RESIST_MELEE",	POINT_RESIST_MELEE	},
	{ "CASTING_SPEED",	POINT_CASTING_SPEED	},
	{ "REFLECT_MELEE",	POINT_REFLECT_MELEE	},
	{ "ATT_BONUS",	POINT_ATT_BONUS		},
	{ "DEF_BONUS",	POINT_DEF_BONUS		},
	{ "RESIST_NORMAL",	POINT_RESIST_NORMAL_DAMAGE },
	{ "DODGE",		POINT_DODGE		},
	{ "KILL_HP_RECOVER",POINT_KILL_HP_RECOVERY	},
	{ "KILL_SP_RECOVER",POINT_KILL_SP_RECOVER	},
	{ "HIT_HP_RECOVER",	POINT_HIT_HP_RECOVERY	},
	{ "HIT_SP_RECOVER",	POINT_HIT_SP_RECOVERY	},
	{ "CRITICAL",	POINT_CRITICAL_PCT	},
	{ "MANASHIELD",	POINT_MANASHIELD	},
	{ "SKILL_DAMAGE_BONUS", POINT_SKILL_DAMAGE_BONUS	},
	{ "NORMAL_HIT_DAMAGE_BONUS", POINT_NORMAL_HIT_DAMAGE_BONUS	},
	{ "\n",		POINT_NONE		},
};

int FindPointType(const char * c_sz)
{
	for (int i = 0; *kPointOnTypes[i].c_pszName != '\n'; ++i)
	{
		if (!strcasecmp(c_sz, kPointOnTypes[i].c_pszName))
			return kPointOnTypes[i].iPointOn;
	}
	return -1;
}

bool CSkillManager::Initialize(TSkillTable * pTab, int iSize)
{
	char buf[1024];
	std::map<DWORD, CSkillProto *> map_pkSkillProto;

	TSkillTable * t = pTab;
	bool bError = false;

	for (int i = 0; i < iSize; ++i, ++t)
	{
		CSkillProto * pkProto = M2_NEW CSkillProto;

		pkProto->dwVnum = t->dwVnum;
		strlcpy(pkProto->szName, t->szName, sizeof(pkProto->szName));
		pkProto->dwType = t->bType;
		pkProto->bMaxLevel = t->bMaxLevel;
		pkProto->dwFlag = t->dwFlag;
		pkProto->dwAffectFlag = t->dwAffectFlag;
		pkProto->dwAffectFlag2 = t->dwAffectFlag2;

		pkProto->bLevelStep = t->bLevelStep;
		pkProto->bLevelLimit = t->bLevelLimit;
		pkProto->iSplashRange = t->dwSplashRange;
		pkProto->dwTargetRange = t->dwTargetRange;
		pkProto->preSkillVnum = t->preSkillVnum;
		pkProto->preSkillLevel = t->preSkillLevel;

		pkProto->lMaxHit = t->lMaxHit;

		pkProto->bSkillAttrType = t->bSkillAttrType;

		int iIdx = FindPointType(t->szPointOn);

		if (iIdx < 0)
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn: %s", t->szName, t->szPointOn);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		pkProto->bPointOn = iIdx;

		int iIdx2 = FindPointType(t->szPointOn2);

		if (iIdx2 < 0)
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn2: %s", t->szName, t->szPointOn2);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		pkProto->bPointOn2 = iIdx2;

		int iIdx3 = FindPointType(t->szPointOn3);

		if (iIdx3 < 0)
		{
			if (t->szPointOn3[0] == 0)
			{
				iIdx3 = POINT_NONE;
			}
			else
			{
				snprintf(buf, sizeof(buf), "SkillManager::Initialize : cannot find point type on skill: %s szPointOn3: %s", t->szName, t->szPointOn3);
				sys_err("%s", buf);
				SendLog(buf);
				bError = true;
				M2_DELETE(pkProto);
				continue;
			}
		}

		pkProto->bPointOn3 = iIdx3;

		if (!pkProto->kSplashAroundDamageAdjustPoly.Analyze(t->szSplashAroundDamageAdjustPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szSplashAroundDamageAdjustPoly: %s", t->szName, t->szSplashAroundDamageAdjustPoly);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kPointPoly.Analyze(t->szPointPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly: %s", t->szName, t->szPointPoly);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kPointPoly2.Analyze(t->szPointPoly2))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly2: %s", t->szName, t->szPointPoly2);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kPointPoly3.Analyze(t->szPointPoly3))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szPointPoly3: %s", t->szName, t->szPointPoly3);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kSPCostPoly.Analyze(t->szSPCostPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szSPCostPoly: %s", t->szName, t->szSPCostPoly);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kGrandMasterAddSPCostPoly.Analyze(t->szGrandMasterAddSPCostPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szGrandMasterAddSPCostPoly: %s", t->szName, t->szGrandMasterAddSPCostPoly);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kDurationPoly.Analyze(t->szDurationPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly: %s", t->szName, t->szDurationPoly);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kDurationPoly2.Analyze(t->szDurationPoly2))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly2: %s", t->szName, t->szDurationPoly2);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kDurationPoly3.Analyze(t->szDurationPoly3))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationPoly3: %s", t->szName, t->szDurationPoly3);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kDurationSPCostPoly.Analyze(t->szDurationSPCostPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szDurationSPCostPoly: %s", t->szName, t->szDurationSPCostPoly);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		if (!pkProto->kCooldownPoly.Analyze(t->szCooldownPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szCooldownPoly: %s", t->szName, t->szCooldownPoly);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		sys_log(0, "Master %s", t->szMasterBonusPoly);
		if (!pkProto->kMasterBonusPoly.Analyze(t->szMasterBonusPoly))
		{
			snprintf(buf, sizeof(buf), "SkillManager::Initialize : syntax error skill: %s szMasterBonusPoly: %s", t->szName, t->szMasterBonusPoly);
			sys_err("%s", buf);
			SendLog(buf);
			bError = true;
			M2_DELETE(pkProto);
			continue;
		}

		sys_log(0, "#%-3d %-24s type %u flag %u affect %u point_poly: %s", 
				pkProto->dwVnum, pkProto->szName, pkProto->dwType, pkProto->dwFlag, pkProto->dwAffectFlag, t->szPointPoly);

		map_pkSkillProto.insert(std::map<DWORD, CSkillProto *>::value_type(pkProto->dwVnum, pkProto));
	}

	if (!bError)
	{
		// 기존 테이블의 내용을 지운다.
		itertype(m_map_pkSkillProto) it = m_map_pkSkillProto.begin();

		while (it != m_map_pkSkillProto.end()) {
			M2_DELETE(it->second);
			++it;
		}

		m_map_pkSkillProto.clear();

		// 새로운 내용을 삽입
		it = map_pkSkillProto.begin();

		while (it != map_pkSkillProto.end())
		{
			m_map_pkSkillProto.insert(std::map<DWORD, CSkillProto *>::value_type(it->first, it->second));
			++it;
		}

		SendLog("Skill Prototype reloaded!");
	}
	else
		SendLog("There were erros when loading skill table");

	return !bError;
}

CSkillProto * CSkillManager::Get(DWORD dwVnum)
{
	std::map<DWORD, CSkillProto *>::iterator it = m_map_pkSkillProto.find(dwVnum);

	if (it == m_map_pkSkillProto.end())
		return NULL;

	return it->second;
}

CSkillProto * CSkillManager::Get(const char * c_pszSkillName)
{
	std::map<DWORD, CSkillProto *>::iterator it = m_map_pkSkillProto.begin();

	while (it != m_map_pkSkillProto.end())
	{
		if (!strcasecmp(it->second->szName, c_pszSkillName))
			return it->second;

		it++;
	}

	return NULL;
}

