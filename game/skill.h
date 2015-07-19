#ifndef __INC_METIN_II_GAME_CSkillManager_H__
#define __INC_METIN_II_GAME_CSkillManager_H__

#include "../../libpoly/Poly.h"

enum ESkillFlags
{
	SKILL_FLAG_ATTACK			= (1 << 0),	// 공격 기술
	SKILL_FLAG_USE_MELEE_DAMAGE		= (1 << 1),	// 기본 밀리 타격치를 b 값으로 사용
	SKILL_FLAG_COMPUTE_ATTGRADE		= (1 << 2),	// 공격등급을 계산한다
	SKILL_FLAG_SELFONLY			= (1 << 3),	// 자신에게만 쓸 수 있음
	SKILL_FLAG_USE_MAGIC_DAMAGE		= (1 << 4),	// 기본 마법 타격치를 b 값으로 사용
	SKILL_FLAG_USE_HP_AS_COST		= (1 << 5),	// HP를 SP대신 쓴다
	SKILL_FLAG_COMPUTE_MAGIC_DAMAGE	= (1 << 6),
	SKILL_FLAG_SPLASH			= (1 << 7),
	SKILL_FLAG_GIVE_PENALTY		= (1 << 8),	// 쓰고나면 잠시동안(3초) 2배 데미지를 받는다.
	SKILL_FLAG_USE_ARROW_DAMAGE		= (1 << 9),	// 기본 화살 타격치를 b 값으로 사용
	SKILL_FLAG_PENETRATE		= (1 << 10),	// 방어무시
	SKILL_FLAG_IGNORE_TARGET_RATING	= (1 << 11),	// 상대 레이팅 무시
	SKILL_FLAG_SLOW			= (1 << 12),	// 슬로우 공격
	SKILL_FLAG_STUN			= (1 << 13),	// 스턴 공격
	SKILL_FLAG_HP_ABSORB		= (1 << 14),	// HP 흡수
	SKILL_FLAG_SP_ABSORB		= (1 << 15),	// SP 흡수
	SKILL_FLAG_FIRE_CONT		= (1 << 16),	// FIRE 지속 데미지
	SKILL_FLAG_REMOVE_BAD_AFFECT	= (1 << 17),	// 나쁜효과 제거
	SKILL_FLAG_REMOVE_GOOD_AFFECT	= (1 << 18),	// 나쁜효과 제거
	SKILL_FLAG_CRUSH			= (1 << 19),	// 상대방을 날림
	SKILL_FLAG_POISON			= (1 << 20),	// 독 공격
	SKILL_FLAG_TOGGLE			= (1 << 21),	// 토글
	SKILL_FLAG_DISABLE_BY_POINT_UP	= (1 << 22),	// 찍어서 올릴 수 없다.
	SKILL_FLAG_CRUSH_LONG		= (1 << 23),	// 상대방을 멀리 날림
	SKILL_FLAG_WIND		= (1 << 24),	// 바람 속성 
	SKILL_FLAG_ELEC		= (1 << 25),	// 전기 속성
	SKILL_FLAG_FIRE		= (1 << 26),	// 불 속성
};

enum
{
	SKILL_PENALTY_DURATION = 3,
	SKILL_TYPE_HORSE = 5,
};

enum ESkillIndexes
{
	SKILL_RESERVED = 0,

	// 무사 전사 계열
	// A
	SKILL_SAMYEON = 1,		// 삼연참(세번베기)
	SKILL_PALBANG,		// 팔방풍우
	// S
	SKILL_JEONGWI,		// 전귀혼
	SKILL_GEOMKYUNG,		// 검경
	SKILL_TANHWAN,		// 탄환격

	// 무사 기공 계열
	// A
	SKILL_GIGONGCHAM = 16,	// 기공참
	SKILL_GYOKSAN,		// 격산타우
	SKILL_DAEJINGAK,		// 대진각
	// S
	SKILL_CHUNKEON,		// 천근추
	SKILL_GEOMPUNG,		// 검풍

	// 자객 암살 계열
	// A
	SKILL_AMSEOP = 31,		// 암습 
	SKILL_GUNGSIN,		// 궁신탄영 
	SKILL_CHARYUN,		// 차륜살 
	// S
	SKILL_EUNHYUNG,		// 은형법 
	SKILL_SANGONG,		// 산공분

	// 자객 궁수 계열
	// A
	SKILL_YEONSA = 46,		// 연사 
	SKILL_KWANKYEOK,		// 관격술 
	SKILL_HWAJO,		// 화조파
	// S
	SKILL_GYEONGGONG,		// 경공술 
	SKILL_GIGUNG,		// 기궁

	// 수라 검
	// A
	SKILL_SWAERYUNG = 61,	// 쇄령지 
	SKILL_YONGKWON,		// 용권파 
	// S
	SKILL_GWIGEOM,		// 귀검
	SKILL_TERROR,		// 공포 
	SKILL_JUMAGAP,		// 주마갑 
	SKILL_PABEOB,		// 파법술

	// 수라 마법
	// A
	SKILL_MARYUNG = 76,		// 마령 
	SKILL_HWAYEOMPOK,		// 화염폭 
	SKILL_MUYEONG,		// 무영진 
	// S
	SKILL_MANASHILED,		// 흑신수호
	SKILL_TUSOK,		// 투속마령 
	SKILL_MAHWAN,		// 마환격

	// 무당 용신
	// A
	SKILL_BIPABU = 91,
	SKILL_YONGBI,		// 용비광사파 
	SKILL_PAERYONG,		// 패룡나한무
	// S
	//SKILL_BUDONG,		// 부동박부 
	SKILL_HOSIN,		// 호신 
	SKILL_REFLECT,		// 보호
	SKILL_GICHEON,		// 기천대공

