#include "stdafx.h"
#include "refine.h"

CRefineManager::CRefineManager()
{
}

CRefineManager::~CRefineManager()
{
}

bool CRefineManager::Initialize(TRefineTable * table, int size)
{
	for (int i = 0; i < size; ++i, ++table)
	{
		sys_log(0, "REFINE %d prob %d cost %d", table->id, table->prob, table->cost);
		m_map_RefineRecipe.insert(std::make_pair(table->id, *table));
	}

	sys_log(0, "REFINE: COUNT %d", m_map_RefineRecipe.size());
	return true;
}

const TRefineTable* CRefineManager::GetRefineRecipe(DWORD vnum)
{
	if (vnum == 0)
		return NULL;

	itertype(m_map_RefineRecipe) it = m_map_RefineRecipe.find(vnum);
	sys_log(0, "REFINE: FIND %u %s", vnum, it == m_map_RefineRecipe.end() ? "FALSE" : "TRUE");

	if (it == m_map_RefineRecipe.end())
	{
		return NULL;
	}

	return &it->second;
}
