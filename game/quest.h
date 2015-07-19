#ifndef __METIN2_SERVER_QUEST_H__
#define __METIN2_SERVER_QUEST_H__

#define INDUCTION_LEVEL3	(1 << 0)
#define INDUCTION_LEVEL8	(1 << 1)
#define INDUCTION_LEVEL2	(1 << 2)

//#define QUEST_DIR "./quest/"

#include "lua_incl.h"

namespace quest
{
	enum
	{
		QUEST_NO_NPC,
		QUEST_ATTR_NPC_START = 100000,
		QUEST_ATTR0_NPC = QUEST_ATTR_NPC_START,
		QUEST_ATTR1_NPC = 100000,
		QUEST_ATTR2_NPC = 100001,
		QUEST_ATTR3_NPC = 100002,
		QUEST_ATTR4_NPC = 100003,
		QUEST_ATTR5_NPC = 100004,
		QUEST_ATTR6_NPC = 100005,
		QUEST_ATTR7_NPC = 100006,
		QUEST_ATTR8_NPC = 100007,
		QUEST_ATTR9_NPC = 100008,
		QUEST_ATTR10_NPC = 100009,
		QUEST_ATTR11_NPC = 100010,
		QUEST_ATTR12_NPC = 100011,
		QUEST_ATTR13_NPC = 100012,
		QUEST_ATTR14_NPC = 100013,
		QUEST_ATTR15_NPC = 100014,
	};

	enum
	{
		QUEST_CLICK_EVENT,
		QUEST_KILL_EVENT,
		QUEST_TIMER_EVENT,
		QUEST_LEVELUP_EVENT,
		QUEST_LOGIN_EVENT,
		QUEST_LOGOUT_EVENT,
		QUEST_BUTTON_EVENT,
		QUEST_INFO_EVENT,
		QUEST_CHAT_EVENT,
		QUEST_ATTR_IN_EVENT,
		QUEST_ATTR_OUT_EVENT,
		QUEST_ITEM_USE_EVENT,
		QUEST_SERVER_TIMER_EVENT,
		QUEST_ENTER_STATE_EVENT,
		QUEST_LEAVE_STATE_EVENT,
		QUEST_LETTER_EVENT,
		QUEST_ITEM_TAKE_EVENT,
		QUEST_TARGET_EVENT,
		QUEST_PARTY_KILL_EVENT,
		QUEST_UNMOUNT_EVENT,
		QUEST_ITEM_PICK_EVENT,
		QUEST_SIG_USE_EVENT,
		QUEST_ITEM_INFORMER_EVENT,
		QUEST_EVENT_COUNT
	};

	enum 
	{
		SUSPEND_STATE_NONE,
		SUSPEND_STATE_PAUSE,
		SUSPEND_STATE_SELECT,
		SUSPEND_STATE_INPUT,
		SUSPEND_STATE_CONFIRM,
		SUSPEND_STATE_SELECT_ITEM,
	};

	enum EQuestConfirmType
	{
		CONFIRM_NO,
		CONFIRM_YES,
		CONFIRM_TIMEOUT,
	};

	struct AStateScriptType
	{
		int		GetSize() const { return m_code.size(); }
		const char*	GetCode() const { return &m_code[0]; }

		std::vector<char> m_code;
	};

	struct AArgScript
	{
		//
		// script syntax example
		// 
		// when namespace.func.arg with when_condition begin ...
		//                     ---      --------------
		//                      |             + 
		std::string arg;  // <--+             |
		std::vector<char> when_condition;// <-+
		AStateScriptType script;
		unsigned int quest_index;
		int state_index;

		AArgScript()
			: quest_index(0), state_index(0)
		{}
	};

	struct QuestState
	{
		lua_State *	co;
		int		ico;
		short int	args;
		BYTE		suspend_state;
		int		iIndex;
		bool		bStart;
		int		st;

		std::string	_title;
		std::string	_clock_name;
		std::string	_counter_name;
		int		_clock_value;
		int		_counter_value;
		std::string	_icon_file;

		std::vector<AArgScript *> chat_scripts;

		QuestState()
			: co(NULL), ico(0), args(0), suspend_state(SUSPEND_STATE_NONE), iIndex(0), bStart(false), st(-1),
			_clock_value(0), _counter_value(0)
		{}
	};
}
#endif
