#include "stdafx.h"
#include "constants.h"
#include "utils.h"
#include "horse_rider.h"
#include "config.h"
#include "char_manager.h"

const int HORSE_HEALTH_DROP_INTERVAL = 3 * 24 * 60 * 60;
const int HORSE_STAMINA_CONSUME_INTERVAL = 6 * 60;
const int HORSE_STAMINA_REGEN_INTERVAL = 12 * 60;
//const int HORSE_HP_DROP_INTERVAL = 60;
//const int HORSE_STAMINA_CONSUME_INTERVAL = 3;
//const int HORSE_STAMINA_REGEN_INTERVAL = 6;

THorseStat c_aHorseStat[HORSE_MAX_LEVEL+1] =
/*
   int iMinLevel;	// 탑승할 수 있는 최소 레벨
   int iNPCRace;
   int iMaxHealth;	// 말의 최대 체력
   int iMaxStamina;	// 말의 최대 스테미너
   int iST;
   int iDX;
   int iHT;
   int iIQ;
   int iDamMean;
   int iDamMin;
   int iDamMax;
   int iDef;
 */
{
	{  0,	0,	1,	1,	0,	0,	0,	0,	0,	0,	0,	0  },
	{ 25,	20101,	3,	4,	26,	35,	18,	9,	54,	43,	64,	32 },	// 1 (초급)
	{ 25,	20101,	4,	4,	27,	36,	18,	9,	55,	44,	66,	33 },
	{ 25,	20101,	5,	5,	28,	38,	19,	9,	56,	44,	67,	33 },
	{ 25,	20101,	7,	5,	29,	39,	19,	10,	57,	45,	68,	34 },
	{ 25,	20101,	8,	6,	30,	40,	20,	10,	58,	46,	69,	34 },
	{ 25,	20101,	9,	6,	31,	41,	21,	10,	59,	47,	70,	35 },
	{ 25,	20101,	11,	7,	32,	42,	21,	11,	60,	48,	72,	36 },
	{ 25,	20101,	12,	7,	33,	44,	22,	11,	61,	48,	73,	36 },
	{ 25,	20101,	13,	8,	34,	45,	22,	11,	62,	49,	74,	37 },
	{ 25,	20101,	15,	10,	35,	46,	23,	12,	63,	50,	75,	37 },
	{ 35,	20104,	18,	30,	40,	53,	27,	13,	69,	55,	82,	41 },	// 11 (중급)
	{ 35,	20104,	19,	35,	41,	54,	27,	14,	70,	56,	84,	42 },
	{ 35,	20104,	21,	40,	42,	56,	28,	14,	71,	56,	85,	42 },
	{ 35,	20104,	22,	50,	43,	57,	28,	14,	72,	57,	86,	43 },
	{ 35,	20104,	24,	55,	44,	58,	29,	15,	73,	58,	87,	43 },
	{ 35,	20104,	25,	60,	44,	59,	30,	15,	74,	59,	88,	44 },
	{ 35,	20104,	27,	65,	45,	60,	30,	15,	75,	60,	90,	45 },
	{ 35,	20104,	28,	70,	46,	62,	31,	15,	76,	60,	91,	45 },
	{ 35,	20104,	30,	80,	47,	63,	31,	16,	77,	61,	92,	46 },
	{ 35,	20104,	32,	100,	48,	64,	32,	16,	78,	62,	93,	46 },
	{ 50,	20107,	35,	120,	53,	71,	36,	18,	84,	67,	100,	50 },	// 21 (고급)
	{ 50,	20107,	36,	125,	55,	74,	37,	18,	86,	68,	103,	51 },
	{ 50,	20107,	37,	130,	57,	76,	38,	19,	88,	70,	105,	52 },
	{ 50,	20107,	38,	135,	59,	78,	39,	20,	90,	72,	108,	54 },
	{ 50,	20107,	40,	140,	60,	80,	40,	20,	91,	72,	109,	54 },
	{ 50,	20107,	42,	145,	61,	81,	40,	20,	92,	73,	110,	55 },
	{ 50,	20107,	44,	150,	62,	83,	42,	21,	94,	75,	112,	56 },
	{ 50,	20107,	46,	160,	63,	84,	42,	21,	95,	76,	114,	57 },
	{ 50,	20107,	48,	170,	65,	87,	43,	22,	97,	77,	116,	58 },
	{ 50,	20107,	50,	200,	67,	89,	45,	22,	99,	79,	118,	59 }
};

CHorseRider::CHorseRider()
{
	Initialize();
}

