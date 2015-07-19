#include "stdafx.h"
#include "char.h"
#include "mob_manager.h"
#include "affect.h"
#include "item.h"
#include "polymorph.h"

CPolymorphUtils::CPolymorphUtils()
{
	m_mapSPDType.insert(std::make_pair(101, 101));
	m_mapSPDType.insert(std::make_pair(1901, 1901));
}

POLYMORPH_BONUS_TYPE CPolymorphUtils::GetBonusType(DWORD dwVnum)
{
	boost::unordered_map<DWORD, DWORD>::iterator iter;

	iter = m_mapSPDType.find(dwVnum);

	if (iter != m_mapSPDType.end())
		return POLYMORPH_SPD_BONUS;

	iter = m_mapATKType.find(dwVnum);

	if (iter != m_mapATKType.end())
		return POLYMORPH_ATK_BONUS;

	iter = m_mapDEFType.find(dwVnum);

	if (iter != m_mapDEFType.end())
		return POLYMORPH_DEF_BONUS;

	return POLYMORPH_NO_BONUS;
}

bool CPolymorphUtils::PolymorphCharacter(LPCHARACTER pChar, LPITEM pItem, const CMob* pMob)
{
	BYTE bySkillLevel = pChar->GetSkillLevel(POLYMORPH_SKILL_ID);
	DWORD dwDuration = 0;
	DWORD dwBonusPercent = 0;
	int iPolyPercent = 0;

	switch (pChar->GetSkillMasterType(POLYMORPH_SKILL_ID))
	{
		case SKILL_NORMAL:
			dwDuration = 10;
			break;

		case SKILL_MASTER:
			dwDuration = 15;
			break;

		case SKILL_GRAND_MASTER:
			dwDuration = 20;
			break;

		case SKILL_PERFECT_MASTER:
			dwDuration = 25;
			break;

		default:
			return false;
	}

	// dwDuration *= 60;

	// º¯½Å È®·ü = Ä³¸¯ÅÍ ·¹º§ - ¸÷ ·¹º§ + µÐ°©¼­ ·¹º§ + 29 + µÐ°© ½ºÅ³ ·¹º§
	iPolyPercent = pChar->GetLevel() - pMob->m_table.bLevel + pItem->GetSocket(2) + (29 + bySkillLevel);

	if (iPolyPercent <= 0)
	{
		pChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°©¿¡ ½ÇÆÐ ÇÏ¿´½À´Ï´Ù"));
		return false;
	}
	else
	{
		if (number(1, 100) > iPolyPercent)
		{
			pChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°©¿¡ ½ÇÆÐ ÇÏ¿´½À´Ï´Ù"));
			return false;
		}
	}

	pChar->AddAffect(AFFECT_POLYMORPH, POINT_POLYMORPH, pMob->m_table.dwVnum, AFF_POLYMORPH, dwDuration, 0, true);

	// º¯½Å º¸³Ê½º = µÐ°© ½ºÅ³ ·¹º§ + µÐ°©¼­ ·¹º§
	dwBonusPercent = bySkillLevel + pItem->GetSocket(2);

	switch (GetBonusType(pMob->m_table.dwVnum))
	{
		case POLYMORPH_ATK_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_ATT_BONUS, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		case POLYMORPH_DEF_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_DEF_BONUS, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		case POLYMORPH_SPD_BONUS:
			pChar->AddAffect(AFFECT_POLYMORPH, POINT_MOV_SPEED, dwBonusPercent, AFF_POLYMORPH, dwDuration - 1, 0, false);
			break;

		default:
		case POLYMORPH_NO_BONUS:
			break;
	}

	return true;
}

bool CPolymorphUtils::UpdateBookPracticeGrade(LPCHARACTER pChar, LPITEM pItem)
{
	if (pChar == NULL || pItem == NULL)
		return false;

	if (pItem->GetSocket(1) > 0)
		pItem->SetSocket(1, pItem->GetSocket(1) - 1);
	else
		pChar->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("µÐ°©¼­ ¼ö·ÃÀ» ¸¶Ãº½À´Ï´Ù. ½Å¼±¿¡°Ô Ã£¾Æ°¡¼¼¿ä."));

	return true;
}

bool CPolymorphUtils::GiveBook(LPCHARACTER pChar, DWORD dwMobVnum, DWORD dwPracticeCount, BYTE BookLevel, BYTE LevelLimit)
{
	// ¼ÒÄÏ0                ¼ÒÄÏ1       ¼ÒÄÏ2
	// µÐ°©ÇÒ ¸ó½ºÅÍ ¹øÈ£   ¼ö·ÃÁ¤µµ    µÐ°©¼­ ·¹º§
	if (pChar == NULL)
		return false;

	LPITEM pItem = pChar->AutoGiveItem(POLYMORPH_BOOK_ID, 1);

	if (pItem == NULL)
		return false;

	if (CMobManager::instance().Get(dwMobVnum) == NULL)
	{
		sys_err("Wrong Polymorph vnum passed: CPolymorphUtils::GiveBook(PID(%d), %d %d %d %d)", 
				pChar->GetPlayerID(), dwMobVnum, dwPracticeCount, BookLevel, LevelLimit);
		return false;
	}

	pItem->SetSocket(0, dwMobVnum);			// µÐ°©ÇÒ ¸ó½ºÅÍ ¹øÈ£
	pItem->SetSocket(1, dwPracticeCount);		// ¼ö·ÃÇØ¾ßÇÒ È½¼ö
	pItem->SetSocket(2, BookLevel);			// ¼ö·Ã·¹º§
	return true;
}

bool CPolymorphUtils::BookUpgrade(LPCHARACTER pChar, LPITEM pItem)
{
	if (pChar == NULL || pItem == NULL)
		return false;

	pItem->SetSocket(1, pItem->GetSocket(2) * 50);
	pItem->SetSocket(2, pItem->GetSocket(2)+1);
	return true;
}
