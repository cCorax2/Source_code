
#ifndef THREE_WAY_WAR_EVENT_
#define THREE_WAY_WAR_EVENT_

#include <boost/unordered_map.hpp>

#include "../../common/stl.h"

struct ForkedSungziMapInfo
{
	int m_iForkedSung;
	int m_iForkedSungziStartPosition[3][2];
	std::string m_stMapName;
	int m_iBossMobVnum;
};

struct ForkedPassMapInfo
{
	int m_iForkedPass[3];
	int m_iForkedPassStartPosition[3][2];
	std::string m_stMapName[3];
};

class CThreeWayWar : public singleton<CThreeWayWar>
{
	public:
		CThreeWayWar ();
		virtual ~CThreeWayWar ();

		void Initialize ();
		bool LoadSetting (const char* szFileName);

		int GetKillScore (BYTE empire) const;
		void SetKillScore (BYTE empire, int count);

		void SetReviveTokenForPlayer (DWORD PlayerID, int count);
		int GetReviveTokenForPlayer (DWORD PlayerID);
		void DecreaseReviveTokenForPlayer (DWORD PlayerID);

		const ForkedPassMapInfo& GetEventPassMapInfo () const;
		const ForkedSungziMapInfo& GetEventSungZiMapInfo () const;

		bool IsThreeWayWarMapIndex (int iMapIndex) const;
		bool IsSungZiMapIndex (int iMapIndex) const;

		void RandomEventMapSet ();

		void RegisterUser (DWORD PlayerID);
		bool IsRegisteredUser (DWORD PlayerID) const;

		void onDead (LPCHARACTER pChar, LPCHARACTER pKiller);

		void SetRegenFlag (int flag) { RegenFlag_ = flag; }
		int GetRegenFlag () const { return RegenFlag_; }

		void RemoveAllMonstersInThreeWay () const;

	private:
		int KillScore_[3];
		int RegenFlag_;

		std::set<int>	MapIndexSet_;
		std::vector<ForkedPassMapInfo>	PassInfoMap_;
		std::vector<ForkedSungziMapInfo>	SungZiInfoMap_;

		boost::unordered_map<DWORD, DWORD>	RegisterUserMap_;
		boost::unordered_map<DWORD, int>	ReviveTokenMap_;
};

const char* GetSungziMapPath();
const char* GetPassMapPath( BYTE bEmpire );
int GetPassMapIndex( BYTE bEmpire );
int GetSungziMapIndex();

int GetSungziStartX( BYTE bEmpire );
int GetSungziStartY( BYTE bEmpire );
int GetPassStartX( BYTE bEmpire );
int GetPassStartY( BYTE bEmpire );

#endif /* THREE_WAY_WAR_EVENT_ */

