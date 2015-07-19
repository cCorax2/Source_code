// vim: ts=8 sw=4
#ifndef __INC_PRIV_MANAGER_H
#define __INC_PRIV_MANAGER_H

#include "Peer.h"
#include <queue>
#include <utility>

struct TPrivEmpireData
{
    BYTE type;
    int value;
    bool bRemoved;
    BYTE empire;

	// ADD_EMPIRE_PRIV_TIME
    time_t end_time_sec;

    TPrivEmpireData(BYTE type, int value, BYTE empire, time_t end_time_sec)
	: type(type), value(value), bRemoved(false), empire(empire), end_time_sec(end_time_sec)
    {}
	// END_OF_ADD_EMPIRE_PRIV_TIME
};

/**
 * @version 05/06/08	Bang2ni - 지속시간 추가
 */
struct TPrivGuildData
{
    BYTE type;
    int value;
    bool bRemoved;
    DWORD guild_id;

	// ADD_GUILD_PRIV_TIME
    time_t end_time_sec;	///< 지속시간

    TPrivGuildData(BYTE type, int value, DWORD guild_id, time_t _end_time_sec)
	: type(type), value(value), bRemoved(false), guild_id(guild_id), end_time_sec(_end_time_sec )
    {}
	// END_OF_ADD_GUILD_PRIV_TIME
};

struct TPrivCharData
{
    BYTE type;
    int value;
    bool bRemoved;
    DWORD pid;
    TPrivCharData(BYTE type, int value, DWORD pid)
	: type(type), value(value), bRemoved(false), pid(pid)
    {}
};

/**
 * @version 05/06/08	Bang2ni - Guild privilege 관련 함수 지속 시간 추가
 */
class CPrivManager : public singleton<CPrivManager>
{
    public: 
	CPrivManager();
	virtual ~CPrivManager();

	// ADD_GUILD_PRIV_TIME
	void AddGuildPriv(DWORD guild_id, BYTE type, int value, time_t time_sec);
	// END_OF_ADD_GUILD_PRIV_TIME

	// ADD_EMPIRE_PRIV_TIME	
	void AddEmpirePriv(BYTE empire, BYTE type, int value, time_t time_sec);
	// END_OF_ADD_EMPIRE_PRIV_TIME
	
	void AddCharPriv(DWORD pid, BYTE type, int value);

	void Update();

	void SendPrivOnSetup(CPeer* peer);

    private:

	// ADD_GUILD_PRIV_TIME
	void SendChangeGuildPriv(DWORD guild_id, BYTE type, int value, time_t end_time_sec);
	// END_OF_ADD_GUILD_PRIV_TIME

	// ADD_EMPIRE_PRIV_TIME	
	void SendChangeEmpirePriv(BYTE empire, BYTE type, int value, time_t end_time_sec);
	// END_OF_ADD_EMPIRE_PRIV_TIME

	void SendChangeCharPriv(DWORD pid, BYTE type, int value);

	typedef std::pair<time_t, TPrivCharData *> stPairChar;
	typedef std::pair<time_t, TPrivGuildData*> stPairGuild;
	typedef std::pair<time_t, TPrivEmpireData*> stPairEmpire;

	std::priority_queue<stPairChar, std::vector<stPairChar>, std::greater<stPairChar> >
	    m_pqPrivChar;
	std::priority_queue<stPairGuild, std::vector<stPairGuild>, std::greater<stPairGuild> > 
	    m_pqPrivGuild;
	std::priority_queue<stPairEmpire, std::vector<stPairEmpire>, std::greater<stPairEmpire> >
	    m_pqPrivEmpire;

	TPrivEmpireData* m_aaPrivEmpire[MAX_PRIV_NUM][EMPIRE_MAX_NUM];
	std::map<DWORD, TPrivGuildData*> m_aPrivGuild[MAX_PRIV_NUM];
	std::map<DWORD, TPrivCharData*> m_aPrivChar[MAX_PRIV_NUM];
};

#endif
