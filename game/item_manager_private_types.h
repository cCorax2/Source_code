class CItemDropInfo
{
public:
	CItemDropInfo(int iLevelStart, int iLevelEnd, int iPercent, DWORD dwVnum) :
	  m_iLevelStart(iLevelStart), m_iLevelEnd(iLevelEnd), m_iPercent(iPercent), m_dwVnum(dwVnum)
	  {
	  }

	  int	m_iLevelStart;
	  int	m_iLevelEnd;
	  int	m_iPercent; // 1 ~ 1000
	  DWORD	m_dwVnum;

	  friend bool operator < (const CItemDropInfo & l, const CItemDropInfo & r)
	  {
		  return l.m_iLevelEnd < r.m_iLevelEnd;
	  }
};

extern std::vector<CItemDropInfo> g_vec_pkCommonDropItem[MOB_RANK_MAX_NUM];

typedef struct SDropItem
{
	int		iLvStart;
	int		iLvEnd;
	float	fPercent;
	char	szItemName[ITEM_NAME_MAX_LEN + 1];
	int		iCount;
} TDropItem;

