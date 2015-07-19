#include "stdafx.h"
#include "constants.h"
#include "item.h"
#include "item_manager.h"
#include "unique_item.h"
#include "packet.h"
#include "desc.h"
#include "char.h"
#include "dragon_soul_table.h"
#include "log.h"
#include "DragonSoul.h"
#include <boost/lexical_cast.hpp>

typedef std::vector <std::string> TTokenVector;

int Gamble(std::vector<float>& vec_probs)
{
	float range = 0.f;
	for (int i = 0; i < vec_probs.size(); i++)
	{
		range += vec_probs[i];
	}
	float fProb = fnumber(0.f, range);
	float sum = 0.f;
	for (int idx = 0; idx < vec_probs.size(); idx++)
	{
		sum += vec_probs[idx];
		if (sum >= fProb)
			return idx;
	}
	return -1;
}

// 가중치 테이블(prob_lst)을 받아 random_set.size()개의 index를 선택하여 random_set을 return
bool MakeDistinctRandomNumberSet(std::list <float> prob_lst, OUT std::vector<int>& random_set)
{
	int size = prob_lst.size();
	int n = random_set.size();
	if (size < n)
		return false;

	std::vector <int> select_bit(size, 0);
	for (int i = 0; i < n; i++)
	{
		float range = 0.f;
		for (std::list <float>::iterator it = prob_lst.begin(); it != prob_lst.end(); it++)
		{
			range += *it;
		}
		float r = fnumber (0.f, range);
		float sum = 0.f;
		int idx = 0;
		for (std::list <float>::iterator it = prob_lst.begin(); it != prob_lst.end(); it++)
		{
			while (select_bit[idx++]);

			sum += *it;
			if (sum >= r)
			{
				select_bit[idx - 1] = 1;
				random_set[i] = idx - 1;
				prob_lst.erase(it);
				break;
			}
		}
	}
	return true;
}

/* 용혼석 Vnum에 대한 comment	
 * ITEM VNUM을 10만 자리부터, FEDCBA라고 한다면
 * FE : 용혼석 종류.	D : 등급
 * C : 단계			B : 강화		
 * A : 여벌의 번호들... 	
 */

BYTE GetType(DWORD dwVnum)
{
	return (dwVnum / 10000);
}

BYTE GetGradeIdx(DWORD dwVnum)
{
	return (dwVnum / 1000) % 10;
}

BYTE GetStepIdx(DWORD dwVnum)
{
	return (dwVnum / 100) % 10;
}

BYTE GetStrengthIdx(DWORD dwVnum)
{
	return (dwVnum / 10) % 10;
}

bool DSManager::ReadDragonSoulTableFile(const char * c_pszFileName)
{
	m_pTable = new DragonSoulTable();
	return m_pTable->ReadDragonSoulTableFile(c_pszFileName);
}

void DSManager::GetDragonSoulInfo(DWORD dwVnum, BYTE& bType, BYTE& bGrade, BYTE& bStep, BYTE& bStrength) const
{
	bType = GetType(dwVnum);
	bGrade = GetGradeIdx(dwVnum);
	bStep = GetStepIdx(dwVnum);
	bStrength = GetStrengthIdx(dwVnum);
}

bool DSManager::IsValidCellForThisItem(const LPITEM pItem, const TItemPos& Cell) const
{
	if (NULL == pItem)
		return false;

	WORD wBaseCell = GetBasePosition(pItem);
	if (WORD_MAX == wBaseCell)
		return false;

	if (Cell.window_type != DRAGON_SOUL_INVENTORY
		|| (Cell.cell < wBaseCell || Cell.cell >= wBaseCell + DRAGON_SOUL_BOX_SIZE))
	{
		return false;
	}
	else
		return true;

}

WORD DSManager::GetBasePosition(const LPITEM pItem) const
{
	if (NULL == pItem)
		return WORD_MAX;

	BYTE type, grade_idx, step_idx, strength_idx;
	GetDragonSoulInfo(pItem->GetVnum(), type, grade_idx, step_idx, strength_idx);

	BYTE col_type = pItem->GetSubType();
	BYTE row_type = grade_idx;
	if (row_type > DRAGON_SOUL_GRADE_MAX)
		return WORD_MAX;

	return col_type * DRAGON_SOUL_STEP_MAX * DRAGON_SOUL_BOX_SIZE + row_type * DRAGON_SOUL_BOX_SIZE;
}

