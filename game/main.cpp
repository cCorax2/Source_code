#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "event.h"
#include "minilzo.h"
#include "packet.h"
#include "desc_manager.h"
#include "item_manager.h"
#include "char.h"
#include "char_manager.h"
#include "mob_manager.h"
#include "motion.h"
#include "sectree_manager.h"
#include "shop_manager.h"
#include "regen.h"
#include "text_file_loader.h"
#include "skill.h"
#include "pvp.h"
#include "party.h"
#include "questmanager.h"
#include "profiler.h"
#include "lzo_manager.h"
#include "messenger_manager.h"
#include "db.h"
#include "log.h"
#include "p2p.h"
#include "guild_manager.h"
#include "dungeon.h"
#include "cmd.h"
#include "refine.h"
#include "banword.h"
#include "priv_manager.h"
#include "war_map.h"
#include "building.h"
#include "login_sim.h"
#include "target.h"
#include "marriage.h"
#include "wedding.h"
#include "fishing.h"
#include "item_addon.h"
#include "TrafficProfiler.h"
#include "locale_service.h"
#include "arena.h"
#include "OXEvent.h"
#include "monarch.h"
#include "polymorph.h"
#include "blend_item.h"
#include "castle.h"
#include "passpod.h"
#include "ani.h"
#include "BattleArena.h"
#include "over9refine.h"
#include "horsename_manager.h"
#include "pcbang.h"
#include "MarkManager.h"
#include "spam.h"
#include "panama.h"
#include "threeway_war.h"
#include "auth_brazil.h"
#include "DragonLair.h"
#include "HackShield.h"
#include "skill_power.h"
#include "SpeedServer.h"
#include "XTrapManager.h"
#include "DragonSoul.h"
#include <boost/bind.hpp>
#ifndef __WIN32__
	#include "limit_time.h"
#endif

//#define __FILEMONITOR__

#if defined (__FreeBSD__) && defined(__FILEMONITOR__)
	#include "FileMonitor_FreeBSD.h"
#endif

#ifdef __AUCTION__
#include "auction_manager.h"
#endif

#ifndef __WIN32__
#include <gtest/gtest.h>
#endif

#ifdef USE_STACKTRACE
#include <execinfo.h>
#endif

// 윈도우에서 테스트할 때는 항상 서버키 체크
#ifdef _WIN32
	//#define _USE_SERVER_KEY_
#endif
#include "check_server.h"

extern void WriteVersion();
//extern const char * _malloc_options;
#if defined(__FreeBSD__) && defined(DEBUG_ALLOC)
extern void (*_malloc_message)(const char* p1, const char* p2, const char* p3, const char* p4);
// FreeBSD _malloc_message replacement
void WriteMallocMessage(const char* p1, const char* p2, const char* p3, const char* p4) {
	FILE* fp = ::fopen(DBGALLOC_LOG_FILENAME, "a");
	if (fp == NULL) {
		return;
	}
	::fprintf(fp, "%s %s %s %s\n", p1, p2, p3, p4);
	::fclose(fp);
}
#endif

// TRAFFIC_PROFILER
static const DWORD	TRAFFIC_PROFILE_FLUSH_CYCLE = 3600;	///< TrafficProfiler 의 Flush cycle. 1시간 간격
// END_OF_TRAFFIC_PROFILER

// 게임과 연결되는 소켓
volatile int	num_events_called = 0;
int             max_bytes_written = 0;
int             current_bytes_written = 0;
int             total_bytes_written = 0;
BYTE		g_bLogLevel = 0;

socket_t	tcp_socket = 0;
socket_t	udp_socket = 0;
socket_t	p2p_socket = 0;

LPFDWATCH	main_fdw = NULL;

int		io_loop(LPFDWATCH fdw);

int		start(int argc, char **argv);
int		idle();
void	destroy();

void 	test();

enum EProfile
{
	PROF_EVENT,
	PROF_CHR_UPDATE,
	PROF_IO,
	PROF_HEARTBEAT,
	PROF_MAX_NUM
};

