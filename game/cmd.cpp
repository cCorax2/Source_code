#include "stdafx.h"
#include "utils.h"
#include "config.h"
#include "char.h"
#include "locale_service.h"
#include "log.h"
#include "desc.h"

ACMD(do_user_horse_ride);
ACMD(do_user_horse_back);
ACMD(do_user_horse_feed);

ACMD(do_pcbang_update);
ACMD(do_pcbang_check);

// ADD_COMMAND_SLOW_STUN
ACMD(do_slow);
ACMD(do_stun);
// END_OF_ADD_COMMAND_SLOW_STUN

ACMD(do_warp);
ACMD(do_goto);
ACMD(do_item);
ACMD(do_mob);
ACMD(do_mob_ld);
ACMD(do_mob_aggresive);
ACMD(do_mob_coward);
ACMD(do_mob_map);
ACMD(do_purge);
ACMD(do_weaken);
ACMD(do_item_purge);
ACMD(do_state);
ACMD(do_notice);
ACMD(do_map_notice);
ACMD(do_big_notice);
ACMD(do_who);
ACMD(do_user);
ACMD(do_disconnect);
ACMD(do_kill);
ACMD(do_emotion_allow);
ACMD(do_emotion);
ACMD(do_transfer);
ACMD(do_set);
ACMD(do_cmd);
ACMD(do_reset);
ACMD(do_greset);
ACMD(do_mount);
ACMD(do_fishing);
ACMD(do_refine_rod);

// REFINE_PICK
ACMD(do_max_pick);
ACMD(do_refine_pick);
// END_OF_REFINE_PICK

ACMD(do_fishing_simul);
ACMD(do_console);
ACMD(do_restart);
ACMD(do_advance);
ACMD(do_stat);
ACMD(do_respawn);
ACMD(do_skillup);
ACMD(do_guildskillup);
ACMD(do_pvp);
ACMD(do_point_reset);
ACMD(do_safebox_size);
ACMD(do_safebox_close);
ACMD(do_safebox_password);
ACMD(do_safebox_change_password);
ACMD(do_mall_password);
ACMD(do_mall_close);
ACMD(do_ungroup);
ACMD(do_makeguild);
ACMD(do_deleteguild);
ACMD(do_shutdown);
ACMD(do_group);
ACMD(do_group_random);
ACMD(do_invisibility);
ACMD(do_event_flag);
ACMD(do_get_event_flag);
ACMD(do_private);
ACMD(do_qf);
ACMD(do_clear_quest);
ACMD(do_book);
ACMD(do_reload);
ACMD(do_war);
ACMD(do_nowar);
ACMD(do_setskill);
ACMD(do_setskillother);
ACMD(do_level);
ACMD(do_polymorph);
ACMD(do_polymorph_item);
/*
   ACMD(do_b1);
   ACMD(do_b2);
   ACMD(do_b3);
   ACMD(do_b4);
   ACMD(do_b5);
   ACMD(do_b6);
   ACMD(do_b7);
 */
ACMD(do_close_shop);
ACMD(do_set_walk_mode);
ACMD(do_set_run_mode);
ACMD(do_set_skill_group);
ACMD(do_set_skill_point);
ACMD(do_cooltime);
ACMD(do_detaillog);
ACMD(do_monsterlog);

ACMD(do_gwlist);
ACMD(do_stop_guild_war);
ACMD(do_cancel_guild_war);
ACMD(do_guild_state);

ACMD(do_pkmode);
ACMD(do_mobile);
ACMD(do_mobile_auth);
ACMD(do_messenger_auth);

ACMD(do_getqf);
ACMD(do_setqf);
ACMD(do_delqf);
ACMD(do_set_state);

ACMD(do_forgetme);
ACMD(do_aggregate);
ACMD(do_attract_ranger);
ACMD(do_pull_monster);
ACMD(do_setblockmode);
ACMD(do_priv_empire);
ACMD(do_priv_guild);
ACMD(do_mount_test);
ACMD(do_unmount);
ACMD(do_observer);
ACMD(do_observer_exit);
ACMD(do_socket_item);
ACMD(do_xmas);
ACMD(do_stat_minus);
ACMD(do_stat_reset);
ACMD(do_view_equip);
ACMD(do_block_chat);
ACMD(do_vote_block_chat);