	// 무당 뇌신
	// A
	SKILL_NOEJEON = 106,	// 뇌전령 
	SKILL_BYEURAK,		// 벼락 
	SKILL_CHAIN,		// 체인라이트닝 
	// S
	SKILL_JEONGEOP,		// 정업인 
	SKILL_KWAESOK,		// 이동속도업
	SKILL_JEUNGRYEOK,		// 증력술

	// 공통 스킬
	// 7
	SKILL_7_A_ANTI_TANHWAN = 112,
	SKILL_7_B_ANTI_AMSEOP,
	SKILL_7_C_ANTI_SWAERYUNG,
	SKILL_7_D_ANTI_YONGBI,

	// 8
	SKILL_8_A_ANTI_GIGONGCHAM,
	SKILL_8_B_ANTI_YEONSA,
	SKILL_8_C_ANTI_MAHWAN,
	SKILL_8_D_ANTI_BYEURAK,

	// 보조 스킬

	SKILL_LEADERSHIP = 121,	// 통솔력
	SKILL_COMBO	= 122,		// 연계기
	SKILL_CREATE = 123,		// 제조
	SKILL_MINING = 124,

	SKILL_LANGUAGE1 = 126,	// 신수어 능력
	SKILL_LANGUAGE2 = 127,	// 천조어 능력
	SKILL_LANGUAGE3 = 128,	// 진노어 능력
	SKILL_POLYMORPH = 129,	// 둔갑

	SKILL_HORSE			= 130,	// 승마 스킬
	SKILL_HORSE_SUMMON		= 131,	// 말 소환 스킬
	SKILL_HORSE_WILDATTACK	= 137,	// 난무
	SKILL_HORSE_CHARGE		= 138,	// 돌격
	SKILL_HORSE_ESCAPE		= 139,	// 탈출
	SKILL_HORSE_WILDATTACK_RANGE = 140,	// 난무(활)

	SKILL_ADD_HP	=	141,			// 증혈
	SKILL_RESIST_PENETRATE	=	142,	// 철통

	GUILD_SKILL_START = 151,
	GUILD_SKILL_EYE = 151,
	GUILD_SKILL_BLOOD = 152,
	GUILD_SKILL_BLESS = 153,
	GUILD_SKILL_SEONGHWI = 154,
	GUILD_SKILL_ACCEL = 155,
	GUILD_SKILL_BUNNO = 156,
	GUILD_SKILL_JUMUN = 157,
	GUILD_SKILL_TELEPORT = 158,
	GUILD_SKILL_DOOR = 159,
	GUILD_SKILL_END = 162,

	GUILD_SKILL_COUNT = GUILD_SKILL_END - GUILD_SKILL_START + 1,

};

class CSkillProto
{
	public:
		char	szName[64];
		DWORD	dwVnum;			// 번호

		DWORD	dwType;			// 0: 전직업, 1: 무사, 2: 자객, 3: 수라, 4: 무당
		BYTE	bMaxLevel;		// 최대 수련도
		BYTE	bLevelLimit;		// 레벨제한
		int	iSplashRange;		// 스플래쉬 거리 제한

		BYTE	bPointOn;		// 어디에 결과값을 적용 시키는가? (타격치, MAX HP, HP REGEN 등등등)
		CPoly	kPointPoly;		// 결과값 만드는 공식

		CPoly	kSPCostPoly;		// 사용 SP 공식
		CPoly	kDurationPoly;		// 지속 시간 공식
		CPoly	kDurationSPCostPoly;	// 지속 SP 공식
		CPoly	kCooldownPoly;		// 쿨다운 시간 공식
		CPoly	kMasterBonusPoly;	// 마스터일 때 보너스 공식
		CPoly	kSplashAroundDamageAdjustPoly;	// 스플래쉬 공격일 경우 주위 적에게 입히는 데미지 감소 비율

		DWORD	dwFlag;			// 스킬옵션
		DWORD	dwAffectFlag;		// 스킬에 맞은 경우 적용되는 Affect

		BYTE	bLevelStep;		// 한번에 올리는데 필요한 스킬 포인트 수
		DWORD	preSkillVnum;		// 배우는데 필요한 이전에 배워야할 스킬
		BYTE	preSkillLevel;		// 이전에 배워야할 스킬의 레벨

		long	lMaxHit;

		BYTE	bSkillAttrType;

		// 2차 적용
		BYTE	bPointOn2;		
		CPoly	kPointPoly2;		
		CPoly	kDurationPoly2;		
		DWORD	dwFlag2;			
		DWORD	dwAffectFlag2;		

		DWORD   dwTargetRange;

		bool	IsChargeSkill()
		{
			return dwVnum == SKILL_TANHWAN || dwVnum == SKILL_HORSE_CHARGE; 
		}

		// 3차 적용
		BYTE bPointOn3;
		CPoly kPointPoly3;
		CPoly kDurationPoly3;

		CPoly kGrandMasterAddSPCostPoly;

		void SetPointVar(const std::string& strName, double dVar);
		void SetDurationVar(const std::string& strName, double dVar);
		void SetSPCostVar(const std::string& strName, double dVar);
};

class CSkillManager : public singleton<CSkillManager>
{
	public:
		CSkillManager();
		virtual ~CSkillManager();

		bool Initialize(TSkillTable * pTab, int iSize);
		CSkillProto * Get(DWORD dwVnum);
		CSkillProto * Get(const char * c_pszSkillName);

	protected:
		std::map<DWORD, CSkillProto *> m_map_pkSkillProto;
};

#endif
