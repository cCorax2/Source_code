#ifndef __INC_ITEM_MANAGER__
#define __INC_ITEM_MANAGER__

#ifdef M2_USE_POOL
#include "pool.h"
#endif

// special_item_group.txt에서 정의하는 속성 그룹
// type attr로 선언할 수 있다.
// 이 속성 그룹을 이용할 수 있는 것은 special_item_group.txt에서 Special type으로 정의된 그룹에 속한 UNIQUE ITEM이다.
class CSpecialAttrGroup
{
public:
	CSpecialAttrGroup(DWORD vnum)
		: m_dwVnum(vnum)
	{}
	struct CSpecialAttrInfo
	{
		CSpecialAttrInfo (DWORD _apply_type, DWORD _apply_value)
			: apply_type(_apply_type), apply_value(_apply_value)
		{}
		DWORD apply_type;
		DWORD apply_value;

	};
	DWORD m_dwVnum;
	std::string	m_stEffectFileName;
	std::vector<CSpecialAttrInfo> m_vecAttrs;
};

class CSpecialItemGroup
{
	public:
		enum EGiveType
		{
			NONE,
			GOLD,
			EXP,
			MOB,
			SLOW,
			DRAIN_HP,
			POISON,
			MOB_GROUP,
		};

		// QUEST 타입은 퀘스트 스크립트에서 vnum.sig_use를 사용할 수 있는 그룹이다.
		//		단, 이 그룹에 들어가기 위해서는 ITEM 자체의 TYPE이 QUEST여야 한다.
		// SPECIAL 타입은 idx, item_vnum, attr_vnum을 입력한다. attr_vnum은 위에 CSpecialAttrGroup의 Vnum이다.
		//		이 그룹에 들어있는 아이템은 같이 착용할 수 없다.
		enum ESIGType { NORMAL, PCT, QUEST, SPECIAL };

		struct CSpecialItemInfo
		{
			DWORD vnum;
			int count;
			int rare;

			CSpecialItemInfo(DWORD _vnum, int _count, int _rare)
				: vnum(_vnum), count(_count), rare(_rare)
				{}
		};

		CSpecialItemGroup(DWORD vnum, BYTE type=0)
			: m_dwVnum(vnum), m_bType(type)
			{}

		void AddItem(DWORD vnum, int count, int prob, int rare)
		{
			if (!prob)
				return;
			if (!m_vecProbs.empty())
				prob += m_vecProbs.back();
			m_vecProbs.push_back(prob);
			m_vecItems.push_back(CSpecialItemInfo(vnum, count, rare));
		}

		bool IsEmpty() const
		{
			return m_vecProbs.empty();
		}

		// Type Multi, 즉 m_bType == PCT 인 경우,
		// 확률을 더해가지 않고, 독립적으로 계산하여 아이템을 생성한다.
		// 따라서 여러 개의 아이템이 생성될 수 있다.
		// by rtsummit
		int GetMultiIndex(std::vector <int> &idx_vec) const
		{
			idx_vec.clear();
			if (m_bType == PCT)
			{
				int count = 0;
				if (number(1,100) <= m_vecProbs[0])
				{
					idx_vec.push_back(0);
					count++;
				}
				for (uint i = 1; i < m_vecProbs.size(); i++)
				{
					if (number(1,100) <= m_vecProbs[i] - m_vecProbs[i-1])
					{
						idx_vec.push_back(i);
						count++;
					}
				}
				return count;
			}
			else
			{
				idx_vec.push_back(GetOneIndex());
				return 1;
			}
		}

		int GetOneIndex() const
		{
			int n = number(1, m_vecProbs.back());
			itertype(m_vecProbs) it = lower_bound(m_vecProbs.begin(), m_vecProbs.end(), n);
			return std::distance(m_vecProbs.begin(), it);
		}

		int GetVnum(int idx) const
		{
			return m_vecItems[idx].vnum;
		}

		int GetCount(int idx) const
		{
			return m_vecItems[idx].count;
		}

		int GetRarePct(int idx) const
		{
			return m_vecItems[idx].rare;
		}

		bool Contains(DWORD dwVnum) const
		{
			for (DWORD i = 0; i < m_vecItems.size(); i++)
			{
				if (m_vecItems[i].vnum == dwVnum)
					return true;
			}
			return false;
		}
		
