#ifndef __INC_METIN_II_DB_MANAGER_H__
#define __INC_METIN_II_DB_MANAGER_H__

#include "../../libsql/AsyncSQL.h"
#include "any_function.h"

enum
{
	QUERY_TYPE_RETURN = 1,
	QUERY_TYPE_FUNCTION = 2,
	QUERY_TYPE_AFTER_FUNCTION = 3,
};

enum
{
	QID_SAFEBOX_SIZE,
	QID_DB_STRING,
	QID_AUTH_LOGIN,
	QID_AUTH_LOGIN_OPENID,
	QID_LOTTO,
	QID_HIGHSCORE_REGISTER,
	QID_HIGHSCORE_SHOW,
	QID_BILLING_GET_TIME,
	QID_BILLING_CHECK,

	// BLOCK_CHAT
	QID_BLOCK_CHAT_LIST,
	// END_OF_BLOCK_CHAT

	// PCBANG_IP_LIST
	QID_PCBANG_IP_LIST_CHECK,
	QID_PCBANG_IP_LIST_SELECT,
	// END_OF_PCBANG_IP_LIST

	// PROTECT_CHILD_FOR_NEWCIBN
	QID_PROTECT_CHILD,
	// END_PROTECT_CHILD_FOR_NEWCIBN

	QID_BRAZIL_CREATE_ID,
	QID_JAPAN_CREATE_ID,
};

typedef struct SUseTime
{
	DWORD	dwLoginKey;
	char        szLogin[LOGIN_MAX_LEN+1];
	BYTE        bBillType;
	DWORD       dwUseSec;
	char        szIP[MAX_HOST_LENGTH+1];
} TUseTime;

class CQueryInfo
{
	public:
		int	iQueryType;
};

class CReturnQueryInfo : public CQueryInfo
{
	public:
		int	iType;
		DWORD	dwIdent;
		void			*	pvData;
};

class CFuncQueryInfo : public CQueryInfo
{
	public:
		any_function f;
};

class CFuncAfterQueryInfo : public CQueryInfo
{
	public:
		any_void_function f;
};

class CLoginData;


class DBManager : public singleton<DBManager>
{
	public:
		DBManager();
		virtual ~DBManager();

		bool			IsConnected();

		bool			Connect(const char * host, const int port, const char * user, const char * pwd, const char * db);
		void			Query(const char * c_pszFormat, ...);

		SQLMsg *		DirectQuery(const char * c_pszFormat, ...);
		void			ReturnQuery(int iType, DWORD dwIdent, void* pvData, const char * c_pszFormat, ...);

		void			Process();
		void			AnalyzeReturnQuery(SQLMsg * pmsg);

		void			SendMoneyLog(BYTE type, DWORD vnum, int gold);

		void			LoginPrepare(BYTE bBillType, DWORD dwBillID, long lRemainSecs, LPDESC d, DWORD * pdwClientKey, int * paiPremiumTimes = NULL);
		void			SendAuthLogin(LPDESC d);
		void			SendLoginPing(const char * c_pszLogin);

		void			InsertLoginData(CLoginData * pkLD);
		void			DeleteLoginData(CLoginData * pkLD);
		CLoginData *		GetLoginData(DWORD dwKey);
		void			SetBilling(DWORD dwKey, bool bOn, bool bSkipPush = false);
		void			PushBilling(CLoginData * pkLD);
		void			FlushBilling(bool bForce=false);
		void			CheckBilling();

		void			StopAllBilling(); // 20050503.ipkn.DB-AUTH 접속 종료시 빌링 테이블 모두 지우기 (재연결시 복구함)

		DWORD			CountQuery()		{ return m_sql.CountQuery(); }
		DWORD			CountQueryResult()	{ return m_sql.CountResult(); }
		void			ResetQueryResult()	{ m_sql.ResetQueryFinished(); }

		// BLOCK EXCEPTION
		void			RequestBlockException(const char *login, int cmd);
		// BLOCK EXCEPTION

		void			LoadDBString();
		const std::string &	GetDBString(const std::string& key);
		const std::vector<std::string> & GetGreetMessage();

		template<class Functor> void FuncQuery(Functor f, const char * c_pszFormat, ...); // 결과를 f인자로 호출함 (SQLMsg *) 알아서 해제됨
		template<class Functor> void FuncAfterQuery(Functor f, const char * c_pszFormat, ...); // 끝나고 나면 f가 호출됨 void			f(void) 형태

		size_t EscapeString(char* dst, size_t dstSize, const char *src, size_t srcSize);

	private:
		SQLMsg *				PopResult();

		CAsyncSQL				m_sql;
		CAsyncSQL				m_sql_direct;
		bool					m_bIsConnect;

		std::map<std::string, std::string>	m_map_dbstring;
		std::vector<std::string>		m_vec_GreetMessage;
		std::map<DWORD, CLoginData *>		m_map_pkLoginData;
		std::map<std::string, CLoginData *>	mapLDBilling;
		std::vector<TUseTime>			m_vec_kUseTime;
};

template <class Functor> void DBManager::FuncQuery(Functor f, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, 4096, c_pszFormat, args);
	va_end(args);

	CFuncQueryInfo * p = M2_NEW CFuncQueryInfo;

	p->iQueryType = QUERY_TYPE_FUNCTION;
	p->f = f;

	m_sql.ReturnQuery(szQuery, p);
}

template <class Functor> void DBManager::FuncAfterQuery(Functor f, const char* c_pszFormat, ...)
{
	char szQuery[4096];
	va_list args;

	va_start(args, c_pszFormat);
	vsnprintf(szQuery, 4096, c_pszFormat, args);
	va_end(args);

	CFuncAfterQueryInfo * p = M2_NEW CFuncAfterQueryInfo;

	p->iQueryType = QUERY_TYPE_AFTER_FUNCTION;
	p->f = f;

	m_sql.ReturnQuery(szQuery, p);
}

////////////////////////////////////////////////////////////////
typedef struct SHighscoreRegisterQueryInfo
{
	char    szBoard[20+1]; 
	DWORD   dwPID;
	int     iValue;
	bool    bOrder;
} THighscoreRegisterQueryInfo;

extern void SendBillingExpire(const char * c_pszLogin, BYTE bBillType, int iSecs, CLoginData * pkLD);
extern void VCardUse(LPCHARACTER CardOwner, LPCHARACTER CardTaker, LPITEM item);


// ACCOUNT_DB
class AccountDB : public singleton<AccountDB>
{
	public:
		AccountDB();

		bool IsConnected();
		bool Connect(const char * host, const int port, const char * user, const char * pwd, const char * db);
		bool ConnectAsync(const char * host, const int port, const char * user, const char * pwd, const char * db, const char * locale);

		SQLMsg* DirectQuery(const char * query);		
		void ReturnQuery(int iType, DWORD dwIdent, void * pvData, const char * c_pszFormat, ...);
		void AsyncQuery(const char* query);

		void SetLocale(const std::string & stLocale);

		void Process();

	private:
		SQLMsg * PopResult();
		void AnalyzeReturnQuery(SQLMsg * pMsg);

		CAsyncSQL2	m_sql_direct;
		CAsyncSQL2	m_sql;
		bool		m_IsConnect;

};
//END_ACCOUNT_DB

#endif