static DWORD s_dwProfiler[PROF_MAX_NUM];

int g_shutdown_disconnect_pulse;
int g_shutdown_disconnect_force_pulse;
int g_shutdown_core_pulse;
bool g_bShutdown=false;

extern int speed_server;
#ifdef __AUCTION__
extern int auction_server;
#endif
extern void CancelReloadSpamEvent();

void ContinueOnFatalError()
{
#ifdef USE_STACKTRACE
	void* array[200];
	std::size_t size;
	char** symbols;

	size = backtrace(array, 200);
	symbols = backtrace_symbols(array, size);

	std::ostringstream oss;
	oss << std::endl;
	for (std::size_t i = 0; i < size; ++i) {
		oss << "  Stack> " << symbols[i] << std::endl;
	}

	free(symbols);

	sys_err("FatalError on %s", oss.str().c_str());
#else
	sys_err("FatalError");
#endif
}

void ShutdownOnFatalError()
{
	if (!g_bShutdown)
	{
		sys_err("ShutdownOnFatalError!!!!!!!!!!");
		{
			char buf[256];

			strlcpy(buf, LC_TEXT("서버에 치명적인 오류가 발생하여 자동으로 재부팅됩니다."), sizeof(buf));
			SendNotice(buf);
			strlcpy(buf, LC_TEXT("10초후 자동으로 접속이 종료되며,"), sizeof(buf));
			SendNotice(buf);
			strlcpy(buf, LC_TEXT("5분 후에 정상적으로 접속하실수 있습니다."), sizeof(buf));
			SendNotice(buf);
		}

		g_bShutdown = true;
		g_bNoMoreClient = true;

		g_shutdown_disconnect_pulse = thecore_pulse() + PASSES_PER_SEC(10);
		g_shutdown_disconnect_force_pulse = thecore_pulse() + PASSES_PER_SEC(20);
		g_shutdown_core_pulse = thecore_pulse() + PASSES_PER_SEC(30);
	}
}

namespace
{
	struct SendDisconnectFunc
	{
		void operator () (LPDESC d)
		{
			if (d->GetCharacter())
			{
				if (d->GetCharacter()->GetGMLevel() == GM_PLAYER)
					d->GetCharacter()->ChatPacket(CHAT_TYPE_COMMAND, "quit Shutdown(SendDisconnectFunc)");
			}
		}
	};

	struct DisconnectFunc
	{
		void operator () (LPDESC d)
		{
			if (d->GetType() == DESC_TYPE_CONNECTOR)
				return;

			if (d->IsPhase(PHASE_P2P))
				return;

			d->SetPhase(PHASE_CLOSE);
		}
	};
}

extern std::map<DWORD, CLoginSim *> g_sim; // first: AID
extern std::map<DWORD, CLoginSim *> g_simByPID;
extern std::vector<TPlayerTable> g_vec_save;
unsigned int save_idx = 0;

