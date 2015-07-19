#ifndef __CLASS_ARENA_MANAGER__
#define __CLASS_ARENA_MANAGER__

#include <lua.h>

enum MEMBER_IDENTITY
{
	MEMBER_NO,
	MEMBER_DUELIST,
	MEMBER_OBSERVER,

	MEMBER_MAX
};

class CArena
{
	friend class CArenaMap;

	private :
	DWORD m_dwPIDA;
	DWORD m_dwPIDB;

	LPEVENT m_pEvent;
	LPEVENT m_pTimeOutEvent;

	PIXEL_POSITION m_StartPointA;
	PIXEL_POSITION m_StartPointB;
	PIXEL_POSITION m_ObserverPoint;

	DWORD m_dwSetCount;
	DWORD m_dwSetPointOfA;
	DWORD m_dwSetPointOfB;

	std::map<DWORD, LPCHARACTER> m_mapObserver;

	protected :
	CArena(WORD startA_X, WORD startA_Y, WORD startB_X, WORD startB_Y);

	bool StartDuel(LPCHARACTER pCharFrom, LPCHARACTER pCharTo, int nSetPoint, int nMinute = 5);

	bool IsEmpty() const	{ return ((m_dwPIDA==0) && (m_dwPIDB==0)); }
	bool IsMember(DWORD dwPID) const	{ return ((m_dwPIDA==dwPID) || (m_dwPIDB==dwPID)); }

	bool CheckArea(WORD startA_X, WORD startA_Y, WORD startB_X, WORD startB_Y);
	void Clear();

	bool CanAttack(DWORD dwPIDA, DWORD dwPIDB);
	bool OnDead(DWORD dwPIDA, DWORD dwPIDB);

	bool IsObserver(DWORD pid);
	bool IsMyObserver(WORD ObserverX, WORD ObserverY);
	bool AddObserver(LPCHARACTER pChar);
	bool RegisterObserverPtr(LPCHARACTER pChar);

	public :
	DWORD GetPlayerAPID() { return m_dwPIDA; }
	DWORD GetPlayerBPID() { return m_dwPIDB; }

	LPCHARACTER GetPlayerA() { return CHARACTER_MANAGER::instance().FindByPID(m_dwPIDA); }
	LPCHARACTER GetPlayerB() { return CHARACTER_MANAGER::instance().FindByPID(m_dwPIDB); }

	PIXEL_POSITION GetStartPointA() { return m_StartPointA; }
	PIXEL_POSITION GetStartPointB() { return m_StartPointB; }

	PIXEL_POSITION GetObserverPoint() { return m_ObserverPoint; }

	void EndDuel();
	void ClearEvent() { m_pEvent = NULL; }
	void OnDisconnect(DWORD pid);
	void RemoveObserver(DWORD pid);

	void SendPacketToObserver(const void * c_pvData, int iSize);
	void SendChatPacketToObserver(BYTE type, const char * format, ...);
};

class CArenaMap
{
	friend class CArenaManager;

	private :
	DWORD m_dwMapIndex;
	std::list<CArena*> m_listArena;

	protected :
	void Destroy();

	bool AddArena(DWORD mapIdx, WORD startA_X, WORD startA_Y, WORD startB_X, WORD startB_Y);
	void SendArenaMapListTo(LPCHARACTER pChar, DWORD dwMapIndex);

	bool StartDuel(LPCHARACTER pCharFrom, LPCHARACTER pCharTo, int nSetPoint, int nMinute = 5);
	void EndAllDuel();
	bool EndDuel(DWORD pid);

	int GetDuelList(lua_State* L, int index);

	bool CanAttack(LPCHARACTER pCharAttacker, LPCHARACTER pCharVictim);
	bool OnDead(LPCHARACTER pCharKiller, LPCHARACTER pCharVictim);

	bool AddObserver(LPCHARACTER pChar, WORD ObserverX, WORD ObserverY);
	bool RegisterObserverPtr(LPCHARACTER pChar, DWORD mapIdx, WORD ObserverX, WORD ObserverY);

	MEMBER_IDENTITY IsMember(DWORD PID);
};

class CArenaManager : public singleton<CArenaManager>
{
	private :
		std::map<DWORD, CArenaMap*> m_mapArenaMap;

	public :
		bool Initialize();
		void Destroy();

		bool StartDuel(LPCHARACTER pCharFrom, LPCHARACTER pCharTo, int nSetPoint, int nMinute = 5);

		bool AddArena(DWORD mapIdx, WORD startA_X, WORD startA_Y, WORD startB_X, WORD startB_Y);

		void SendArenaMapListTo(LPCHARACTER pChar);

		void EndAllDuel();
		bool EndDuel(DWORD pid);

		void GetDuelList(lua_State* L);

		bool CanAttack(LPCHARACTER pCharAttacker, LPCHARACTER pCharVictim);

		bool OnDead(LPCHARACTER pCharKiller, LPCHARACTER pCharVictim);

		bool AddObserver(LPCHARACTER pChar, DWORD mapIdx, WORD ObserverX, WORD ObserverY);
		bool RegisterObserverPtr(LPCHARACTER pChar, DWORD mapIdx, WORD ObserverX, WORD ObserverY);

		bool IsArenaMap(DWORD dwMapIndex);
		MEMBER_IDENTITY IsMember(DWORD dwMapIndex, DWORD PID);

		bool IsLimitedItem( long lMapIndex, DWORD dwVnum );
};

#endif /*__CLASS_ARENA_MANAGER__*/

