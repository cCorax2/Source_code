// vim: ts=8 sw=4
#ifndef __INC_MONEY_LOG
#define __INC_MONEY_LOG

#include <map>

class CMoneyLog : public singleton<CMoneyLog>
{
    public:
	CMoneyLog();
	virtual ~CMoneyLog();

	void Save();
	void AddLog(BYTE bType, DWORD dwVnum, int iGold);

    private:
	std::map<DWORD, int> m_MoneyLogContainer[MONEY_LOG_TYPE_MAX_NUM];
};

#endif
