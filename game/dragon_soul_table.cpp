#include "stdafx.h"
#include "constants.h"
#include "group_text_parse_tree.h"
#include "dragon_soul_table.h"
#include "item_manager.h"
#include <boost/lexical_cast.hpp>
const std::string g_astGradeName[] =
{
	"grade_normal",
	"grade_brilliant",
	"grade_rare",
	"grade_ancient",
	"grade_legendary",
};

const std::string g_astStepName[] =
{
	"step_lowest",
	"step_low",
	"step_mid",
	"step_high",
	"step_highest",
};

const std::string g_astMaterialName[] = 
{
	"material_leather",
	"material_blood",
	"material_root",
	"material_needle",
	"material_jewel",
	"material_ds_refine_normal", 
	"material_ds_refine_blessed", 
	"material_ds_refine_holly",
};

bool DragonSoulTable::ReadDragonSoulTableFile(const char * c_pszFileName)
{
	m_pLoader = new CGroupTextParseTreeLoader;
	CGroupTextParseTreeLoader& loader = *m_pLoader;

	if (false == loader.Load(c_pszFileName))
	{
		sys_err ("dragon_soul_table.txt load error");
		return false;
	}

	// Group VnumMapper Reading.
	if (!ReadVnumMapper())
		return false;

	// Group BasicApplys Reading.
	if (!ReadBasicApplys())
		return false;

	// Group AdditionalApplys Reading.
	if (!ReadAdditionalApplys())
		return false;

	m_pApplyNumSettingNode = loader.GetGroup("applynumsettings");
	m_pWeightTableNode = loader.GetGroup("weighttables");
	m_pRefineGradeTableNode = loader.GetGroup("refinegradetables");
	m_pRefineStepTableNode = loader.GetGroup("refinesteptables");
	m_pRefineStrengthTableNode = loader.GetGroup("refinestrengthtables");
	m_pDragonHeartExtTableNode = loader.GetGroup("dragonheartexttables");
	m_pDragonSoulExtTableNode = loader.GetGroup("dragonsoulexttables");

	if (CheckApplyNumSettings()
		&& CheckWeightTables()
		&& CheckRefineGradeTables()
		&& CheckRefineStepTables()
		&& CheckRefineStrengthTables()
		&& CheckDragonHeartExtTables()
		&& CheckDragonSoulExtTables())
	{
		return true;
	}
	else
	{
		sys_err ("DragonSoul table Check failed.");
		return false;
	}
}

bool DragonSoulTable::GetDragonSoulGroupName(BYTE bType, std::string& stGroupName) const
{
	DragonSoulTable::TMapTypeToName::const_iterator it = m_map_type_to_name.find (bType);
	if (it != m_map_type_to_name.end())
	{
		stGroupName = it->second;
		return true;
	}
	else
	{
		sys_err ("Invalid DragonSoul Type(%d)", bType);
		return false;
	}
}

bool DragonSoulTable::ReadVnumMapper()
{
	std::string stName;

	// Group VnumMapper Reading.
	CGroupNode* pGroupNode = m_pLoader->GetGroup("vnummapper");

	if (NULL == pGroupNode)
	{
		sys_err ("dragon_soul_table.txt need VnumMapper.");
		return false;
	}
	{
		int n = pGroupNode->GetRowCount();
		if (0 == n)
		{
			sys_err	("Group VnumMapper is Empty.");
			return false;
		}

		std::set <BYTE> setTypes;

		for (int i = 0; i < n; i++)
		{
			const CGroupNode::CGroupNodeRow* pRow;
			pGroupNode->GetRow(i, &pRow);
			
			std::string stDragonSoulName;
			BYTE bType;
			if (!pRow->GetValue("dragonsoulname", stDragonSoulName))
			{
				sys_err ("In Group VnumMapper, No DragonSoulName column.");
				return false;
			}
			if (!pRow->GetValue("type", bType))
			{
				sys_err ("In Group VnumMapper, %s's Vnum is invalid", stDragonSoulName.c_str());
				return false;
			}

			if (setTypes.end() != setTypes.find(bType))
			{
				sys_err ("In Group VnumMapper, duplicated vnum exist.");
				return false;
			}
			m_map_name_to_type.insert(TMapNameToType::value_type(stDragonSoulName, bType));
			m_map_type_to_name.insert(TMapTypeToName::value_type(bType, stDragonSoulName));
			m_vecDragonSoulTypes.push_back(bType);
			m_vecDragonSoulNames.push_back(stDragonSoulName);
		}
	}
	return true;
}

