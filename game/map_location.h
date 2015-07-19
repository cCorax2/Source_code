
#include "../../common/stl.h"

class CMapLocation : public singleton<CMapLocation>
{
	public:
		typedef struct SLocation
		{
			long        addr;
			WORD        port;
		} TLocation;    

		bool    Get(long x, long y, long & lMapIndex, long & lAddr, WORD & wPort);
		bool	Get(int iIndex, long & lAddr, WORD & wPort);
		void    Insert(long lIndex, const char * c_pszHost, WORD wPort);

	protected:
		std::map<long, TLocation> m_map_address;
};      

