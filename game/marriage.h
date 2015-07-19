#ifndef __MARRIAGE_H
#define __MARRIAGE_H

namespace marriage
{
	struct TWeddingInfo
	{
		DWORD dwMapIndex;
	};

	extern const int MARRIAGE_POINT_PER_DAY;
	struct TMarriage
	{
		DWORD m_pid1;
		DWORD m_pid2;
		int   love_point;
		time_t marry_time;
		LPCHARACTER ch1;
		LPCHARACTER ch2;
		bool bSave;
		bool is_married;
		std::string name1;
		std::string name2;

		TWeddingInfo * pWeddingInfo;

		TMarriage(DWORD pid1, DWORD pid2, int _love_point, time_t _marry_time, const char* name1, const char* name2) : 
			m_pid1(pid1), 
			m_pid2(pid2), 
			love_point(_love_point),
			marry_time(_marry_time), 
			is_married(false),
			name1(name1),
			name2(name2),
			pWeddingInfo(NULL),
			eventNearCheck(NULL)
		{
			ch1 = ch2 = NULL;
			bSave = false;
			isLastNear = false;
			byLastLovePoint = 0;
		}

		~TMarriage();

		void Login(LPCHARACTER ch);
		void Logout(DWORD pid);

		bool IsOnline()
		{
			return ch1 && ch2;
		}

		bool IsNear();

		DWORD GetOther(DWORD PID) const
		{
			if (m_pid1 == PID)
				return m_pid2;

			if (m_pid2 == PID)
				return m_pid1;

			return 0;
		}

		int GetMarriagePoint();
		int GetMarriageGrade();

		int GetBonus(DWORD dwItemVnum, bool bShare = true, LPCHARACTER me = NULL);

		void WarpToWeddingMap(DWORD dwPID);
		void Save();
		void SetMarried();

		void Update(DWORD point);
		void RequestEndWedding();

		void StartNearCheckEvent();
		void StopNearCheckEvent();
		void NearCheck();

		bool isLastNear;
		BYTE byLastLovePoint;
		LPEVENT eventNearCheck;
	};

	class CManager : public singleton<CManager>
	{
		public:
			CManager();
			virtual ~CManager();

			bool	Initialize();
			void	Destroy();

			TMarriage*	Get(DWORD dwPlayerID);

			bool	IsMarriageUniqueItem(DWORD dwItemVnum);

			bool	IsMarried(DWORD dwPlayerID);
			bool	IsEngaged(DWORD dwPlayerID);
			bool	IsEngagedOrMarried(DWORD dwPlayerID);

			void	RequestAdd(DWORD dwPID1, DWORD dwPID2, const char* szName1, const char* szName2);
			void	Add(DWORD dwPID1, DWORD dwPID2, time_t tMarryTime, const char* szName1, const char* szName2);

			void	RequestUpdate(DWORD dwPID1, DWORD dwPID2, int iUpdatePoint, BYTE byMarried);
			void	Update(DWORD dwPID1, DWORD dwPID2, long lTotalPoint, BYTE byMarried);

			void	RequestRemove(DWORD dwPID1, DWORD dwPID2);
			void	Remove(DWORD dwPID1, DWORD dwPID2);

			//void	P2PLogin(DWORD dwPID);
			//void	P2PLogout(DWORD dwPID);

			void	Login(LPCHARACTER ch);

			void	Logout(DWORD pid);
			void	Logout(LPCHARACTER ch);

			void	WeddingReady(DWORD dwPID1, DWORD dwPID2, DWORD dwMapIndex);
			void	WeddingStart(DWORD dwPID1, DWORD dwPID2);
			void	WeddingEnd(DWORD dwPID1, DWORD dwPID2);

			void	RequestEndWedding(DWORD dwPID1, DWORD dwPID2);

			template <typename Func>
				Func	for_each_wedding(Func f);

		private:
			TR1_NS::unordered_set<TMarriage*> m_Marriages;
			std::map<DWORD, TMarriage *> m_MarriageByPID;
			std::set<std::pair<DWORD, DWORD> > m_setWedding;
	};

	template <typename Func>
		Func CManager::for_each_wedding(Func f)
		{
			for (itertype(m_setWedding) it = m_setWedding.begin(); it!=m_setWedding.end(); ++it)
			{
				TMarriage* pMarriage = Get(it->first);
				if (pMarriage)
					f(pMarriage);
			}
			return f;
		}

}

#endif