bool DragonSoulTable::ReadBasicApplys()
{
	CGroupNode* pGroupNode = m_pLoader->GetGroup("basicapplys");

	if (NULL == pGroupNode)
	{
		sys_err ("dragon_soul_table.txt need BasicApplys.");
		return false;
	}

	for (uint i = 0; i < m_vecDragonSoulNames.size(); i++)
	{
		CGroupNode* pChild;
		if (NULL == (pChild = pGroupNode->GetChildNode(m_vecDragonSoulNames[i])))
		{
			sys_err ("In Group BasicApplys, %s group is not defined.", m_vecDragonSoulNames[i].c_str());
			return false;
		}
		TVecApplys vecApplys;
		int n = pChild->GetRowCount();
		
		// BasicApply Group은 Key가 1부터 시작함.
		for (int j = 1; j <= n; j++)
		{
			std::stringstream ss;
			ss << j;
			const CGroupNode::CGroupNodeRow* pRow = NULL;

			pChild->GetRow(ss.str(), &pRow);
			if (NULL == pRow)
			{
				sys_err("In Group BasicApplys, No %d row.", j);
			}
			EApplyTypes at;
			int av;

			std::string stTypeName;
			if (!pRow->GetValue("apply_type", stTypeName))
			{
				sys_err ("In Group BasicApplys, %s group's apply_type is empty.", m_vecDragonSoulNames[i].c_str());
				return false;
			}
			if (!(at = (EApplyTypes)FN_get_apply_type(stTypeName.c_str())))
			{
				sys_err ("In Group BasicApplys, %s group's apply_type %s is invalid.", m_vecDragonSoulNames[i].c_str(), stTypeName.c_str());
				return false;
			}
			if (!pRow->GetValue("apply_value", av))
			{
				sys_err ("In Group BasicApplys, %s group's apply_value %s is invalid.", m_vecDragonSoulNames[i].c_str(), av);
				return false;
			}
			vecApplys.push_back(SApply(at, av));
		}
		m_map_basic_applys_group.insert (TMapApplyGroup::value_type (m_map_name_to_type[m_vecDragonSoulNames[i]], vecApplys));
	}

	return true;
}

bool DragonSoulTable::ReadAdditionalApplys()
{
	CGroupNode* pGroupNode = m_pLoader->GetGroup("additionalapplys");
	if (NULL == pGroupNode)
	{
		sys_err ("dragon_soul_table.txt need AdditionalApplys.");
		return false;
	}

	for (int i = 0; i < m_vecDragonSoulNames.size(); i++)
	{
		CGroupNode* pChild;
		if (NULL == (pChild = pGroupNode->GetChildNode(m_vecDragonSoulNames[i])))
		{
			sys_err ("In Group AdditionalApplys, %s group is not defined.", m_vecDragonSoulNames[i].c_str());
			return false;
		}
		TVecApplys vecApplys;
		
		int n = pChild->GetRowCount();

		for (int j = 0; j < n; j++)
		{
			const CGroupNode::CGroupNodeRow* pRow;

			pChild->GetRow(j, &pRow);

			EApplyTypes at;
			int av;
			float prob;
			std::string stTypeName;
			if (!pRow->GetValue("apply_type", stTypeName))
			{
				sys_err ("In Group AdditionalApplys, %s group's apply_type is empty.", m_vecDragonSoulNames[i].c_str());
				return false;
			}
			if (!(at = (EApplyTypes)FN_get_apply_type(stTypeName.c_str())))
			{
				sys_err ("In Group AdditionalApplys, %s group's apply_type %s is invalid.", m_vecDragonSoulNames[i].c_str(), stTypeName.c_str());
				return false;
			}
			if (!pRow->GetValue("apply_value", av))
			{
				sys_err ("In Group AdditionalApplys, %s group's apply_value %s is invalid.", m_vecDragonSoulNames[i].c_str(), av);
				return false;
			}
			if (!pRow->GetValue("prob", prob))
			{
				sys_err ("In Group AdditionalApplys, %s group's probability %s is invalid.", m_vecDragonSoulNames[i].c_str(), prob);
				return false;
			}
			vecApplys.push_back(SApply(at, av, prob));
		}
		m_map_additional_applys_group.insert (TMapApplyGroup::value_type (m_map_name_to_type[m_vecDragonSoulNames[i]], vecApplys));
	}

	return true;
}

