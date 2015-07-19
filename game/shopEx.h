#ifndef __INC_METIN_II_GAME_SHOP_SECONDARY_COIN_H__
#define __INC_METIN_II_GAME_SHOP_SECONDARY_COIN_H__
#include "typedef.h"
#include "shop.h"

struct SShopTable;
typedef struct SShopTableEx : SShopTable
{
	std::string name;
	EShopCoinType coinType;
} TShopTableEx;

class CGroupNode;

// 확장 shop.
// 명도전을 화폐로 쓸 수 있고, 아이템을 여러 탭에 나눠 배치할 수 있다.
// 단, pc 샵은 구현하지 않음.
// 클라와 통신할 때에 탭은 pos 45 단위로 구분.
// 기존 샵의 m_itemVector은 사용하지 않는다.
class CShopEx: public CShop
{
public:
	bool			Create(DWORD dwVnum, DWORD dwNPCVnum);
	bool			AddShopTable(TShopTableEx& shopTable);

	virtual bool	AddGuest(LPCHARACTER ch,DWORD owner_vid, bool bOtherEmpire);
	virtual void	SetPCShop(LPCHARACTER ch) { return; }
	virtual bool	IsPCShop() { return false; }
	virtual int		Buy(LPCHARACTER ch, BYTE pos);
	virtual bool	IsSellingItem(DWORD itemID) { return false; }
	
	size_t			GetTabCount() { return m_vec_shopTabs.size(); }
private:
	std::vector <TShopTableEx> m_vec_shopTabs;
};
typedef CShopEx* LPSHOPEX;

#endif
