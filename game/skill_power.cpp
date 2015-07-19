
#include "stdafx.h"

#include "../../common/length.h"

#include "skill_power.h"

bool CTableBySkill::Check() const
{
	for (int job = 0; job < (JOB_MAX_NUM * 2); ++job)
	{
		if (!m_aiSkillPowerByLevelFromType[job])
		{
			fprintf( stderr, "[NO SETTING SKILL] aiSkillPowerByLevelFromType[%d]", job);
			return false;
		}
	}

	return true;
}

void CTableBySkill::DeleteAll()
{
	for ( int job = 0; job < JOB_MAX_NUM * 2; ++job )
	{
		DeleteSkillPowerByLevelFromType( job );
	}

	DeleteSkillDamageByLevelTable();
}

int CTableBySkill::GetSkillPowerByLevelFromType(int job, int skillgroup, int skilllevel, bool bMob) const
{
	if (bMob)
	{
		return m_aiSkillPowerByLevelFromType[0][skilllevel];
	}

	if (job >= JOB_MAX_NUM || skillgroup == 0)
		return 0;

	int idx = (job * 2) + (skillgroup - 1);

	return m_aiSkillPowerByLevelFromType[idx][skilllevel];
}

void CTableBySkill::SetSkillPowerByLevelFromType(int idx, const int* aTable)
{
	DeleteSkillPowerByLevelFromType(idx);

	int* aiSkillTable = M2_NEW int[SKILL_MAX_LEVEL+1];

	memcpy (aiSkillTable, aTable, sizeof(int) * (SKILL_MAX_LEVEL + 1));
	m_aiSkillPowerByLevelFromType[idx] = aiSkillTable;
}

void CTableBySkill::DeleteSkillPowerByLevelFromType(int idx)
{
	if (NULL != m_aiSkillPowerByLevelFromType[idx])
	{
		M2_DELETE_ARRAY(m_aiSkillPowerByLevelFromType[idx]);
		m_aiSkillPowerByLevelFromType[idx] = NULL;
	}
}

int CTableBySkill::GetSkillDamageByLevel( int Level ) const
{
	if ( Level < 0 || Level >= PLAYER_MAX_LEVEL_CONST )
		return 0;

	return m_aiSkillDamageByLevel[Level];
}

void CTableBySkill::SetSkillDamageByLevelTable( const int* aTable )
{
	DeleteSkillDamageByLevelTable();

	int* aiSkillDamageTable = M2_NEW int[PLAYER_MAX_LEVEL_CONST];

	memcpy ( aiSkillDamageTable, aTable, sizeof( int ) * ( PLAYER_MAX_LEVEL_CONST ) );

	m_aiSkillDamageByLevel = aiSkillDamageTable;
}

void CTableBySkill::DeleteSkillDamageByLevelTable()
{
	if (NULL != m_aiSkillDamageByLevel)
	{
		M2_DELETE_ARRAY(m_aiSkillDamageByLevel);
		m_aiSkillDamageByLevel = NULL;
	}
}

