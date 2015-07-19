#include "stdafx.h"
#include <sstream>
#ifndef __WIN32__
#include <ifaddrs.h>
#endif

#include "constants.h"
#include "utils.h"
#include "log.h"
#include "desc.h"
#include "desc_manager.h"
#include "item_manager.h"
#include "p2p.h"
#include "char.h"
#include "ip_ban.h"
#include "war_map.h"
#include "locale_service.h"
#include "config.h"
#include "dev_log.h"
#include "db.h"
#include "skill_power.h"
#include "check_server.h"

using std::string;

BYTE	g_bChannel = 0;
WORD	mother_port = 50080;
int		passes_per_sec = 25;
WORD	db_port = 0;
WORD	p2p_port = 50900;
char	db_addr[ADDRESS_MAX_LEN + 1];
int		save_event_second_cycle = passes_per_sec * 120;	// 3분
int		ping_event_second_cycle = passes_per_sec * 60;
bool	g_bNoMoreClient = false;
bool	g_bNoRegen = false;
bool	g_bNoPasspod = false;

// TRAFFIC_PROFILER
bool		g_bTrafficProfileOn = false;
DWORD		g_dwTrafficProfileFlushCycle = 3600;
// END_OF_TRAFFIC_PROFILER

int			test_server = 0;
int			speed_server = 0;
#ifdef __AUCTION__
int			auction_server = 0;
#endif
bool		distribution_test_server = false;
bool		china_event_server = false;
bool		guild_mark_server = true;
BYTE		guild_mark_min_level = 3;
bool		no_wander = false;
int		g_iUserLimit = 32768;

char		g_szPublicIP[16] = "0";
char		g_szInternalIP[16] = "0";
bool		g_bSkillDisable = false;
int			g_iFullUserCount = 1200;
int			g_iBusyUserCount = 650;
//Canada
//int			g_iFullUserCount = 600;
//int			g_iBusyUserCount = 350;
//Brazil
//int			g_iFullUserCount = 650;
//int			g_iBusyUserCount = 450;
bool		g_bEmpireWhisper = true;
BYTE		g_bAuthServer = false;

bool		g_bCheckClientVersion = true;
string	g_stClientVersion = "1215955205";

BYTE		g_bBilling = false;

string	g_stAuthMasterIP;
WORD		g_wAuthMasterPort = 0;

static std::set<DWORD> s_set_dwFileCRC;
static std::set<DWORD> s_set_dwProcessCRC;

string g_stHostname = "";
string g_table_postfix = "";

string g_stQuestDir = "./quest";
//string g_stQuestObjectDir = "./quest/object";
string g_stDefaultQuestObjectDir = "./quest/object";
std::set<string> g_setQuestObjectDir;

std::vector<std::string>	g_stAdminPageIP;
std::string	g_stAdminPagePassword = "SHOWMETHEMONEY";

string g_stBlockDate = "30000705";

extern string g_stLocale;

char	teen_addr[ADDRESS_MAX_LEN + 1] = {0};
WORD	teen_port	= 0;

int SPEEDHACK_LIMIT_COUNT   = 50;
int SPEEDHACK_LIMIT_BONUS   = 80;
int g_iSyncHackLimitCount = 10;

//시야 = VIEW_RANGE + VIEW_BONUS_RANGE
//VIEW_BONUSE_RANGE : 클라이언트와 시야 처리에서너무 딱 떨어질경우 문제가 발생할수있어 500CM의 여분을 항상준다.
int VIEW_RANGE = 5000;
int VIEW_BONUS_RANGE = 500;

int g_server_id = 0;
string g_strWebMallURL = "www.metin2.de";

unsigned int g_uiSpamBlockDuration = 60 * 15; // 기본 15분
unsigned int g_uiSpamBlockScore = 100; // 기본 100점
unsigned int g_uiSpamReloadCycle = 60 * 10; // 기본 10분

bool		g_bCheckMultiHack = true;

int			g_iSpamBlockMaxLevel = 10;

void		LoadStateUserCount();
void		LoadValidCRCList();
bool		LoadClientVersion();
bool            g_protectNormalPlayer   = false;        // 범법자가 "평화모드" 인 일반유저를 공격하지 못함
bool            g_noticeBattleZone      = false;        // 중립지대에 입장하면 안내메세지를 알려줌

bool		isHackShieldEnable = false;
int			HackShield_FirstCheckWaitTime = passes_per_sec * 30;
int			HackShield_CheckCycleTime = passes_per_sec * 180;

bool		bXTrapEnabled = false;

int gPlayerMaxLevel = 99;

bool g_BlockCharCreation = false;


//OPENID
int		openid_server = 0;
char	openid_host[256];
char	openid_uri[256];

bool is_string_true(const char * string)
{
	bool	result = 0;
	if (isnhdigit(*string))
	{
		str_to_number(result, string);
		return result > 0 ? true : false;
	}
	else if (LOWER(*string) == 't')
		return true;
	else
		return false;
}

