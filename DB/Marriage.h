// vim: ts=4 sw=4
#ifndef __MARRIAGE_H
#define __MARRIAGE_H

#include <set>
#include <queue>
#include <deque>

#include "Peer.h"

namespace marriage
{
	struct TWeddingInfo
	{
		DWORD dwTime;
		DWORD dwPID1;
		DWORD dwPID2;
		TWeddingInfo(DWORD time, DWORD pid1, DWORD pid2)
			: dwTime(time),
			dwPID1(pid1),
			dwPID2(pid2)
		{
		}
	};

	struct TWedding
	{
		DWORD dwTime;
		DWORD dwMapIndex;
		DWORD dwPID1;
		DWORD dwPID2;

		TWedding(DWORD time, DWORD dwMapIndex, DWORD pid1, DWORD pid2)
			: dwTime(time),
			dwMapIndex(dwMapIndex),
			dwPID1(pid1),
			dwPID2(pid2)
		{
		}
	};

	extern bool operator < (const TWedding& lhs, const TWedding& rhs);
	extern bool operator > (const TWedding& lhs, const TWedding& rhs);
	extern bool operator > (const TWeddingInfo& lhs, const TWeddingInfo& rhs);

	struct TMarriage
	{
		DWORD pid1;
		DWORD pid2;
		int   love_point;
		DWORD time;
		BYTE is_married; // false : æ‡»• ªÛ≈¬, true : ∞·»• ªÛ≈¬
		std::string name1;
		std::string name2;

		TMarriage(DWORD _pid1, DWORD _pid2, int _love_point, DWORD _time, BYTE _is_married, const char* name1, const char* name2)
			: pid1(_pid1), pid2(_pid2), love_point(_love_point), time(_time), is_married(_is_married), name1(name1), name2(name2)
		{
		}

		DWORD GetOther(DWORD PID)
		{
			if (pid1 == PID)
				return pid2;

			if (pid2 == PID)
				return pid1;

			return 0;
		}
	};

	class CManager : public singleton<CManager>
	{
		public:
			CManager();
			virtual ~CManager();

			bool Initialize();

			TMarriage* Get(DWORD dwPlayerID);
			bool IsMarried(DWORD dwPlayerID)
			{
				return Get(dwPlayerID) != NULL;
			}

			//void	Reserve(DWORD dwPID1, DWORD dwPID2);
			void	Add(DWORD dwPID1, DWORD dwPID2, const char* szName1, const char* szName2);
			void	Remove(DWORD dwPID1, DWORD dwPID2);
			void	Update(DWORD dwPID1, DWORD dwPID2, INT iLovePoint, BYTE byMarried);

			void	EngageToMarriage(DWORD dwPID1, DWORD dwPID2);

			void	ReadyWedding(DWORD dwMapIndex, DWORD dwPID1, DWORD dwPID2);
			void	EndWedding(DWORD dwPID1, DWORD dwPID2);

			void	OnSetup(CPeer* peer);

			void	Update();

		private:
			std::set<TMarriage *> m_Marriages;
			std::map<DWORD, TMarriage *> m_MarriageByPID;

			std::priority_queue<TWedding, std::vector<TWedding>, std::greater<TWedding> > m_pqWeddingStart;

			std::priority_queue<TWeddingInfo, std::vector<TWeddingInfo>, std::greater<TWeddingInfo> > m_pqWeddingEnd;

			std::map<std::pair<DWORD, DWORD>, TWedding> m_mapRunningWedding;
	};
}

#endif