		// Group의 Type이 Special인 경우에
		// dwVnum에 매칭되는 AttrVnum을 return해준다.
		DWORD GetAttrVnum(DWORD dwVnum) const
		{
			if (CSpecialItemGroup::SPECIAL != m_bType)
				return 0;
			for (itertype(m_vecItems) it = m_vecItems.begin(); it != m_vecItems.end(); it++)
			{
				if (it->vnum == dwVnum)
				{
					return it->count;
				}
			}
			return 0;
		}

		// Group의 Size를 return해준다.
		int GetGroupSize() const
		{
			return m_vecProbs.size();
		}

		DWORD m_dwVnum;
		BYTE	m_bType;
		std::vector<int> m_vecProbs;
		std::vector<CSpecialItemInfo> m_vecItems; // vnum, count
};

class CMobItemGroup
{
	public:
		struct SMobItemGroupInfo
		{
			DWORD dwItemVnum;
			int iCount;
			int iRarePct;

			SMobItemGroupInfo(DWORD dwItemVnum, int iCount, int iRarePct)
				: dwItemVnum(dwItemVnum),
			iCount(iCount),
			iRarePct(iRarePct)
			{
			}
		};

		CMobItemGroup(DWORD dwMobVnum, int iKillDrop, const std::string& r_stName)
			:
			m_dwMobVnum(dwMobVnum),
		m_iKillDrop(iKillDrop),
		m_stName(r_stName)
		{
		}

		int GetKillPerDrop() const
		{
			return m_iKillDrop;
		}

		void AddItem(DWORD dwItemVnum, int iCount, int iPartPct, int iRarePct)
		{
			if (!m_vecProbs.empty())
				iPartPct += m_vecProbs.back();
			m_vecProbs.push_back(iPartPct);
			m_vecItems.push_back(SMobItemGroupInfo(dwItemVnum, iCount, iRarePct));
		}

		// MOB_DROP_ITEM_BUG_FIX
		bool IsEmpty() const
		{
			return m_vecProbs.empty();
		}

		int GetOneIndex() const
		{
			int n = number(1, m_vecProbs.back());
			itertype(m_vecProbs) it = lower_bound(m_vecProbs.begin(), m_vecProbs.end(), n);
			return std::distance(m_vecProbs.begin(), it);
		}
		// END_OF_MOB_DROP_ITEM_BUG_FIX

		const SMobItemGroupInfo& GetOne() const
		{
			return m_vecItems[GetOneIndex()];
		}

	private:
		DWORD m_dwMobVnum;
		int m_iKillDrop;
		std::string m_stName;
		std::vector<int> m_vecProbs;
		std::vector<SMobItemGroupInfo> m_vecItems;
};

class CDropItemGroup
{
	struct SDropItemGroupInfo
	{
		DWORD	dwVnum;
		DWORD	dwPct;
		int	iCount;

		SDropItemGroupInfo(DWORD dwVnum, DWORD dwPct, int iCount)
			: dwVnum(dwVnum), dwPct(dwPct), iCount(iCount)
			{}
	};

	public:
	CDropItemGroup(DWORD dwVnum, DWORD dwMobVnum, const std::string& r_stName)
		:
		m_dwVnum(dwVnum),
	m_dwMobVnum(dwMobVnum),
	m_stName(r_stName)
	{
	}

	const std::vector<SDropItemGroupInfo> & GetVector()
	{
		return m_vec_items;
	}

	void AddItem(DWORD dwItemVnum, DWORD dwPct, int iCount)
	{
		m_vec_items.push_back(SDropItemGroupInfo(dwItemVnum, dwPct, iCount));
	}

	private:
	DWORD m_dwVnum;
	DWORD m_dwMobVnum;
	std::string m_stName;
	std::vector<SDropItemGroupInfo> m_vec_items;
};

class CLevelItemGroup
{
	struct SLevelItemGroupInfo
	{
		DWORD dwVNum;
		DWORD dwPct;
		int iCount;

		SLevelItemGroupInfo(DWORD dwVnum, DWORD dwPct, int iCount)
			: dwVNum(dwVnum), dwPct(dwPct), iCount(iCount)
		{ }
	};

	private :
		DWORD m_dwLevelLimit;
		std::string m_stName;
		std::vector<SLevelItemGroupInfo> m_vec_items;

	public :
		CLevelItemGroup(DWORD dwLevelLimit)
			: m_dwLevelLimit(dwLevelLimit)
		{}

		DWORD GetLevelLimit() { return m_dwLevelLimit; }

		void AddItem(DWORD dwItemVnum, DWORD dwPct, int iCount)
		{
			m_vec_items.push_back(SLevelItemGroupInfo(dwItemVnum, dwPct, iCount));
		}

