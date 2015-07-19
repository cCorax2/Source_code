// vim: ts=4 sw=4
#ifndef METIN2_MONARCH_H
#define METIN2_MONARCH_H

#include "../../libthecore/include/stdafx.h"
#include <map>
#include <vector>
#include "../../common/singleton.h"
#include "../../common/tables.h"

class CMonarch : public singleton<CMonarch>
{
	public:
		typedef std::map<DWORD, MonarchElectionInfo*> MAP_MONARCHELECTION;
		typedef std::vector<MonarchCandidacy> VEC_MONARCHCANDIDACY;

		CMonarch();
		virtual ~CMonarch();

		bool VoteMonarch(DWORD pid, DWORD selectedpid);
		void ElectMonarch();

		bool IsCandidacy(DWORD pid);
		bool AddCandidacy(DWORD pid, const char * name);
		bool DelCandidacy(const char * name);

		bool LoadMonarch();
		bool SetMonarch(const char * name);
		bool DelMonarch(int Empire);
		bool DelMonarch(const char * name);

		bool IsMonarch(int Empire, DWORD pid);
		bool AddMoney(int Empire, int64_t Money);
		bool DecMoney(int Empire, int64_t Money);
		bool TakeMoney(int Empire, DWORD pid, int64_t Money);

		TMonarchInfo* GetMonarch()
		{
			return &m_MonarchInfo;
		}

		VEC_MONARCHCANDIDACY& GetVecMonarchCandidacy()
		{
			return m_vec_MonarchCandidacy;
		}

		size_t MonarchCandidacySize()
		{
			return m_vec_MonarchCandidacy.size();
		}

	private:
		int GetCandidacyIndex(DWORD pid);

		MAP_MONARCHELECTION m_map_MonarchElection;
		VEC_MONARCHCANDIDACY m_vec_MonarchCandidacy;
		TMonarchInfo m_MonarchInfo;

		MonarchElectionInfo* GetMonarchElection(DWORD pid)
		{
			MAP_MONARCHELECTION::iterator it = m_map_MonarchElection.find(pid);

			if (it != m_map_MonarchElection.end())
				return it->second;

			return NULL;
		}
};

#endif
