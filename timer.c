/*!
 * \file      timer.c
 *
 * \brief     Timer objects and scheduling management implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include "utilities.h"
#include "board.h"
#include "timer.h"
#include "my_timer.h"
#include "my_RFM9x.h"

/*!
 * Safely execute call back
 */
#define ExecuteCallBack( _callback_, context ) \
    do                                         \
    {                                          \
        if( _callback_ == NULL )               \
        {                                      \
            while( 1 );                        \
        }                                      \
        else                                   \
        {                                      \
            _callback_( context );             \
        }                                      \
    }while( 0 );

/*!
 * Timers list head pointer
 */
static TimerEvent_t *TimerListHead = NULL;

/*!
 * \brief Adds or replace the head timer of the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  obj Timer object to be become the new head
 * \param [IN]  remainingTime Remaining time of the previous head to be replaced
 */
static void TimerInsertNewHeadTimer(TimerEvent_t *obj);

/*!
 * \brief Adds a timer to the list.
 *
 * \remark The list is automatically sorted. The list head always contains the
 *         next timer to expire.
 *
 * \param [IN]  obj Timer object to be added to the list
 * \param [IN]  remainingTime Remaining time of the running head after which the object may be added
 */
static void TimerInsertTimer(TimerEvent_t *obj);

/*!
 * \brief Sets a timeout with the duration "timestamp"
 *
 * \param [IN] timestamp Delay duration
 */
static void TimerSetTimeout(TimerEvent_t *obj);

/*!
 * \brief Check if the Object to be added is not already in the list
 *
 * \param [IN] timestamp Delay duration
 * \retval true (the object is already in the list) or false
 */
static bool TimerExists(TimerEvent_t *obj);

void timer_init(TimerEvent_t *obj, void (*callback)(void *context)) {
	obj->Timestamp = 0;
	obj->ReloadValue = 0;
	obj->IsStarted = false;
	obj->IsNext2Expire = false;
	obj->Callback = callback;
	obj->Context = NULL;
	obj->Next = NULL;
}

void timer_set_context(TimerEvent_t *obj, void *context) {
	obj->Context = context;
}

void timer_start(TimerEvent_t *obj) {
	uint32_t elapsedTime = 0;

	CRITICAL_SECTION_BEGIN();

	if ((obj == NULL) || (TimerExists(obj) == true)) {
		CRITICAL_SECTION_END();
		return;
	}

	obj->Timestamp = obj->ReloadValue;
	obj->IsStarted = true;
	obj->IsNext2Expire = false;

	if (TimerListHead == NULL) {
		set_timer_context();
		// Inserts a timer at time now + obj->Timestamp
		TimerInsertNewHeadTimer(obj);
	} else {
		elapsedTime = get_timer_elapsed_time();
		obj->Timestamp += elapsedTime;

		if (obj->Timestamp < TimerListHead->Timestamp) {
			TimerInsertNewHeadTimer(obj);
		} else {
			TimerInsertTimer(obj);
		}
	}
	CRITICAL_SECTION_END();
}

static void TimerInsertTimer(TimerEvent_t *obj) {
	TimerEvent_t *cur = TimerListHead;
	TimerEvent_t *next = TimerListHead->Next;

	while (cur->Next != NULL) {
		if (obj->Timestamp > next->Timestamp) {
			cur = next;
			next = next->Next;
		} else {
			cur->Next = obj;
			obj->Next = next;
			return;
		}
	}
	cur->Next = obj;
	obj->Next = NULL;
}

static void TimerInsertNewHeadTimer(TimerEvent_t *obj) {
	TimerEvent_t *cur = TimerListHead;

	if (cur != NULL) {
		cur->IsNext2Expire = false;
	}

	obj->Next = cur;
	TimerListHead = obj;
	TimerSetTimeout(TimerListHead);
}

bool timer_is_started(TimerEvent_t *obj) {
	return obj->IsStarted;
}

