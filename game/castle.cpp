/*********************************************************************
 * date        : 2007.04.07
 * file        : castle.cpp
 * author      : mhh
 * description : 
 * 봉화 번호   : 11506 - 11510
 * 메틴석 번호 : 8012 - 8014, 8024-8027
 */

#define _castle_cpp_

#include "stdafx.h"
#include "constants.h"
#include "config.h"
#include "char_manager.h"
#include "castle.h"
#include "start_position.h"
#include "monarch.h"
#include "questlua.h"
#include "log.h"
#include "char.h"
#include "sectree_manager.h"

#define EMPIRE_NONE		0	// 아무국가 아님
#define EMPIRE_RED		1	// 신수
#define EMPIRE_YELLOW	2	// 천조
#define EMPIRE_BLUE		3	// 진노


#define SIEGE_EVENT_PULSE	PASSES_PER_SEC(60*5)	// 5분


#define GET_CAHR_MANAGER()								CHARACTER_MANAGER::instance()
#define GET_CASTLE(empire)								(s_castle+(empire))
#define GET_GUARD(empire, region_index, guard_index)	(GET_CASTLE(empire)->guard[region_index][guard_index])
#define GET_GUARD_REGION(empire, region_index)			(s_guard_region[empire][region_index])
#define GET_GUARD_GROUP(empire, region_index, guard_index)	(GET_CASTLE(empire)->guard_group[region_index][guard_index])
#define GET_FROG(empire, index)							(GET_CASTLE(empire)->frog[index])
#define GET_FROG_POS(empire, index)						(s_frog_pos[empire][index])
#define GET_TOWER(empire, index)							(GET_CASTLE(empire)->tower[index])

#define DO_ALL_EMPIRE(empire)	for (int empire = 1; empire < 4; ++empire)
#define DO_ALL_TOWER(i)			for (int i = 0; i < MAX_CASTLE_TOWER; ++i)
#define DO_ALL_FROG(i)			for (int i = 0; i < MAX_CASTLE_FROG; ++i)


#define GET_SIEGE_STATE()			s_siege_state
#define GET_SIEGE_EMPIRE()			s_sige_empire
#define GET_SIEGE_EVENT(empire)		(GET_CASTLE(empire)->siege_event)
#define GET_STONE_EVENT(empire)		(GET_CASTLE(empire)->stone_event)

#define GET_TOWER_REGION(empire)	s_tower_region[empire]
#define GET_STONE_REGION(empire)	s_tower_region[empire]


static CASTLE_DATA	*s_castle		= NULL;
static CASTLE_STATE	s_siege_state	= CASTLE_SIEGE_NONE;
static int			s_sige_empire	= EMPIRE_NONE;


struct POSITION
{
	int	x, y;
};

static POSITION	s_frog_pos[4][MAX_CASTLE_FROG] = {
	// EMPIRE_NONE
	{
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },

		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },

		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },

		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 }
	},
	// EMPIRE_RED
	{
		{ 225, 45 },
		{ 231, 45 },
		{ 237, 45 },
		{ 243, 45 },
		{ 249, 45 },

		{ 225, 50 },
		{ 231, 50 },
		{ 237, 50 },
		{ 243, 50 },
		{ 249, 50 },

		{ 261, 45 },
		{ 267, 45 },
		{ 273, 45 },
		{ 279, 45 },
		{ 285, 45 },

		{ 261, 50 },
		{ 267, 50 },
		{ 273, 50 },
		{ 279, 50 },
		{ 285, 50 }
	},
	// EMPIRE_YELLOW
	{
		{ 221, 36 },
		{ 227, 36 },
		{ 233, 36 },
		{ 239, 36 },
		{ 245, 36 },

		{ 269, 36 },
		{ 275, 36 },
		{ 281, 36 },
		{ 287, 36 },
		{ 293, 36 },

		{ 221, 41 },
		{ 227, 41 },
		{ 233, 41 },
		{ 239, 41 },
		{ 245, 41 },

		{ 269, 41 },
		{ 275, 41 },
		{ 281, 41 },
		{ 287, 41 },
		{ 293, 41 }
	},
	// EMPIRE_BLUE
	{
		{ 225, 45 },
		{ 231, 45 },
		{ 237, 45 },
		{ 243, 45 },
		{ 249, 45 },

		{ 225, 50 },
		{ 231, 50 },
		{ 237, 50 },
		{ 243, 50 },
		{ 249, 50 },

		{ 261, 45 },
		{ 267, 45 },
		{ 273, 45 },
		{ 279, 45 },
		{ 285, 45 },

		{ 261, 50 },
		{ 267, 50 },
		{ 273, 50 },
		{ 279, 50 },
		{ 285, 50 }
	}
};


