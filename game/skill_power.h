
#ifndef METIN2_TABLE_BY_SKILL_
#define METIN2_TABLE_BY_SKILL_

class CTableBySkill : public singleton<CTableBySkill>
{
	public:
		CTableBySkill()
			: m_aiSkillDamageByLevel(NULL)
		{
			//스킬 레벨당 추가데미지 초기화
			for ( int job = 0; job < JOB_MAX_NUM * 2; ++job )
				m_aiSkillPowerByLevelFromType[job] = NULL;
		}

		~CTableBySkill()
		{
			DeleteAll();
		}

		//테이블 세팅 체크
		bool 	Check() const;

		//삭제
		void 	DeleteAll();

		//스킬레벨단위 스킬파워 테이블
		int 	GetSkillPowerByLevelFromType( int job, int skillgroup, int skilllevel, bool bMob ) const;
		void 	SetSkillPowerByLevelFromType( int idx, const int* aTable );
		void	DeleteSkillPowerByLevelFromType( int idx );

		//레벨당 추가 스킬데미지  테이블
		int 	GetSkillDamageByLevel( int Level ) const;
		void	SetSkillDamageByLevelTable( const int* aTable );
		void 	DeleteSkillDamageByLevelTable();

	private:
		int * m_aiSkillPowerByLevelFromType[JOB_MAX_NUM*2];
		int * m_aiSkillDamageByLevel;
};

#endif /* METIN2_TABLE_BY_SKILL_ */

