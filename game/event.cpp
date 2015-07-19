/*
 *    Filename: event.c
 * Description: 이벤트 관련 (timed event)
 *
 *      Author: 김한주 (aka. 비엽, Cronan), 송영진 (aka. myevan, 빗자루)
 */
#include "stdafx.h"

#include "event_queue.h"

extern void ContinueOnFatalError();
extern void ShutdownOnFatalError();

#ifdef M2_USE_POOL
MemoryPool event_info_data::pool_;
static ObjectPool<EVENT> event_pool;
#endif

static CEventQueue cxx_q;

/* 이벤트를 생성하고 리턴한다 */
LPEVENT event_create_ex(TEVENTFUNC func, event_info_data* info, long when)
{
	LPEVENT new_event = NULL;

	/* 반드시 다음 pulse 이상의 시간이 지난 후에 부르도록 한다. */
	if (when < 1)
		when = 1;

#ifdef M2_USE_POOL
	new_event = event_pool.Construct();
#else
	new_event = M2_NEW event;
#endif

	assert(NULL != new_event);

	new_event->func = func;
	new_event->info	= info;
	new_event->q_el	= cxx_q.Enqueue(new_event, when, thecore_heart->pulse);
	new_event->is_processing = FALSE;
	new_event->is_force_to_end = FALSE;

	return (new_event);
}

/* 시스템으로 부터 이벤트를 제거한다 */
void event_cancel(LPEVENT * ppevent)
{
	LPEVENT event;

	if (!ppevent)
	{
		sys_err("null pointer");
		return;
	}

	if (!(event = *ppevent))
		return;

	if (event->is_processing)
	{
		event->is_force_to_end = TRUE;

		if (event->q_el)
			event->q_el->bCancel = TRUE;

		*ppevent = NULL;
		return;
	}

	// 이미 취소 되었는가?
	if (!event->q_el)
	{
		*ppevent = NULL;
		return;
	}

	if (event->q_el->bCancel)
	{
		*ppevent = NULL;
		return;
	}

	event->q_el->bCancel = TRUE;

	*ppevent = NULL;
}

void event_reset_time(LPEVENT event, long when)
{
	if (!event->is_processing)
	{
		if (event->q_el)
			event->q_el->bCancel = TRUE;

		event->q_el = cxx_q.Enqueue(event, when, thecore_heart->pulse);
	}
}

/* 이벤트를 실행할 시간에 도달한 이벤트들을 실행한다 */
int event_process(int pulse)
{
	long	new_time;
	int		num_events = 0;

	// event_q 즉 이벤트 큐의 헤드의 시간보다 현재의 pulse 가 적으면 루프문이 
	// 돌지 않게 된다.
	while (pulse >= cxx_q.GetTopKey())
	{
		TQueueElement * pElem = cxx_q.Dequeue();

		if (pElem->bCancel)
		{
			cxx_q.Delete(pElem);
			continue;
		}

		new_time = pElem->iKey;

		LPEVENT the_event = pElem->pvData;
		long processing_time = event_processing_time(the_event);
		cxx_q.Delete(pElem);

		/*
		 * 리턴 값은 새로운 시간이며 리턴 값이 0 보다 클 경우 이벤트를 다시 추가한다. 
		 * 리턴 값을 0 이상으로 할 경우 event 에 할당된 메모리 정보를 삭제하지 않도록
		 * 주의한다.
		 */
		the_event->is_processing = TRUE;

		if (!the_event->info)
		{
			the_event->q_el = NULL;
			ContinueOnFatalError();
		}
		else
		{
			//sys_log(0, "EVENT: %s %d event %p info %p", the_event->file, the_event->line, the_event, the_event->info);
			new_time = (the_event->func) (get_pointer(the_event), processing_time);
			
			if (new_time <= 0 || the_event->is_force_to_end)
			{
				the_event->q_el = NULL;
			}
			else
			{
				the_event->q_el = cxx_q.Enqueue(the_event, new_time, pulse);
				the_event->is_processing = FALSE;
			}
		}

		++num_events;
	}

	return num_events;
}

/* 이벤트가 수행시간을 pulse 단위로 리턴해 준다 */
long event_processing_time(LPEVENT event)
{
	long start_time;

	if (!event->q_el)
		return 0;

	start_time = event->q_el->iStartTime;
	return (thecore_heart->pulse - start_time);
}

/* 이벤트가 남은 시간을 pulse 단위로 리턴해 준다 */
long event_time(LPEVENT event)
{
	long when;

	if (!event->q_el)
		return 0;

	when = event->q_el->iKey;
	return (when - thecore_heart->pulse);
}

/* 모든 이벤트를 제거한다 */
void event_destroy(void)
{
	TQueueElement * pElem;

	while ((pElem = cxx_q.Dequeue()))
	{
		LPEVENT the_event = (LPEVENT) pElem->pvData;

		if (!pElem->bCancel)
		{
			// no op here
		}

		cxx_q.Delete(pElem);
	}
}

int event_count()
{
	return cxx_q.Size();
}

void intrusive_ptr_add_ref(EVENT* p) {
	++(p->ref_count);
}

void intrusive_ptr_release(EVENT* p) {
	if ( --(p->ref_count) == 0 ) {
#ifdef M2_USE_POOL
		event_pool.Destroy(p);
#else
		M2_DELETE(p);
#endif
	}
}
