#include "stdafx.h"
#include "HB.h"
#include "Main.h"
#include "DBManager.h"

#include <memory>

PlayerHB::PlayerHB()
{
	m_iExpireTime = 3600; // 1 hour hotbackup default.
}

PlayerHB::~PlayerHB()
{
}

bool PlayerHB::Initialize()
{
	char szQuery[128];
	snprintf(szQuery, sizeof(szQuery), "SHOW CREATE TABLE player%s", GetTablePostfix());

	std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery));

	if (pMsg->Get()->uiNumRows == 0)
		return false;

	MYSQL_ROW row = mysql_fetch_row(pMsg->Get()->pSQLResult);
	m_stCreateTableQuery = row[1];
	return true;
}

//
// @version	05/07/05 Bang2ni - id 에 해당하는 data 가 없을 때 쿼리하고 data 를 insert  하는코드 추가.
//
void PlayerHB::Put(DWORD id)
{
	itertype(m_map_data) it = m_map_data.find(id);

	if (it == m_map_data.end())
	{
		Query(id);
		m_map_data.insert(std::pair< DWORD, time_t >(id, get_dword_time()));
		return;
	}

	if (time(0) - it->second > m_iExpireTime)
		Query(id);
}

//
// @version	05/07/05 Bang2ni - Query string 버퍼가 작아서 늘려줌.
//
bool PlayerHB::Query(DWORD id)
{
	time_t ct = time(0);
	struct tm curr_tm = *localtime(&ct);
	char szTableName[64];
	snprintf(szTableName, sizeof(szTableName), "hb_%02d%02d%02d%02d_player%s", 
			curr_tm.tm_year - 100, curr_tm.tm_mon + 1, curr_tm.tm_mday, curr_tm.tm_hour, GetTablePostfix());

	char szQuery[4096];

	if (m_stTableName.compare(szTableName))
	{
		char szFind[32];
		snprintf(szFind, sizeof(szFind), "CREATE TABLE `player%s`", GetTablePostfix());
		int pos = m_stCreateTableQuery.find(szFind);

		if (pos < 0)
		{
			sys_err("cannot find %s ", szFind);	
		//	sys_err("cannot find %s in %s", szFind, m_stCreateTableQuery.c_str());
			return false;
		}

		snprintf(szQuery, sizeof(szQuery), "CREATE TABLE IF NOT EXISTS %s%s", szTableName, m_stCreateTableQuery.c_str() + strlen(szFind));
	//	sys_log(0, "%s", szQuery);
		std::auto_ptr<SQLMsg> pMsg(CDBManager::instance().DirectQuery(szQuery, SQL_HOTBACKUP));
		m_stTableName = szTableName;
	}

	snprintf(szQuery, sizeof(szQuery), "REPLACE INTO %s SELECT * FROM %splayer%s WHERE id=%u", m_stTableName.c_str(), GetPlayerDBName(), GetTablePostfix(), id);
	CDBManager::instance().AsyncQuery(szQuery, SQL_HOTBACKUP);
	return true;
}

