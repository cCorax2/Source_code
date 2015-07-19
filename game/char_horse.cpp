#include "stdafx.h"
#include "config.h"
#include "char.h"
#include "char_manager.h"
#include "packet.h"
#include "guild.h"
#include "vector.h"
#include "questmanager.h"
#include "item.h"
#include "horsename_manager.h"
#include "locale_service.h"
#include "arena.h"

#include "../../common/VnumHelper.h"

bool CHARACTER::StartRiding()
{
	if (IsDead() == true)
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("쓰러진 상태에서는 말에 탈 수 없습니다."));
		return false;
	}
	
	if (IsPolymorphed())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("변신 상태에서는 말에 탈 수 없습니다."));
		return false;
	}

	// 턱시도 입은 상태의 말 타기 금지
	LPITEM armor = GetWear(WEAR_BODY);

	if (armor && (armor->GetVnum() >= 11901 && armor->GetVnum() <= 11904))
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("예복을 입은 상태에서 말을 탈 수 없습니다."));
		return false;
	}

	if (LC_IsCanada() == true)
	{
		if (CArenaManager::instance().IsArenaMap(GetMapIndex()) == true)
			return false;
	}


	DWORD dwMountVnum = m_chHorse ? m_chHorse->GetRaceNum() : GetMyHorseVnum();

	if (false == CHorseRider::StartRiding())
	{
		if (GetHorseLevel() <= 0)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 소유하고 있지 않습니다."));
		else if (GetHorseHealth() <= 0)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말이 죽어있는 상태 입니다."));
		else if (GetHorseStamina() <= 0)
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말의 스테미너가 부족하여 말을 탈 수 없습니다."));

		return false;
	}

	// 소환한 말 없애고
	HorseSummon(false);

	MountVnum(dwMountVnum);

	if(test_server)
		sys_log(0, "Ride Horse : %s ", GetName());

	return true;
}

bool CHARACTER::StopRiding()
{
	if (CHorseRider::StopRiding())
	{
		quest::CQuestManager::instance().Unmount(GetPlayerID());

		if (!IsDead() && !IsStun())
		{
			DWORD dwOldVnum = GetMountVnum();
			MountVnum(0);

			// [NOTE] 말에서 내릴 땐 자기가 탔던걸 소환하도록 수정
			HorseSummon(true, false, dwOldVnum);
		}
		else
		{
			m_dwMountVnum = 0;
			ComputePoints();
			UpdatePacket();
		}

		PointChange(POINT_ST, 0);
		PointChange(POINT_DX, 0);
		PointChange(POINT_HT, 0);
		PointChange(POINT_IQ, 0);

		return true;
	}

	return false;
}

EVENTFUNC(horse_dead_event)
{
	char_event_info* info = dynamic_cast<char_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "horse_dead_event> <Factor> Null pointer" );
		return 0;
	}

	// <Factor>
	LPCHARACTER ch = info->ch;
	if (ch == NULL) {
		return 0;
	}
	ch->HorseSummon(false);
	return 0;
}

void CHARACTER::SetRider(LPCHARACTER ch)
{
	if (m_chRider)
		m_chRider->ClearHorseInfo();

	m_chRider = ch;

	if (m_chRider)
		m_chRider->SendHorseInfo();
}

LPCHARACTER CHARACTER::GetRider() const
{
	return m_chRider;
}


void CHARACTER::HorseSummon(bool bSummon, bool bFromFar, DWORD dwVnum, const char* pPetName)
{
	if ( bSummon )
	{
		//NOTE : summon했는데 이미 horse가 있으면 아무것도 안한다.
		if( m_chHorse != NULL )
			return;

		if (GetHorseLevel() <= 0)
			return;

		// 무언가를 타고 있다면 실패
		if (IsRiding())
			return;

		sys_log(0, "HorseSummon : %s lv:%d bSummon:%d fromFar:%d", GetName(), GetLevel(), bSummon, bFromFar);

		long x = GetX();
		long y = GetY();

		if (GetHorseHealth() <= 0)
			bFromFar = false;

		if (bFromFar)
		{
			x += (number(0, 1) * 2 - 1) * number(2000, 2500);
			y += (number(0, 1) * 2 - 1) * number(2000, 2500);
		}
		else
		{
			x += number(-100, 100);
			y += number(-100, 100);
		}

		m_chHorse = CHARACTER_MANAGER::instance().SpawnMob(
				(0 == dwVnum) ? GetMyHorseVnum() : dwVnum, 
				GetMapIndex(), 
				x, y,
				GetZ(), false, (int)(GetRotation()+180), false);

		if (!m_chHorse)
		{
			ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말 소환에 실패하였습니다."));
			return;
		}

		if (GetHorseHealth() <= 0)
		{
			// 죽은거처럼 있게 하는 처리
			m_chHorse->SetPosition(POS_DEAD);

			// 일정시간있다 사라지게 하자.
			char_event_info* info = AllocEventInfo<char_event_info>();
			info->ch = this;
			m_chHorse->m_pkDeadEvent = event_create(horse_dead_event, info, PASSES_PER_SEC(60));
		}

		m_chHorse->SetLevel(GetHorseLevel());

		const char* pHorseName = CHorseNameManager::instance().GetHorseName(GetPlayerID());

		if ( pHorseName != NULL && strlen(pHorseName) != 0 )
		{
			m_chHorse->m_stName = pHorseName;
		}
		else
		{
			m_chHorse->m_stName = GetName();
			m_chHorse->m_stName += LC_TEXT("님의 말");
		}

		if (!m_chHorse->Show(GetMapIndex(), x, y, GetZ()))
		{
			M2_DESTROY_CHARACTER(m_chHorse);
			sys_err("cannot show monster");
			m_chHorse = NULL;
			return;
		}

		if ((GetHorseHealth() <= 0))
		{
			TPacketGCDead pack;
			pack.header	= HEADER_GC_DEAD;
			pack.vid    = m_chHorse->GetVID();
			PacketAround(&pack, sizeof(pack));
		}

		m_chHorse->SetRider(this);
	}
	else
	{
		if (!m_chHorse)
			return;

		LPCHARACTER chHorse = m_chHorse;

		chHorse->SetRider(NULL); // m_chHorse assign to NULL

		if (!bFromFar)
		{
			M2_DESTROY_CHARACTER(chHorse);
		}
		else
		{
			// 멀어지면서 사라지는 처리 하기
			chHorse->SetNowWalking(false);
			float fx, fy;
			chHorse->SetRotation(GetDegreeFromPositionXY(chHorse->GetX(), chHorse->GetY(), GetX(), GetY())+180);
			GetDeltaByDegree(chHorse->GetRotation(), 3500, &fx, &fy);
			chHorse->Goto((long)(chHorse->GetX()+fx), (long) (chHorse->GetY()+fy));
			chHorse->SendMovePacket(FUNC_WAIT, 0, 0, 0, 0);
		}

		m_chHorse = NULL;
	}
}

