#include "stdafx.h"
#include "../../libgame/include/attribute.h"
#include "sectree_manager.h"
#include "char.h"
#include "char_manager.h"
#include "item.h"
#include "item_manager.h"
#include "desc_manager.h"
#include "packet.h"

SECTREE::SECTREE()
{
	Initialize();
}

SECTREE::~SECTREE()
{
	Destroy();
}

void SECTREE::Initialize()
{
	m_id.package = 0;
	m_pkAttribute = NULL;
	m_iPCCount = 0;
	isClone = false;
}

void SECTREE::Destroy()
{
	if (!m_set_entity.empty())
	{
		sys_err("Sectree: entity set not empty!!");

		ENTITY_SET::iterator it = m_set_entity.begin();

		for ( ; it != m_set_entity.end(); ++it)
		{
			LPENTITY ent = *it;

			if (ent->IsType(ENTITY_CHARACTER))
			{
				LPCHARACTER ch = (LPCHARACTER) ent;

				sys_err("Sectree: destroying character: %s is_pc %d", ch->GetName(), ch->IsPC() ? 1 : 0);

				if (ch->GetDesc())
					DESC_MANAGER::instance().DestroyDesc(ch->GetDesc());
				else
					M2_DESTROY_CHARACTER(ch);
			}
			else if (ent->IsType(ENTITY_ITEM))
			{
				LPITEM item = (LPITEM) ent;

				sys_err("Sectree: destroying Item: %s", item->GetName());

				M2_DESTROY_ITEM(item);
			}
			else
				sys_err("Sectree: unknown type: %d", ent->GetType());
		}
	}
	m_set_entity.clear();

	if (!isClone && m_pkAttribute)
	{
		M2_DELETE(m_pkAttribute);
		m_pkAttribute = NULL;
	}
}

SECTREEID SECTREE::GetID()
{
	return m_id;
}

void SECTREE::IncreasePC()
{
	LPSECTREE_LIST::iterator it_tree = m_neighbor_list.begin();

	while (it_tree != m_neighbor_list.end())
	{
		++(*it_tree)->m_iPCCount;
		++it_tree;
	}
}

void SECTREE::DecreasePC()
{
	LPSECTREE_LIST::iterator it_tree = m_neighbor_list.begin();

	while (it_tree != m_neighbor_list.end())
	{
		LPSECTREE tree = *it_tree++;

		if (--tree->m_iPCCount <= 0)
		{
			if (tree->m_iPCCount < 0)
			{
				sys_err("tree pc count lower than zero (value %d coord %d %d)", tree->m_iPCCount, tree->m_id.coord.x, tree->m_id.coord.y);
				tree->m_iPCCount = 0;
			}

			ENTITY_SET::iterator it_entity = tree->m_set_entity.begin();

			while (it_entity != tree->m_set_entity.end())
			{
				LPENTITY pkEnt = *(it_entity++);

				if (pkEnt->IsType(ENTITY_CHARACTER))
				{
					LPCHARACTER ch = (LPCHARACTER) pkEnt;
					ch->StopStateMachine();
				}
			}
		}
	}
}

bool SECTREE::InsertEntity(LPENTITY pkEnt)
{
	LPSECTREE pkCurTree;

	if ((pkCurTree = pkEnt->GetSectree()) == this)
		return false;

	if (m_set_entity.find(pkEnt) != m_set_entity.end()) {
		sys_err("entity %p already exist in this sectree!", get_pointer(pkEnt));
		return false;
	}

	if (pkCurTree)
		pkCurTree->m_set_entity.erase(pkEnt);

	pkEnt->SetSectree(this);
	//pkEnt->UpdateSectree();

	m_set_entity.insert(pkEnt);

	if (pkEnt->IsType(ENTITY_CHARACTER))
	{
		LPCHARACTER pkChr = (LPCHARACTER) pkEnt;

		if (pkChr->IsPC())
		{
			IncreasePC();

			if (pkCurTree)
				pkCurTree->DecreasePC();
		}
		else if (m_iPCCount > 0 && !pkChr->IsWarp() && !pkChr->IsGoto()) // PC가 아니고 이 곳에 PC가 있다면 Idle event를 시작 시킨다.
		{
			pkChr->StartStateMachine();
		}
	}

	return true;
}

void SECTREE::RemoveEntity(LPENTITY pkEnt)
{
	ENTITY_SET::iterator it = m_set_entity.find(pkEnt);

	if (it == m_set_entity.end()) {
		return;
	}
	m_set_entity.erase(it);

	pkEnt->SetSectree(NULL);

	if (pkEnt->IsType(ENTITY_CHARACTER))
	{
		if (((LPCHARACTER) pkEnt)->IsPC())
			DecreasePC();
	}
}

void SECTREE::BindAttribute(CAttribute * pkAttribute)
{
	m_pkAttribute = pkAttribute;
}

void SECTREE::CloneAttribute(LPSECTREE tree)
{
	m_pkAttribute = tree->m_pkAttribute;
	isClone = true;
}

void SECTREE::SetAttribute(DWORD x, DWORD y, DWORD dwAttr)
{
	assert(m_pkAttribute != NULL);
	m_pkAttribute->Set(x, y, dwAttr);
}

void SECTREE::RemoveAttribute(DWORD x, DWORD y, DWORD dwAttr)
{
	assert(m_pkAttribute != NULL);
	m_pkAttribute->Remove(x, y, dwAttr);
}

DWORD SECTREE::GetAttribute(long x, long y)
{
	assert(m_pkAttribute != NULL);
	return m_pkAttribute->Get((x % SECTREE_SIZE) / CELL_SIZE, (y % SECTREE_SIZE) / CELL_SIZE);
}

bool SECTREE::IsAttr(long x, long y, DWORD dwFlag)
{
	if (IS_SET(GetAttribute(x, y), dwFlag))
		return true;

	return false;
}

int SECTREE::GetEventAttribute(long x, long y)
{
	return GetAttribute(x, y) >> 8;
}