// BLOCK_CHAT
ACMD(do_block_chat_list);
// END_OF_BLOCK_CHAT

ACMD(do_party_request);
ACMD(do_party_request_deny);
ACMD(do_party_request_accept);
ACMD(do_build);
ACMD(do_clear_land);

ACMD(do_horse_state);
ACMD(do_horse_level);
ACMD(do_horse_ride);
ACMD(do_horse_summon);
ACMD(do_horse_unsummon);
ACMD(do_horse_set_stat);

ACMD(do_save_attribute_to_image);
ACMD(do_affect_remove);

ACMD(do_change_attr);
ACMD(do_add_attr);
ACMD(do_add_socket);

ACMD(do_inputall)
{
	ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("명령어를 모두 입력하세요."));
}

ACMD(do_show_arena_list);
ACMD(do_end_all_duel);
ACMD(do_end_duel);
ACMD(do_duel);

ACMD(do_stat_plus_amount);

ACMD(do_break_marriage);

ACMD(do_oxevent_show_quiz);
ACMD(do_oxevent_log);
ACMD(do_oxevent_get_attender);

ACMD(do_effect);
ACMD(do_threeway_war_info );
ACMD(do_threeway_war_myinfo );
//
//군주 전용기능
ACMD(do_monarch_warpto);
ACMD(do_monarch_transfer);
ACMD(do_monarch_info);
ACMD(do_elect);
ACMD(do_monarch_tax);
ACMD(do_monarch_mob);
ACMD(do_monarch_notice);

//군주 관리 기능
ACMD(do_rmcandidacy);
ACMD(do_setmonarch);
ACMD(do_rmmonarch);

ACMD(do_hair);
//gift notify quest command
ACMD(do_gift);
// 큐브관련
ACMD(do_inventory);
ACMD(do_cube);
// 공성전
ACMD(do_siege);
ACMD(do_temp);
ACMD(do_frog);

ACMD(do_check_monarch_money);

ACMD(do_reset_subskill );
ACMD(do_flush);

ACMD(do_eclipse);
ACMD(do_weeklyevent);

ACMD(do_event_helper);

ACMD(do_in_game_mall);

ACMD(do_get_mob_count);

ACMD(do_dice);
ACMD(do_special_item);

ACMD(do_click_mall);

ACMD(do_ride);
ACMD(do_get_item_id_list);
ACMD(do_set_socket);
#ifdef __AUCTION__
// temp_auction 임시
ACMD(do_get_auction_list);
ACMD (do_get_my_auction_list);
ACMD (do_get_my_purchase_list);
ACMD(do_get_item_id_list);
ACMD(do_enroll_auction);
ACMD (do_auction_bid);
ACMD (do_auction_impur);
ACMD (do_enroll_wish);
ACMD (do_enroll_sale);

ACMD (do_get_auctioned_item);
ACMD (do_buy_sold_item);
ACMD (do_cancel_auction);
ACMD (do_cancel_wish);
ACMD (do_cancel_sale);

ACMD (do_rebid);
ACMD (do_bid_cancel);
#endif
// 코스츔 상태보기 및 벗기
ACMD(do_costume);
ACMD(do_set_stat);

// 무적
ACMD (do_can_dead);

ACMD (do_full_set);
// 직군과 레벨에 따른 최고 아이템
ACMD (do_item_full_set);
// 직군에 따른 최고 옵션의 속성 셋팅
ACMD (do_attr_full_set);
// 모든 스킬 마스터
ACMD (do_all_skill_master);
// 아이템 착용. icon이 없어 클라에서 확인 할 수 없는 아이템 착용을 위해 만듦.
ACMD (do_use_item);
ACMD (do_dragon_soul);
ACMD (do_ds_list);
ACMD (do_clear_affect);

struct command_info cmd_info[] =
{
	{ "!RESERVED!",	NULL,			0,			POS_DEAD,	GM_IMPLEMENTOR	}, /* 반드시 이 것이 처음이어야 한다. */
	{ "who",		do_who,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "war",		do_war,			0,			POS_DEAD,	GM_PLAYER	},
	{ "warp",		do_warp,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "user",		do_user,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "notice",		do_notice,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "notice_map",	do_map_notice,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "big_notice",	do_big_notice,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "nowar",		do_nowar,		0,			POS_DEAD,	GM_PLAYER	},
	{ "purge",		do_purge,		0,			POS_DEAD,	GM_WIZARD	},
	{ "weaken",		do_weaken,		0,			POS_DEAD,	GM_GOD		},
	{ "dc",		do_disconnect,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "transfer",	do_transfer,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "goto",		do_goto,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "level",		do_level,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "eventflag",	do_event_flag,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "geteventflag",	do_get_event_flag,	0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "item",		do_item,		0,			POS_DEAD,	GM_GOD		},

