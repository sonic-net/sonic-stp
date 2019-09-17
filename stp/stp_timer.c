/*
 * Copyright 2019 Broadcom. All rights reserved. 
 * The term "Broadcom" refers to Broadcom Inc. and/or its subsidiaries.
 */
#include "stp_inc.h"


//get time in secs
uint32_t sys_get_seconds()
{
    struct timespec ts = {0,0};
    if (-1 == clock_gettime(CLOCK_MONOTONIC, &ts))
    {
        STP_LOG_CRITICAL("clock_gettime Failed : %s",strerror(errno));
        sys_assert(0);
    }
    return ts.tv_sec;
}

void start_timer(TIMER *timer, UINT32 value)
{
	timer->active = true;
	timer->value = value;
}

void stop_timer(TIMER *timer)
{
	timer->active = false;
	timer->value = 0;
}

bool timer_expired(TIMER *timer, UINT32 timer_limit)
{
	if (timer->active)
	{
		timer->value++;
		if (timer->value >= timer_limit)
		{
			stop_timer(timer);
			return true;
		}
	}

	return false;
}

bool is_timer_active(TIMER *timer)
{
	return ((timer->active) ? true : false);
}

bool get_timer_value(TIMER *timer, UINT32 *value_in_ticks)
{
	if (!timer->active)
		return false;

	*value_in_ticks = timer->value;
	return true;
}