bool DSManager::RefreshItemAttributes(LPITEM pDS)
{
	if (!pDS->IsDragonSoul())
	{
		sys_err ("This item(ID : %d) is not DragonSoul.", pDS->GetID());
		return false;
	}

	BYTE ds_type, grade_idx, step_idx, strength_idx;
	GetDragonSoulInfo(pDS->GetVnum(), ds_type, grade_idx, step_idx, strength_idx);

	DragonSoulTable::TVecApplys vec_basic_applys;
	DragonSoulTable::TVecApplys vec_addtional_applys;

	if (!m_pTable->GetBasicApplys(ds_type, vec_basic_applys))
	{
		sys_err ("There is no BasicApply about %d type dragon soul.", ds_type);
		return false;
	}

	if (!m_pTable->GetAdditionalApplys(ds_type, vec_addtional_applys))
	{
		sys_err ("There is no AdditionalApply about %d type dragon soul.", ds_type);
		return false;
	}

	// add_min과 add_max는 더미로 읽음.
	int basic_apply_num, add_min, add_max;
	if (!m_pTable->GetApplyNumSettings(ds_type, grade_idx, basic_apply_num, add_min, add_max))
	{
		sys_err ("In ApplyNumSettings, INVALID VALUES Group type(%d), GRADE idx(%d)", ds_type, grade_idx);
		return false;
	}

	float fWeight = 0.f;
	if (!m_pTable->GetWeight(ds_type, grade_idx, step_idx, strength_idx, fWeight))
	{
		return false;
	}
	fWeight /= 100.f;

	int n = MIN(basic_apply_num, vec_basic_applys.size());
	for (int i = 0; i < n; i++)
	{	
		const SApply& basic_apply = vec_basic_applys[i];
		BYTE bType = basic_apply.apply_type;
		short sValue = (short)(ceil((float)basic_apply.apply_value * fWeight - 0.01f));

		pDS->SetForceAttribute(i, bType, sValue);
	}

	for (int i = DRAGON_SOUL_ADDITIONAL_ATTR_START_IDX; i < ITEM_ATTRIBUTE_MAX_NUM; i++)
	{
		BYTE bType = pDS->GetAttributeType(i);
		short sValue = 0;
		if (APPLY_NONE == bType)
			continue;
		for (int j = 0; j < vec_addtional_applys.size(); j++)
		{
			if (vec_addtional_applys[j].apply_type == bType)
			{
				sValue = vec_addtional_applys[j].apply_value;
				break;
			}
		}
		pDS->SetForceAttribute(i, bType, (short)(ceil((float)sValue * fWeight - 0.01f)));
	}
	return true;
}

bool DSManager::PutAttributes(LPITEM pDS)
{
	if (!pDS->IsDragonSoul())
	{
		sys_err ("This item(ID : %d) is not DragonSoul.", pDS->GetID());
		return false;
	}

	BYTE ds_type, grade_idx, step_idx, strength_idx;
	GetDragonSoulInfo(pDS->GetVnum(), ds_type, grade_idx, step_idx, strength_idx);

	DragonSoulTable::TVecApplys vec_basic_applys;
	DragonSoulTable::TVecApplys vec_addtional_applys;

	if (!m_pTable->GetBasicApplys(ds_type, vec_basic_applys))
	{
		sys_err ("There is no BasicApply about %d type dragon soul.", ds_type);
		return false;
	}
	if (!m_pTable->GetAdditionalApplys(ds_type, vec_addtional_applys))
	{
		sys_err ("There is no AdditionalApply about %d type dragon soul.", ds_type);
		return false;
	}

	
	int basic_apply_num, add_min, add_max;
	if (!m_pTable->GetApplyNumSettings(ds_type, grade_idx, basic_apply_num, add_min, add_max))
	{
		sys_err ("In ApplyNumSettings, INVALID VALUES Group type(%d), GRADE idx(%d)", ds_type, grade_idx);
		return false;
	}

	float fWeight = 0.f;
	if (!m_pTable->GetWeight(ds_type, grade_idx, step_idx, strength_idx, fWeight))
	{
		return false;
	}
	fWeight /= 100.f;

	int n = MIN(basic_apply_num, vec_basic_applys.size());
	for (int i = 0; i < n; i++)
	{	
		const SApply& basic_apply = vec_basic_applys[i];
		BYTE bType = basic_apply.apply_type;
		short sValue = (short)(ceil((float)basic_apply.apply_value * fWeight - 0.01f));

		pDS->SetForceAttribute(i, bType, sValue);
	}

	BYTE additional_attr_num = MIN(number (add_min, add_max), 3);

	std::vector <int> random_set;
	if (additional_attr_num > 0)
	{
		random_set.resize(additional_attr_num);
		std::list <float> list_probs;
		for (int i = 0; i < vec_addtional_applys.size(); i++)
		{
			list_probs.push_back(vec_addtional_applys[i].prob);
		}
		if (!MakeDistinctRandomNumberSet(list_probs, random_set))
		{
			sys_err ("MakeDistinctRandomNumberSet error.");
			return false;
		}

		for (int i = 0; i < additional_attr_num; i++)
		{
			int r = random_set[i];
			const SApply& additional_attr = vec_addtional_applys[r];
			BYTE bType = additional_attr.apply_type;
			short sValue = (short)(ceil((float)additional_attr.apply_value * fWeight - 0.01f));

			pDS->SetForceAttribute(DRAGON_SOUL_ADDITIONAL_ATTR_START_IDX + i, bType, sValue);
		}
	}

	return true;
}

