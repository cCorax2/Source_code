#include "stdafx.h"
#include "utils.h"
#include "char.h"
#include "char_manager.h"
#include "motion.h"
#include "packet.h"
#include "buffer_manager.h"
#include "unique_item.h"
#include "wedding.h"

#define NEED_TARGET	(1 << 0)
#define NEED_PC		(1 << 1)
#define WOMAN_ONLY	(1 << 2)
#define OTHER_SEX_ONLY	(1 << 3)
#define SELF_DISARM	(1 << 4)
#define TARGET_DISARM	(1 << 5)
#define BOTH_DISARM	(SELF_DISARM | TARGET_DISARM)

struct emotion_type_s
{
	const char *	command;
	const char *	command_to_client;
	long	flag;
	float	extra_delay;
} emotion_types[] = {
	{ "키스",	"french_kiss",	NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		2.0f },
	{ "뽀뽀",	"kiss",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		1.5f },
	{ "따귀",	"slap",		NEED_PC | SELF_DISARM,				1.5f },
	{ "박수",	"clap",		0,						1.0f },
	{ "와",		"cheer1",	0,						1.0f },
	{ "만세",	"cheer2",	0,						1.0f },
	
	// DANCE
	{ "댄스1",	"dance1",	0,						1.0f },
	{ "댄스2",	"dance2",	0,						1.0f },
	{ "댄스3",	"dance3",	0,						1.0f },
	{ "댄스4",	"dance4",	0,						1.0f },
	{ "댄스5",	"dance5",	0,						1.0f },
	{ "댄스6",	"dance6",	0,						1.0f },
	// END_OF_DANCE
	{ "축하",	"congratulation",	0,				1.0f	},
	{ "용서",	"forgive",			0,				1.0f	},
	{ "화남",	"angry",			0,				1.0f	},
	{ "유혹",	"attractive",		0,				1.0f	},
	{ "슬픔",	"sad",				0,				1.0f	},
	{ "브끄",	"shy",				0,				1.0f	},
	{ "응원",	"cheerup",			0,				1.0f	},
	{ "질투",	"banter",			0,				1.0f	},
	{ "기쁨",	"joy",				0,				1.0f	},
	{ "\n",	"\n",		0,						0.0f },
	/*
	//{ "키스",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		MOTION_ACTION_FRENCH_KISS,	 1.0f },
	{ "뽀뽀",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		MOTION_ACTION_KISS,		 1.0f },
	{ "껴안기",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		MOTION_ACTION_SHORT_HUG,	 1.0f },
	{ "포옹",		NEED_PC | OTHER_SEX_ONLY | BOTH_DISARM,		MOTION_ACTION_LONG_HUG,		 1.0f },
	{ "어깨동무",	NEED_PC | SELF_DISARM,				MOTION_ACTION_PUT_ARMS_SHOULDER, 0.0f },
	{ "팔짱",		NEED_PC	| WOMAN_ONLY | SELF_DISARM,		MOTION_ACTION_FOLD_ARM,		 0.0f },
	{ "따귀",		NEED_PC | SELF_DISARM,				MOTION_ACTION_SLAP,		 1.5f },

	{ "휘파람",		0,						MOTION_ACTION_CHEER_01,		 0.0f },
	{ "만세",		0,						MOTION_ACTION_CHEER_02,		 0.0f },
	{ "박수",		0,						MOTION_ACTION_CHEER_03,		 0.0f },

	{ "호호",		0,						MOTION_ACTION_LAUGH_01,		 0.0f },
	{ "킥킥",		0,						MOTION_ACTION_LAUGH_02,		 0.0f },
	{ "우하하",		0,						MOTION_ACTION_LAUGH_03,		 0.0f },

	{ "엉엉",		0,						MOTION_ACTION_CRY_01,		 0.0f },
	{ "흑흑",		0,						MOTION_ACTION_CRY_02,		 0.0f },

	{ "인사",		0,						MOTION_ACTION_GREETING_01,	0.0f },
	{ "바이",		0,						MOTION_ACTION_GREETING_02,	0.0f },
	{ "정중인사",	0,						MOTION_ACTION_GREETING_03,	0.0f },

	{ "비난",		0,						MOTION_ACTION_INSULT_01,	0.0f },
	{ "모욕",		SELF_DISARM,					MOTION_ACTION_INSULT_02,	0.0f },
	{ "우웩",		0,						MOTION_ACTION_INSULT_03,	0.0f },

	{ "갸우뚱",		0,						MOTION_ACTION_ETC_01,		0.0f },
	{ "끄덕끄덕",	0,						MOTION_ACTION_ETC_02,		0.0f },
	{ "도리도리",	0,						MOTION_ACTION_ETC_03,		0.0f },
	{ "긁적긁적",	0,						MOTION_ACTION_ETC_04,		0.0f },
	{ "퉤",		0,						MOTION_ACTION_ETC_05,		0.0f },
	{ "뿡",		0,						MOTION_ACTION_ETC_06,		0.0f },
	 */
};