/* 경비병 경비구역 */
struct GUARD_REGION
{
	int	sx, sy, ex, ey;
};

static GUARD_REGION s_guard_region[4][4] = {
	// NULL_EMPIRE
	{
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 },
		{ 0, 0, 0, 0 }
	},
	// EMPIRE_RED
	{
		{ 74, 170, 96, 180 },
		{ 237, 135, 270, 146 },
		{ 235, 260, 278, 273 },
		{ 421, 167, 435, 205 }
	},
	// EMPIRE_YELLOW
	{
		{ 109, 172, 128, 202 },
		{ 237, 140, 282, 153 },
		{ 232, 261, 279, 276 },
		{ 390, 173, 403, 205 },
	},
	// EMPIRE_BLUE
	{
		{ 74, 170, 96, 120 },
		{ 237, 135, 270, 146 },
		{ 235, 260, 278, 273 },
		{ 421, 167, 435, 205 }
	}
};

static GUARD_REGION s_tower_region[4] = {
	// NULL_EMPIRE
	{ 0, 0, 0, 0 },
	// EMPIRE_RED
	{ 85, 135, 420, 265 },
	// EMPIRE_YELLOW
	{ 120, 130, 405, 276 },
	// EMPIRE_BLUE
	{ 85, 135, 420, 265 }
};


static long FN_castle_map_index(int empire);

EVENTINFO(castle_event_info)
{
	int		empire;
	int		pulse;

	castle_event_info()
	: empire( 0 )
	, pulse( 0 )
	{
	}
};

EVENTFUNC(castle_siege_event)
{
	char	buf[1024] = {0};
	struct castle_event_info	*info = dynamic_cast<castle_event_info*>( event->info );

	if ( info == NULL )
	{
		sys_err( "castle_siege_event> <Factor> Null pointer" );
		return 0;
	}

	info->pulse += SIEGE_EVENT_PULSE;

	// 공성 시작후 30분 이내라면 안내만 하자
	if (info->pulse < PASSES_PER_SEC(30*60))
	{
		snprintf(buf, sizeof(buf), LC_TEXT("%s에서 봉화를 둘러싸고 전투가 진행중입니다."),
				EMPIRE_NAME(GET_SIEGE_EMPIRE()));
		BroadcastNotice(buf);

		return SIEGE_EVENT_PULSE;
	}

	switch (GET_SIEGE_STATE())
	{
		case CASTLE_SIEGE_NONE:
			break;

		case CASTLE_SIEGE_STRUGGLE:
			{
				snprintf(buf, sizeof(buf), LC_TEXT("%s이 수성에 성공했습니다."), EMPIRE_NAME(GET_SIEGE_EMPIRE()));
				BroadcastNotice(buf);

				snprintf(buf, sizeof(buf), LC_TEXT("지금부터 %s은 30분간 봉화를 파괴하여 보상을 획득 할 수 있습니다."), EMPIRE_NAME(GET_SIEGE_EMPIRE()));
				BroadcastNotice(buf);

				GET_SIEGE_STATE() = CASTLE_SIEGE_END;

				return PASSES_PER_SEC(60*30);	// 30분
			}
			break;
		case CASTLE_SIEGE_END:
			BroadcastNotice(LC_TEXT("30분이 경과했습니다.. 봉화가 사라집니다."));
			castle_end_siege();
			break;
	}
	return 0;
}


static DWORD FN_random_stone()
{
	DWORD	vnum[7] = {
		8012,
		8013,
		8014,
		8024,
		8025,
		8026,
		8027
	};

	int	index = number(0, 6);

	return vnum[index];
}

EVENTINFO(castle_stone_event_info)
{
	int	empire;
	int	spawn_count;

	castle_stone_event_info()
	: empire( 0 )
	, spawn_count( 0 )
	{
	}
};


