/*********************************************************************
 * date        : 2007.04.07
 * file        : castle.h
 * author      : mhh
 * description : 
 */

#ifndef _castle_h_
#define _castle_h_


#define MAX_CASTLE_GUARD_REGION		4	// 경비병 배치 구역
#define MAX_CASTLE_GUARD_PER_REGION	10	// 한지역에 배치할수있는 경비병그룹
#define MAX_CASTLE_FROG				20	// 황금 두꺼비
#define MAX_CASTLE_TOWER			10	// 봉화 최대 소환 개수
#define MIN_CASTLE_TOWER			5	// 봉화 최소 소환 개수


#define CASTLE_FROG_PRICE		100000000	// 황금두꺼비 가격 (1억)
#define CASTLE_FROG_VNUM		11505	// 황금두꺼비 번호
//#define CASTLE_TOWER_VNUM		11506	// 봉화 번호
#define IS_CASTLE_MAP(map)		(181==(map)||182==(map)||(183)==(map))
//#define IS_CASTLE_TOWER(vnum)	(11506==(vnum)||11507==(vnum)||11508==(vnum)||11509==(vnum) || 11510==(vnum))


enum CASTLE_STATE
{
	CASTLE_SIEGE_NONE,			// 평화모드
	CASTLE_SIEGE_STRUGGLE,		// 공성중
	CASTLE_SIEGE_END			// 수성에 성공했다면 1시간동안 봉화를 깰 수 있다.
};


struct CASTLE_DATA
{
	LPCHARACTER	frog[MAX_CASTLE_FROG];	// 황금두꺼비

	LPCHARACTER	guard[MAX_CASTLE_GUARD_REGION][MAX_CASTLE_GUARD_PER_REGION];	// 경비병 리더
	DWORD		guard_group[MAX_CASTLE_GUARD_REGION][MAX_CASTLE_GUARD_PER_REGION];	// 경비병 리더

	LPCHARACTER	tower[MAX_CASTLE_TOWER];	// 봉화

	LPEVENT		siege_event;
	LPEVENT		stone_event;
};




/* extern functions */
bool		castle_boot();
void		castle_save();
int			castle_siege(int empire, int tower_count);
void		castle_start_siege(int empire, int tower_count);
void		castle_end_siege();

LPCHARACTER castle_spawn_frog(int empire);
LPCHARACTER	castle_spawn_guard(int empire, DWORD group_vnum, int region_index);
bool		castle_spawn_tower(int empire, int tower_count);

void		castle_guard_die(LPCHARACTER ch, LPCHARACTER killer);
void		castle_frog_die(LPCHARACTER ch, LPCHARACTER killer);
void		castle_tower_die(LPCHARACTER ch, LPCHARACTER killer);

int			castle_guard_count(int empire, int region_index);
int			castle_frog_count(int empire);

bool		castle_is_guard_vnum(DWORD vnum);
int			castle_cost_of_hiring_guard(DWORD vnum);
bool		castle_can_attack(LPCHARACTER ch, LPCHARACTER victim);

bool		castle_frog_to_empire_money(LPCHARACTER ch);
bool		castle_is_my_castle(int empire, int map_index);
bool		castle_is_tower_vnum(DWORD vnum);

#endif	/* _castle_h_ */