void heartbeat(LPHEART ht, int pulse) 
{
	DWORD t;

	t = get_dword_time();
	num_events_called += event_process(pulse);
	s_dwProfiler[PROF_EVENT] += (get_dword_time() - t);

	t = get_dword_time();

	// 1초마다
	if (!(pulse % ht->passes_per_sec))
	{
#ifdef ENABLE_LIMIT_TIME
		if ((unsigned)get_global_time() >= GLOBAL_LIMIT_TIME)
		{
			g_bShutdown = true;
		}
#endif

		if (g_bAuthServer && LC_IsBrazil() && !test_server)
			auth_brazil_log();

		if (!g_bAuthServer)
		{
			TPlayerCountPacket pack;
			pack.dwCount = DESC_MANAGER::instance().GetLocalUserCount();
			db_clientdesc->DBPacket(HEADER_GD_PLAYER_COUNT, 0, &pack, sizeof(TPlayerCountPacket));
		}
		else
		{
			DESC_MANAGER::instance().ProcessExpiredLoginKey();
			DBManager::instance().FlushBilling();
			/*
			   if (!(pulse % (ht->passes_per_sec * 600)))
			   DBManager::instance().CheckBilling();
			 */
		}

		{
			int count = 0;
			itertype(g_sim) it = g_sim.begin();

			while (it != g_sim.end())
			{
				if (!it->second->IsCheck())
				{
					it->second->SendLogin();

					if (++count > 50)
					{
						sys_log(0, "FLUSH_SENT");
						break;
					}
				}

				it++;
			}

			if (save_idx < g_vec_save.size())
			{
				count = MIN(100, g_vec_save.size() - save_idx);

				for (int i = 0; i < count; ++i, ++save_idx)
					db_clientdesc->DBPacket(HEADER_GD_PLAYER_SAVE, 0, &g_vec_save[save_idx], sizeof(TPlayerTable));

				sys_log(0, "SAVE_FLUSH %d", count);
			}
		}
	}

	//
	// 25 PPS(Pulse per second) 라고 가정할 때
	//

	// 약 1.16초마다
	if (!(pulse % (passes_per_sec + 4)))
		CHARACTER_MANAGER::instance().ProcessDelayedSave();

	//4초 마다
#if defined (__FreeBSD__) && defined(__FILEMONITOR__)
	if (!(pulse % (passes_per_sec * 5)))
	{
		FileMonitorFreeBSD::Instance().Update(pulse); 
	}
#endif

	// 약 5.08초마다
	if (!(pulse % (passes_per_sec * 5 + 2)))
	{
		ITEM_MANAGER::instance().Update();
		DESC_MANAGER::instance().UpdateLocalUserCount();
	}

	s_dwProfiler[PROF_HEARTBEAT] += (get_dword_time() - t);

	DBManager::instance().Process();
	AccountDB::instance().Process();
	CPVPManager::instance().Process();

	if (g_bShutdown)
	{
		if (thecore_pulse() > g_shutdown_disconnect_pulse)
		{
			const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), ::SendDisconnectFunc());
			g_shutdown_disconnect_pulse = INT_MAX;
		}
		else if (thecore_pulse() > g_shutdown_disconnect_force_pulse)
		{
			const DESC_MANAGER::DESC_SET & c_set_desc = DESC_MANAGER::instance().GetClientSet();
			std::for_each(c_set_desc.begin(), c_set_desc.end(), ::DisconnectFunc());
		}
		else if (thecore_pulse() > g_shutdown_disconnect_force_pulse + PASSES_PER_SEC(5))
		{
			thecore_shutdown();
		}
	}
}

static void CleanUpForEarlyExit() {
	CancelReloadSpamEvent();
}