bool DSManager::DragonSoulItemInitialize(LPITEM pItem)
{
	if (NULL == pItem || !pItem->IsDragonSoul())
		return false;
	PutAttributes(pItem);
	int time = DSManager::instance().GetDuration(pItem);
	if (time > 0)
		pItem->SetSocket(ITEM_SOCKET_REMAIN_SEC, time);
	return true;
}

DWORD DSManager::MakeDragonSoulVnum(BYTE bType, BYTE grade, BYTE step, BYTE refine)
{
	return bType * 10000 + grade * 1000 + step * 100 + refine * 10;
}

int DSManager::GetDuration(const LPITEM pItem) const
{
	return pItem->GetDuration();
}

// 용혼석을 받아서 용심을 추출하는 함수
bool DSManager::ExtractDragonHeart(LPCHARACTER ch, LPITEM pItem, LPITEM pExtractor)
{
	if (NULL == ch || NULL == pItem)
		return false;
	if (pItem->IsEquipped())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("착용 중인 용혼석은 추출할 수 없습니다."));
		return false;
	}

	DWORD dwVnum = pItem->GetVnum();
	BYTE ds_type, grade_idx, step_idx, strength_idx;
	GetDragonSoulInfo(dwVnum, ds_type, grade_idx, step_idx, strength_idx);

	int iBonus = 0;

	if (NULL != pExtractor)
	{
		iBonus = pExtractor->GetValue(0);
	}

	std::vector <float> vec_chargings;
	std::vector <float> vec_probs;

	if (!m_pTable->GetDragonHeartExtValues(ds_type, grade_idx, vec_chargings, vec_probs))
	{
		return false;
	}

	int idx = Gamble(vec_probs);

	float sum = 0.f;
	if (-1 == idx)
	{
		sys_err ("Gamble is failed. ds_type(%d), grade_idx(%d)", ds_type, grade_idx);
		return false;
	}

	float fCharge = vec_chargings[idx] * (100 + iBonus) / 100.f;
	fCharge = std::MINMAX <float> (0.f, fCharge, 100.f);

	if (fCharge < FLT_EPSILON)
	{
		pItem->SetCount(pItem->GetCount() - 1);
		if (NULL != pExtractor)
		{
			pExtractor->SetCount(pExtractor->GetCount() - 1);
		}
		LogManager::instance().ItemLog(ch, pItem, "DS_HEART_EXTRACT_FAIL", "");
	
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("용심 추출에 실패하였습니다."));
		return false;
	}
	else
	{
		LPITEM pDH = ITEM_MANAGER::instance().CreateItem(DRAGON_HEART_VNUM);
		
		if (NULL == pDH)
		{
			sys_err ("Cannot create DRAGON_HEART(%d).", DRAGON_HEART_VNUM);
			return NULL;
		}

		pItem->SetCount(pItem->GetCount() - 1);
		if (NULL != pExtractor)
		{
			pExtractor->SetCount(pExtractor->GetCount() - 1);
		}

		int iCharge = (int)(fCharge + 0.5f);
		pDH->SetSocket(ITEM_SOCKET_CHARGING_AMOUNT_IDX, iCharge);
		ch->AutoGiveItem(pDH, true);

		std::string s = boost::lexical_cast <std::string> (iCharge);
		s += "%s";
		LogManager::instance().ItemLog(ch, pItem, "DS_HEART_EXTRACT_SUCCESS", s.c_str());
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("용심 추출에 성공하였습니다."));
		return true;
	}
}

