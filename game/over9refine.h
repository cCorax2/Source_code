
#ifndef OVER_9_REFINE_MANAGER_H_
#define OVER_9_REFINE_MANAGER_H_

#include <boost/unordered_map.hpp>

class COver9RefineManager : public singleton<COver9RefineManager>
{
	private :
		typedef boost::unordered_map<DWORD, DWORD> OVER9ITEM_MAP;
		OVER9ITEM_MAP m_mapItem;

	public :
		void enableOver9Refine(DWORD dwVnumFrom, DWORD dwVnumTo);

		int canOver9Refine(DWORD dwVnum);

		bool Change9ToOver9(LPCHARACTER pChar, LPITEM item);
		bool Over9Refine(LPCHARACTER pChat, LPITEM item);

		DWORD GetMaterialVnum(DWORD baseVnum);
};

#endif /* OVER_9_REFINE_MANAGER_H_ */