bool DragonSoulTable::CheckApplyNumSettings ()
{
	// Group ApplyNumSettings Reading.
	if (NULL == m_pApplyNumSettingNode)
	{
		sys_err ("dragon_soul_table.txt need ApplyNumSettings.");
		return false;
	}
	else
	{
		for (int i = 0; i < m_vecDragonSoulTypes.size(); i++)
		{
			for (int j = 0; j < DRAGON_SOUL_GRADE_MAX; j++)
			{
				int basis, add_min, add_max;
				if (!GetApplyNumSettings(m_vecDragonSoulTypes[i], j, basis, add_min, add_max))
				{
					sys_err ("In %s group of ApplyNumSettings, values in Grade(%s) row is invalid.", 
						m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str());
					return false;
				}
			}
		}
	}

	return true;
}

bool DragonSoulTable::CheckWeightTables ()
{
	// Group WeightTables Reading.
	if (NULL == m_pWeightTableNode)
	{
		sys_err ("dragon_soul_table.txt need WeightTables.");
		return false;
	}
	else
	{
		for (int i = 0; i < m_vecDragonSoulTypes.size(); i++)
		{
			for (int j = 0; j < DRAGON_SOUL_GRADE_MAX; j++)
			{
				for (int k = 0; k < DRAGON_SOUL_STEP_MAX; k++)
				{
					for (int l = 0; l < DRAGON_SOUL_STRENGTH_MAX; l++)
					{
						float fWeight;
						if (!GetWeight(m_vecDragonSoulTypes[i], j, k, l, fWeight))
						{
							sys_err ("In %s group of WeightTables, value(Grade(%s), Step(%s), Strength(%d) is invalid.", 
								m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str(), g_astStepName[k].c_str(), l);
						}
					}
				}
			}
		}
	}
	return true;
}

bool DragonSoulTable::CheckRefineGradeTables()
{
	// Group UpgradeTables Reading.
	if (NULL == m_pRefineGradeTableNode)
	{
		sys_err ("dragon_soul_table.txt need RefineGradeTables.");
		return false;
	}
	else
	{
		for (int i = 0; i < m_vecDragonSoulTypes.size(); i++)
		{
			for (int j = 0; j < DRAGON_SOUL_GRADE_MAX - 1; j++)
			{
				int need_count, fee;
				std::vector <float> vec_probs;
				if (!GetRefineGradeValues(m_vecDragonSoulTypes[i], j, need_count, fee, vec_probs))
				{
					sys_err ("In %s group of RefineGradeTables, values in Grade(%s) row is invalid.", 
						m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str());
					return false;
				}
				if (need_count < 1)
				{
					sys_err ("In %s group of RefineGradeTables, need_count of Grade(%s) is less than 1.", 
						m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str());
					return false;
				}
				if (fee < 0)
				{
					sys_err ("In %s group of RefineGradeTables, fee of Grade(%s) is less than 0.", 
						m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str());
					return false;
				}
				if (DRAGON_SOUL_GRADE_MAX != vec_probs.size())
				{
					sys_err ("In %s group of RefineGradeTables, probability list size is not %d.", DRAGON_SOUL_GRADE_MAX);
					return false;
				}
				for (int k = 0; k < vec_probs.size(); k++)
				{
					if (vec_probs[k] < 0.f)
					{
						sys_err ("In %s group of RefineGradeTables, probability(index : %d) is less than 0.", k);
						return false;
					}
				}
			}
		}
	}

	return true;
}