// 특정 용혼석을 장비창에서 제거할 때에 성공 여부를 결정하고, 실패시 부산물을 주는 함수.
bool DSManager::PullOut(LPCHARACTER ch, TItemPos DestCell, LPITEM& pItem, LPITEM pExtractor)
{
	if (NULL == ch || NULL == pItem)
	{
		sys_err ("NULL POINTER. ch(%p) or pItem(%p)", ch, pItem);
		return false;
	}

	// 목표 위치가 valid한지 검사 후, valid하지 않다면 임의의 빈 공간을 찾는다.
	if (!IsValidCellForThisItem(pItem, DestCell))
	{
		int iEmptyCell = ch->GetEmptyDragonSoulInventory(pItem);
		if (iEmptyCell < 0)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("소지품에 빈 공간이 없습니다."));
			return false;
		}
		else
		{
			DestCell.window_type = DRAGON_SOUL_INVENTORY;
			DestCell.cell = iEmptyCell;
		}
	}

	if (!pItem->IsEquipped() || !pItem->RemoveFromCharacter())
		return false;

	bool bSuccess;
	DWORD dwByProduct = 0;
	int iBonus = 0;
	float fProb;
	float fDice;
	// 용혼석 추출 성공 여부 결정.
	{
		DWORD dwVnum = pItem->GetVnum(); 

		BYTE ds_type, grade_idx, step_idx, strength_idx;
		GetDragonSoulInfo(pItem->GetVnum(), ds_type, grade_idx, step_idx, strength_idx);

		// 추출 정보가 없다면 일단 무조건 성공하는 것이라 생각하자.
		if (!m_pTable->GetDragonSoulExtValues(ds_type, grade_idx, fProb, dwByProduct))
		{
			pItem->AddToCharacter(ch, DestCell);
			return true;
		}


		if (NULL != pExtractor)
		{
			iBonus = pExtractor->GetValue(ITEM_VALUE_DRAGON_SOUL_POLL_OUT_BONUS_IDX);
			pExtractor->SetCount(pExtractor->GetCount() - 1);
		}
		fDice = fnumber(0.f, 100.f);
		bSuccess = fDice <= (fProb * (100 + iBonus) / 100.f);
	}

	// 캐릭터의 용혼석 추출 및 추가 혹은 제거. 부산물 제공.
	{
		char buf[128];

		if (bSuccess)
		{
			if (pExtractor)
			{
				sprintf(buf, "dice(%d) prob(%d + %d) EXTR(VN:%d)", (int)fDice, (int)fProb, iBonus, pExtractor->GetVnum());
			}
			else
			{
				sprintf(buf, "dice(%d) prob(%d)", fDice, fProb);
			}
			LogManager::instance().ItemLog(ch, pItem, "DS_PULL_OUT_SUCCESS", buf);
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("용혼석 추출에 성공하였습니다."));
			pItem->AddToCharacter(ch, DestCell);
			return true;
		}
		else
		{
			if (pExtractor)
			{
				sprintf(buf, "dice(%d) prob(%d + %d) EXTR(VN:%d) ByProd(VN:%d)", (int)fDice, (int)fProb, iBonus, pExtractor->GetVnum(), dwByProduct);
			}
			else
			{
				sprintf(buf, "dice(%d) prob(%d) ByProd(VNUM:%d)", (int)fDice, (int)fProb, dwByProduct);
			}
			LogManager::instance().ItemLog(ch, pItem, "DS_PULL_OUT_FAILED", buf);
			M2_DESTROY_ITEM(pItem);
			pItem = NULL;
			if (dwByProduct)
			{
				LPITEM pByProduct = ch->AutoGiveItem(dwByProduct, true);
				if (pByProduct)
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("용혼석 추출에 실패하여 %s를 얻었습니다."), pByProduct->GetName());
				else
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("용혼석 추출에 실패하였습니다."));
			}
			else
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("용혼석 추출에 실패하였습니다."));
		}
	}

	return bSuccess;
}

