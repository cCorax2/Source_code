#ifndef __INC_METIN_II_GAME_PVP_H__
#define __INC_METIN_II_GAME_PVP_H__

class CHARACTER;

// CPVP에는 DWORD 아이디 두개를 받아서 m_dwCRC를 만들어서 가지고 있는다.
// CPVPManager에서 이렇게 만든 CRC를 통해 검색한다.
class CPVP
{
	public:
		friend class CPVPManager;

		typedef struct _player
		{
			DWORD	dwPID;
			DWORD	dwVID;
			bool	bAgree;
			bool	bCanRevenge;

			_player() : dwPID(0), dwVID(0), bAgree(false), bCanRevenge(false)
			{
			}
		} TPlayer;

		CPVP(DWORD dwID1, DWORD dwID2);
		CPVP(CPVP & v);
		~CPVP();

		void	Win(DWORD dwPID); // dwPID가 이겼다!
		bool	CanRevenge(DWORD dwPID); // dwPID가 복수할 수 있어?
		bool	IsFight();
		bool	Agree(DWORD dwPID);

		void	SetVID(DWORD dwPID, DWORD dwVID);
		void	Packet(bool bDelete = false);

		void	SetLastFightTime();
		DWORD	GetLastFightTime();

		DWORD 	GetCRC() { return m_dwCRC; }

	protected:
		TPlayer	m_players[2];
		DWORD	m_dwCRC;
		bool	m_bRevenge;

		DWORD   m_dwLastFightTime;
};

class CPVPManager : public singleton<CPVPManager>
{
	typedef std::map<DWORD, TR1_NS::unordered_set<CPVP*> > CPVPSetMap;

	public:
	CPVPManager();
	virtual ~CPVPManager();

	void			Insert(LPCHARACTER pkChr, LPCHARACTER pkVictim);
	bool			CanAttack(LPCHARACTER pkChr, LPCHARACTER pkVictim);
	bool			Dead(LPCHARACTER pkChr, DWORD dwKillerPID);	// PVP에 있었나 없었나를 리턴
	void			GiveUp(LPCHARACTER pkChr, DWORD dwKillerPID);
	void			Connect(LPCHARACTER pkChr);
	void			Disconnect(LPCHARACTER pkChr);

	void			SendList(LPDESC d);
	void			Delete(CPVP * pkPVP);

	void			Process();

	public:
	CPVP *			Find(DWORD dwCRC);
	protected:
	void			ConnectEx(LPCHARACTER pkChr, bool bDisconnect);

	std::map<DWORD, CPVP *>	m_map_pkPVP;
	CPVPSetMap		m_map_pkPVPSetByID;
};

#endif
