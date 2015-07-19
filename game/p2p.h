
#ifndef P2P_MANAGER_H_
#define P2P_MANAGER_H_

#include <boost/unordered_map.hpp>

#include "input.h"
#include "../../common/stl.h"

typedef struct _CCI
{
	char	szName[CHARACTER_NAME_MAX_LEN + 1];
	DWORD	dwPID;
	BYTE	bEmpire;
	long	lMapIndex;
	BYTE	bChannel;

	LPDESC	pkDesc;
} CCI;

class P2P_MANAGER : public singleton<P2P_MANAGER>
{
	public:
		P2P_MANAGER();
		~P2P_MANAGER();

		// 아래 Register* Unregister* pair들은 내부적으로 사실 같은 루틴을 사용한다.
		// 단지 명시적으로 표시하기 위한 것
		void			RegisterAcceptor(LPDESC d);
		void			UnregisterAcceptor(LPDESC d);

		void			RegisterConnector(LPDESC d);
		void			UnregisterConnector(LPDESC d);

		void			EraseUserByDesc(LPDESC d);	// 해당 desc에 있는 유저들을 지운다.

		void			FlushOutput();

		void			Boot(LPDESC d);	// p2p 처리에 필요한 정보를 보내준다. (전 캐릭터의 로그인 정보 등)

		void			Send(const void * c_pvData, int iSize, LPDESC except = NULL);

		void			Login(LPDESC d, const TPacketGGLogin * p);
		void			Logout(const char * c_pszName);

		CCI *			Find(const char * c_pszName);
		CCI *			FindByPID(DWORD pid);

		int				GetCount();
		int				GetEmpireUserCount(int idx);
		int				GetDescCount();
		void			GetP2PHostNames(std::string& hostNames);

	private:
		void			Logout(CCI * pkCCI);

		CInputProcessor *	m_pkInputProcessor;
		int			m_iHandleCount;

		typedef boost::unordered_map<std::string, CCI *, stringhash> TCCIMap;
		typedef boost::unordered_map<DWORD, CCI*> TPIDCCIMap;

		TR1_NS::unordered_set<LPDESC> m_set_pkPeers;
		TCCIMap			m_map_pkCCI;
		TPIDCCIMap		m_map_dwPID_pkCCI;
		int			m_aiEmpireUserCount[EMPIRE_MAX_NUM];
};

#endif /* P2P_MANAGER_H_ */