bool DSManager::DoRefineGrade(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE])
{
	if (NULL == ch)
		return false;

	if (NULL == aItemPoses)
	{
		return false;
	}

	if (!ch->DragonSoul_RefineWindow_CanRefine())
	{
		sys_err ("%s do not activate DragonSoulRefineWindow. But how can he come here?", ch->GetName());
		ch->ChatPacket(CHAT_TYPE_INFO, "[SYSTEM ERROR]You cannot upgrade dragon soul without refine window.");
		return false;
	}

	// 혹시나 모를 중복되는 item pointer 없애기 위해서 set 사용
	// 이상한 패킷을 보낼 경우, 중복된 TItemPos가 있을 수도 있고, 잘못된 TItemPos가 있을 수도 있다.
	std::set <LPITEM> set_items;
	for (int i = 0; i < DRAGON_SOUL_REFINE_GRID_SIZE; i++)
	{
		if (aItemPoses[i].IsEquipPosition())
			return false;
		LPITEM pItem = ch->GetItem(aItemPoses[i]);
		if (NULL != pItem)
		{
			// 용혼석이 아닌 아이템이 개량창에 있을 수 없다.
			if (!pItem->IsDragonSoul())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("등급 개량에 필요한 재료가 아닙니다."));
				SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));

				return false;
			}

			set_items.insert(pItem);
		}
	}

	if (set_items.size() == 0)
	{
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL, NPOS);
		return false;
	}

	int count = set_items.size();
	int need_count = 0;
	int fee = 0;
	std::vector <float> vec_probs;
	float prob_sum;

	BYTE ds_type, grade_idx, step_idx, strength_idx;
	int result_grade;

	// 가장 처음 것을 강화의 기준으로 삼는다.
	std::set <LPITEM>::iterator it = set_items.begin();
	{
		LPITEM pItem = *it;

		GetDragonSoulInfo(pItem->GetVnum(), ds_type, grade_idx, step_idx, strength_idx);
		
		if (!m_pTable->GetRefineGradeValues(ds_type, grade_idx, need_count, fee, vec_probs))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("등급 개량할 수 없는 용혼석입니다."));
			SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));

			return false;
		}
	}
	while (++it != set_items.end())
	{
		LPITEM pItem = *it;

		// 클라 ui에서 장착한 아이템은 개량창에 올릴 수 없도록 막았기 때문에,
		// 별도의 알림 처리는 안함.
		if (pItem->IsEquipped())
		{
			return false;
		}
		
		if (ds_type != GetType(pItem->GetVnum()) || grade_idx != GetGradeIdx(pItem->GetVnum()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("등급 개량에 필요한 재료가 아닙니다."));
			SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));

			return false;
		}
	}

	// 클라에서 한번 갯수 체크를 하기 때문에 count != need_count라면 invalid 클라일 가능성이 크다.
	if (count != need_count)
	{
		sys_err ("Possiblity of invalid client. Name %s", ch->GetName());
		BYTE bSubHeader = count < need_count? DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL : DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL;
		SendRefineResultPacket(ch, bSubHeader, NPOS);
		return false;
	}

	if (ch->GetGold() < fee)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개량을 하기 위한 돈이 부족합니다."));
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY, NPOS);
		return false;
	}
	
	if (-1 == (result_grade = Gamble(vec_probs)))
	{
		sys_err ("Gamble failed. See RefineGardeTables' probabilities");
		return false;
	}

	LPITEM pResultItem = ITEM_MANAGER::instance().CreateItem(MakeDragonSoulVnum(ds_type, (BYTE)result_grade, 0, 0));

	if (NULL == pResultItem)
	{
		sys_err ("INVALID DRAGON SOUL(%d)", MakeDragonSoulVnum(ds_type, (BYTE)result_grade, 0, 0));
		return false;
	}

	ch->PointChange(POINT_GOLD, -fee);
	int left_count = need_count;
	
	for (std::set <LPITEM>::iterator it = set_items.begin(); it != set_items.end(); it++)
	{
		LPITEM pItem = *it;
		int n = pItem->GetCount();
		if (left_count > n)
		{
			pItem->RemoveFromCharacter();
			M2_DESTROY_ITEM(pItem);
			left_count -= n;
		}
		else
		{
			pItem->SetCount(n - left_count);
		}
	}

	ch->AutoGiveItem(pResultItem, true);

	if (result_grade > grade_idx)
	{
		char buf[128];
		sprintf(buf, "GRADE : %d -> %d", grade_idx, result_grade);
		LogManager::instance().ItemLog(ch, pResultItem, "DS_GRADE_REFINE_SUCCESS", buf);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("등급 개량에 성공했습니다."));
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_SUCCEED, TItemPos (pResultItem->GetWindow(), pResultItem->GetCell()));
		return true;
	}
	else
	{
		char buf[128];
		sprintf(buf, "GRADE : %d -> %d", grade_idx, result_grade);
		LogManager::instance().ItemLog(ch, pResultItem, "DS_GRADE_REFINE_FAIL", buf);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("등급 개량에 실패했습니다."));
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL, TItemPos (pResultItem->GetWindow(), pResultItem->GetCell()));
		return false;
	}
}