int main(int argc, char **argv)
{
#ifdef DEBUG_ALLOC
	DebugAllocator::StaticSetUp();
#endif

#ifndef __WIN32__
	// <Factor> start unit tests if option is set
	if ( argc > 1 ) 
	{
		if ( strcmp( argv[1], "unittest" ) == 0 )
		{
			::testing::InitGoogleTest(&argc, argv);
			return RUN_ALL_TESTS();
		}
	}
#endif

	ilInit(); // DevIL Initialize

	WriteVersion();
	
	SECTREE_MANAGER	sectree_manager;
	CHARACTER_MANAGER	char_manager;
	ITEM_MANAGER	item_manager;
	CShopManager	shop_manager;
	CMobManager		mob_manager;
	CMotionManager	motion_manager;
	CPartyManager	party_manager;
	CSkillManager	skill_manager;
	CPVPManager		pvp_manager;
	LZOManager		lzo_manager;
	DBManager		db_manager;
	AccountDB 		account_db;

	LogManager		log_manager;
	MessengerManager	messenger_manager;
	P2P_MANAGER		p2p_manager;
	CGuildManager	guild_manager;
	CGuildMarkManager mark_manager;
	CDungeonManager	dungeon_manager;
	CRefineManager	refine_manager;
	CBanwordManager	banword_manager;
	CPrivManager	priv_manager;
	CWarMapManager	war_map_manager;
	building::CManager	building_manager;
	CTargetManager	target_manager;
	marriage::CManager	marriage_manager;
	marriage::WeddingManager wedding_manager;
	CItemAddonManager	item_addon_manager;
	CArenaManager arena_manager;
	COXEventManager OXEvent_manager;
	CMonarch		Monarch;
	CHorseNameManager horsename_manager;
	CPCBangManager pcbang_manager;

	DESC_MANAGER	desc_manager;

	TrafficProfiler	trafficProfiler;
	CTableBySkill SkillPowerByLevel;
	CPolymorphUtils polymorph_utils;
	CProfiler		profiler;
	CPasspod		passpod;
	CBattleArena	ba;
	COver9RefineManager	o9r;
	SpamManager		spam_mgr;
	CThreeWayWar	threeway_war;
	CDragonLairManager	dl_manager;

	CHackShieldManager	HSManager;
	CXTrapManager		XTManager;

	CSpeedServerManager SSManager;
	DSManager dsManager;

#ifdef __AUCTION__
	AuctionManager auctionManager;
#endif

	if (!start(argc, argv)) {
		CleanUpForEarlyExit();
		return 0;
	}

	quest::CQuestManager quest_manager;

	if (!quest_manager.Initialize()) {
		CleanUpForEarlyExit();
		return 0;
	}

	MessengerManager::instance().Initialize();
	CGuildManager::instance().Initialize();
	fishing::Initialize();
	OXEvent_manager.Initialize();
	if (speed_server)
		CSpeedServerManager::instance().Initialize();

	Cube_init();
	Blend_Item_init();
	ani_init();
	PanamaLoad();

#if defined(_WIN32) && defined(_USE_SERVER_KEY_)
	if (CheckServer::IsFail())
	{
		return 1;
	}
#endif

	if ( g_bTrafficProfileOn )
		TrafficProfiler::instance().Initialize( TRAFFIC_PROFILE_FLUSH_CYCLE, "ProfileLog" );

	//if game server
	if (!g_bAuthServer)
	{
		//hackshield
		if (isHackShieldEnable)
		{
			if (!HSManager.Initialize())
			{
				fprintf(stderr, "Failed To Initialize HS");
				CleanUpForEarlyExit();
				return 0;
			}
		}

		//xtrap
		if(bXTrapEnabled)
		{
			if (!XTManager.LoadXTrapModule())
			{
				CleanUpForEarlyExit();
				return 0;
			}
#if defined (__FreeBSD__) && defined(__FILEMONITOR__)
			//PFN_FileChangeListener pNotifyFunc = boost::bind( &CXTrapManager::NotifyMapFileChanged, CXTrapManager::instance(), _1 );
			PFN_FileChangeListener pNotifyFunc = &(CXTrapManager::NotifyMapFileChanged);

			const std::string strMap1Name = "map1.CS3";
			const std::string strMap2Name = "map2.CS3";

			FileMonitorFreeBSD::Instance().AddWatch( strMap1Name, pNotifyFunc );
			FileMonitorFreeBSD::Instance().AddWatch( strMap2Name, pNotifyFunc );
#endif
		}
	}

	// Client PackageCrypt

	//TODO : make it config
	const std::string strPackageCryptInfoDir = "package/";
	if( !desc_manager.LoadClientPackageCryptInfo( strPackageCryptInfoDir.c_str() ) )
	{
		sys_err("Failed to Load ClientPackageCryptInfo File(%s)", strPackageCryptInfoDir.c_str());	
	}

#if defined (__FreeBSD__) && defined(__FILEMONITOR__)
	PFN_FileChangeListener pPackageNotifyFunc =  &(DESC_MANAGER::NotifyClientPackageFileChanged);
	//FileMonitorFreeBSD::Instance().AddWatch( strPackageCryptInfoName, pPackageNotifyFunc );
#endif

	while (idle());

	sys_log(0, "<shutdown> Starting...");
	g_bShutdown = true;
	g_bNoMoreClient = true;

	if (g_bAuthServer)
	{
		DBManager::instance().FlushBilling(true);

		int iLimit = DBManager::instance().CountQuery() / 50;
		int i = 0;

		do
		{
			DWORD dwCount = DBManager::instance().CountQuery();
			sys_log(0, "Queries %u", dwCount);

			if (dwCount == 0)
				break;

			usleep(500000);

			if (++i >= iLimit)
				if (dwCount == DBManager::instance().CountQuery())
					break;
		} while (1);
	}

	sys_log(0, "<shutdown> Destroying CArenaManager...");
	arena_manager.Destroy();
	sys_log(0, "<shutdown> Destroying COXEventManager...");
	OXEvent_manager.Destroy();

	sys_log(0, "<shutdown> Disabling signal timer...");
	signal_timer_disable();

	sys_log(0, "<shutdown> Shutting down CHARACTER_MANAGER...");
	char_manager.GracefulShutdown();
	sys_log(0, "<shutdown> Shutting down ITEM_MANAGER...");
	item_manager.GracefulShutdown();

	sys_log(0, "<shutdown> Flushing db_clientdesc...");
	db_clientdesc->FlushOutput();
	sys_log(0, "<shutdown> Flushing p2p_manager...");
	p2p_manager.FlushOutput();

	sys_log(0, "<shutdown> Destroying CShopManager...");
	shop_manager.Destroy();
	sys_log(0, "<shutdown> Destroying CHARACTER_MANAGER...");
	char_manager.Destroy();
	sys_log(0, "<shutdown> Destroying ITEM_MANAGER...");
	item_manager.Destroy();
	sys_log(0, "<shutdown> Destroying DESC_MANAGER...");
	desc_manager.Destroy();
	sys_log(0, "<shutdown> Destroying quest::CQuestManager...");
	quest_manager.Destroy();
	sys_log(0, "<shutdown> Destroying building::CManager...");
	building_manager.Destroy();

	if (!g_bAuthServer)
	{
		if (isHackShieldEnable)
		{
			sys_log(0, "<shutdown> Releasing HackShield manager...");
			HSManager.Release();
		}
	}

	sys_log(0, "<shutdown> Flushing TrafficProfiler...");
	trafficProfiler.Flush();

	destroy();

#ifdef DEBUG_ALLOC
	DebugAllocator::StaticTearDown();
#endif

	return 1;
}