EVENTFUNC(castle_stone_event)
{
	struct castle_stone_event_info	*info = dynamic_cast<castle_stone_event_info*>( event->info );

	if (info == NULL)
	{
		sys_err( "castle_stone_event> <Factor> Null pointer" );
		return 0;
	}

	long	map_index	= FN_castle_map_index(GET_SIEGE_EMPIRE());

	SECTREE_MAP	*sectree_map = SECTREE_MANAGER::instance().GetMap(map_index);

	if (NULL == sectree_map)
		return 0;

	/* 15마리씩  2번 소환 */
	const int SPAWN_COUNT = 15;

	if (info->spawn_count < (SPAWN_COUNT * 2))
	{
		for (int i = 0; i < SPAWN_COUNT; ++i)
		{
			DWORD	sx = sectree_map->m_setting.iBaseX + 100 * GET_TOWER_REGION(info->empire).sx;
			DWORD	sy = sectree_map->m_setting.iBaseY + 100 * GET_TOWER_REGION(info->empire).sy;
			DWORD	ex = sectree_map->m_setting.iBaseX + 100 * GET_TOWER_REGION(info->empire).ex;
			DWORD	ey = sectree_map->m_setting.iBaseY + 100 * GET_TOWER_REGION(info->empire).ey;

			CHARACTER_MANAGER::instance().SpawnMobRange(FN_random_stone(),
														FN_castle_map_index(info->empire),
														sx, sy, ex, ey);
		}

		info->spawn_count += SPAWN_COUNT;

		if (info->spawn_count < (SPAWN_COUNT * 2))
			return PASSES_PER_SEC(30 * 60);	// 30분
		else
			return 0;
	}

	return 0;
}



LPCHARACTER castle_spawn_frog_force(int empire, int empty_index);

static long FN_castle_map_index(int empire)
{
	switch (empire)
	{
		case EMPIRE_RED:	return 181;
		case EMPIRE_YELLOW:	return 183;
		case EMPIRE_BLUE:	return 182;
	}
	return 0;
}

static int FN_empty_frog_index(int empire)
{
	DO_ALL_FROG(i)
	{
		if (NULL == GET_FROG(empire, i))
			return i;
	}
	return (-1);
}

static POSITION* FN_empty_frog_pos(int empire)
{
	int	frog_index = FN_empty_frog_index(empire);

	if (frog_index < 0)
		return NULL;

	switch (empire)
	{
		case EMPIRE_RED:
		case EMPIRE_YELLOW:
		case EMPIRE_BLUE:
			return &GET_FROG_POS(empire, frog_index);
	}

	return NULL;
}

static int FN_empty_guard_pos(int empire, int region_index)
{
	for (int i = 0; i < MAX_CASTLE_GUARD_PER_REGION; ++i)
	{
		if (NULL == GET_GUARD(empire, region_index, i))
		{
			return i;
		}
	}

	return -1;
}

static bool FN_is_castle_map(int map_index)
{
	switch (map_index)
	{
		case 181:
		case 182:
		case 183:
			return true;
	}
	return false;
}


bool castle_boot()
{
	FILE	*fp;
	char	one_line[256];
	const char	*delim			= " \t\r\n";
	char	*v;
	int		empire			= 0;
	int		guard_region	= 0;

	CREATE(s_castle, CASTLE_DATA, 4);

	const char	*castle_file = "castle_data.txt";

	if ((fp = fopen(castle_file, "r")) == 0)
		return false;

	while (fgets(one_line, 256, fp))
	{
		int value = 0;

		if (one_line[0] == '#')
			continue;

		const char* token_string = strtok(one_line, delim);

		if (NULL == token_string)
			continue;

		TOKEN("section")
		{
			continue;
		}
		else TOKEN("empire")
		{
			v = strtok(NULL, delim);
			if (v)
			{
				str_to_number(empire, v);
			}
			else
			{
				fclose(fp);
				sys_err("wrong empire number is null");
				return false;
			}
		}
		else TOKEN("frog")
		{
			int	pos = 0;

			while ((v = strtok(NULL, delim)))
			{
				str_to_number(value, v);
				if (value)
				{
					castle_spawn_frog_force(empire, pos);
				}
				++pos;
			}
		}
		else TOKEN("guard")
		{
			int	group_vnum		= 0;

			while ((v = strtok(NULL, delim)))
			{
				str_to_number(group_vnum, v);
				if (group_vnum)
				{
					castle_spawn_guard(empire, group_vnum, guard_region);
				}
			}

			++guard_region;
		}
		else TOKEN("end")
		{
			guard_region = 0;
		}
	}

	fclose(fp);

	return true;
}