bool DSManager::DoRefineStep(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE])
{
	if (NULL == ch)
		return false;
	if (NULL == aItemPoses)
	{
		return false;
	}

	if (!ch->DragonSoul_RefineWindow_CanRefine())
	{
		sys_err ("%s do not activate DragonSoulRefineWindow. But how can he come here?", ch->GetName());
		ch->ChatPacket(CHAT_TYPE_INFO, "[SYSTEM ERROR]You cannot use dragon soul refine window.");
		return false;
	}

	// 혹시나 모를 중복되는 item pointer 없애기 위해서 set 사용
	// 이상한 패킷을 보낼 경우, 중복된 TItemPos가 있을 수도 있고, 잘못된 TItemPos가 있을 수도 있다.
	std::set <LPITEM> set_items;
	for (int i = 0; i < DRAGON_SOUL_REFINE_GRID_SIZE; i++)
	{
		LPITEM pItem = ch->GetItem(aItemPoses[i]);
		if (NULL != pItem)
		{
			// 용혼석이 아닌 아이템이 개량창에 있을 수 없다.
			if (!pItem->IsDragonSoul())
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("단계 개량에 필요한 재료가 아닙니다."));
				SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));
				return false;
			}
			set_items.insert(pItem);
		}
	}

	if (set_items.size() == 0)
	{
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL, NPOS);
		return false;
	}

	std::string stGroupName;
	int count = set_items.size();
	int need_count = 0;
	int fee = 0;
	std::vector <float> vec_probs;

	BYTE ds_type, grade_idx, step_idx, strength_idx;
	int result_step;

	// 가장 처음 것을 강화의 기준으로 삼는다.
	std::set <LPITEM>::iterator it = set_items.begin(); 
	{
		LPITEM pItem = *it;
		GetDragonSoulInfo(pItem->GetVnum(), ds_type, grade_idx, step_idx, strength_idx);

		if (!m_pTable->GetRefineStepValues(ds_type, step_idx, need_count, fee, vec_probs))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("단계 개량할 수 없는 용혼석입니다."));
			SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));
			return false;
		}
	}

	while(++it != set_items.end())
	{
		LPITEM pItem = *it;
		// 클라 ui에서 장착한 아이템은 개량창에 올릴 수 없도록 막았기 때문에,
		// 별도의 알림 처리는 안함.
		if (pItem->IsEquipped())
		{
			return false;
		}
		if (ds_type != GetType(pItem->GetVnum()) || grade_idx != GetGradeIdx(pItem->GetVnum()) || step_idx != GetStepIdx(pItem->GetVnum()))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("단계 개량에 필요한 재료가 아닙니다."));
			SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));
			return false;
		}
	}

	// 클라에서 한번 갯수 체크를 하기 때문에 count != need_count라면 invalid 클라일 가능성이 크다.
	if (count != need_count)
	{
		sys_err ("Possiblity of invalid client. Name %s", ch->GetName());
		BYTE bSubHeader = count < need_count? DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL : DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL;
		SendRefineResultPacket(ch, bSubHeader, NPOS);
		return false;
	}
	
	if (ch->GetGold() < fee)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개량을 하기 위한 돈이 부족합니다."));
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY, NPOS);
		return false;
	}
	
	float sum = 0.f;

	if (-1 == (result_step = Gamble(vec_probs)))
	{
		sys_err ("Gamble failed. See RefineStepTables' probabilities");
		return false;
	}

	LPITEM pResultItem = ITEM_MANAGER::instance().CreateItem(MakeDragonSoulVnum(ds_type, grade_idx, (BYTE)result_step, 0));

	if (NULL == pResultItem)
	{
		sys_err ("INVALID DRAGON SOUL(%d)", MakeDragonSoulVnum(ds_type, grade_idx, (BYTE)result_step, 0));
		return false;
	}

	ch->PointChange(POINT_GOLD, -fee);
	int left_count = need_count;
	for (std::set <LPITEM>::iterator it = set_items.begin(); it != set_items.end(); it++)
	{
		LPITEM pItem = *it;
		int n = pItem->GetCount();
		if (left_count > n)
		{
			pItem->RemoveFromCharacter();
			M2_DESTROY_ITEM(pItem);
			left_count -= n;
		}
		else
		{
			pItem->SetCount(n - left_count);
		}
	}

	ch->AutoGiveItem(pResultItem, true);
	if (result_step > step_idx)
	{
		char buf[128];
		sprintf(buf, "STEP : %d -> %d", step_idx, result_step);
		LogManager::instance().ItemLog(ch, pResultItem, "DS_STEP_REFINE_SUCCESS", buf);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("단계 개량에 성공했습니다."));
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_SUCCEED, TItemPos (pResultItem->GetWindow(), pResultItem->GetCell()));
		return true;
	}
	else
	{
		char buf[128];
		sprintf(buf, "STEP : %d -> %d", step_idx, result_step);
		LogManager::instance().ItemLog(ch, pResultItem, "DS_STEP_REFINE_FAIL", buf);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("단계 개량에 실패했습니다."));
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL, TItemPos (pResultItem->GetWindow(), pResultItem->GetCell()));
		return false;
	}
}

bool IsDragonSoulRefineMaterial(LPITEM pItem)
{
	if (pItem->GetType() != ITEM_MATERIAL)
		return false;
	return (pItem->GetSubType() == MATERIAL_DS_REFINE_NORMAL ||
		pItem->GetSubType() == MATERIAL_DS_REFINE_BLESSED ||
		pItem->GetSubType() == MATERIAL_DS_REFINE_HOLLY);
}