static std::set<int> s_set_map_allows;

bool map_allow_find(int index)
{
	if (g_bAuthServer)
		return false;

	if (s_set_map_allows.find(index) == s_set_map_allows.end())
		return false;

	return true;
}

void map_allow_log()
{
	std::set<int>::iterator i;

	for (i = s_set_map_allows.begin(); i != s_set_map_allows.end(); ++i)
		sys_log(0, "MAP_ALLOW: %d", *i);
}

void map_allow_add(int index)
{
	if (map_allow_find(index) == true)
	{
		fprintf(stdout, "!!! FATAL ERROR !!! multiple MAP_ALLOW setting!!\n");
		exit(1);
	}

	fprintf(stdout, "MAP ALLOW %d\n", index);
	s_set_map_allows.insert(index);
}

void map_allow_copy(long * pl, int size)
{
	int iCount = 0;
	std::set<int>::iterator it = s_set_map_allows.begin();

	while (it != s_set_map_allows.end())
	{
		int i = *(it++);
		*(pl++) = i;

		if (++iCount > size)
			break;
	}
}

static void FN_add_adminpageIP(char *line)
{
	char	*last;
	const char *delim = " \t\r\n";
	char *v = strtok_r(line, delim, &last);

	while (v)
	{
		g_stAdminPageIP.push_back(v);
		v = strtok_r(NULL, delim, &last);
	}
}

static void FN_log_adminpage()
{
	itertype(g_stAdminPageIP) iter = g_stAdminPageIP.begin();

	while (iter != g_stAdminPageIP.end())
	{
		dev_log(LOG_DEB0, "ADMIN_PAGE_IP = %s", (*iter).c_str());
		++iter;
	}

	dev_log(LOG_DEB0, "ADMIN_PAGE_PASSWORD = %s", g_stAdminPagePassword.c_str());
}