void castle_save()
{
	if (NULL == s_castle)
		return;

	const char	*castle_file = "castle_data.txt";
	FILE		*fp;

	fp = fopen(castle_file, "w");

	if (NULL == fp)
	{
		sys_err("<FAIL> fopen(%s)", castle_file);
		return;
	}

	// write castle data
	DO_ALL_EMPIRE(empire)
	{
		fprintf(fp, "section\n");

		// write empire
		fprintf(fp, "\tempire %d\n", empire);

		// write frog
		fprintf(fp, "\tfrog ");
		for (int i = 0; i < MAX_CASTLE_FROG; ++i)
		{
			fprintf(fp, " %d", GET_FROG(empire, i) ? 1 : 0);
		}
		fprintf(fp, "\n");

		// write guard group
		for (int region_index = 0; region_index < MAX_CASTLE_GUARD_REGION; ++region_index)
		{
			fprintf(fp, "\tguard ");
			for (int guard_index = 0; guard_index < MAX_CASTLE_GUARD_PER_REGION; ++guard_index)
			{
				fprintf(fp, " %u", GET_GUARD_GROUP(empire, region_index, guard_index));
			}
			fprintf(fp, "\n");
		}
		fprintf(fp, "end\n");
	}

	fclose(fp);
}

int castle_siege(int empire, int tower_count)
{
	// check siege map
	{
		if (NULL == SECTREE_MANAGER::instance().GetMap(181)) return 0;
		if (NULL == SECTREE_MANAGER::instance().GetMap(182)) return 0;
		if (NULL == SECTREE_MANAGER::instance().GetMap(183)) return 0;
	}

	switch (GET_SIEGE_STATE())
	{
		case CASTLE_SIEGE_NONE:
			castle_start_siege(empire, tower_count);
			return 1;
			break;

		case CASTLE_SIEGE_STRUGGLE:
		case CASTLE_SIEGE_END:
			castle_end_siege();
			return 2;
			break;
	}

	return 0;
}

void castle_start_siege(int empire, int tower_count)
{
	if (CASTLE_SIEGE_NONE != GET_SIEGE_STATE())
		return;

	GET_SIEGE_STATE()	= CASTLE_SIEGE_STRUGGLE;
	GET_SIEGE_EMPIRE()	= empire;

	castle_spawn_tower(empire, tower_count);

	/* 공성 타이머 시작 */
	{
		castle_event_info* info = AllocEventInfo<castle_event_info>();

		info->empire = empire;
		info->pulse	= 0;

		GET_SIEGE_EVENT(empire) = event_create(castle_siege_event, info, /*5분*/SIEGE_EVENT_PULSE);
	}

	/* 메틴석 소환 타이머 시작 */
	{
		castle_stone_event_info* info = AllocEventInfo<castle_stone_event_info>();

		info->spawn_count = 0;
		info->empire = empire;

		GET_STONE_EVENT(empire) = event_create(castle_stone_event, info, /* 1초 */PASSES_PER_SEC(1));
	}
}

void castle_end_siege()
{
	GET_SIEGE_EMPIRE()	= EMPIRE_NONE;
	GET_SIEGE_STATE()	= CASTLE_SIEGE_NONE;

	DO_ALL_EMPIRE(empire)
	{
		if (GET_SIEGE_EVENT(empire))
		{
			event_cancel(&GET_SIEGE_EVENT(empire));
		}

		DO_ALL_TOWER(i)
		{
			if (GET_TOWER(empire, i))
			{
				LPCHARACTER	npc = GET_TOWER(empire, i);
				M2_DESTROY_CHARACTER(npc);
				GET_TOWER(empire, i) = NULL;
			}
		}
	}
}