bool DSManager::DoRefineStrength(LPCHARACTER ch, TItemPos (&aItemPoses)[DRAGON_SOUL_REFINE_GRID_SIZE])
{
	if (NULL == ch)
		return false;
	if (NULL == aItemPoses)
	{
		return false;
	}

	if (!ch->DragonSoul_RefineWindow_CanRefine())
	{
		sys_err ("%s do not activate DragonSoulRefineWindow. But how can he come here?", ch->GetName());
		ch->ChatPacket(CHAT_TYPE_INFO, "[SYSTEM ERROR]You cannot use dragon soul refine window.");
		return false;
	}

	// 혹시나 모를 중복되는 item pointer 없애기 위해서 set 사용
	// 이상한 패킷을 보낼 경우, 중복된 TItemPos가 있을 수도 있고, 잘못된 TItemPos가 있을 수도 있다.
	std::set <LPITEM> set_items;
	for (int i = 0; i < DRAGON_SOUL_REFINE_GRID_SIZE; i++)
	{
		LPITEM pItem = ch->GetItem(aItemPoses[i]);
		if (pItem)
		{
			set_items.insert(pItem);
		}
	}
	if (set_items.size() == 0)
	{
		return false;
	}

	int fee;

	LPITEM pRefineStone = NULL;
	LPITEM pDragonSoul = NULL;
	for (std::set <LPITEM>::iterator it = set_items.begin(); it != set_items.end(); it++)
	{
		LPITEM pItem = *it;
		// 클라 ui에서 장착한 아이템은 개량창에 올릴 수 없도록 막았기 때문에,
		// 별도의 알림 처리는 안함.
		if (pItem->IsEquipped())
		{
			return false;
		}

		// 용혼석과 강화석만이 개량창에 있을 수 있다.
		// 그리고 하나씩만 있어야한다.
		if (pItem->IsDragonSoul())
		{
			if (pDragonSoul != NULL)
			{
				SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));
				return false;	
			}
			pDragonSoul = pItem;
		}
		else if(IsDragonSoulRefineMaterial(pItem))
		{
			if (pRefineStone != NULL)
			{
				SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_TOO_MUCH_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));
				return false;	
			}
			pRefineStone = pItem;
		}
		else
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화에 필요한 재료가 아닙니다."));
			SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL, TItemPos(pItem->GetWindow(), pItem->GetCell()));
			return false;
		}
	}

	BYTE bType, bGrade, bStep, bStrength;
	
	if (!pDragonSoul || !pRefineStone)
	{
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MATERIAL, NPOS);

		return false;
	}
	
	if (NULL != pDragonSoul)
	{
		GetDragonSoulInfo(pDragonSoul->GetVnum(), bType, bGrade, bStep, bStrength);

		float fWeight = 0.f;
		// 가중치 값이 없다면 강화할 수 없는 용혼석
		if (!m_pTable->GetWeight(bType, bGrade, bStep, bStrength + 1, fWeight))
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화할 수 없는 용혼석입니다."));
			SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE, TItemPos(pDragonSoul->GetWindow(), pDragonSoul->GetCell()));
			return false;
		}
		// 강화했을 때 가중치가 0이라면 더 이상 강화되서는 안된다.
		if (fWeight < FLT_EPSILON)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화할 수 없는 용혼석입니다."));
			SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_MAX_REFINE, TItemPos(pDragonSoul->GetWindow(), pDragonSoul->GetCell()));
			return false;
		}
	}

	float fProb;
	if (!m_pTable->GetRefineStrengthValues(bType, pRefineStone->GetSubType(), bStrength, fee, fProb))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화할 수 없는 용혼석입니다."));
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_INVALID_MATERIAL, TItemPos(pDragonSoul->GetWindow(), pDragonSoul->GetCell()));

		return false;
	}

	if (ch->GetGold() < fee)
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("개량을 하기 위한 돈이 부족합니다."));
		SendRefineResultPacket(ch, DS_SUB_HEADER_REFINE_FAIL_NOT_ENOUGH_MONEY, NPOS);
		return false;
	}
	
	ch->PointChange(POINT_GOLD, -fee);
	LPITEM pResult = NULL;
	BYTE bSubHeader;

	if (fnumber(0.f, 100.f) <= fProb)
	{
		pResult = ITEM_MANAGER::instance().CreateItem(MakeDragonSoulVnum(bType, bGrade, bStep, bStrength + 1));
		if (NULL == pResult)
		{
			sys_err ("INVALID DRAGON SOUL(%d)", MakeDragonSoulVnum(bType, bGrade, bStep, bStrength + 1));
			return false;
		}
		pDragonSoul->RemoveFromCharacter();

		pDragonSoul->CopyAttributeTo(pResult);
		RefreshItemAttributes(pResult);

		pDragonSoul->SetCount(pDragonSoul->GetCount() - 1);
		pRefineStone->SetCount(pRefineStone->GetCount() - 1);

		char buf[128];
		sprintf(buf, "STRENGTH : %d -> %d", bStrength, bStrength + 1);
		LogManager::instance().ItemLog(ch, pDragonSoul, "DS_STRENGTH_REFINE_SUCCESS", buf);
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화에 성공했습니다."));
		ch->AutoGiveItem(pResult, true);
		bSubHeader = DS_SUB_HEADER_REFINE_SUCCEED;
	}
	else
	{
		if (bStrength != 0)
		{
			pResult = ITEM_MANAGER::instance().CreateItem(MakeDragonSoulVnum(bType, bGrade, bStep, bStrength - 1));
			if (NULL == pResult)
			{
				sys_err ("INVALID DRAGON SOUL(%d)", MakeDragonSoulVnum(bType, bGrade, bStep, bStrength - 1));
				return false;
			}
			pDragonSoul->CopyAttributeTo(pResult);
			RefreshItemAttributes(pResult);
		}
		bSubHeader = DS_SUB_HEADER_REFINE_FAIL;

		char buf[128];
		sprintf(buf, "STRENGTH : %d -> %d", bStrength, bStrength - 1);
		// strength강화는 실패시 깨질 수도 있어, 원본 아이템을 바탕으로 로그를 남김.
		LogManager::instance().ItemLog(ch, pDragonSoul, "DS_STRENGTH_REFINE_FAIL", buf);

		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("강화에 실패했습니다."));
		pDragonSoul->SetCount(pDragonSoul->GetCount() - 1);
		pRefineStone->SetCount(pRefineStone->GetCount() - 1);
		if (NULL != pResult)
			ch->AutoGiveItem(pResult, true);
		
	}
	
	SendRefineResultPacket(ch, bSubHeader, NULL == pResult? NPOS : TItemPos (pResult->GetWindow(), pResult->GetCell()));

	return true;
}