void usage()
{
	printf("Option list\n"
			"-p <port>    : bind port number (port must be over 1024)\n"
			"-l <level>   : sets log level\n"
			"-v           : log to stdout\n"
			"-r           : do not load regen tables\n"
			"-t           : traffic proflie on\n");
}

int start(int argc, char **argv)
{
	std::string st_localeServiceName;

	bool bVerbose = false;
	char ch;

	//_malloc_options = "A";
#if defined(__FreeBSD__) && defined(DEBUG_ALLOC)
	_malloc_message = WriteMallocMessage;
#endif
#ifdef ENABLE_LIMIT_TIME
	if ((unsigned)get_global_time() >= GLOBAL_LIMIT_TIME)
	{
		return 0;
	}
#endif

	while ((ch = getopt(argc, argv, "npverltI")) != -1)
	{
		char* ep = NULL;

		switch (ch)
		{
			case 'I': // IP
				strlcpy(g_szPublicIP, argv[optind], sizeof(g_szPublicIP));

				printf("IP %s\n", g_szPublicIP);

				optind++;
				optreset = 1;
				break;

			case 'p': // port
				mother_port = strtol(argv[optind], &ep, 10);

				if (mother_port <= 1024)
				{
					usage();
					return 0;
				}

				printf("port %d\n", mother_port);

				optind++;
				optreset = 1;
				break;

			case 'l':
				{
					long l = strtol(argv[optind], &ep, 10);

					log_set_level(l);

					optind++;
					optreset = 1;
				}
				break;

				// LOCALE_SERVICE
			case 'n': 
				{
					if (optind < argc)
					{
						st_localeServiceName = argv[optind++];
						optreset = 1;
					}
				}
				break;
				// END_OF_LOCALE_SERVICE

			case 'v': // verbose
				bVerbose = true;
				break;

			case 'r':
				g_bNoRegen = true;
				break;

				// TRAFFIC_PROFILER
			case 't':
				g_bTrafficProfileOn = true;
				break;
				// END_OF_TRAFFIC_PROFILER
		}
	}

	// LOCALE_SERVICE
	config_init(st_localeServiceName);
	// END_OF_LOCALE_SERVICE

#ifdef __WIN32__
	// In Windows dev mode, "verbose" option is [on] by default.
	bVerbose = true;
#endif
	if (!bVerbose)
		freopen("stdout", "a", stdout);

	bool is_thecore_initialized = thecore_init(25, heartbeat);

	if (!is_thecore_initialized)
	{
		fprintf(stderr, "Could not initialize thecore, check owner of pid, syslog\n");
		exit(0);
	}

	if (false == CThreeWayWar::instance().LoadSetting("forkedmapindex.txt"))
	{
		if (false == g_bAuthServer)
		{
			fprintf(stderr, "Could not Load ThreeWayWar Setting file");
			exit(0);
		}
	}

	signal_timer_disable();
	
	main_fdw = fdwatch_new(4096);

	if ((tcp_socket = socket_tcp_bind(g_szPublicIP, mother_port)) == INVALID_SOCKET)
	{
		perror("socket_tcp_bind: tcp_socket");
		return 0;
	}

	
#ifndef __UDP_BLOCK__
	if ((udp_socket = socket_udp_bind(g_szPublicIP, mother_port)) == INVALID_SOCKET)
	{
		perror("socket_udp_bind: udp_socket");
		return 0;
	}
#endif	

	// if internal ip exists, p2p socket uses internal ip, if not use public ip
	//if ((p2p_socket = socket_tcp_bind(*g_szInternalIP ? g_szInternalIP : g_szPublicIP, p2p_port)) == INVALID_SOCKET)
	if ((p2p_socket = socket_tcp_bind(g_szPublicIP, p2p_port)) == INVALID_SOCKET)
	{
		perror("socket_tcp_bind: p2p_socket");
		return 0;
	}

	fdwatch_add_fd(main_fdw, tcp_socket, NULL, FDW_READ, false);
#ifndef __UDP_BLOCK__
	fdwatch_add_fd(main_fdw, udp_socket, NULL, FDW_READ, false);
#endif
	fdwatch_add_fd(main_fdw, p2p_socket, NULL, FDW_READ, false);

	db_clientdesc = DESC_MANAGER::instance().CreateConnectionDesc(main_fdw, db_addr, db_port, PHASE_DBCLIENT, true);
	if (!g_bAuthServer) {
		db_clientdesc->UpdateChannelStatus(0, true);
	}

	if (g_bAuthServer)
	{
		if (g_stAuthMasterIP.length() != 0)
		{
			fprintf(stderr, "SlaveAuth");
			g_pkAuthMasterDesc = DESC_MANAGER::instance().CreateConnectionDesc(main_fdw, g_stAuthMasterIP.c_str(), g_wAuthMasterPort, PHASE_P2P, true); 
			P2P_MANAGER::instance().RegisterConnector(g_pkAuthMasterDesc);
			g_pkAuthMasterDesc->SetP2P(g_stAuthMasterIP.c_str(), g_wAuthMasterPort, g_bChannel);

		}
		else
		{
			fprintf(stderr, "MasterAuth %d", LC_GetLocalType());
		}
	}
	/* game server to teen server */
	else
	{
		if (teen_addr[0] && teen_port)
			g_TeenDesc = DESC_MANAGER::instance().CreateConnectionDesc(main_fdw, teen_addr, teen_port, PHASE_TEEN, true);

		extern unsigned int g_uiSpamBlockDuration;
		extern unsigned int g_uiSpamBlockScore;
		extern unsigned int g_uiSpamReloadCycle;

		sys_log(0, "SPAM_CONFIG: duration %u score %u reload cycle %u\n",
				g_uiSpamBlockDuration, g_uiSpamBlockScore, g_uiSpamReloadCycle);

		extern void LoadSpamDB();
		LoadSpamDB();
	}

	signal_timer_enable(30);
	return 1;
}