void timer_irq_handler(void) {
	TimerEvent_t *cur;
	TimerEvent_t *next;

	uint32_t old = get_timer_context();
	uint32_t now = set_timer_context();
	uint32_t deltaContext = now - old; // intentional wrap around

	// Update timeStamp based upon new Time Reference
	// because delta context should never exceed 2^32
	if (TimerListHead != NULL) {
		for (cur = TimerListHead; cur->Next != NULL; cur = cur->Next) {
			next = cur->Next;
			if (next->Timestamp > deltaContext) {
				next->Timestamp -= deltaContext;
			} else {
				next->Timestamp = 0;
			}
		}
	}

	// Execute immediately the alarm callback
	if (TimerListHead != NULL) {
		cur = TimerListHead;
		TimerListHead = TimerListHead->Next;
		cur->IsStarted = false;
		ExecuteCallBack(cur->Callback, cur->Context);
	}

	// Remove all the expired object from the list
	while ((TimerListHead != NULL)
			&& (TimerListHead->Timestamp < get_timer_elapsed_time())) {
		cur = TimerListHead;
		TimerListHead = TimerListHead->Next;
		cur->IsStarted = false;
		ExecuteCallBack(cur->Callback, cur->Context);
	}

	// Start the next TimerListHead if it exists AND NOT running
	if ((TimerListHead != NULL) && (TimerListHead->IsNext2Expire == false)) {
		TimerSetTimeout(TimerListHead);
	}
	stop_lora_timer();
	SX1276OnTimeoutIrq();

}

void timer_stop(TimerEvent_t *obj) {
//	CRITICAL_SECTION_BEGIN( )
//	;

	TimerEvent_t *prev = TimerListHead;
	TimerEvent_t *cur = TimerListHead;

	// List is empty or the obj to stop does not exist
	if ((TimerListHead == NULL) || (obj == NULL)) {
//		CRITICAL_SECTION_END( );
		return;
	}

	obj->IsStarted = false;

	if (TimerListHead == obj) // Stop the Head
			{
		if (TimerListHead->IsNext2Expire == true) // The head is already running
				{
			TimerListHead->IsNext2Expire = false;
			if (TimerListHead->Next != NULL) {
				TimerListHead = TimerListHead->Next;
				TimerSetTimeout(TimerListHead);
			} else {
				stop_alarm();
				TimerListHead = NULL;
			}
		} else // Stop the head before it is started
		{
			if (TimerListHead->Next != NULL) {
				TimerListHead = TimerListHead->Next;
			} else {
				TimerListHead = NULL;
			}
		}
	} else // Stop an object within the list
	{
		while (cur != NULL) {
			if (cur == obj) {
				if (cur->Next != NULL) {
					cur = cur->Next;
					prev->Next = cur;
				} else {
					cur = NULL;
					prev->Next = cur;
				}
				break;
			} else {
				prev = cur;
				cur = cur->Next;
			}
		}
	}
//	CRITICAL_SECTION_END( );
}

static bool TimerExists(TimerEvent_t *obj) {
	TimerEvent_t *cur = TimerListHead;

	while (cur != NULL) {
		if (cur == obj) {
			return true;
		}
		cur = cur->Next;
	}
	return false;
}

void timer_reset(TimerEvent_t *obj) {
	timer_stop(obj);
	timer_start(obj);
}

void timer_set_value(TimerEvent_t *obj, uint32_t value) {
	uint32_t minValue = 0;
	uint32_t ticks = lora_timer_ms_to_tick(value);

	timer_stop(obj);

	minValue = TIMERTIME_T_MIN;

	if (ticks < minValue) {
		ticks = minValue;
	}

	obj->Timestamp = ticks;
	obj->ReloadValue = ticks;
}

TimerTime_t timer_get_current_time(void) {
	uint32_t now = get_timer_value();
	return lora_tick_to_ms(now);
}

TimerTime_t timer_get_elapsed_time(TimerTime_t past) {
	if (past == 0) {
		return 0;
	}
	uint32_t nowInTicks = get_timer_value();
	uint32_t pastInTicks = lora_timer_ms_to_tick(past);

	// Intentional wrap around. Works Ok if tick duration below 1ms
	return lora_tick_to_ms(nowInTicks - pastInTicks);
}

static void TimerSetTimeout(TimerEvent_t *obj) {
	int32_t minTicks = get_minimum_timeout();
	obj->IsNext2Expire = true;

	// In case deadline too soon
	if (obj->Timestamp < (get_timer_elapsed_time() + minTicks)) {
		obj->Timestamp = get_timer_elapsed_time() + minTicks;
	}
	set_alarm(obj->Timestamp);
}