LPCHARACTER castle_spawn_frog(int empire)
{
	int		dir = 1;
	long	map_index	= FN_castle_map_index(empire);

	/* 황금두꺼비 소환할 곳이 있나? */
	POSITION	*empty_pos = FN_empty_frog_pos(empire);
	if (NULL == empty_pos)
		return NULL;

	SECTREE_MAP	*sectree_map = SECTREE_MANAGER::instance().GetMap(map_index);
	if (NULL == sectree_map)
		return NULL;
	DWORD x = sectree_map->m_setting.iBaseX + 100*empty_pos->x;
	DWORD y = sectree_map->m_setting.iBaseY + 100*empty_pos->y;

	LPCHARACTER frog = CHARACTER_MANAGER::instance().SpawnMob(CASTLE_FROG_VNUM, map_index,
															x, y, 0 ,
															false, dir);
	if (frog)
	{
		frog->SetEmpire(empire);
		int empty_index	= FN_empty_frog_index(empire);
		// 스폰성공
		GET_FROG(empire, empty_index) = frog;
		return frog;
	}
	return NULL;
}

LPCHARACTER castle_spawn_frog_force(int empire, int empty_index)
{
	int		dir = 1;
	long	map_index	= FN_castle_map_index(empire);

	POSITION	*empty_pos = &GET_FROG_POS(empire, empty_index);

	SECTREE_MAP	*sectree_map = SECTREE_MANAGER::instance().GetMap(map_index);
	if (NULL == sectree_map)
	{
		return NULL;
	}
	DWORD x = sectree_map->m_setting.iBaseX + 100*empty_pos->x;
	DWORD y = sectree_map->m_setting.iBaseY + 100*empty_pos->y;

	LPCHARACTER frog = CHARACTER_MANAGER::instance().SpawnMob(CASTLE_FROG_VNUM, map_index,
															x, y, 0 ,
															false, dir);
	if (frog)
	{
		frog->SetEmpire(empire);
		GET_FROG(empire, empty_index) = frog;
		return frog;
	}
	return NULL;
}


LPCHARACTER castle_spawn_guard(int empire, DWORD group_vnum, int region_index)
{
	LPCHARACTER	mob;
	int	sx, sy, ex, ey;
	long	map_index	= FN_castle_map_index(empire);

	SECTREE_MAP	*sectree_map = SECTREE_MANAGER::instance().GetMap(map_index);
	if (NULL == sectree_map)
		return NULL;

	if (castle_guard_count(empire, region_index) >= MAX_CASTLE_GUARD_PER_REGION)
		return NULL;

	sx = sectree_map->m_setting.iBaseX + 100*GET_GUARD_REGION(empire, region_index).sx;
	sy = sectree_map->m_setting.iBaseY + 100*GET_GUARD_REGION(empire, region_index).sy;
	ex = sectree_map->m_setting.iBaseX + 100*GET_GUARD_REGION(empire, region_index).ex;
	ey = sectree_map->m_setting.iBaseY + 100*GET_GUARD_REGION(empire, region_index).ey;

	mob = CHARACTER_MANAGER::instance().SpawnGroup(group_vnum, map_index,
													sx, sy, ex, ey);
	if (mob)
	{
		mob->SetEmpire(empire);

		int	pos = FN_empty_guard_pos(empire, region_index);
		GET_GUARD(empire, region_index, pos) = mob;
		GET_GUARD_GROUP(empire, region_index, pos) = group_vnum;
	}

	return mob;
}


static DWORD FN_random_tower()
{
	DWORD vnum[5] =
	{
		11506,
		11507,
		11508,
		11509,
		11510
	};

	int index = number(0, 4);
	return vnum[index];
}

static void FN_spawn_tower(int empire, LPSECTREE_MAP sectree_map)
{
	DO_ALL_TOWER(i)
	{
		if (GET_TOWER(empire, i))
			continue;

		DWORD	sx = sectree_map->m_setting.iBaseX + 100 * GET_TOWER_REGION(empire).sx;
		DWORD	sy = sectree_map->m_setting.iBaseY + 100 * GET_TOWER_REGION(empire).sy;
		DWORD	ex = sectree_map->m_setting.iBaseX + 100 * GET_TOWER_REGION(empire).ex;
		DWORD	ey = sectree_map->m_setting.iBaseY + 100 * GET_TOWER_REGION(empire).ey;

		GET_TOWER(empire, i) =
			CHARACTER_MANAGER::instance().SpawnMobRange(FN_random_tower(),
														FN_castle_map_index(empire),
														sx, sy, ex, ey);
		GET_TOWER(empire, i)->SetEmpire(empire);
		return;
	}
}