CHorseRider::~CHorseRider()
{
	Destroy();
}

void CHorseRider::Initialize()
{
	m_eventStaminaRegen = NULL;
	m_eventStaminaConsume = NULL;
	memset(&m_Horse, 0, sizeof(m_Horse));
}

void CHorseRider::Destroy()
{
	event_cancel(&m_eventStaminaRegen);
	event_cancel(&m_eventStaminaConsume);
}

void CHorseRider::EnterHorse()
{
	if (GetHorseLevel() <= 0)
		return;

	if (GetHorseHealth() <= 0)
		return;

	if (IsHorseRiding())
	{
		m_Horse.bRiding = !m_Horse.bRiding;
		StartRiding();
	}
	else
	{
		StartStaminaRegenEvent();
	}
	CheckHorseHealthDropTime(false);
}

bool CHorseRider::ReviveHorse()
{
	if (GetHorseLevel() <= 0)
		return false;

	if (GetHorseHealth()>0)
		return false;

	int level = GetHorseLevel();

	m_Horse.sHealth = c_aHorseStat[level].iMaxHealth;
	m_Horse.sStamina = c_aHorseStat[level].iMaxStamina;

	// 2005.03.24.ipkn.말 살린후 다시 죽는 현상 수정
	ResetHorseHealthDropTime();

	StartStaminaRegenEvent();
	return true;
}

short CHorseRider::GetHorseMaxHealth()
{
	int level = GetHorseLevel();
	return c_aHorseStat[level].iMaxHealth;
}

short CHorseRider::GetHorseMaxStamina()
{
	int level = GetHorseLevel();
	return c_aHorseStat[level].iMaxStamina;
}

void CHorseRider::FeedHorse()
{
	// 말을 가지고 살아있을때만
	if (GetHorseLevel() > 0 && GetHorseHealth() > 0)
	{
		UpdateHorseHealth(+1);
		// 20050324. ipkn 말 먹였을때도 체력 감소 딜레이를 늘린다.
		ResetHorseHealthDropTime();
	}
}

void CHorseRider::SetHorseData(const THorseInfo& crInfo)
{
	m_Horse = crInfo;
}

// Stamina
void CHorseRider::UpdateHorseDataByLogoff(DWORD dwLogoffTime)
{
	if (GetHorseLevel() <= 0)
		return;

	if (dwLogoffTime >= 12 * 60)
		UpdateHorseStamina(dwLogoffTime / 12 / 60, false); // 로그오프 12분당 1씩 회복
}

void CHorseRider::UpdateHorseStamina(int iStamina, bool bSend)
{
	int level = GetHorseLevel();

	m_Horse.sStamina = MINMAX(0, m_Horse.sStamina + iStamina, c_aHorseStat[level].iMaxStamina);

	if (GetHorseStamina() == 0 && IsHorseRiding())
	{
		StopRiding();
	}

	if (bSend)
		SendHorseInfo();
}

bool CHorseRider::StartRiding()
{
	if (m_Horse.bRiding)
		return false;

	if (GetHorseLevel() <= 0)
		return false;

	if (GetHorseHealth() <= 0)
		return false;

	if (GetHorseStamina() <= 0)
		return false;

	m_Horse.bRiding = true;
	StartStaminaConsumeEvent();
	SendHorseInfo();
	return true;
}

bool CHorseRider::StopRiding()
{
	if (!m_Horse.bRiding)
		return false;

	m_Horse.bRiding = false;
	StartStaminaRegenEvent();
	return true;
}

EVENTINFO(horserider_info) 
{
	CHorseRider* hr;

	horserider_info() 
	: hr( 0 )
	{
	}
};

EVENTFUNC(horse_stamina_consume_event)
{
	horserider_info* info = dynamic_cast<horserider_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "horse_stamina_consume_event> <Factor> Null pointer" );
		return 0;
	}

	CHorseRider* hr = info->hr;

	if (hr->GetHorseHealth() <= 0)
	{
		hr->m_eventStaminaConsume = NULL;
		return 0;
	}

	hr->UpdateHorseStamina(-1);
	hr->UpdateRideTime(HORSE_STAMINA_CONSUME_INTERVAL);

	int delta = PASSES_PER_SEC(HORSE_STAMINA_CONSUME_INTERVAL);

	if (hr->GetHorseStamina() == 0)
	{
		hr->m_eventStaminaConsume = NULL;
		delta = 0;
	}

	hr->CheckHorseHealthDropTime();
	sys_log(0, "HORSE STAMINA - %p", get_pointer(event));
	return delta;
}

