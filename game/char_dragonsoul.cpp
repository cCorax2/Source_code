#include "stdafx.h"
#include "char.h"
#include "item.h"
#include "desc.h"
#include "DragonSoul.h"
#include "log.h"

// 용혼석 초기화
// 용혼석 on/off는 Affect로 저장되기 때문에,
// 용혼석 Affect가 있다면 덱에 있는 용혼석을 activate해야한다.
// 또한 용혼석 사용 자격은 QuestFlag로 저장해 놓았기 때문에, 
// 퀘스트 Flag에서 용혼석 사용 자격을 읽어온다.

// 캐릭터의 affect, quest가 load 되기 전에 DragonSoul_Initialize를 호출하면 안된다.
// affect가 가장 마지막에 로드되어 LoadAffect에서 호출함.
void CHARACTER::DragonSoul_Initialize()
{
	for (int i = INVENTORY_MAX_NUM + WEAR_MAX_NUM; i < DRAGON_SOUL_EQUIP_SLOT_END; i++)
	{
		LPITEM pItem = GetItem(TItemPos(INVENTORY, i));
		if (NULL != pItem)
			pItem->SetSocket(ITEM_SOCKET_DRAGON_SOUL_ACTIVE_IDX, 0);
	}
	
	if (FindAffect(AFFECT_DRAGON_SOUL_DECK_0))
	{
		DragonSoul_ActivateDeck(DRAGON_SOUL_DECK_0);
	}
	else if (FindAffect(AFFECT_DRAGON_SOUL_DECK_1))
	{
		DragonSoul_ActivateDeck(DRAGON_SOUL_DECK_1);
	}
}

int CHARACTER::DragonSoul_GetActiveDeck() const
{
	return m_pointsInstant.iDragonSoulActiveDeck;
}

bool CHARACTER::DragonSoul_IsDeckActivated() const
{
	return m_pointsInstant.iDragonSoulActiveDeck >= 0;
}

bool CHARACTER::DragonSoul_IsQualified() const
{
	return FindAffect(AFFECT_DRAGON_SOUL_QUALIFIED) != NULL;
}

void CHARACTER::DragonSoul_GiveQualification()
{
	if(NULL == FindAffect(AFFECT_DRAGON_SOUL_QUALIFIED))
	{
		LogManager::instance().CharLog(this, 0, "DS_QUALIFIED", "");
	}
	AddAffect(AFFECT_DRAGON_SOUL_QUALIFIED, APPLY_NONE, 0, AFF_NONE, INFINITE_AFFECT_DURATION, 0, false, false);
	//SetQuestFlag("dragon_soul.is_qualified", 1);
	//// 자격있다면 POINT_DRAGON_SOUL_IS_QUALIFIED는 무조건 1
	//PointChange(POINT_DRAGON_SOUL_IS_QUALIFIED, 1 - GetPoint(POINT_DRAGON_SOUL_IS_QUALIFIED));
}

bool CHARACTER::DragonSoul_ActivateDeck(int deck_idx)
{
	if (deck_idx < DRAGON_SOUL_DECK_0 || deck_idx >= DRAGON_SOUL_DECK_MAX_NUM)
	{
		return false;
	}
	
	if (DragonSoul_GetActiveDeck() == deck_idx)
		return true;

	DragonSoul_DeactivateAll();

	if (!DragonSoul_IsQualified())
	{
		ChatPacket(CHAT_TYPE_INFO, LC_TEXT("용혼석 상자가 활성화되지 않았습니다."));
		return false;
	}

	AddAffect(AFFECT_DRAGON_SOUL_DECK_0 + deck_idx, APPLY_NONE, 0, 0, INFINITE_AFFECT_DURATION, 0, false);

	m_pointsInstant.iDragonSoulActiveDeck = deck_idx;

	for (int i = DRAGON_SOUL_EQUIP_SLOT_START + DS_SLOT_MAX * deck_idx; 
		i < DRAGON_SOUL_EQUIP_SLOT_START + DS_SLOT_MAX * (deck_idx + 1); i++)
	{
		LPITEM pItem = GetInventoryItem(i);
		if (NULL != pItem)
			DSManager::instance().ActivateDragonSoul(pItem);
	}
}

void CHARACTER::DragonSoul_DeactivateAll()
{
	for (int i = DRAGON_SOUL_EQUIP_SLOT_START; i < DRAGON_SOUL_EQUIP_SLOT_END; i++)
	{
		DSManager::instance().DeactivateDragonSoul(GetInventoryItem(i), true);
	}
	m_pointsInstant.iDragonSoulActiveDeck = -1;
	RemoveAffect(AFFECT_DRAGON_SOUL_DECK_0);
	RemoveAffect(AFFECT_DRAGON_SOUL_DECK_1);
}

void CHARACTER::DragonSoul_CleanUp()
{
	for (int i = DRAGON_SOUL_EQUIP_SLOT_START; i < DRAGON_SOUL_EQUIP_SLOT_END; i++)
	{
		DSManager::instance().DeactivateDragonSoul(GetInventoryItem(i), true);
	}
}

bool CHARACTER::DragonSoul_RefineWindow_Open(LPENTITY pEntity)
{
	if (NULL == m_pointsInstant.m_pDragonSoulRefineWindowOpener)
	{
		m_pointsInstant.m_pDragonSoulRefineWindowOpener = pEntity;
	}

	TPacketGCDragonSoulRefine PDS;
	PDS.header = HEADER_GC_DRAGON_SOUL_REFINE;
	PDS.bSubType = DS_SUB_HEADER_OPEN;
	LPDESC d = GetDesc();

	if (NULL == d)
	{
		sys_err ("User(%s)'s DESC is NULL POINT.", GetName());
		return false;
	}

	d->Packet(&PDS, sizeof(PDS));
	return true;
}

bool CHARACTER::DragonSoul_RefineWindow_Close()
{
	m_pointsInstant.m_pDragonSoulRefineWindowOpener = NULL;
	return true;
}

bool CHARACTER::DragonSoul_RefineWindow_CanRefine()
{
	return NULL != m_pointsInstant.m_pDragonSoulRefineWindowOpener;
}