bool castle_spawn_tower(int empire, int tower_count)
{
	int	map_index = FN_castle_map_index(empire);
	SECTREE_MAP	*sectree_map = SECTREE_MANAGER::instance().GetMap(map_index);
	if (NULL == sectree_map)
		return false;

	// 초기화
	DO_ALL_TOWER(i)
	{
		if (GET_TOWER(empire, i))
				GET_TOWER(empire, i)->Dead(NULL, true);
		GET_TOWER(empire, i) = NULL;
	}

	int	spawn_count = MINMAX(MIN_CASTLE_TOWER, tower_count, MAX_CASTLE_TOWER);	// 5 ~ 10마리

	for (int j = 0; j < spawn_count; ++j)
	{
		FN_spawn_tower(empire, sectree_map);
	}

	// broad cast
	{
		char buf[1024];
		snprintf(buf, sizeof(buf), LC_TEXT("%s에 전쟁의 시작을 알리는 봉화가 나타났습니다."), EMPIRE_NAME(empire));
		BroadcastNotice(buf);
	}
	return true;
}

/* 경비병리더가 죽으면 단순하게 슬롯만 비운다. */
void castle_guard_die(LPCHARACTER ch, LPCHARACTER killer)
{
	int	empire = ch->GetEmpire();

	for (int region_index = 0; region_index < MAX_CASTLE_GUARD_REGION; ++region_index)
	{
		for (int i = 0; i < MAX_CASTLE_GUARD_PER_REGION; ++i)
		{
			if (GET_GUARD(empire, region_index, i) == ch)
			{
				GET_GUARD(empire, region_index, i) = NULL;
				GET_GUARD_GROUP(empire, region_index, i) = 0;
			}
		}
	}

	castle_save();
}


/* 황금 두꺼비가 죽으면 killer에게 1천만냥 */
void castle_frog_die(LPCHARACTER ch, LPCHARACTER killer)
{
	if (NULL == ch || NULL == killer)
		return;

	int	empire = ch->GetEmpire();

	DO_ALL_FROG(i)
	{
		if (ch == GET_FROG(empire, i))
		{
			GET_FROG(empire, i) = NULL;

			killer->PointChange(POINT_GOLD, 10000000 /*1천만*/, true);
			//CMonarch::instance().SendtoDBAddMoney(30000000/*3천만*/, killer->GetEmpire(), killer);
			castle_save();
			return;
		}
	}
}

/* 봉화가 모두 죽으면(?) 공성전이 끝난다 */
void castle_tower_die(LPCHARACTER ch, LPCHARACTER killer)
{
	char	buf[1024] = {0};

	if (NULL == ch || NULL == killer)
		return;

	int	killer_empire = killer->GetEmpire();

	switch (GET_SIEGE_STATE())
	{
		case CASTLE_SIEGE_NONE:
			break;

		case CASTLE_SIEGE_STRUGGLE:
		case CASTLE_SIEGE_END:
			{
				int	siege_end = true;
				snprintf(buf, sizeof(buf), LC_TEXT("%s이 봉화를 파괴했습니다."), EMPIRE_NAME(killer_empire));
				BroadcastNotice(buf);

				LogManager::instance().CharLog(killer, 0, "CASTLE_TORCH_KILL", "");

				DO_ALL_TOWER(i)
				{
					if (ch == GET_TOWER(GET_SIEGE_EMPIRE(), i))
						GET_TOWER(GET_SIEGE_EMPIRE(), i) = NULL;
				}

				DO_ALL_TOWER(i)
				{
					if (GET_TOWER(GET_SIEGE_EMPIRE(), i))
						siege_end = false;
				}

				if (siege_end)
				{
					if (GET_SIEGE_STATE() == CASTLE_SIEGE_STRUGGLE)
					{
						snprintf(buf, sizeof(buf), LC_TEXT("%s이 수성에 실패하여 전쟁에 패배하였습니다.."), EMPIRE_NAME(GET_SIEGE_EMPIRE()));
						BroadcastNotice(buf);
					}
					else
					{
						snprintf(buf, sizeof(buf), LC_TEXT("%s이 모든 봉화를 파괴하였습니다."), EMPIRE_NAME(GET_SIEGE_EMPIRE()));
						BroadcastNotice(buf);
					}
					castle_end_siege();
				}
			}
			break;
	}
}


