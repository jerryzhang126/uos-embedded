/*
 * etimer_threads.c
 *
 *  Created on: 21.10.2015
 *      Author: a_lityagin
 */

#include <uos-conf.h>
#include <kernel/uos.h>
#include <kernel/internal.h>
#include <runtime/arch.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef USER_TIMERS

#define INLINE_ETIMER
#include "etimer_threads.h"



bool_t etimer_mutex_timedlock(mutex_t* m, etimer* t, etimer_time_t timeout)
{
    if (mutex_trylock(m))
        return true;
    etimer_assign_task(t, task_current);
    etimer_set(t, timeout);
    bool_t res = mutex_lock_until(m, (etimer_is_timeout), (void*)t);
    etimer_stop(t);
    return res;
}

bool_t mutex_etimedlock(mutex_t* m, etimer_time_t timeout){
    etimer t;
    list_init(&t.item);
    return etimer_mutex_timedlock(m, &t, timeout);
}

bool_t etimer_mutex_timedwait(mutex_t* m, etimer* t, etimer_time_t timeout){
    etimer_assign_task(t, task_current);
    etimer_set(t, timeout);
    bool_t res = mutex_wait_until(m, (etimer_is_timeout), (void*)t);
    etimer_stop(t);
    return res;
}

bool_t mutex_etimedwait(mutex_t* m, etimer_time_t timeout){
    etimer t;
    list_init(&t.item);
    return etimer_mutex_timedwait(m, &t, timeout);
}

//* выполняет ожидание таймаута usec
//* \arg sanity == 0 - ожидает до завершения таймаута
//*      sanity != 0 - ожидает до ближайшего просыпания нитки
//* \return = 0 - таймаут завершен
int etimer_uswait(unsigned usec, bool_t sanity)
{
    if (usec == 0){
        task_yield();
        return 0; 
    }

    etimer t;
    list_init(&t.item);
    etimer_assign_task(&t, task_current);

    arch_state_t x;
    arch_intr_disable (&x);

    etimer_set(&t, usec);
    bool_t ok = 0;
    do {
    /* Suspend the task. */
    list_unlink (&task_current->item);
    task_schedule ();
    ok = !etimer_is_wait(&t);
    if (ok)
        break;
    } while (sanity == 0);
    if (!ok)
        etimer_stop(&t);

    arch_intr_restore (x);
    return (ok)? 0: -1;
}

#endif //USER_TIMERS

