#ifndef __INC_METIN_II_DRAGON_SOUL_TABLE_H__
#define __INC_METIN_II_DRAGON_SOUL_TABLE_H__

struct SApply
{
	SApply (EApplyTypes at, int av, float p = 0.f) : apply_type(at), apply_value(av), prob(p) {}
	EApplyTypes apply_type;
	int apply_value;
	float prob;
};
const int DRAGON_SOUL_ADDITIONAL_ATTR_START_IDX = 3;

class CGroupNode;
class CGroupTextParseTreeLoader;

class DragonSoulTable
{
public:
	DragonSoulTable();
	~DragonSoulTable();
	typedef std::vector <SApply> TVecApplys;
	typedef std::map <BYTE, TVecApplys> TMapApplyGroup;
	
	bool	ReadDragonSoulTableFile(const char * c_pszFileName);
	bool	GetDragonSoulGroupName(BYTE bType, std::string& stGroupName) const;

	bool	GetBasicApplys(BYTE ds_type, OUT TVecApplys& vec_basic_applys);
	bool	GetAdditionalApplys(BYTE ds_type, OUT TVecApplys& vec_additional_attrs);

	bool	GetApplyNumSettings(BYTE ds_type, BYTE grade_idx, OUT int& basis, OUT int& add_min, OUT int& add_max);
	bool	GetWeight(BYTE ds_type, BYTE grade_idx, BYTE step_index, BYTE strength_idx, OUT float& fWeight);
	bool	GetRefineGradeValues(BYTE ds_type, BYTE grade_idx, OUT int& need_count, OUT int& fee, OUT std::vector<float>& vec_probs);
	bool	GetRefineStepValues(BYTE ds_type, BYTE step_idx, OUT int& need_count, OUT int& fee, OUT std::vector<float>& vec_probs);
	bool	GetRefineStrengthValues(BYTE ds_type, BYTE material_type, BYTE strength_idx, OUT int& fee, OUT float& prob);
	bool	GetDragonHeartExtValues(BYTE ds_type, BYTE grade_idx, OUT std::vector<float>& vec_chargings, OUT std::vector<float>& vec_probs);
	bool	GetDragonSoulExtValues(BYTE ds_type, BYTE grade_idx, OUT float& prob, OUT DWORD& by_product);

private:
	CGroupTextParseTreeLoader* m_pLoader;
	std::string stFileName;

	// caching m_pLoader's child nodes.
	CGroupNode*	m_pApplyNumSettingNode;
	CGroupNode*	m_pWeightTableNode;
	CGroupNode*	m_pRefineGradeTableNode;
	CGroupNode*	m_pRefineStepTableNode;
	CGroupNode*	m_pRefineStrengthTableNode;
	CGroupNode*	m_pDragonHeartExtTableNode;
	CGroupNode*	m_pDragonSoulExtTableNode;

	typedef std::map <std::string, BYTE> TMapNameToType;
	typedef std::map <BYTE, std::string> TMapTypeToName;
	std::vector <std::string> m_vecDragonSoulNames;
	std::vector <BYTE> m_vecDragonSoulTypes;

	TMapNameToType m_map_name_to_type;
	TMapTypeToName m_map_type_to_name;
	TMapApplyGroup m_map_basic_applys_group;
	TMapApplyGroup m_map_additional_applys_group;

	bool	ReadVnumMapper();
	bool	ReadBasicApplys();
	bool	ReadAdditionalApplys();
	// table check functions.
	bool	CheckApplyNumSettings();
	bool	CheckWeightTables();
	bool	CheckRefineGradeTables();
	bool	CheckRefineStepTables();
	bool	CheckRefineStrengthTables();
	bool	CheckDragonHeartExtTables();
	bool	CheckDragonSoulExtTables();
};

#endif