EVENTFUNC(horse_stamina_regen_event)
{
	horserider_info* info = dynamic_cast<horserider_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "horse_stamina_regen_event> <Factor> Null pointer" );
		return 0;
	}

	CHorseRider* hr = info->hr;

	if (hr->GetHorseHealth()<=0)
	{
		hr->m_eventStaminaRegen = NULL;
		return 0;
	}

	hr->UpdateHorseStamina(+1);
	int delta = PASSES_PER_SEC(HORSE_STAMINA_REGEN_INTERVAL);
	if (hr->GetHorseStamina() == hr->GetHorseMaxStamina())
	{
		delta = 0;
		hr->m_eventStaminaRegen = NULL;
	}

	hr->CheckHorseHealthDropTime();
	sys_log(0, "HORSE STAMINA + %p", get_pointer(event));


	return delta;
}

void CHorseRider::StartStaminaConsumeEvent()
{
	if (GetHorseLevel() <= 0)
		return;

	if (GetHorseHealth() <= 0)
		return;

	sys_log(0,"HORSE STAMINA REGEN EVENT CANCEL %p", get_pointer(m_eventStaminaRegen));
	event_cancel(&m_eventStaminaRegen);

	if (m_eventStaminaConsume)
		return;

	horserider_info* info = AllocEventInfo<horserider_info>();

	info->hr = this;
	m_eventStaminaConsume = event_create(horse_stamina_consume_event, info, PASSES_PER_SEC(HORSE_STAMINA_CONSUME_INTERVAL));
	sys_log(0,"HORSE STAMINA CONSUME EVENT CREATE %p", get_pointer(m_eventStaminaConsume));
}

void CHorseRider::StartStaminaRegenEvent()
{
	if (GetHorseLevel() <= 0)
		return;

	if (GetHorseHealth() <= 0)
		return;

	sys_log(0,"HORSE STAMINA CONSUME EVENT CANCEL %p", get_pointer(m_eventStaminaConsume));
	event_cancel(&m_eventStaminaConsume);

	if (m_eventStaminaRegen)
		return;

	horserider_info* info = AllocEventInfo<horserider_info>();

	info->hr = this;
	m_eventStaminaRegen = event_create(horse_stamina_regen_event, info, PASSES_PER_SEC(HORSE_STAMINA_REGEN_INTERVAL));
	sys_log(0,"HORSE STAMINA REGEN EVENT CREATE %p", get_pointer(m_eventStaminaRegen));
}

// Health
void CHorseRider::ResetHorseHealthDropTime()
{
	m_Horse.dwHorseHealthDropTime = get_global_time() + HORSE_HEALTH_DROP_INTERVAL;
}

void CHorseRider::CheckHorseHealthDropTime(bool bSend)
{
	DWORD now = get_global_time();

	while (m_Horse.dwHorseHealthDropTime < now)
	{
		m_Horse.dwHorseHealthDropTime += HORSE_HEALTH_DROP_INTERVAL;
		UpdateHorseHealth(-1, bSend);
	}
}

void CHorseRider::UpdateHorseHealth(int iHealth, bool bSend)
{
	int level = GetHorseLevel();

	m_Horse.sHealth = MINMAX(0, m_Horse.sHealth + iHealth, c_aHorseStat[level].iMaxHealth);

	if (level && m_Horse.sHealth == 0)
		HorseDie();

	if (bSend)
		SendHorseInfo();
}

void CHorseRider::HorseDie()
{
	sys_log(0, "HORSE DIE %p %p", get_pointer(m_eventStaminaRegen), get_pointer(m_eventStaminaConsume));
	UpdateHorseStamina(-m_Horse.sStamina);
	event_cancel(&m_eventStaminaRegen);
	event_cancel(&m_eventStaminaConsume);
}

void CHorseRider::SetHorseLevel(int iLevel)
{
	m_Horse.bLevel = iLevel = MINMAX(0, iLevel, HORSE_MAX_LEVEL);

	m_Horse.sStamina = c_aHorseStat[iLevel].iMaxStamina;
	m_Horse.sHealth = c_aHorseStat[iLevel].iMaxHealth;
	m_Horse.dwHorseHealthDropTime = 0;

	ResetHorseHealthDropTime();

	SendHorseInfo();
}

BYTE CHorseRider::GetHorseGrade()
{
	BYTE grade = 0;

	if (GetHorseLevel())
		grade = (GetHorseLevel() - 1) / 10 + 1;

	return grade;
}

