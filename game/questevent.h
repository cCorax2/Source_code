#ifndef __QUEST_EVENT_H
#define __QUEST_EVENT_H

namespace quest
{
	EVENTINFO(quest_server_event_info)
	{
		int		time_cycle;
		unsigned int 	npc_id;
		unsigned int	arg;
		char*		name;

		quest_server_event_info()
		: time_cycle( 0 )
		, npc_id( 0 )
		, arg( 0 )
		, name( 0 )
		{
		}
	};

	EVENTINFO(quest_event_info)
	{
		int		time_cycle;
		unsigned int	player_id;
		unsigned int 	npc_id;
		char*		name;

		quest_event_info()
		: time_cycle( 0 )
		, player_id( 0 )
		, npc_id( 0 )
		, name( 0 )
		{
		}
	};

	extern LPEVENT quest_create_server_timer_event(const char* name, double when, unsigned int timernpc = QUEST_NO_NPC, bool loop = false, unsigned int arg = 0);
	extern LPEVENT quest_create_timer_event(const char* name, unsigned int player_id, double when, unsigned int npc_id=QUEST_NO_NPC, bool loop = false);
	extern void CancelTimerEvent(LPEVENT* ppEvent);
}

#endif