bool DragonSoulTable::CheckRefineStepTables ()
{
	// Group ImproveTables Reading.
	if (NULL == m_pRefineStrengthTableNode)
	{
		sys_err ("dragon_soul_table.txt need RefineStepTables.");
		return false;
	}
	else
	{
		for (int i = 0; i < m_vecDragonSoulTypes.size(); i++)
		{
			for (int j = 0; j < DRAGON_SOUL_STEP_MAX - 1; j++)
			{
				int need_count, fee;
				std::vector <float> vec_probs;
				if (!GetRefineStepValues(m_vecDragonSoulTypes[i], j, need_count, fee, vec_probs))
				{
					sys_err ("In %s group of RefineStepTables, values in Step(%s) row is invalid.", 
						m_vecDragonSoulNames[i].c_str(), g_astStepName[j].c_str());
					return false;
				}
				if (need_count < 1)
				{
					sys_err ("In %s group of RefineStepTables, need_count of Step(%s) is less than 1.", 
						m_vecDragonSoulNames[i].c_str(), g_astStepName[j].c_str());
					return false;
				}
				if (fee < 0)
				{
					sys_err ("In %s group of RefineStepTables, fee of Step(%s) is less than 0.", 
						m_vecDragonSoulNames[i].c_str(), g_astStepName[j].c_str());
					return false;
				}
				if (DRAGON_SOUL_GRADE_MAX != vec_probs.size())
				{
					sys_err ("In %s group of RefineStepTables, probability list size is not %d.", 
						m_vecDragonSoulNames[i].c_str(), DRAGON_SOUL_GRADE_MAX);
					return false;
				}
				for (int k = 0; k < vec_probs.size(); k++)
				{
					if (vec_probs[k] < 0.f)
					{
						sys_err ("In %s group of RefineStepTables, probability(index : %d) is less than 0.", 
							m_vecDragonSoulNames[i].c_str(), k);
						return false;
					}
				}
			}
		}
	}

	return true;
}


bool DragonSoulTable::CheckRefineStrengthTables()
{
	CGroupNode* pGroupNode = m_pRefineStrengthTableNode;
	// Group RefineTables Reading.
	if (NULL == pGroupNode)
	{
		sys_err ("dragon_soul_table.txt need RefineStrengthTables.");
		return false;
	}
	for (int i = 0; i < m_vecDragonSoulTypes.size(); i++)
	{
		for (int j = MATERIAL_DS_REFINE_NORMAL; j <= MATERIAL_DS_REFINE_HOLLY; j++)
		{
			int fee;
			float prob;
			for (int k = 0; k < DRAGON_SOUL_STRENGTH_MAX -1; k++)
			{
				if (!GetRefineStrengthValues(m_vecDragonSoulTypes[i], j, k, fee, prob))
				{
					sys_err ("In %s group of RefineStrengthTables, value(Material(%s), Strength(%d)) or fee are invalid.", 
						m_vecDragonSoulNames[i].c_str(), g_astMaterialName[j].c_str(), k);
					return false;
				}
				if (fee < 0)
				{
					sys_err ("In %s group of RefineStrengthTables, fee of Material(%s) is less than 0.", 
						m_vecDragonSoulNames[i].c_str(), g_astMaterialName[j].c_str());
					return false;
				}
				if (prob < 0.f)
				{
					sys_err ("In %s group of RefineStrengthTables, probability(Material(%s), Strength(%d)) is less than 0.", 
						m_vecDragonSoulNames[i].c_str(), g_astMaterialName[j].c_str(), k);
					return false;
				}
			}
		}
	}

	return true;
}