std::set<std::pair<DWORD, DWORD> > s_emotion_set;

ACMD(do_emotion_allow)
{
	if ( ch->GetArena() )
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("대련장에서 사용하실 수 없습니다."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	if (!*arg1)
		return;

	DWORD	val = 0; str_to_number(val, arg1);
	s_emotion_set.insert(std::make_pair(ch->GetVID(), val));
}

bool CHARACTER_CanEmotion(CHARACTER& rch)
{
	// 결혼식 맵에서는 사용할 수 있다.
	if (marriage::WeddingManager::instance().IsWeddingMap(rch.GetMapIndex()))
		return true;

	// 열정의 가면 착용시 사용할 수 있다.
	if (rch.IsEquipUniqueItem(UNIQUE_ITEM_EMOTION_MASK))
		return true;

	if (rch.IsEquipUniqueItem(UNIQUE_ITEM_EMOTION_MASK2))
		return true;

	return false;
}

ACMD(do_emotion)
{
	int i;
	{
		if (ch->IsRiding())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 탄 상태에서 감정표현을 할 수 없습니다."));
			return;
		}
	}

	for (i = 0; *emotion_types[i].command != '\n'; ++i)
	{
		if (!strcmp(cmd_info[cmd].command, emotion_types[i].command))
			break;

		if (!strcmp(cmd_info[cmd].command, emotion_types[i].command_to_client))
			break;
	}

	if (*emotion_types[i].command == '\n')
	{
		sys_err("cannot find emotion");
		return;
	}

	if (!CHARACTER_CanEmotion(*ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("열정의 가면을 착용시에만 할 수 있습니다."));
		return;
	}

	if (IS_SET(emotion_types[i].flag, WOMAN_ONLY) && SEX_MALE==GET_SEX(ch))
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("여자만 할 수 있습니다."));
		return;
	}

	char arg1[256];
	one_argument(argument, arg1, sizeof(arg1));

	LPCHARACTER victim = NULL;

	if (*arg1)
		victim = ch->FindCharacterInView(arg1, IS_SET(emotion_types[i].flag, NEED_PC));

	if (IS_SET(emotion_types[i].flag, NEED_TARGET | NEED_PC))
	{
		if (!victim)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("그런 사람이 없습니다."));
			return;
		}
	}

	if (victim)
	{
		if (!victim->IsPC() || victim == ch)
			return;

		if (victim->IsRiding())
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("말을 탄 상대와 감정표현을 할 수 없습니다."));
			return;
		}

		long distance = DISTANCE_APPROX(ch->GetX() - victim->GetX(), ch->GetY() - victim->GetY());

		if (distance < 10)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("너무 가까이 있습니다."));
			return;
		}

		if (distance > 500)
		{
			ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("너무 멀리 있습니다"));
			return;
		}

		if (IS_SET(emotion_types[i].flag, OTHER_SEX_ONLY))
		{
			if (GET_SEX(ch)==GET_SEX(victim))
			{
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이성간에만 할 수 있습니다."));
				return;
			}
		}

		if (IS_SET(emotion_types[i].flag, NEED_PC))
		{
			if (s_emotion_set.find(std::make_pair(victim->GetVID(), ch->GetVID())) == s_emotion_set.end())
			{
				if (true == marriage::CManager::instance().IsMarried( ch->GetPlayerID() ))
				{
					const marriage::TMarriage* marriageInfo = marriage::CManager::instance().Get( ch->GetPlayerID() );

					const DWORD other = marriageInfo->GetOther( ch->GetPlayerID() );

					if (0 == other || other != victim->GetPlayerID())
					{
						ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 행동은 상호동의 하에 가능 합니다."));
						return;
					}
				}
				else
				{
					ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("이 행동은 상호동의 하에 가능 합니다."));
					return;
				}
			}

			s_emotion_set.insert(std::make_pair(ch->GetVID(), victim->GetVID()));
		}
	}

	char chatbuf[256+1];
	int len = snprintf(chatbuf, sizeof(chatbuf), "%s %u %u", 
			emotion_types[i].command_to_client,
			(DWORD) ch->GetVID(), victim ? (DWORD) victim->GetVID() : 0);

	if (len < 0 || len >= (int) sizeof(chatbuf))
		len = sizeof(chatbuf) - 1;

	++len;  // \0 문자 포함

	TPacketGCChat pack_chat;
	pack_chat.header = HEADER_GC_CHAT;
	pack_chat.size = sizeof(TPacketGCChat) + len;
	pack_chat.type = CHAT_TYPE_COMMAND;
	pack_chat.id = 0;
	TEMP_BUFFER buf;
	buf.write(&pack_chat, sizeof(TPacketGCChat));
	buf.write(chatbuf, len);

	ch->PacketAround(buf.read_peek(), buf.size());

	if (victim)
		sys_log(1, "ACTION: %s TO %s", emotion_types[i].command, victim->GetName());
	else
		sys_log(1, "ACTION: %s", emotion_types[i].command);
}