DWORD CHARACTER::GetMyHorseVnum() const
{
	int delta = 0;

	if (GetGuild())
	{
		++delta;

		if (GetGuild()->GetMasterPID() == GetPlayerID())
			++delta;
	}

	return c_aHorseStat[GetHorseLevel()].iNPCRace + delta;
}

void CHARACTER::HorseDie()
{
	CHorseRider::HorseDie();
	HorseSummon(false);
}

bool CHARACTER::ReviveHorse()
{
	if (CHorseRider::ReviveHorse())
	{
		HorseSummon(false);
		HorseSummon(true);
		return true;
	}
	return false;
}

void CHARACTER::ClearHorseInfo()
{
	if (!IsHorseRiding())
	{
		ChatPacket(CHAT_TYPE_COMMAND, "hide_horse_state");

		m_bSendHorseLevel = 0;
		m_bSendHorseHealthGrade = 0;
		m_bSendHorseStaminaGrade = 0;
	}

	m_chHorse = NULL;

}

void CHARACTER::SendHorseInfo()
{
	if (m_chHorse || IsHorseRiding())
	{
		int iHealthGrade;
		int iStaminaGrade;
		/*
		   HP	
3: 70% < ~ <= 100%
2: 30% < ~ <= 70%
1:  0% < ~ <= 30%
0: 사망

STM

3: 71% < ~ <= 100%
2: 31% < ~ <= 70%
1: 10% < ~ <= 30%
0:	 ~ <= 10%
		 */
		if (GetHorseHealth() == 0)
			iHealthGrade = 0;
		else if (GetHorseHealth() * 10 <= GetHorseMaxHealth() * 3) 
			iHealthGrade = 1;
		else if (GetHorseHealth() * 10 <= GetHorseMaxHealth() * 7) 
			iHealthGrade = 2;
		else
			iHealthGrade = 3;

		if (GetHorseStamina() * 10 <= GetHorseMaxStamina())
			iStaminaGrade = 0;
		else if (GetHorseStamina() * 10 <= GetHorseMaxStamina() * 3) 
			iStaminaGrade = 1;
		else if (GetHorseStamina() * 10 <= GetHorseMaxStamina() * 7) 
			iStaminaGrade = 2;
		else
			iStaminaGrade = 3;

		if (m_bSendHorseLevel != GetHorseLevel() || 
				m_bSendHorseHealthGrade != iHealthGrade || 
				m_bSendHorseStaminaGrade != iStaminaGrade)
		{
			ChatPacket(CHAT_TYPE_COMMAND, "horse_state %d %d %d", GetHorseLevel(), iHealthGrade, iStaminaGrade);

			// FIX : 클라이언트에 "말 상태 버프" 아이콘을 표시하지 않을 목적으로 함수 초입에 return함으로써 아래 코드를 무시한다면
			// 말을 무한대로 소환하는 무시무시한 버그가 생김.. 정확한 원인은 파악 안해봐서 모름.
			m_bSendHorseLevel = GetHorseLevel();
			m_bSendHorseHealthGrade = iHealthGrade;
			m_bSendHorseStaminaGrade = iStaminaGrade;
		}
	}
}
		
bool CHARACTER::CanUseHorseSkill()
{
	if(IsRiding())
	{
		if (GetHorseGrade() == 3)
			return true;
		else
			return false;

		if(GetMountVnum())
		{
			if (GetMountVnum() >= 20209 && GetMountVnum() <= 20212)
				return true;

			//라마단 흑마
			if (CMobVnumHelper::IsRamadanBlackHorse(GetMountVnum()))
				return true;
		}
		else
			return false;

	}

	return false;
}

void CHARACTER::SetHorseLevel(int iLevel)
{
	CHorseRider::SetHorseLevel(iLevel);
	SetSkillLevel(SKILL_HORSE, GetHorseLevel());
}