		const std::vector<SLevelItemGroupInfo> & GetVector()
		{
			return m_vec_items;
		}
};

class CBuyerThiefGlovesItemGroup
{
	struct SThiefGroupInfo
	{
		DWORD	dwVnum;
		DWORD	dwPct;
		int	iCount;

		SThiefGroupInfo(DWORD dwVnum, DWORD dwPct, int iCount)
			: dwVnum(dwVnum), dwPct(dwPct), iCount(iCount)
			{}
	};

	public:
	CBuyerThiefGlovesItemGroup(DWORD dwVnum, DWORD dwMobVnum, const std::string& r_stName)
		:
		m_dwVnum(dwVnum),
	m_dwMobVnum(dwMobVnum),
	m_stName(r_stName)
	{
	}

	const std::vector<SThiefGroupInfo> & GetVector()
	{
		return m_vec_items;
	}

	void AddItem(DWORD dwItemVnum, DWORD dwPct, int iCount)
	{
		m_vec_items.push_back(SThiefGroupInfo(dwItemVnum, dwPct, iCount));
	}

	private:
	DWORD m_dwVnum;
	DWORD m_dwMobVnum;
	std::string m_stName;
	std::vector<SThiefGroupInfo> m_vec_items;
};

class ITEM;

class ITEM_MANAGER : public singleton<ITEM_MANAGER>
{
	public:
		ITEM_MANAGER();
		virtual ~ITEM_MANAGER();

		bool                    Initialize(TItemTable * table, int size);
		void			Destroy();
		void			Update();	// 매 루프마다 부른다.
		void			GracefulShutdown();

		DWORD			GetNewID();
		bool			SetMaxItemID(TItemIDRangeTable range); // 최대 고유 아이디를 지정
		bool			SetMaxSpareItemID(TItemIDRangeTable range);

		// DelayedSave: 어떠한 루틴 내에서 저장을 해야 할 짓을 많이 하면 저장
		// 쿼리가 너무 많아지므로 "저장을 한다" 라고 표시만 해두고 잠깐
		// (예: 1 frame) 후에 저장시킨다.
		void			DelayedSave(LPITEM item);
		void			FlushDelayedSave(LPITEM item); // Delayed 리스트에 있다면 지우고 저장한다. 끊김 처리시 사용 됨.
		void			SaveSingleItem(LPITEM item);

		LPITEM                  CreateItem(DWORD vnum, DWORD count = 1, DWORD dwID = 0, bool bTryMagic = false, int iRarePct = -1, bool bSkipSave = false);
#ifndef DEBUG_ALLOC
		void DestroyItem(LPITEM item);
#else
		void DestroyItem(LPITEM item, const char* file, size_t line);
#endif
		void			RemoveItem(LPITEM item, const char * c_pszReason=NULL); // 사용자로 부터 아이템을 제거

		LPITEM			Find(DWORD id);
		LPITEM                  FindByVID(DWORD vid);
		TItemTable *            GetTable(DWORD vnum);
		bool			GetVnum(const char * c_pszName, DWORD & r_dwVnum);
		bool			GetVnumByOriginalName(const char * c_pszName, DWORD & r_dwVnum);

