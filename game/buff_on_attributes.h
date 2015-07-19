#ifndef __METIN2_BUFF_ON_ATTRIBUTES_H
#define __METIN2_BUFF_ON_ATTRIBUTES_H

class CHARACTER;

class CBuffOnAttributes
{
public:
	CBuffOnAttributes(LPCHARACTER pOwner, BYTE m_point_type, std::vector <BYTE>* vec_buff_targets);
	~CBuffOnAttributes();

	// 장착 중 이면서, m_p_vec_buff_wear_targets에 해당하는 아이템인 경우, 해당 아이템으로 인해 붙은 효과를 제거.
	void RemoveBuffFromItem(LPITEM pItem);
	// m_p_vec_buff_wear_targets에 해당하는 아이템인 경우, 해당 아이템의 attribute에 대한 효과 추가.
	void AddBuffFromItem(LPITEM pItem);
	// m_bBuffValue를 바꾸고, 버프의 값도 바꿈.
	void ChangeBuffValue(BYTE bNewValue);
	// CHRACTRE::ComputePoints에서 속성치를 초기화하고 다시 계산하므로, 
	// 버프 속성치들을 강제적으로 owner에게 줌.
	void GiveAllAttributes();

	// m_p_vec_buff_wear_targets에 해당하는 모든 아이템의 attribute를 type별로 합산하고,
	// 그 attribute들의 (m_bBuffValue)% 만큼을 버프로 줌.
	bool On(BYTE bValue);
	// 버프 제거 후, 초기화
	void Off();

	void Initialize();
private:
	LPCHARACTER m_pBuffOwner;
	BYTE m_bPointType;
	BYTE m_bBuffValue;
	std::vector <BYTE>* m_p_vec_buff_wear_targets;
	
	// apply_type, apply_value 페어의 맵
	typedef std::map <BYTE, int> TMapAttr;
	// m_p_vec_buff_wear_targets에 해당하는 모든 아이템의 attribute를 type별로 합산하여 가지고 있음.
	TMapAttr m_map_additional_attrs;

};

#endif
