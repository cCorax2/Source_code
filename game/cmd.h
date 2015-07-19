#ifndef __INC_METIN_II_GAME_CMD_H__
#define __INC_METIN_II_GAME_CMD_H__

#define ACMD(name)  void (name)(LPCHARACTER ch, const char *argument, int cmd, int subcmd)
#define CMD_NAME(name) cmd_info[cmd].command

struct command_info
{
	const char * command;
	void (*command_pointer) (LPCHARACTER ch, const char *argument, int cmd, int subcmd);
	int subcmd;
	int minimum_position;
	int gm_level;
};

extern struct command_info cmd_info[];

extern void interpret_command(LPCHARACTER ch, const char * argument, size_t len);
extern void interpreter_set_privilege(const char * cmd, int lvl);

enum SCMD_ACTION
{
	SCMD_SLAP,
	SCMD_KISS,
	SCMD_FRENCH_KISS,
	SCMD_HUG,
	SCMD_LONG_HUG,
	SCMD_SHOLDER,
	SCMD_FOLD_ARM
};

enum SCMD_CMD
{
	SCMD_LOGOUT,
	SCMD_QUIT,
	SCMD_PHASE_SELECT,
	SCMD_SHUTDOWN,
};

enum SCMD_RESTART
{
	SCMD_RESTART_TOWN,
	SCMD_RESTART_HERE
};

enum SCMD_XMAS
{
	SCMD_XMAS_BOOM,
	SCMD_XMAS_SNOW,
	SCMD_XMAS_SANTA,
};

extern void Shutdown(int iSec);
extern void SendNotice(const char * c_pszBuf);		// 이 게임서버에만 공지
extern void SendLog(const char * c_pszBuf);		// 운영자에게만 공지
extern void BroadcastNotice(const char * c_pszBuf);	// 전 서버에 공지
extern void SendNoticeMap(const char* c_pszBuf, int nMapIndex, bool bBigFont); // 지정 맵에만 공지
extern void SendMonarchNotice(BYTE bEmpire, const char * c_pszBuf);		// 같은 제국에게 공지

// LUA_ADD_BGM_INFO
void CHARACTER_SetBGMVolumeEnable();
void CHARACTER_AddBGMInfo(unsigned mapIndex, const char* name, float vol);
// END_OF_LUA_ADD_BGM_INFO

// LUA_ADD_GOTO_INFO
extern void CHARACTER_AddGotoInfo(const std::string& c_st_name, BYTE empire, int mapIndex, DWORD x, DWORD y);
// END_OF_LUA_ADD_GOTO_INFO

#endif