bool DragonSoulTable::CheckDragonHeartExtTables()
{
	// Group DragonHeartExtTables Reading.
	if (NULL == m_pDragonHeartExtTableNode)
	{
		sys_err ("dragon_soul_table.txt need DragonHeartExtTables.");
		return false;
	}
	for (int i = 0; i < m_vecDragonSoulTypes.size(); i++)
	{
		for (int j = 0; j < DRAGON_SOUL_GRADE_MAX; j++)
		{
			std::vector <float> vec_chargings;
			std::vector <float> vec_probs;

			if (!GetDragonHeartExtValues(m_vecDragonSoulTypes[i], j, vec_chargings, vec_probs))
			{
				sys_err ("In %s group of DragonHeartExtTables, CHARGING row or Grade(%s) row are invalid.", 
					m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str());
				return false;
			}
			if (vec_chargings.size() != vec_probs.size())
			{
				sys_err ("In %s group of DragonHeartExtTables, CHARGING row size(%d) are not equal Grade(%s) row size(%d).", 
					m_vecDragonSoulNames[i].c_str(), vec_chargings.size(), vec_probs.size());
				return false;
			}
			for (int k = 0; k < vec_chargings.size(); k++)
			{
				if (vec_chargings[k] < 0.f)
				{
					sys_err ("In %s group of DragonHeartExtTables, CHARGING value(index : %d) is less than 0", 
						m_vecDragonSoulNames[i].c_str(), k);
					return false;
				}
			}
			for (int k = 0; k < vec_probs.size(); k++)
			{
				if (vec_probs[k] < 0.f)
				{
					sys_err ("In %s group of DragonHeartExtTables, Probability(Grade : %s, index : %d) is less than 0", 
						m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str(), k);
					return false;
				}
			}
		}
	}

	return true;
}

bool DragonSoulTable::CheckDragonSoulExtTables()
{
	// Group DragonSoulExtTables Reading.
	if (NULL == m_pDragonSoulExtTableNode)
	{
		sys_err ("dragon_soul_table.txt need DragonSoulExtTables.");
		return false;
	}
	for (int i = 0; i < m_vecDragonSoulTypes.size(); i++)
	{
		for (int j = 0; j < DRAGON_SOUL_GRADE_MAX; j++)
		{
			float prob;
			DWORD by_product;
			if (!GetDragonSoulExtValues(m_vecDragonSoulTypes[i], j, prob, by_product))
			{
				sys_err ("In %s group of DragonSoulExtTables, Grade(%s) row is invalid.", 
					m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str());
				return false;
			}
			if (prob < 0.f)
			{
				sys_err ("In %s group of DragonSoulExtTables, Probability(Grade : %s) is less than 0", 
					m_vecDragonSoulNames[i].c_str(), g_astGradeName[j].c_str());
				return false;
			}
			if (0 != by_product && NULL == ITEM_MANAGER::instance().GetTable(by_product))
			{
				sys_err ("In %s group of DragonSoulExtTables, ByProduct(%d) of Grade %s is not exist.", 
					m_vecDragonSoulNames[i].c_str(), by_product, g_astGradeName[j].c_str());
				return false;
			}
		}
	}
	return true;
}

bool DragonSoulTable::GetBasicApplys(BYTE ds_type, OUT TVecApplys& vec_basic_applys)
{
	TMapApplyGroup::iterator it = m_map_basic_applys_group.find(ds_type);
	if (m_map_basic_applys_group.end() == it)
	{
		return false;
	}
	vec_basic_applys = it->second;
	return true;
}

bool DragonSoulTable::GetAdditionalApplys(BYTE ds_type, OUT TVecApplys& vec_additional_applys)
{
	TMapApplyGroup::iterator it = m_map_additional_applys_group.find(ds_type);
	if (m_map_additional_applys_group.end() == it)
	{
		return false;
	}
	vec_additional_applys = it->second;
	return true;
}