	{ "mob",		do_mob,			0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "mob_ld",		do_mob_ld,			0,			POS_DEAD,	GM_HIGH_WIZARD	}, /* 몹의 위치와 방향을 설정해 소환 /mob_ld vnum x y dir */
	{ "ma",		do_mob_aggresive,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "mc",		do_mob_coward,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "mm",		do_mob_map,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "kill",		do_kill,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "ipurge",		do_item_purge,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "group",		do_group,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "grrandom",	do_group_random,	0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "set",		do_set,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "reset",		do_reset,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "greset",		do_greset,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "advance",	do_advance,		0,			POS_DEAD,	GM_GOD		},
	{ "book",		do_book,		0,			POS_DEAD,	GM_IMPLEMENTOR  },

	{ "console",	do_console,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "shutdow",	do_inputall,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "shutdown",	do_shutdown,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "stat",		do_stat,		0,			POS_DEAD,	GM_PLAYER	},
	{ "stat-",		do_stat_minus,		0,			POS_DEAD,	GM_PLAYER	},
	{ "stat_reset",	do_stat_reset,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "state",		do_state,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	// ADD_COMMAND_SLOW_STUN
	{ "stun",		do_stun,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "slow",		do_slow,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	// END_OF_ADD_COMMAND_SLOW_STUN

	{ "respawn",	do_respawn,		0,			POS_DEAD,	GM_WIZARD	},

	{ "makeguild",	do_makeguild,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "deleteguild",	do_deleteguild,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "mount",		do_mount,		0,			POS_MOUNTING,	GM_PLAYER	},
	{ "restart_here",	do_restart,		SCMD_RESTART_HERE,	POS_DEAD,	GM_PLAYER	},
	{ "restart_town",	do_restart,		SCMD_RESTART_TOWN,	POS_DEAD,	GM_PLAYER	},
	{ "phase_selec",	do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "phase_select",	do_cmd,			SCMD_PHASE_SELECT,	POS_DEAD,	GM_PLAYER	},
	{ "qui",		do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "quit",		do_cmd,			SCMD_QUIT,		POS_DEAD,	GM_PLAYER	},
	{ "logou",		do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "logout",		do_cmd,			SCMD_LOGOUT,		POS_DEAD,	GM_PLAYER	},
	{ "skillup",	do_skillup,		0,			POS_DEAD,	GM_PLAYER	},
	{ "gskillup",	do_guildskillup,	0,			POS_DEAD,	GM_PLAYER	},
	{ "pvp",		do_pvp,			0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox",	do_safebox_size,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "safebox_close",	do_safebox_close,	0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox_passwor",do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox_password",do_safebox_password,	0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox_change_passwor", do_inputall,	0,			POS_DEAD,	GM_PLAYER	},
	{ "safebox_change_password", do_safebox_change_password,	0,	POS_DEAD,	GM_PLAYER	},
	{ "mall_passwor",	do_inputall,		0,			POS_DEAD,	GM_PLAYER	},
	{ "mall_password",	do_mall_password,	0,			POS_DEAD,	GM_PLAYER	},
	{ "mall_close",	do_mall_close,		0,			POS_DEAD,	GM_PLAYER	},

	// Group Command
	{ "ungroup",	do_ungroup,		0,			POS_DEAD,	GM_PLAYER	},

	// REFINE_ROD_HACK_BUG_FIX
	{ "refine_rod",	do_refine_rod,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	// END_OF_REFINE_ROD_HACK_BUG_FIX

	// REFINE_PICK 
	{ "refine_pick",	do_refine_pick,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "max_pick",	do_max_pick,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	// END_OF_REFINE_PICK

	{ "fish_simul",	do_fishing_simul,	0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "invisible",	do_invisibility,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "qf",		do_qf,			0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "clear_quest",	do_clear_quest,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "close_shop",	do_close_shop,		0,			POS_DEAD,	GM_PLAYER	},

	{ "set_walk_mode",	do_set_walk_mode,	0,			POS_DEAD,	GM_PLAYER	},
	{ "set_run_mode",	do_set_run_mode,	0,			POS_DEAD,	GM_PLAYER	},
	{ "setjob",do_set_skill_group,	0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "setskill",	do_setskill,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "setskillother",	do_setskillother,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "setskillpoint",  do_set_skill_point,	0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "reload",		do_reload,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "cooltime",	do_cooltime,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "gwlist",		do_gwlist,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "gwstop",		do_stop_guild_war,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "gwcancel",	do_cancel_guild_war, 0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "gstate",		do_guild_state,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "pkmode",		do_pkmode,		0,			POS_DEAD,	GM_PLAYER	},
	{ "messenger_auth",	do_messenger_auth,	0,			POS_DEAD,	GM_PLAYER	},

	{ "getqf",		do_getqf,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "setqf",		do_setqf,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "delqf",		do_delqf,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "set_state",	do_set_state,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "로그를보여줘",	do_detaillog,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "몬스터보여줘",	do_monsterlog,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "detaillog",	do_detaillog,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "monsterlog",	do_monsterlog,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "forgetme",	do_forgetme,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "aggregate",	do_aggregate,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "attract_ranger",	do_attract_ranger,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "pull_monster",	do_pull_monster,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "setblockmode",	do_setblockmode,	0,			POS_DEAD,	GM_PLAYER	},
	{ "polymorph",	do_polymorph,		0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "polyitem",	do_polymorph_item,	0,			POS_DEAD,	GM_HIGH_WIZARD },
	{ "priv_empire",	do_priv_empire,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "priv_guild",	do_priv_guild,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "mount_test",	do_mount_test,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "unmount",	do_unmount,		0,			POS_DEAD,	GM_PLAYER	},
	{ "private",	do_private,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "party_request",	do_party_request,	0,			POS_DEAD,	GM_PLAYER	},
	{ "party_request_accept", do_party_request_accept,0,		POS_DEAD,	GM_PLAYER	},
	{ "party_request_deny", do_party_request_deny,0,			POS_DEAD,	GM_PLAYER	},
	{ "observer",	do_observer,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "observer_exit",	do_observer_exit,	0,			POS_DEAD,	GM_PLAYER	},
	{ "socketitem",	do_socket_item,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "saveati",	do_save_attribute_to_image, 0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "xmas_boom",	do_xmas,		SCMD_XMAS_BOOM,		POS_DEAD,	GM_HIGH_WIZARD	},
	{ "xmas_snow",	do_xmas,		SCMD_XMAS_SNOW,		POS_DEAD,	GM_HIGH_WIZARD	},
	{ "xmas_santa",	do_xmas,		SCMD_XMAS_SANTA,	POS_DEAD,	GM_HIGH_WIZARD	},
	{ "view_equip",	do_view_equip,		0,			POS_DEAD,	GM_PLAYER   	},
	{ "jy",				do_block_chat,		0,			POS_DEAD,	GM_HIGH_WIZARD	},

	// BLOCK_CHAT
	{ "vote_block_chat", do_vote_block_chat,		0,			POS_DEAD,	GM_PLAYER	},
	{ "block_chat",		do_block_chat,		0,			POS_DEAD,	GM_PLAYER	},
	{ "block_chat_list",do_block_chat_list,	0,			POS_DEAD,	GM_PLAYER	},
	// END_OF_BLOCK_CHAT

	{ "build",		do_build,		0,		POS_DEAD,	GM_PLAYER	},
	{ "clear_land", do_clear_land,	0,		POS_DEAD,	GM_HIGH_WIZARD	},

	{ "affect_remove",	do_affect_remove,	0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "horse_state",	do_horse_state,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_level",	do_horse_level,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_ride",	do_horse_ride,		0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_summon",	do_horse_summon,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_unsummon",	do_horse_unsummon,	0,			POS_DEAD,	GM_HIGH_WIZARD	},
	{ "horse_set_stat", do_horse_set_stat,	0,			POS_DEAD,	GM_HIGH_WIZARD	},

	{ "pcbang_update", 	do_pcbang_update,	0,			POS_DEAD,	GM_LOW_WIZARD	},
	{ "pcbang_check", 	do_pcbang_check,	0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "emotion_allow",	do_emotion_allow,	0,			POS_FIGHTING,	GM_PLAYER	},
	{ "kiss",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "slap",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "french_kiss",	do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "clap",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "cheer1",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "cheer2",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},

	// DANCE
	{ "dance1",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance2",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance3",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance4",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance5",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	{ "dance6",		do_emotion,		0,			POS_FIGHTING,	GM_PLAYER	},
	// END_OF_DANCE

	{ "congratulation",	do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "forgive",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "angry",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "attractive",	do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "sad",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "shy",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "cheerup",	do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "banter",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},
	{ "joy",		do_emotion,	0,	POS_FIGHTING,	GM_PLAYER	},


	{ "change_attr",	do_change_attr,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "add_attr",	do_add_attr,		0,			POS_DEAD,	GM_IMPLEMENTOR	},
	{ "add_socket",	do_add_socket,		0,			POS_DEAD,	GM_IMPLEMENTOR	},

	{ "user_horse_ride",	do_user_horse_ride,		0,		POS_FISHING,	GM_PLAYER	},
	{ "user_horse_back",	do_user_horse_back,		0,		POS_FISHING,	GM_PLAYER	},
	{ "user_horse_feed",	do_user_horse_feed,		0,		POS_FISHING,	GM_PLAYER	},

	{ "show_arena_list",	do_show_arena_list,		0,		POS_DEAD,	GM_LOW_WIZARD	},
	{ "end_all_duel",		do_end_all_duel,		0,		POS_DEAD,	GM_LOW_WIZARD	},
	{ "end_duel",			do_end_duel,			0,		POS_DEAD,	GM_LOW_WIZARD	},
	{ "duel",				do_duel,				0,		POS_DEAD,	GM_LOW_WIZARD	},

	{ "con+",			do_stat_plus_amount,	POINT_HT,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "int+",			do_stat_plus_amount,	POINT_IQ,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "str+",			do_stat_plus_amount,	POINT_ST,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "dex+",			do_stat_plus_amount,	POINT_DX,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "break_marriage",	do_break_marriage,		0,			POS_DEAD,	GM_LOW_WIZARD	},

	{ "show_quiz",			do_oxevent_show_quiz,	0,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "log_oxevent",		do_oxevent_log,			0,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "get_oxevent_att",	do_oxevent_get_attender,0,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "effect",				do_effect,				0,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "threeway_info",		do_threeway_war_info,	0,	POS_DEAD,	GM_LOW_WIZARD},
	{ "threeway_myinfo",	do_threeway_war_myinfo, 0,	POS_DEAD,	GM_LOW_WIZARD},
	{ "mto",				do_monarch_warpto,		0, 	POS_DEAD,	GM_PLAYER},
	{ "mtr",				do_monarch_transfer,	0,	POS_DEAD,	GM_PLAYER},
	{ "minfo",		do_monarch_info,		0,  POS_DEAD,   GM_PLAYER},	
	{ "mtax",			do_monarch_tax,			0,	POS_DEAD,	GM_PLAYER},
	{ "mmob",			do_monarch_mob,			0, 	POS_DEAD,	GM_PLAYER},
	{ "elect",				do_elect,				0,	POS_DEAD,	GM_HIGH_WIZARD},
	{ "rmcandidacy",		do_rmcandidacy,			0, 	POS_DEAD,	GM_LOW_WIZARD},
	{ "setmonarch",			do_setmonarch,			0, 	POS_DEAD,	GM_LOW_WIZARD},
	{ "rmmonarch",			do_rmmonarch,			0, 	POS_DEAD, 	GM_LOW_WIZARD},
	{ "hair",				do_hair,				0,	POS_DEAD,	GM_PLAYER	},
	{ "inventory",			do_inventory,			0,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "cube",				do_cube,				0,	POS_DEAD,	GM_PLAYER	},
	{ "siege",				do_siege,				0,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "temp",				do_temp,				0,	POS_DEAD,	GM_IMPLEMENTOR	},
	{ "frog",				do_frog,				0,	POS_DEAD,	GM_HIGH_WIZARD	},
	{ "check_mmoney",		do_check_monarch_money,	0,	POS_DEAD,	GM_IMPLEMENTOR	},
	{ "reset_subskill",		do_reset_subskill,		0,	POS_DEAD,	GM_HIGH_WIZARD },
	{ "flush",				do_flush,				0,	POS_DEAD,	GM_IMPLEMENTOR },
	{ "gift",				do_gift,				0,  POS_DEAD,   GM_PLAYER	},	//gift

	{ "mnotice",			do_monarch_notice,		0,	POS_DEAD,	GM_PLAYER	},
	
	{ "eclipse",			do_eclipse,				0,	POS_DEAD,	GM_HIGH_WIZARD	},

	{ "weeklyevent",		do_weeklyevent,			0,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "eventhelper",		do_event_helper,		0,	POS_DEAD,	GM_HIGH_WIZARD	},

	{ "in_game_mall",		do_in_game_mall,		0,	POS_DEAD,	GM_PLAYER	},

	{ "get_mob_count",		do_get_mob_count,		0,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "dice",				do_dice,				0,	POS_DEAD,	GM_PLAYER		},
	{ "주사위",				do_dice,				0,	POS_DEAD,	GM_PLAYER		},
	{ "special_item",			do_special_item,	0,	POS_DEAD,	GM_IMPLEMENTOR		},

	{ "click_mall",			do_click_mall,			0,	POS_DEAD,	GM_PLAYER		},

	{ "ride",				do_ride,				0,	POS_DEAD,	GM_PLAYER	},

	{ "item_id_list",	do_get_item_id_list,	0,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "set_socket",		do_set_socket,			0,	POS_DEAD,	GM_LOW_WIZARD	},
#ifdef __AUCTION__
	// auction 임시
	{ "auction_list",	do_get_auction_list,	0,	POS_DEAD,	GM_PLAYER	},
	{ "my_auction_list", do_get_my_auction_list, 0, POS_DEAD,	GM_PLAYER	},
	{ "my_purchase_list", do_get_my_purchase_list, 0, POS_DEAD,	GM_PLAYER	},


	{ "enroll_auction",		do_enroll_auction, 		0,	POS_DEAD,	GM_PLAYER	},
	{ "bid", do_auction_bid, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "impur", do_auction_impur, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "enroll_wish", do_enroll_wish, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "enroll_sale", do_enroll_sale, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "get_auctioned_item", do_get_auctioned_item, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "buy_sold_item", do_buy_sold_item, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "cancel_auction", do_cancel_auction, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "cancel_wish", do_cancel_wish, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "cancel_sale", do_cancel_sale, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "rebid", do_rebid, 	0,	POS_DEAD,	GM_PLAYER	},
	{ "bid_cancel", do_bid_cancel, 	0,	POS_DEAD,	GM_PLAYER	},
	
#endif
	{ "costume",			do_costume, 			0,	POS_DEAD,	GM_PLAYER	},

	{ "tcon",			do_set_stat,	POINT_HT,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "tint",			do_set_stat,	POINT_IQ,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "tstr",			do_set_stat,	POINT_ST,	POS_DEAD,	GM_LOW_WIZARD	},
	{ "tdex",			do_set_stat,	POINT_DX,	POS_DEAD,	GM_LOW_WIZARD	},

	{ "cannot_dead",			do_can_dead,	1,	POS_DEAD,		GM_LOW_WIZARD},
	{ "can_dead",				do_can_dead,	0,	POS_DEAD,		GM_LOW_WIZARD},

	{ "full_set",	do_full_set, 0, POS_DEAD,		GM_LOW_WIZARD},
	{ "item_full_set",	do_item_full_set, 0, POS_DEAD,		GM_LOW_WIZARD},
	{ "attr_full_set",	do_attr_full_set, 0, POS_DEAD,		GM_LOW_WIZARD},
	{ "all_skill_master",	do_all_skill_master,	0,	POS_DEAD,	GM_LOW_WIZARD},
	{ "use_item",		do_use_item,	0, POS_DEAD,		GM_LOW_WIZARD},

	{ "dragon_soul",				do_dragon_soul,				0,	POS_DEAD,	GM_PLAYER	},
	{ "ds_list",				do_ds_list,				0,	POS_DEAD,	GM_PLAYER	},
	{ "do_clear_affect", do_clear_affect, 	0, POS_DEAD,		GM_LOW_WIZARD},

	{ "\n",		NULL,			0,			POS_DEAD,	GM_IMPLEMENTOR	}  /* 반드시 이 것이 마지막이어야 한다. */
};

void interpreter_set_privilege(const char *cmd, int lvl)
{
	int i;

	for (i = 0; *cmd_info[i].command != '\n'; ++i)
	{
		if (!str_cmp(cmd, cmd_info[i].command))
		{
			cmd_info[i].gm_level = lvl;
			sys_log(0, "Setting command privilege: %s -> %d", cmd, lvl);
			break;
		}
	}
}

void double_dollar(const char *src, size_t src_len, char *dest, size_t dest_len)
{       
	const char * tmp = src;
	size_t cur_len = 0;

	// \0 넣을 자리 확보
	dest_len -= 1;

	while (src_len-- && *tmp)
	{
		if (*tmp == '$')
		{
			if (cur_len + 1 >= dest_len)
				break;

			*(dest++) = '$';
			*(dest++) = *(tmp++);
			cur_len += 2;
		}
		else
		{
			if (cur_len >= dest_len)
				break;

			*(dest++) = *(tmp++);
			cur_len += 1;
		}
	}

	*dest = '\0';
}

void interpret_command(LPCHARACTER ch, const char * argument, size_t len)
{
	if (NULL == ch)
	{
		sys_err ("NULL CHRACTER");
		return ;
	}

	char cmd[128 + 1];  // buffer overflow 문제가 생기지 않도록 일부러 길이를 짧게 잡음
	char new_line[256 + 1];
	const char * line;
	int icmd;

	if (len == 0 || !*argument)
		return;

	double_dollar(argument, len, new_line, sizeof(new_line));

	size_t cmdlen;
	line = first_cmd(new_line, cmd, sizeof(cmd), &cmdlen);

	for (icmd = 1; *cmd_info[icmd].command != '\n'; ++icmd)
	{
		if (cmd_info[icmd].command_pointer == do_cmd)
		{
			if (!strcmp(cmd_info[icmd].command, cmd)) // do_cmd는 모든 명령어를 쳐야 할 수 있다.
				break;
		}
		else if (!strncmp(cmd_info[icmd].command, cmd, cmdlen))
			break;
	}

	if (ch->GetPosition() < cmd_info[icmd].minimum_position)
	{
		switch (ch->GetPosition())
		{
			case POS_MOUNTING:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("탄 상태에서는 할 수 없습니다."));
				break;

			case POS_DEAD:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("쓰러진 상태에서는 할 수 없습니다."));
				break;

			case POS_SLEEPING:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("꿈속에서 어떻게요?"));
				break;

			case POS_RESTING:
			case POS_SITTING:
				ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("먼저 일어 나세요."));
				break;
				/*
				   case POS_FIGHTING:
				   ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("목숨을 걸고 전투 중 입니다. 집중 하세요."));
				   break;
				 */
			default:
				sys_err("unknown position %d", ch->GetPosition());
				break;
		}

		return;
	}

	if (*cmd_info[icmd].command == '\n')
	{   
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("그런 명령어는 없습니다"));
		return;
	}

	if (cmd_info[icmd].gm_level && cmd_info[icmd].gm_level > ch->GetGMLevel())
	{
		ch->ChatPacket(CHAT_TYPE_INFO, LC_TEXT("그런 명령어는 없습니다"));
		return;
	}

	if (strncmp("phase", cmd_info[icmd].command, 5) != 0) // 히든 명령어 처리 
		sys_log(0, "COMMAND: %s: %s", ch->GetName(), cmd_info[icmd].command);

	((*cmd_info[icmd].command_pointer) (ch, line, icmd, cmd_info[icmd].subcmd));

	if (ch->GetGMLevel() >= GM_LOW_WIZARD)
	{
		if (cmd_info[icmd].gm_level >= GM_LOW_WIZARD)
		{
			if (LC_IsEurope() == true || /*LC_IsNewCIBN() == true || */LC_IsCanada() == true || LC_IsBrazil() == true)
			{
				char buf[1024];
				snprintf( buf, sizeof(buf), "%s", argument );

				LogManager::instance().GMCommandLog(ch->GetPlayerID(), ch->GetName(), ch->GetDesc()->GetHostName(), g_bChannel, buf);
			}
		}
	}
}