void DSManager::SendRefineResultPacket(LPCHARACTER ch, BYTE bSubHeader, const TItemPos& pos)
{
	TPacketGCDragonSoulRefine pack;
	pack.bSubType = bSubHeader;

	if (pos.IsValidItemPosition())
	{
		pack.Pos = pos;
	}
	LPDESC d = ch->GetDesc();
	if (NULL == d)
	{
		return ;
	}
	else
	{
		d->Packet(&pack, sizeof(pack));
	}
}

int DSManager::LeftTime(LPITEM pItem) const
{
	if (pItem == NULL)
		return false;

	// 일단은 timer based on wear인 용혼석만 시간 다 되어도 안 없어진다.
	if (pItem->GetProto()->cLimitTimerBasedOnWearIndex >= 0)
	{
		return pItem->GetSocket(ITEM_SOCKET_REMAIN_SEC);
	}
	// 다른 limit type인 용혼석들은 시간 되면 모두 사라지기 때문에 여기 들어온 아이템은 일단 시간이 남았다고 판단.
	else
	{
		return INT_MAX;
	}
}

bool DSManager::IsTimeLeftDragonSoul(LPITEM pItem) const
{
	if (pItem == NULL)
		return false;

	// 일단은 timer based on wear인 용혼석만 시간 다 되어도 안 없어진다.
	if (pItem->GetProto()->cLimitTimerBasedOnWearIndex >= 0)
	{
		return pItem->GetSocket(ITEM_SOCKET_REMAIN_SEC) > 0;
	}
	// 다른 limit type인 용혼석들은 시간 되면 모두 사라지기 때문에 여기 들어온 아이템은 일단 시간이 남았다고 판단.
	else
	{
		return true;
	}
}

bool DSManager::IsActiveDragonSoul(LPITEM pItem) const
{
	return pItem->GetSocket(ITEM_SOCKET_DRAGON_SOUL_ACTIVE_IDX);
}

bool DSManager::ActivateDragonSoul(LPITEM pItem)
{
	if (NULL == pItem)
		return false;
	LPCHARACTER pOwner = pItem->GetOwner();
	if (NULL == pOwner)
		return false;

	int deck_idx = pOwner->DragonSoul_GetActiveDeck();
	
	if (deck_idx < 0)
		return false;
	
	if (INVENTORY_MAX_NUM + WEAR_MAX_NUM + DS_SLOT_MAX * deck_idx <= pItem->GetCell() &&
			pItem->GetCell() < INVENTORY_MAX_NUM + WEAR_MAX_NUM + DS_SLOT_MAX * (deck_idx + 1))
	{
		if (IsTimeLeftDragonSoul(pItem) && !IsActiveDragonSoul(pItem))
		{
			char buf[128];
			sprintf (buf, "LEFT TIME(%d)", LeftTime(pItem));
			LogManager::instance().ItemLog(pOwner, pItem, "DS_ACTIVATE", buf);
			pItem->ModifyPoints(true);
			pItem->SetSocket(ITEM_SOCKET_DRAGON_SOUL_ACTIVE_IDX, 1);

			pItem->StartTimerBasedOnWearExpireEvent();
		}
		return true;
	}
	else
		return false;
}

bool DSManager::DeactivateDragonSoul(LPITEM pItem, bool bSkipRefreshOwnerActiveState)
{
	if (NULL == pItem)
		return false;

	LPCHARACTER pOwner = pItem->GetOwner();
	if (NULL == pOwner)
		return false;

	if (!IsActiveDragonSoul(pItem))
		return false;

	char buf[128];
	pItem->StopTimerBasedOnWearExpireEvent();
	pItem->SetSocket(ITEM_SOCKET_DRAGON_SOUL_ACTIVE_IDX, 0);
	pItem->ModifyPoints(false);

	sprintf (buf, "LEFT TIME(%d)", LeftTime(pItem));
	LogManager::instance().ItemLog(pOwner, pItem, "DS_DEACTIVATE", buf);

	if (false == bSkipRefreshOwnerActiveState)
		RefreshDragonSoulState(pOwner);

	return true;
}

void DSManager::RefreshDragonSoulState(LPCHARACTER ch)
{
	if (NULL == ch)
		return ;
	for (int i = WEAR_MAX_NUM; i < WEAR_MAX_NUM + DS_SLOT_MAX * DRAGON_SOUL_DECK_MAX_NUM; i++)
	{
		LPITEM pItem = ch->GetWear(i);
		if (pItem != NULL)
		{
			if(IsActiveDragonSoul(pItem))
			{
				return;
			}
		}
	}
	ch->DragonSoul_DeactivateAll();
}

DSManager::DSManager()
{
	m_pTable = NULL;
}

DSManager::~DSManager()
{
	if (m_pTable)
		delete m_pTable;
}