bool DragonSoulTable::GetApplyNumSettings(BYTE ds_type, BYTE grade_idx, OUT int& basis, OUT int& add_min, OUT int& add_max)
{
	if (grade_idx >= DRAGON_SOUL_GRADE_MAX)
	{
		sys_err ("Invalid dragon soul grade_idx(%d).", grade_idx);
		return false;
	}

	std::string stValue;
	std::string stDragonSoulName;
	if (!GetDragonSoulGroupName(ds_type, stDragonSoulName))
	{
		sys_err ("Invalid dragon soul type(%d).", ds_type);
		return false;
	}

	if (!m_pApplyNumSettingNode->GetGroupValue(stDragonSoulName, "basis", g_astGradeName[grade_idx], basis))
	{
		sys_err ("Invalid basis value. DragonSoulGroup(%s) Grade(%s)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}

	if (!m_pApplyNumSettingNode->GetGroupValue(stDragonSoulName, "add_min", g_astGradeName[grade_idx], add_min))
	{
		sys_err ("Invalid add_min value. DragonSoulGroup(%s) Grade(%s)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}

	if (!m_pApplyNumSettingNode->GetGroupValue(stDragonSoulName, "add_max", g_astGradeName[grade_idx], add_max))
	{
		sys_err ("Invalid add_max value. DragonSoulGroup(%s) Grade(%s)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}

	return true;
}

bool DragonSoulTable::GetWeight(BYTE ds_type, BYTE grade_idx, BYTE step_index, BYTE strength_idx, OUT float& fWeight)
{
	if (grade_idx >= DRAGON_SOUL_GRADE_MAX || step_index >= DRAGON_SOUL_STEP_MAX || strength_idx >= DRAGON_SOUL_STRENGTH_MAX)
	{
		sys_err ("Invalid dragon soul grade_idx(%d) step_index(%d) strength_idx(%d).", grade_idx, step_index, strength_idx);
		return false;
	}

	std::string stValue;
	std::string stDragonSoulName;
	if (!GetDragonSoulGroupName(ds_type, stDragonSoulName))
	{
		sys_err ("Invalid dragon soul type(%d).", ds_type);
		return false;
	}

	CGroupNode* pDragonSoulGroup = m_pWeightTableNode->GetChildNode(stDragonSoulName);
	if (NULL != pDragonSoulGroup)
	{
		if (pDragonSoulGroup->GetGroupValue(g_astGradeName[grade_idx], g_astStepName[step_index], strength_idx, fWeight))
		{
			return true;
		}
	}
	// default group을 살펴봄.
	pDragonSoulGroup = m_pWeightTableNode->GetChildNode("default");
	if (NULL != pDragonSoulGroup)
	{
		if (!pDragonSoulGroup->GetGroupValue(g_astGradeName[grade_idx], g_astStepName[step_index], strength_idx, fWeight))
		{
			sys_err ("Invalid float. DragonSoulGroup(%s) Grade(%s) Row(%s) Col(%d))", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str(), g_astStepName[step_index].c_str(), strength_idx);
			return false;
		}
		else
			return true;
	}
	sys_err ("Invalid value. DragonSoulGroup(%s) Grade(%s) Row(%s) Col(%d))", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str(), g_astStepName[step_index].c_str(), strength_idx);
	return false;
}

bool DragonSoulTable::GetRefineGradeValues(BYTE ds_type, BYTE grade_idx, OUT int& need_count, OUT int& fee, OUT std::vector<float>& vec_probs)
{
	if (grade_idx >= DRAGON_SOUL_GRADE_MAX -1)
	{
		sys_err ("Invalid dragon soul grade_idx(%d).", grade_idx);
		return false;
	}

	std::string stValue;
	std::string stDragonSoulName;
	if (!GetDragonSoulGroupName(ds_type, stDragonSoulName))
	{
		sys_err ("Invalid dragon soul type(%d).", ds_type);
		return false;
	}

	const CGroupNode::CGroupNodeRow * pRow;
	if (!m_pRefineGradeTableNode->GetGroupRow(stDragonSoulName, g_astGradeName[grade_idx], &pRow))
	{
		sys_err ("Invalid row. DragonSoulGroup(%s) Grade(%s)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}
	
	if (!pRow->GetValue("need_count", need_count))
	{
		sys_err ("Invalid value. DragonSoulGroup(%s) Grade(%s) Col(need_count)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}

	if (!pRow->GetValue("fee", fee))
	{
		sys_err ("Invalid value. DragonSoulGroup(%s) Grade(%s) Col(fee)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}

	vec_probs.resize(DRAGON_SOUL_GRADE_MAX);
	for (int i = 0; i < DRAGON_SOUL_GRADE_MAX; i++)
	{
		if (!pRow->GetValue(g_astGradeName[i], vec_probs[i]))
		{
			sys_err ("Invalid value. DragonSoulGroup(%s) Grade(%s) Col(%s)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str(), g_astGradeName[i].c_str());
			return false;
		}
	}

	return true;
}

bool DragonSoulTable::GetRefineStepValues(BYTE ds_type, BYTE step_idx, OUT int& need_count, OUT int& fee, OUT std::vector<float>& vec_probs)
{
	if (step_idx >= DRAGON_SOUL_STEP_MAX - 1)
	{
		sys_err ("Invalid dragon soul step_idx(%d).", step_idx);
		return false;
	}

	std::string stValue;
	std::string stDragonSoulName;
	if (!GetDragonSoulGroupName(ds_type, stDragonSoulName))
	{
		sys_err ("Invalid dragon soul type(%d).", ds_type);
		return false;
	}

	const CGroupNode::CGroupNodeRow * pRow;
	if (!m_pRefineStepTableNode->GetGroupRow(stDragonSoulName, g_astStepName[step_idx], &pRow))
	{
		sys_err ("Invalid row. DragonSoulGroup(%s) Step(%s)", stDragonSoulName.c_str(), g_astStepName[step_idx].c_str());
		return false;
	}

	if (!pRow->GetValue("need_count", need_count))
	{
		sys_err ("Invalid value. DragonSoulGroup(%s) Step(%s) Col(need_count)", stDragonSoulName.c_str(), g_astStepName[step_idx].c_str());
		return false;
	}

	if (!pRow->GetValue("fee", fee))
	{
		sys_err ("Invalid value. DragonSoulGroup(%s) Step(%s) Col(fee)", stDragonSoulName.c_str(), g_astStepName[step_idx].c_str());
		return false;
	}

	vec_probs.resize(DRAGON_SOUL_STEP_MAX);
	for (int i = 0; i < DRAGON_SOUL_STEP_MAX; i++)
	{
		if (!pRow->GetValue(g_astStepName[i], vec_probs[i]))
		{
			sys_err ("Invalid value. DragonSoulGroup(%s) Step(%s) Col(%s)", stDragonSoulName.c_str(), g_astStepName[step_idx].c_str(), g_astStepName[i].c_str());
			return false;
		}
	}

	return true;
}

bool DragonSoulTable::GetRefineStrengthValues(BYTE ds_type, BYTE material_type, BYTE strength_idx, OUT int& fee, OUT float& prob)
{
	if (material_type < MATERIAL_DS_REFINE_NORMAL || material_type > MATERIAL_DS_REFINE_HOLLY)
	{
		sys_err ("Invalid dragon soul material_type(%d).", material_type);
		return false;
	}

	std::string stValue;
	std::string stDragonSoulName;
	if (!GetDragonSoulGroupName(ds_type, stDragonSoulName))
	{
		sys_err ("Invalid dragon soul type(%d).", ds_type);
		return false;
	}

	if (!m_pRefineStrengthTableNode->GetGroupValue(stDragonSoulName, g_astMaterialName[material_type], "fee", fee))
	{
		sys_err ("Invalid fee. DragonSoulGroup(%s) Material(%s)", 
			stDragonSoulName.c_str(), g_astMaterialName[material_type].c_str());
		return false;
	}
	std::string stStrengthIdx = boost::lexical_cast <std::string> ((int)strength_idx);

	if (!m_pRefineStrengthTableNode->GetGroupValue(stDragonSoulName, g_astMaterialName[material_type], stStrengthIdx, prob))
	{
		sys_err ("Invalid prob. DragonSoulGroup(%s) Material(%s) Strength(%d)", 
			stDragonSoulName.c_str(), g_astMaterialName[material_type].c_str(), strength_idx);
		return false;
	}

	return true;
}

bool DragonSoulTable::GetDragonHeartExtValues(BYTE ds_type, BYTE grade_idx, OUT std::vector<float>& vec_chargings, OUT std::vector<float>& vec_probs)
{
	if (grade_idx >= DRAGON_SOUL_GRADE_MAX)
	{
		sys_err ("Invalid dragon soul grade_idx(%d).", grade_idx);
		return false;
	}

	std::string stDragonSoulName;
	if (!GetDragonSoulGroupName(ds_type, stDragonSoulName))
	{
		sys_err ("Invalid dragon soul type(%d).", ds_type);
		return false;
	}

	const CGroupNode::CGroupNodeRow * pRow;
	if (!m_pDragonHeartExtTableNode->GetGroupRow(stDragonSoulName, "charging", &pRow))
	{
		sys_err ("Invalid CHARGING row. DragonSoulGroup(%s)", stDragonSoulName.c_str());
		return false;
	}
	int n = pRow->GetSize();
	vec_chargings.resize(n);
	for (int i = 0; i < n; i++)
	{
		if (!pRow->GetValue(i, vec_chargings[i]))
		{
			sys_err ("Invalid CHARGING value. DragonSoulGroup(%s), Col(%d)", stDragonSoulName.c_str(), i);
			return false;
		}
	}

	if (!m_pDragonHeartExtTableNode->GetGroupRow(stDragonSoulName, g_astGradeName[grade_idx], &pRow))
	{
		sys_err ("Invalid row. DragonSoulGroup(%s) Grade(%s)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}

	int m = pRow->GetSize();
	if (n != m)
	{
		sys_err ("Invalid row size(%d). It must be same CHARGING row size(%d). DragonSoulGroup(%s) Grade(%s)", m, n, 
				stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}
	vec_probs.resize(m);
	for (int i = 0; i < m; i++)
	{
		if (!pRow->GetValue(i, vec_probs[i]))
		{
			sys_err ("Invalid value. DragonSoulGroup(%s), Grade(%s) Col(%d)", stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str(), i);
			return false;
		}
	}

	return true;
}

bool DragonSoulTable::GetDragonSoulExtValues(BYTE ds_type, BYTE grade_idx, OUT float& prob, OUT DWORD& by_product)
{
	if (grade_idx >= DRAGON_SOUL_GRADE_MAX)
	{
		sys_err ("Invalid dragon soul grade_idx(%d).", grade_idx);
		return false;
	}

	std::string stValue;
	std::string stDragonSoulName;
	if (!GetDragonSoulGroupName(ds_type, stDragonSoulName))
	{
		sys_err ("Invalid dragon soul type(%d).", ds_type);
		return false;
	}

	if (!m_pDragonSoulExtTableNode->GetGroupValue(stDragonSoulName, g_astGradeName[grade_idx], "prob", prob))
	{
		sys_err ("Invalid Prob. DragonSoulGroup(%s) Grade(%s)", 
			stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str(), g_astGradeName[grade_idx].c_str());
		return false;
	}

	if (!m_pDragonSoulExtTableNode->GetGroupValue(stDragonSoulName, g_astGradeName[grade_idx], "byproduct", by_product))
	{
		sys_err ("Invalid fee. DragonSoulGroup(%s) Grade(%d)", 
			stDragonSoulName.c_str(), g_astGradeName[grade_idx].c_str(), grade_idx);
		return false;
	}

	return true;
}

DragonSoulTable::DragonSoulTable()
{
	m_pLoader = NULL;
}
DragonSoulTable::~DragonSoulTable ()
{
	if (m_pLoader)
		delete m_pLoader;
}
