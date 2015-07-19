#ifndef __INC_FISHING_H
#define __INC_FISHING_H

#include "item.h"

namespace fishing
{
	enum
	{
		CAMPFIRE_MOB = 12000,
		FISHER_MOB = 9009,
		FISH_MIND_PILL_VNUM = 27610,
	};

	EVENTINFO(fishing_event_info)
	{
		DWORD	pid;
		int		step;
		DWORD	hang_time;
		int		fish_id;

		fishing_event_info()
		: pid(0)
		, step(0)
		, hang_time(0)
		, fish_id(0)
		{
		}
	};

	extern void Initialize();
	extern LPEVENT CreateFishingEvent(LPCHARACTER ch);
	extern void Take(fishing_event_info* info, LPCHARACTER ch);
	extern void Simulation(int level, int count, int map_grade, LPCHARACTER ch);
	extern void UseFish(LPCHARACTER ch, LPITEM item);
	extern void Grill(LPCHARACTER ch, LPITEM item);

	extern bool RefinableRod(LPITEM rod);
	extern int RealRefineRod(LPCHARACTER ch, LPITEM rod);
}
#endif