int castle_guard_count(int empire, int region_index)
{
	int count = 0;

	for (int i = 0; i < MAX_CASTLE_GUARD_PER_REGION; ++i)
	{
		if ( GET_GUARD(empire, region_index, i) )
			++count;
	}
	return count;
}


int castle_frog_count(int empire)
{
	int		count = 0;
	DO_ALL_FROG(i)
	{
		if (GET_FROG(empire, i))
			++count;
	}
	return count;
}

bool castle_is_guard_vnum(DWORD vnum)
{
	switch (vnum)
	{
		/* 상급 창경비병 */
		case 11112:
		case 11114:
		case 11116:
		/* 중급 창경비병 */
		case 11106:
		case 11108:
		case 11110:
		/* 하급 창경비병 */
		case 11100:
		case 11102:
		case 11104:
		/* 상급 활경비병 */
		case 11113:
		case 11115:
		case 11117:
		/* 중급 활경비병 */
		case 11107:
		case 11109:
		case 11111:
		/* 하급 활경비병 */
		case 11101:
		case 11103:
		case 11105:
			return true;
	}

	return false;
}

int castle_cost_of_hiring_guard(DWORD group_vnum)
{
	switch (group_vnum)
	{
		/* 하급 */
		case 9501:	/* 신수 창경비 */
		case 9511:	/* 진노 창경비 */
		case 9521:	/* 천조 창경비 */

		case 9502:	/* 신수 활경비 */
		case 9512:	/* 진노 활경비 */
		case 9522:	/* 천조 활경비 */
			return (100*10000);

		/* 중급 */
		case 9503:	/* 신수 창경비 */
		case 9513:	/* 진노 창경비 */
		case 9523:	/* 천조 창경비 */

		case 9504:	/* 신수 활경비 */
		case 9514:	/* 진노 활경비 */
		case 9524:	/* 천조 활경비 */
			return (300*10000);

		/* 상급 */
		case 9505:	/* 신수 창경비 */
		case 9515:	/* 진노 창경비 */
		case 9525:	/* 천조 창경비 */

		case 9506:	/* 신수 활경비 */
		case 9516:	/* 진노 활경비 */
		case 9526:	/* 천조 활경비 */
			return (1000*10000);
	}

	return 0;
}

bool castle_can_attack(LPCHARACTER ch, LPCHARACTER victim)
{
	if (NULL == ch || NULL == victim)
		return false;

	if (false == FN_is_castle_map(ch->GetMapIndex()))
		return false;

	if (ch->IsPC() && victim->IsPC())
		return true;

	if (CASTLE_SIEGE_END == GET_SIEGE_STATE())
	{
		// 수성에 성공했을때 같은 제국만 봉화를 칠 수 있음
		if (castle_is_tower_vnum(victim->GetRaceNum()))
		{
			if (ch->GetEmpire() == victim->GetEmpire())
				return true;
			else
				return false;
		}
	}

	// 같은 제국은 파괴 불가
	if (ch->GetEmpire() == victim->GetEmpire())
		return false;

	return true;
}

bool castle_frog_to_empire_money(LPCHARACTER ch)
{
	if (NULL == ch) 
		return false;

	int empire = ch->GetEmpire();

	DO_ALL_FROG(i)
	{
		if (NULL == GET_FROG(empire, i))
			continue;

		LPCHARACTER	npc = GET_FROG(empire, i);

		if (false == CMonarch::instance().SendtoDBAddMoney(CASTLE_FROG_PRICE, empire, ch))
			return false;

		GET_FROG(empire, i) = NULL; // 등록해제
		npc->Dead(/*killer*/NULL, /*immediate_dead*/true);
		return true;
	}

	return false;
}

bool castle_is_my_castle(int empire, int map_index)
{
	switch (empire)
	{
		case EMPIRE_RED:	return (181 == map_index);
		case EMPIRE_YELLOW: return (183 == map_index);
		case EMPIRE_BLUE:	return (182 == map_index);
	}
	return false;
}

bool castle_is_tower_vnum(DWORD vnum)
{
	switch (vnum)
	{
		case 11506:
		case 11507:
		case 11508:
		case 11509:
		case 11510:
			return true;
	}
	return false;
}