void destroy()
{
	sys_log(0, "<shutdown> Canceling ReloadSpamEvent...");
	CancelReloadSpamEvent();

	sys_log(0, "<shutdown> regen_free()...");
	regen_free();

	sys_log(0, "<shutdown> Closing sockets...");
	socket_close(tcp_socket);
#ifndef __UDP_BLOCK__
	socket_close(udp_socket);
#endif
	socket_close(p2p_socket);

	sys_log(0, "<shutdown> fdwatch_delete()...");
	fdwatch_delete(main_fdw);

	sys_log(0, "<shutdown> event_destroy()...");
	event_destroy();

	sys_log(0, "<shutdown> CTextFileLoader::DestroySystem()...");
	CTextFileLoader::DestroySystem();

	sys_log(0, "<shutdown> thecore_destroy()...");
	thecore_destroy();
}

int idle()
{
	static struct timeval	pta = { 0, 0 };
	static int			process_time_count = 0;
	struct timeval		now;

	if (pta.tv_sec == 0)
		gettimeofday(&pta, (struct timezone *) 0);

	int passed_pulses;

	if (!(passed_pulses = thecore_idle()))
		return 0;

	assert(passed_pulses > 0);

	DWORD t;

	while (passed_pulses--) {
		heartbeat(thecore_heart, ++thecore_heart->pulse);

		// To reduce the possibility of abort() in checkpointing
		thecore_tick();
	}

	t = get_dword_time();
	CHARACTER_MANAGER::instance().Update(thecore_heart->pulse);
	db_clientdesc->Update(t);
	s_dwProfiler[PROF_CHR_UPDATE] += (get_dword_time() - t);

	t = get_dword_time();
	if (!io_loop(main_fdw)) return 0;
	s_dwProfiler[PROF_IO] += (get_dword_time() - t);

	log_rotate();

	gettimeofday(&now, (struct timezone *) 0);
	++process_time_count;

	if (now.tv_sec - pta.tv_sec > 0)
	{
		pt_log("[%3d] event %5d/%-5d idle %-4ld event %-4ld heartbeat %-4ld I/O %-4ld chrUpate %-4ld | WRITE: %-7d | PULSE: %d",
				process_time_count,
				num_events_called,
				event_count(),
				thecore_profiler[PF_IDLE],
				s_dwProfiler[PROF_EVENT],
				s_dwProfiler[PROF_HEARTBEAT],
				s_dwProfiler[PROF_IO],
				s_dwProfiler[PROF_CHR_UPDATE],
				current_bytes_written,
				thecore_pulse());

		num_events_called = 0;
		current_bytes_written = 0;

		process_time_count = 0; 
		gettimeofday(&pta, (struct timezone *) 0);

		memset(&thecore_profiler[0], 0, sizeof(thecore_profiler));
		memset(&s_dwProfiler[0], 0, sizeof(s_dwProfiler));
	}
#ifdef _USE_SERVER_KEY_
	if (Metin2Server_IsInvalid() && 0 == (thecore_random() % 7146))
	{
		return 0; // shutdown
	}
#endif

#ifdef __WIN32__
	if (_kbhit()) {
		int c = _getch();
		switch (c) {
			case 0x1b: // Esc
				return 0; // shutdown
				break;
			default:
				break;
		}
	}
#endif

	return 1;
}