bool GetIPInfo()
{
#ifndef __WIN32__
	struct ifaddrs* ifaddrp = NULL;

	if (0 != getifaddrs(&ifaddrp))
		return false;

	for( struct ifaddrs* ifap=ifaddrp ; NULL != ifap ; ifap = ifap->ifa_next )
	{
		struct sockaddr_in * sai = (struct sockaddr_in *) ifap->ifa_addr;

		if (!ifap->ifa_netmask ||  // ignore if no netmask
				sai->sin_addr.s_addr == 0 || // ignore if address is 0.0.0.0
				sai->sin_addr.s_addr == 16777343) // ignore if address is 127.0.0.1
			continue;
#else
	WSADATA wsa_data;
	char host_name[100];
	HOSTENT* host_ent;
	int n = 0;

	if (WSAStartup(0x0101, &wsa_data)) {
		return false;
	}

	gethostname(host_name, sizeof(host_name));
	host_ent = gethostbyname(host_name);
	if (host_ent == NULL) {
		return false;
	}
	for ( ; host_ent->h_addr_list[n] != NULL; ++n) {
		struct sockaddr_in addr;
		struct sockaddr_in* sai = &addr;
		memcpy(&sai->sin_addr.s_addr, host_ent->h_addr_list[n], host_ent->h_length);
#endif

		char * netip = inet_ntoa(sai->sin_addr);

		if (!strncmp(netip, "0.0", 7)) // ignore if address is starting with 192
		{
			strlcpy(g_szInternalIP, netip, sizeof(g_szInternalIP));
#ifndef __WIN32__
			fprintf(stderr, "INTERNAL_IP: %s interface %s\n", netip, ifap->ifa_name);
#else
			fprintf(stderr, "INTERNAL_IP: %s\n", netip);
#endif
		}
		else if (!strncmp(netip, "10.", 3))
		{
			strlcpy(g_szInternalIP, netip, sizeof(g_szInternalIP));
#ifndef __WIN32__
			fprintf(stderr, "INTERNAL_IP: %s interface %s\n", netip, ifap->ifa_name);
#else
			fprintf(stderr, "INTERNAL_IP: %s\n", netip);
#endif
		}
		else if (g_szPublicIP[0] == '0')
		{
			strlcpy(g_szPublicIP, netip, sizeof(g_szPublicIP));
#ifndef __WIN32__
			fprintf(stderr, "PUBLIC_IP: %s interface %s\n", netip, ifap->ifa_name);
#else
			fprintf(stderr, "PUBLIC_IP: %s\n", netip);
#endif
		}
	}

#ifndef __WIN32__
	freeifaddrs( ifaddrp );
#else
	WSACleanup();
#endif

	if (g_szPublicIP[0] != '0')
		return true;
	else
		return false;
}

void config_init(const string& st_localeServiceName)
{
	FILE	*fp;

	char	buf[256];
	char	token_string[256];
	char	value_string[256];

	// LOCALE_SERVICE
	string	st_configFileName;

	st_configFileName.reserve(32);
	st_configFileName = "CONFIG";

	if (!st_localeServiceName.empty())
	{
		st_configFileName += ".";
		st_configFileName += st_localeServiceName;
	}
	// END_OF_LOCALE_SERVICE

	if (!(fp = fopen(st_configFileName.c_str(), "r")))
	{
		fprintf(stderr, "Can not open [%s]\n", st_configFileName.c_str());
		exit(1);
	}

	if (!GetIPInfo())
	{
		fprintf(stderr, "Can not get public ip address\n");
		exit(1);
	}

	char db_host[2][64], db_user[2][64], db_pwd[2][64], db_db[2][64];
	// ... 아... db_port는 이미 있는데... 네이밍 어찌해야함...
	int mysql_db_port[2];

	for (int n = 0; n < 2; ++n)
	{
		*db_host[n]	= '\0';
		*db_user[n] = '\0';
		*db_pwd[n]= '\0';
		*db_db[n]= '\0';
		mysql_db_port[n] = 0;
	}

	char log_host[64], log_user[64], log_pwd[64], log_db[64];
	int log_port = 0;

	*log_host = '\0';
	*log_user = '\0';
	*log_pwd = '\0';
	*log_db = '\0';


	// DB에서 로케일정보를 세팅하기위해서는 다른 세팅값보다 선행되어서
	// DB정보만 읽어와 로케일 세팅을 한후 다른 세팅을 적용시켜야한다.
	// 이유는 로케일관련된 초기화 루틴이 곳곳에 존재하기 때문.

	bool isCommonSQL = false;	
	bool isPlayerSQL = false;

	FILE* fpOnlyForDB;

	if (!(fpOnlyForDB = fopen(st_configFileName.c_str(), "r")))
	{
		fprintf(stderr, "Can not open [%s]\n", st_configFileName.c_str());
		exit(1);
	}

	while (fgets(buf, 256, fpOnlyForDB))
	{
		parse_token(buf, token_string, value_string);

		TOKEN("BLOCK_LOGIN")
		{
			g_stBlockDate = value_string;
		}

		TOKEN("adminpage_ip")
		{
			FN_add_adminpageIP(value_string);
			//g_stAdminPageIP[0] = value_string;
		}

		TOKEN("adminpage_ip1")
		{
			FN_add_adminpageIP(value_string);
			//g_stAdminPageIP[0] = value_string;
		}

		TOKEN("adminpage_ip2")
		{
			FN_add_adminpageIP(value_string);
			//g_stAdminPageIP[1] = value_string;
		}

		TOKEN("adminpage_ip3")
		{
			FN_add_adminpageIP(value_string);
			//g_stAdminPageIP[2] = value_string;
		}

		TOKEN("adminpage_password")
		{
			g_stAdminPagePassword = value_string;
		}

		TOKEN("hostname")
		{
			g_stHostname = value_string;
			fprintf(stdout, "HOSTNAME: %s\n", g_stHostname.c_str());
			continue;
		}

		TOKEN("channel")
		{
			str_to_number(g_bChannel, value_string);
			continue;
		}

		TOKEN("player_sql")
		{
			const char * line = two_arguments(value_string, db_host[0], sizeof(db_host[0]), db_user[0], sizeof(db_user[0]));
			line = two_arguments(line, db_pwd[0], sizeof(db_pwd[0]), db_db[0], sizeof(db_db[0]));

			if (NULL != line[0])
			{
				char buf[256];
				one_argument(line, buf, sizeof(buf));
				str_to_number(mysql_db_port[0], buf);
			}

			if (!*db_host[0] || !*db_user[0] || !*db_pwd[0] || !*db_db[0])
			{
				fprintf(stderr, "PLAYER_SQL syntax: logsql <host user password db>\n");
				exit(1);
			}

			char buf[1024];
			snprintf(buf, sizeof(buf), "PLAYER_SQL: %s %s %s %s %d", db_host[0], db_user[0], db_pwd[0], db_db[0], mysql_db_port[0]);
			isPlayerSQL = true;
			continue;
		}

		TOKEN("common_sql")
		{
			const char * line = two_arguments(value_string, db_host[1], sizeof(db_host[1]), db_user[1], sizeof(db_user[1]));
			line = two_arguments(line, db_pwd[1], sizeof(db_pwd[1]), db_db[1], sizeof(db_db[1]));

			if (NULL != line[0])
			{
				char buf[256];
				one_argument(line, buf, sizeof(buf));
				str_to_number(mysql_db_port[1], buf);
			}

			if (!*db_host[1] || !*db_user[1] || !*db_pwd[1] || !*db_db[1])
			{
				fprintf(stderr, "COMMON_SQL syntax: logsql <host user password db>\n");
				exit(1);
			}

			char buf[1024];
			snprintf(buf, sizeof(buf), "COMMON_SQL: %s %s %s %s %d", db_host[1], db_user[1], db_pwd[1], db_db[1], mysql_db_port[1]);
			isCommonSQL = true;
			continue;
		}

		TOKEN("log_sql")
		{
			const char * line = two_arguments(value_string, log_host, sizeof(log_host), log_user, sizeof(log_user));
			line = two_arguments(line, log_pwd, sizeof(log_pwd), log_db, sizeof(log_db));

			if (NULL != line[0])
			{
				char buf[256];
				one_argument(line, buf, sizeof(buf));
				str_to_number(log_port, buf);
			}

			if (!*log_host || !*log_user || !*log_pwd || !*log_db)
			{
				fprintf(stderr, "LOG_SQL syntax: logsql <host user password db>\n");
				exit(1);
			}

			char buf[1024];
			snprintf(buf, sizeof(buf), "LOG_SQL: %s %s %s %s %d", log_host, log_user, log_pwd, log_db, log_port);
			continue;
		}

		
		//OPENID		
		TOKEN("WEB_AUTH")
		{
			const char * line = two_arguments(value_string, openid_host, sizeof(openid_host), openid_uri, sizeof(openid_uri));

			if (!*openid_host || !*openid_uri)
			{
				fprintf(stderr, "WEB_AUTH syntax error (ex: WEB_AUTH <host(metin2.co.kr) uri(/kyw/gameauth.php)>\n");
				exit(1);
			}

			char buf[1024];
			openid_server = 1;
			snprintf(buf, sizeof(buf), "WEB_AUTH: %s %s", openid_host, openid_uri);
			continue;
		}
	}

	//처리가 끝났으니 파일을 닫자.
	fclose(fpOnlyForDB);

	// CONFIG_SQL_INFO_ERROR
	if (!isCommonSQL)
	{
		puts("LOAD_COMMON_SQL_INFO_FAILURE:");
		puts("");
		puts("CONFIG:");
		puts("------------------------------------------------");
		puts("COMMON_SQL: HOST USER PASSWORD DATABASE");
		puts("");
		exit(1);
	}

	if (!isPlayerSQL)
	{
		puts("LOAD_PLAYER_SQL_INFO_FAILURE:");
		puts("");
		puts("CONFIG:");
		puts("------------------------------------------------");
		puts("PLAYER_SQL: HOST USER PASSWORD DATABASE");
		puts("");
		exit(1);
	}

	// Common DB 가 Locale 정보를 가지고 있기 때문에 가장 먼저 접속해야 한다.
	AccountDB::instance().Connect(db_host[1], mysql_db_port[1], db_user[1], db_pwd[1], db_db[1]);

	if (false == AccountDB::instance().IsConnected())
	{
		fprintf(stderr, "cannot start server while no common sql connected\n");
		exit(1);
	}

	fprintf(stdout, "CommonSQL connected\n");

	// 로케일 정보를 가져오자 
	// <경고> 쿼리문에 절대 조건문(WHERE) 달지 마세요. (다른 지역에서 문제가 생길수 있습니다)
	{
		char szQuery[512];
		snprintf(szQuery, sizeof(szQuery), "SELECT mKey, mValue FROM locale");

		std::auto_ptr<SQLMsg> pMsg(AccountDB::instance().DirectQuery(szQuery));

		if (pMsg->Get()->uiNumRows == 0)
		{
			fprintf(stderr, "COMMON_SQL: DirectQuery failed : %s\n", szQuery);
			exit(1);
		}

		MYSQL_ROW row; 

		while (NULL != (row = mysql_fetch_row(pMsg->Get()->pSQLResult)))
		{
			// 로케일 세팅
			if (strcasecmp(row[0], "LOCALE") == 0)
			{
				if (LocaleService_Init(row[1]) == false)
				{
					fprintf(stderr, "COMMON_SQL: invalid locale key %s\n", row[1]);
					exit(1);
				}
			}
		}
	}

	// 로케일 정보를 COMMON SQL에 세팅해준다.
	// 참고로 g_stLocale 정보는 LocaleService_Init() 내부에서 세팅된다.
	fprintf(stdout, "Setting DB to locale %s\n", g_stLocale.c_str());

	AccountDB::instance().SetLocale(g_stLocale);

	AccountDB::instance().ConnectAsync(db_host[1], mysql_db_port[1], db_user[1], db_pwd[1], db_db[1], g_stLocale.c_str());

	// Player DB 접속
	DBManager::instance().Connect(db_host[0], mysql_db_port[0], db_user[0], db_pwd[0], db_db[0]);

	if (!DBManager::instance().IsConnected())
	{
		fprintf(stderr, "PlayerSQL.ConnectError\n");
		exit(1);
	}

	fprintf(stdout, "PlayerSQL connected\n");

	if (false == g_bAuthServer) // 인증 서버가 아닐 경우
	{
		// Log DB 접속
		LogManager::instance().Connect(log_host, log_port, log_user, log_pwd, log_db);

		if (!LogManager::instance().IsConnected())
		{
			fprintf(stderr, "LogSQL.ConnectError\n");
			exit(1);
		}

		fprintf(stdout, "LogSQL connected\n");

		LogManager::instance().BootLog(g_stHostname.c_str(), g_bChannel);
	}

	// SKILL_POWER_BY_LEVEL
	// 스트링 비교의 문제로 인해서 AccountDB::instance().SetLocale(g_stLocale) 후부터 한다.
	// 물론 국내는 별로 문제가 안된다(해외가 문제)
	{
		char szQuery[256];
		snprintf(szQuery, sizeof(szQuery), "SELECT mValue FROM locale WHERE mKey='SKILL_POWER_BY_LEVEL'");
		std::auto_ptr<SQLMsg> pMsg(AccountDB::instance().DirectQuery(szQuery));

		if (pMsg->Get()->uiNumRows == 0)
		{
			fprintf(stderr, "[SKILL_PERCENT] Query failed: %s", szQuery);
			exit(1);
		}

		MYSQL_ROW row; 

		row = mysql_fetch_row(pMsg->Get()->pSQLResult);

		const char * p = row[0];
		int cnt = 0;
		char num[128];
		int aiBaseSkillPowerByLevelTable[SKILL_MAX_LEVEL+1];

		fprintf(stdout, "SKILL_POWER_BY_LEVEL %s\n", p);
		while (*p != '\0' && cnt < (SKILL_MAX_LEVEL + 1))
		{
			p = one_argument(p, num, sizeof(num));
			aiBaseSkillPowerByLevelTable[cnt++] = atoi(num);

			//fprintf(stdout, "%d %d\n", cnt - 1, aiBaseSkillPowerByLevelTable[cnt - 1]);
			if (*p == '\0')
			{
				if (cnt != (SKILL_MAX_LEVEL + 1))
				{
					fprintf(stderr, "[SKILL_PERCENT] locale table has not enough skill information! (count: %d query: %s)", cnt, szQuery);
					exit(1);
				}

				fprintf(stdout, "SKILL_POWER_BY_LEVEL: Done! (count %d)\n", cnt);
				break;
			}
		}

		// 종족별 스킬 세팅
		for (int job = 0; job < JOB_MAX_NUM * 2; ++job)
		{
			snprintf(szQuery, sizeof(szQuery), "SELECT mValue from locale where mKey='SKILL_POWER_BY_LEVEL_TYPE%d' ORDER BY CAST(mValue AS unsigned)", job);
			std::auto_ptr<SQLMsg> pMsg(AccountDB::instance().DirectQuery(szQuery));

			// 세팅이 안되어있으면 기본테이블을 사용한다.
			if (pMsg->Get()->uiNumRows == 0)
			{
				CTableBySkill::instance().SetSkillPowerByLevelFromType(job, aiBaseSkillPowerByLevelTable);
				continue;
			}

			row = mysql_fetch_row(pMsg->Get()->pSQLResult);
			cnt = 0;
			p = row[0];
			int aiSkillTable[SKILL_MAX_LEVEL + 1];

			fprintf(stdout, "SKILL_POWER_BY_JOB %d %s\n", job, p);
			while (*p != '\0' && cnt < (SKILL_MAX_LEVEL + 1))
			{			
				p = one_argument(p, num, sizeof(num));
				aiSkillTable[cnt++] = atoi(num);

				//fprintf(stdout, "%d %d\n", cnt - 1, aiBaseSkillPowerByLevelTable[cnt - 1]);
				if (*p == '\0')
				{
					if (cnt != (SKILL_MAX_LEVEL + 1))
					{
						fprintf(stderr, "[SKILL_PERCENT] locale table has not enough skill information! (count: %d query: %s)", cnt, szQuery);
						exit(1);
					}

					fprintf(stdout, "SKILL_POWER_BY_JOB: Done! (job: %d count: %d)\n", job, cnt);
					break;
				}
			}

			CTableBySkill::instance().SetSkillPowerByLevelFromType(job, aiSkillTable);
		}		
	}
	// END_SKILL_POWER_BY_LEVEL

	// LOG_KEEP_DAYS_EXTEND
	log_set_expiration_days(2);
	// END_OF_LOG_KEEP_DAYS_EXTEND

	while (fgets(buf, 256, fp))
	{
		parse_token(buf, token_string, value_string);

		TOKEN("empire_whisper")
		{
			bool b_value = 0;
			str_to_number(b_value, value_string);
			g_bEmpireWhisper = !!b_value;
			continue;
		}

		TOKEN("mark_server")
		{
			guild_mark_server = is_string_true(value_string);
			continue;
		}

		TOKEN("mark_min_level")
		{
			str_to_number(guild_mark_min_level, value_string);
			guild_mark_min_level = MINMAX(0, guild_mark_min_level, GUILD_MAX_LEVEL);
			continue;
		}

		TOKEN("port")
		{
			str_to_number(mother_port, value_string);
			continue;
		}

		TOKEN("log_keep_days")
		{
			int i = 0;
			str_to_number(i, value_string);
			log_set_expiration_days(MINMAX(1, i, 90));
			continue;
		}

		TOKEN("passes_per_sec")
		{
			str_to_number(passes_per_sec, value_string);
			continue;
		}

		TOKEN("p2p_port")
		{
			str_to_number(p2p_port, value_string);
			continue;
		}

		TOKEN("db_port")
		{
			str_to_number(db_port, value_string);
			continue;
		}

		TOKEN("db_addr")
		{
			strlcpy(db_addr, value_string, sizeof(db_addr));

			for (int n =0; n < ADDRESS_MAX_LEN; ++n)
			{
				if (db_addr[n] == ' ')
					db_addr[n] = '\0';
			}

			continue;
		}

		TOKEN("save_event_second_cycle")
		{
			int	cycle = 0;
			str_to_number(cycle, value_string);
			save_event_second_cycle = cycle * passes_per_sec;
			continue;
		}

		TOKEN("ping_event_second_cycle")
		{
			int	cycle = 0;
			str_to_number(cycle, value_string);
			ping_event_second_cycle = cycle * passes_per_sec;
			continue;
		}

		TOKEN("table_postfix")
		{
			g_table_postfix = value_string;
			continue;
		}

		TOKEN("test_server")
		{
			printf("-----------------------------------------------\n");
			printf("TEST_SERVER\n");
			printf("-----------------------------------------------\n");
			str_to_number(test_server, value_string);
			continue;
		}

		TOKEN("speed_server")
		{
			printf("-----------------------------------------------\n");
			printf("SPEED_SERVER\n");
			printf("-----------------------------------------------\n");
			str_to_number(speed_server, value_string);
			continue;
		}
#ifdef __AUCTION__
		TOKEN("auction_server")
		{
			printf("-----------------------------------------------\n");
			printf("AUCTION_SERVER\n");
			printf("-----------------------------------------------\n");
			str_to_number(auction_server, value_string);
			continue;
		}
#endif
		TOKEN("distribution_test_server")
		{
			str_to_number(distribution_test_server, value_string);
			continue;
		}

		TOKEN("china_event_server")
		{
			str_to_number(china_event_server, value_string);
			continue;
		}

		TOKEN("shutdowned")
		{
			g_bNoMoreClient = true;
			continue;
		}

		TOKEN("no_regen")
		{
			g_bNoRegen = true;
			continue;
		}

		TOKEN("traffic_profile")
		{
			g_bTrafficProfileOn = true;
			continue;
		}


		TOKEN("map_allow")
		{
			char * p = value_string;
			string stNum;

			for (; *p; p++)
			{   
				if (isnhspace(*p))
				{
					if (stNum.length())
					{
						int	index = 0;
						str_to_number(index, stNum.c_str());
						map_allow_add(index);
						stNum.clear();
					}
				}
				else
					stNum += *p;
			}

			if (stNum.length())
			{
				int	index = 0;
				str_to_number(index, stNum.c_str());
				map_allow_add(index);
			}

			continue;
		}

		TOKEN("no_wander")
		{
			no_wander = true;
			continue;
		}

		TOKEN("user_limit")
		{
			str_to_number(g_iUserLimit, value_string);
			continue;
		}

		TOKEN("skill_disable")
		{
			str_to_number(g_bSkillDisable, value_string);
			continue;
		}

		TOKEN("auth_server")
		{
			char szIP[32];
			char szPort[32];

			two_arguments(value_string, szIP, sizeof(szIP), szPort, sizeof(szPort));

			if (!*szIP || (!*szPort && strcasecmp(szIP, "master")))
			{
				fprintf(stderr, "AUTH_SERVER: syntax error: <ip|master> <port>\n");
				exit(1);
			}

			g_bAuthServer = true;

			LoadBanIP("BANIP");

			if (!strcasecmp(szIP, "master"))
				fprintf(stdout, "AUTH_SERVER: I am the master\n");
			else
			{
				g_stAuthMasterIP = szIP;
				str_to_number(g_wAuthMasterPort, szPort);

				fprintf(stdout, "AUTH_SERVER: master %s %u\n", g_stAuthMasterIP.c_str(), g_wAuthMasterPort);
			}
			continue;
		}

		TOKEN("billing")
		{
			g_bBilling = true;
		}

		TOKEN("quest_dir")
		{
			sys_log(0, "QUEST_DIR SETTING : %s", value_string);
			g_stQuestDir = value_string;
		}

		TOKEN("quest_object_dir")
		{
			//g_stQuestObjectDir = value_string;
			std::istringstream is(value_string);
			sys_log(0, "QUEST_OBJECT_DIR SETTING : %s", value_string);
			string dir;
			while (!is.eof())
			{
				is >> dir;
				if (is.fail())
					break;
				g_setQuestObjectDir.insert(dir);
				sys_log(0, "QUEST_OBJECT_DIR INSERT : %s", dir .c_str());
			}
		}

		TOKEN("teen_addr")
		{
			strlcpy(teen_addr, value_string, sizeof(teen_addr));

			for (int n =0; n < ADDRESS_MAX_LEN; ++n)
			{
				if (teen_addr[n] == ' ')
					teen_addr[n] = '\0';
			}

			continue;
		}

		TOKEN("teen_port")
		{
			str_to_number(teen_port, value_string);
		}

		TOKEN("synchack_limit_count")
		{
			str_to_number(g_iSyncHackLimitCount, value_string);
		}

		TOKEN("speedhack_limit_count")
		{
			str_to_number(SPEEDHACK_LIMIT_COUNT, value_string);
		}

		TOKEN("speedhack_limit_bonus")
		{
			str_to_number(SPEEDHACK_LIMIT_BONUS, value_string);
		}

		TOKEN("server_id")
		{
			str_to_number(g_server_id, value_string);
		}

		TOKEN("mall_url")
		{
			g_strWebMallURL = value_string;
		}

		TOKEN("bind_ip")
		{
			strlcpy(g_szPublicIP, value_string, sizeof(g_szPublicIP));
		}

		TOKEN("view_range")
		{
			str_to_number(VIEW_RANGE, value_string);
		}

		TOKEN("spam_block_duration")
		{
			str_to_number(g_uiSpamBlockDuration, value_string);
		}

		TOKEN("spam_block_score")
		{
			str_to_number(g_uiSpamBlockScore, value_string);
			g_uiSpamBlockScore = MAX(1, g_uiSpamBlockScore);
		}

		TOKEN("spam_block_reload_cycle")
		{
			str_to_number(g_uiSpamReloadCycle, value_string);
			g_uiSpamReloadCycle = MAX(60, g_uiSpamReloadCycle); // 최소 1분
		}

		TOKEN("check_multihack")
		{
			str_to_number(g_bCheckMultiHack, value_string);
		}

		TOKEN("spam_block_max_level")
		{
			str_to_number(g_iSpamBlockMaxLevel, value_string);
		}
		TOKEN("protect_normal_player")
		{
			str_to_number(g_protectNormalPlayer, value_string);
		}
		TOKEN("notice_battle_zone")
		{
			str_to_number(g_noticeBattleZone, value_string);
		}

		TOKEN("hackshield_enable")
		{
			int flag = 0;

			str_to_number(flag, value_string);

			//if (1 == flag && LC_IsEurope() )
			if (1 == flag)
			{
				isHackShieldEnable = true;
			}
		}

		TOKEN("hackshield_first_check_time")
		{
			int secs = 30;
			str_to_number(secs, value_string);

			HackShield_FirstCheckWaitTime = passes_per_sec * secs;
		}

		TOKEN("hackshield_check_cycle_time")
		{
			int secs = 180;
			str_to_number(secs, value_string);

			HackShield_CheckCycleTime = passes_per_sec * secs;
		}

		TOKEN("xtrap_enable")
		{
			int flag = 0;
			str_to_number(flag, value_string);

			if (1 == flag )
			{
				bXTrapEnabled = true;
			}
		}

		TOKEN("pk_protect_level")
		{
		    str_to_number(PK_PROTECT_LEVEL, value_string);
		    fprintf(stderr, "PK_PROTECT_LEVEL: %d", PK_PROTECT_LEVEL);
		}

		TOKEN("max_level")
		{
			str_to_number(gPlayerMaxLevel, value_string);

			gPlayerMaxLevel = MINMAX(1, gPlayerMaxLevel, PLAYER_MAX_LEVEL_CONST);

			fprintf(stderr, "PLAYER_MAX_LEVEL: %d\n", gPlayerMaxLevel);
		}

		TOKEN("block_char_creation")
		{
			int tmp = 0;

			str_to_number(tmp, value_string);

			if (0 == tmp)
				g_BlockCharCreation = false;
			else
				g_BlockCharCreation = true;

			continue;
		}

		TOKEN("server_key")
		{
			CheckServer::AddServerKey(value_string);
			continue;
		}
	}

	if (g_setQuestObjectDir.empty())
		g_setQuestObjectDir.insert(g_stDefaultQuestObjectDir);

	if (0 == db_port)
	{
		fprintf(stderr, "DB_PORT not configured\n");
		exit(1);
	}

	if (0 == g_bChannel)
	{
		fprintf(stderr, "CHANNEL not configured\n");
		exit(1);
	}

	if (g_stHostname.empty())
	{
		fprintf(stderr, "HOSTNAME must be configured.\n");
		exit(1);
	}

	// LOCALE_SERVICE 
	LocaleService_LoadLocaleStringFile();
	LocaleService_TransferDefaultSetting();
	LocaleService_LoadEmpireTextConvertTables();
	// END_OF_LOCALE_SERVICE

	fclose(fp);

	if ((fp = fopen("CMD", "r")))
	{
		while (fgets(buf, 256, fp))
		{
			char cmd[32], levelname[32];
			int level;

			two_arguments(buf, cmd, sizeof(cmd), levelname, sizeof(levelname));

			if (!*cmd || !*levelname)
			{
				fprintf(stderr, "CMD syntax error: <cmd> <DISABLE | LOW_WIZARD | WIZARD | HIGH_WIZARD | GOD>\n");
				exit(1);
			}

			if (!strcasecmp(levelname, "LOW_WIZARD"))
				level = GM_LOW_WIZARD;
			else if (!strcasecmp(levelname, "WIZARD"))
				level = GM_WIZARD;
			else if (!strcasecmp(levelname, "HIGH_WIZARD"))
				level = GM_HIGH_WIZARD;
			else if (!strcasecmp(levelname, "GOD"))
				level = GM_GOD;
			else if (!strcasecmp(levelname, "IMPLEMENTOR"))
				level = GM_IMPLEMENTOR;
			else if (!strcasecmp(levelname, "DISABLE"))
				level = GM_IMPLEMENTOR + 1;
			else
			{
				fprintf(stderr, "CMD syntax error: <cmd> <DISABLE | LOW_WIZARD | WIZARD | HIGH_WIZARD | GOD>\n");
				exit(1);
			}

			interpreter_set_privilege(cmd, level);
		}

		fclose(fp);
	}

	LoadValidCRCList();
	LoadStateUserCount();

	CWarMapManager::instance().LoadWarMapInfo(NULL);

	FN_log_adminpage();
}

const char* get_table_postfix()
{
	return g_table_postfix.c_str();
}

void LoadValidCRCList()
{
	s_set_dwProcessCRC.clear();
	s_set_dwFileCRC.clear();

	FILE * fp;
	char buf[256];

	if ((fp = fopen("CRC", "r")))
	{
		while (fgets(buf, 256, fp))
		{
			if (!*buf)
				continue;

			DWORD dwValidClientProcessCRC;
			DWORD dwValidClientFileCRC;

			sscanf(buf, " %u %u ", &dwValidClientProcessCRC, &dwValidClientFileCRC);

			s_set_dwProcessCRC.insert(dwValidClientProcessCRC);
			s_set_dwFileCRC.insert(dwValidClientFileCRC);

			fprintf(stderr, "CLIENT_CRC: %u %u\n", dwValidClientProcessCRC, dwValidClientFileCRC);
		}

		fclose(fp);
	}
}

bool LoadClientVersion()
{
	FILE * fp = fopen("VERSION", "r");

	if (!fp)
		return false;

	char buf[256];
	fgets(buf, 256, fp);

	char * p = strchr(buf, '\n');
	if (p) *p = '\0';

	fprintf(stderr, "VERSION: \"%s\"\n", buf);

	g_stClientVersion = buf;
	fclose(fp);
	return true;
}

void CheckClientVersion()
{
	if (LC_IsEurope())
	{
		g_bCheckClientVersion = true;
	}
	else
	{
		g_bCheckClientVersion = false;
	}

	const DESC_MANAGER::DESC_SET & set = DESC_MANAGER::instance().GetClientSet();
	DESC_MANAGER::DESC_SET::const_iterator it = set.begin();

	while (it != set.end())
	{
		LPDESC d = *(it++);

		if (!d->GetCharacter())
			continue;


		int version = atoi(g_stClientVersion.c_str());
		int date	= atoi(d->GetClientVersion() );

		//if (0 != g_stClientVersion.compare(d->GetClientVersion()) )
		if (version > date)
		{
			d->GetCharacter()->ChatPacket(CHAT_TYPE_NOTICE, LC_TEXT("클라이언트 버전이 틀려 로그아웃 됩니다. 정상적으로 패치 후 접속하세요."));
			d->DelayedDisconnect(10);
		}
	}
}

void LoadStateUserCount()
{
	FILE * fp = fopen("state_user_count", "r");

	if (!fp)
		return;

	if (!LC_IsHongKong())
		fscanf(fp, " %d %d ", &g_iFullUserCount, &g_iBusyUserCount);

	fclose(fp);
}

bool IsValidProcessCRC(DWORD dwCRC)
{
	return s_set_dwProcessCRC.find(dwCRC) != s_set_dwProcessCRC.end();
}

bool IsValidFileCRC(DWORD dwCRC)
{
	return s_set_dwFileCRC.find(dwCRC) != s_set_dwFileCRC.end();
}