		bool			GetDropPct(LPCHARACTER pkChr, LPCHARACTER pkKiller, OUT int& iDeltaPercent, OUT int& iRandRange);
		bool			CreateDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM> & vec_item);

		bool			ReadCommonDropItemFile(const char * c_pszFileName);
		bool			ReadEtcDropItemFile(const char * c_pszFileName);
		bool			ReadDropItemGroup(const char * c_pszFileName);
		bool			ReadMonsterDropItemGroup(const char * c_pszFileName);
		bool			ReadSpecialDropItemFile(const char * c_pszFileName);
		
		// convert name -> vnum special_item_group.txt
		bool			ConvSpecialDropItemFile();
		// convert name -> vnum special_item_group.txt

		DWORD			GetRefineFromVnum(DWORD dwVnum);

		static void		CopyAllAttrTo(LPITEM pkOldItem, LPITEM pkNewItem);		// pkNewItem으로 모든 속성과 소켓 값들을 목사하는 함수.


		const CSpecialItemGroup* GetSpecialItemGroup(DWORD dwVnum);
		const CSpecialAttrGroup* GetSpecialAttrGroup(DWORD dwVnum);

		const std::vector<TItemTable> & GetTable() { return m_vec_prototype; }

		// CHECK_UNIQUE_GROUP
		int			GetSpecialGroupFromItem(DWORD dwVnum) const { itertype(m_ItemToSpecialGroup) it = m_ItemToSpecialGroup.find(dwVnum); return (it == m_ItemToSpecialGroup.end()) ? 0 : it->second; }
		// END_OF_CHECK_UNIQUE_GROUP

	protected:
		int                     RealNumber(DWORD vnum);
		void			CreateQuestDropItem(LPCHARACTER pkChr, LPCHARACTER pkKiller, std::vector<LPITEM> & vec_item, int iDeltaPercent, int iRandRange);

	protected:
		typedef std::map<DWORD, LPITEM> ITEM_VID_MAP;		

		std::vector<TItemTable>		m_vec_prototype;
		std::vector<TItemTable*> m_vec_item_vnum_range_info;
		std::map<DWORD, DWORD>		m_map_ItemRefineFrom;
		int				m_iTopOfTable;

		ITEM_VID_MAP			m_VIDMap;			///< m_dwVIDCount 의 값단위로 아이템을 저장한다.
		DWORD				m_dwVIDCount;			///< 이녀석 VID가 아니라 그냥 프로세스 단위 유니크 번호다.
		DWORD				m_dwCurrentID;
		TItemIDRangeTable	m_ItemIDRange;
		TItemIDRangeTable	m_ItemIDSpareRange;

		TR1_NS::unordered_set<LPITEM> m_set_pkItemForDelayedSave;
		std::map<DWORD, LPITEM>		m_map_pkItemByID;
		std::map<DWORD, DWORD>		m_map_dwEtcItemDropProb;
		std::map<DWORD, CDropItemGroup*> m_map_pkDropItemGroup;
		std::map<DWORD, CSpecialItemGroup*> m_map_pkSpecialItemGroup;
		std::map<DWORD, CSpecialItemGroup*> m_map_pkQuestItemGroup;
		std::map<DWORD, CSpecialAttrGroup*> m_map_pkSpecialAttrGroup;
		std::map<DWORD, CMobItemGroup*> m_map_pkMobItemGroup;
		std::map<DWORD, CLevelItemGroup*> m_map_pkLevelItemGroup;
		std::map<DWORD, CBuyerThiefGlovesItemGroup*> m_map_pkGloveItemGroup;

		// CHECK_UNIQUE_GROUP
		std::map<DWORD, int>		m_ItemToSpecialGroup;
		// END_OF_CHECK_UNIQUE_GROUP
	
	private:
		// 독일에서 기존 캐시 아이템과 같지만, 교환 가능한 캐시 아이템을 만든다고 하여,
		// 오리지널 아이템에 교환 금지 플래그만 삭제한 새로운 아이템들을 만들어,
		// 새로운 아이템 대역을 할당하였다.
		// 문제는 새로운 아이템도 오리지널 아이템과 같은 효과를 내야하는데,
		// 서버건, 클라건, vnum 기반으로 되어있어
		// 새로운 vnum을 죄다 서버에 새로 다 박아야하는 안타까운 상황에 맞닿았다.
		// 그래서 새 vnum의 아이템이면, 서버에서 돌아갈 때는 오리지널 아이템 vnum으로 바꿔서 돌고 하고,
		// 저장할 때에 본래 vnum으로 바꿔주도록 한다.
		// 이를 위해 오리지널 vnum과 새로운 vnum을 연결시켜주는 맵을 만듦.
		typedef std::map <DWORD, DWORD> TMapDW2DW;
		TMapDW2DW	m_map_new_to_ori;

	public:	
		DWORD	GetMaskVnum(DWORD dwVnum);
		std::map<DWORD, TItemTable>  m_map_vid;
		std::map<DWORD, TItemTable>&  GetVIDMap() { return m_map_vid; }
		std::vector<TItemTable>& GetVecProto() { return m_vec_prototype; }	
		
		const static int MAX_NORM_ATTR_NUM = 5;
		const static int MAX_RARE_ATTR_NUM = 2;
		bool ReadItemVnumMaskTable(const char * c_pszFileName);
	private:
#ifdef M2_USE_POOL
		ObjectPool<CItem> pool_;
#endif
};

#ifndef DEBUG_ALLOC
#define M2_DESTROY_ITEM(ptr) ITEM_MANAGER::instance().DestroyItem(ptr)
#else
#define M2_DESTROY_ITEM(ptr) ITEM_MANAGER::instance().DestroyItem(ptr, __FILE__, __LINE__)
#endif

#endif