int io_loop(LPFDWATCH fdw)
{
	LPDESC	d;
	int		num_events, event_idx;

	DESC_MANAGER::instance().DestroyClosed(); // PHASE_CLOSE인 접속들을 끊어준다.
	DESC_MANAGER::instance().TryConnect();

	if ((num_events = fdwatch(fdw, 0)) < 0)
		return 0;

	for (event_idx = 0; event_idx < num_events; ++event_idx)
	{
		d = (LPDESC) fdwatch_get_client_data(fdw, event_idx);

		if (!d)
		{
			if (FDW_READ == fdwatch_check_event(fdw, tcp_socket, event_idx))
			{
				DESC_MANAGER::instance().AcceptDesc(fdw, tcp_socket);
				fdwatch_clear_event(fdw, tcp_socket, event_idx);
			}
			else if (FDW_READ == fdwatch_check_event(fdw, p2p_socket, event_idx))
			{
				DESC_MANAGER::instance().AcceptP2PDesc(fdw, p2p_socket);
				fdwatch_clear_event(fdw, p2p_socket, event_idx);
			}
			/*
			else if (FDW_READ == fdwatch_check_event(fdw, udp_socket, event_idx))
			{
				char			buf[256];
				struct sockaddr_in	cliaddr;
				socklen_t		socklen = sizeof(cliaddr);

				int iBytesRead;

				if ((iBytesRead = socket_udp_read(udp_socket, buf, 256, (struct sockaddr *) &cliaddr, &socklen)) > 0)
				{
					static CInputUDP s_inputUDP;

					s_inputUDP.SetSockAddr(cliaddr);

					int iBytesProceed;
					s_inputUDP.Process(NULL, buf, iBytesRead, iBytesProceed);
				}

				fdwatch_clear_event(fdw, udp_socket, event_idx);
			}
			*/
			continue; 
		}

		int iRet = fdwatch_check_event(fdw, d->GetSocket(), event_idx);

		switch (iRet)
		{
			case FDW_READ:
				if (db_clientdesc == d)
				{
					int size = d->ProcessInput();

					if (size)
						sys_log(1, "DB_BYTES_READ: %d", size);

					if (size < 0)
					{
						d->SetPhase(PHASE_CLOSE);
					}
				}
				else if (d->ProcessInput() < 0)
				{
					d->SetPhase(PHASE_CLOSE);
				}
				break;

			case FDW_WRITE:
				if (db_clientdesc == d)
				{
					int buf_size = buffer_size(d->GetOutputBuffer());
					int sock_buf_size = fdwatch_get_buffer_size(fdw, d->GetSocket());

					int ret = d->ProcessOutput();

					if (ret < 0)
					{
						d->SetPhase(PHASE_CLOSE);
					}

					if (buf_size)
						sys_log(1, "DB_BYTES_WRITE: size %d sock_buf %d ret %d", buf_size, sock_buf_size, ret);
				}
				else if (d->ProcessOutput() < 0)
				{
					d->SetPhase(PHASE_CLOSE);
				}
				else if (g_TeenDesc==d)
				{
					int buf_size = buffer_size(d->GetOutputBuffer());
					int sock_buf_size = fdwatch_get_buffer_size(fdw, d->GetSocket());

					int ret = d->ProcessOutput();

					if (ret < 0)
					{
						d->SetPhase(PHASE_CLOSE);
					}

					if (buf_size)
						sys_log(0, "TEEN::Send(size %d sock_buf %d ret %d)", buf_size, sock_buf_size, ret);
				}
				break;

			case FDW_EOF:
				{
					d->SetPhase(PHASE_CLOSE);
				}
				break;

			default:
				sys_err("fdwatch_check_event returned unknown %d", iRet);
				d->SetPhase(PHASE_CLOSE);
				break;
		}
	}

	return 1;
}

