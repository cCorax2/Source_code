#include "stdafx.h"
#include "constants.h"
#include "log.h"
#include "char.h"
#include "item_manager.h"
#include "item.h"
#include "over9refine.h"

void COver9RefineManager::enableOver9Refine(DWORD dwVnumFrom, DWORD dwVnumTo)
{
	m_mapItem.insert(std::make_pair(dwVnumFrom, dwVnumTo));
}

int COver9RefineManager::canOver9Refine(DWORD dwVnum)
{
	OVER9ITEM_MAP::iterator iter = m_mapItem.find(dwVnum);

	if (iter != m_mapItem.end())
		return 1;

	if (dwVnum % 10 == 9)
		return 0;

	dwVnum -= dwVnum % 10;

	for (iter = m_mapItem.begin(); iter != m_mapItem.end(); ++iter)
		if (iter->second == dwVnum)
			return 2;

	return 0;
}

bool COver9RefineManager::Change9ToOver9(LPCHARACTER pChar, LPITEM item)
{
	OVER9ITEM_MAP::iterator iter = m_mapItem.find(item->GetVnum());

	if (iter == m_mapItem.end())
		return false;

	DWORD dwVnum = iter->second;

	LPITEM over9 = ITEM_MANAGER::instance().CreateItem(dwVnum, 1);

	if (over9 == NULL)
		return false;

	item->CopySocketTo(over9);
	item->CopyAttributeTo(over9);

	int iEmptyCell = pChar->GetEmptyInventory(over9->GetSize());

	if (iEmptyCell == -1)
		return false;

	item->RemoveFromCharacter();

	over9->AddToCharacter(pChar, TItemPos(INVENTORY, iEmptyCell));

	char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "SUCCESS %u %s %u", over9->GetID(), over9->GetName(), over9->GetOriginalVnum());
	LogManager::instance().ItemLog(pChar, item, "REFINE OVER9", szBuf);
	return true;
}

bool COver9RefineManager::Over9Refine(LPCHARACTER pChar, LPITEM item)
{
	DWORD dwVnum = item->GetRefinedVnum();

	if (dwVnum == 0)
		return false;

	LPITEM over9 = ITEM_MANAGER::instance().CreateItem(dwVnum, 1);

	if (over9 == NULL)
		return false;

	item->CopySocketTo(over9);
	item->CopyAttributeTo(over9);

	int iEmptyCell = pChar->GetEmptyInventory(over9->GetSize());

	if (iEmptyCell == -1)
		return false;

	item->RemoveFromCharacter();

	over9->AddToCharacter(pChar, TItemPos(INVENTORY, iEmptyCell));

	char szBuf[256];
	snprintf(szBuf, sizeof(szBuf), "SUCCESS %u %s %u", over9->GetID(), over9->GetName(), over9->GetOriginalVnum());
	LogManager::instance().ItemLog(pChar, item, "REFINE OVER9", szBuf);
	return true;
}

DWORD COver9RefineManager::GetMaterialVnum(DWORD baseVnum)
{
	OVER9ITEM_MAP::iterator iter = m_mapItem.find(baseVnum);

	if (iter != m_mapItem.end())
		return (baseVnum - (baseVnum % 10));

	baseVnum -= baseVnum % 10;

	for (iter = m_mapItem.begin(); iter != m_mapItem.end(); ++iter)
		if (iter->second == baseVnum)
			return (iter->first - (iter->first % 10));

	return 0;
}

