/*
 * etimer_threads.h
 *
 *  Created on: 21.10.2015
 *      Author: a_lityagin
 */

#ifndef ETIMER_THREADS_H_
#define ETIMER_THREADS_H_

#include <runtime/arch.h>
#include <kernel/uos.h>
#include <timer/etimer.h>

#ifdef __cplusplus
extern "C" {
#endif

/* эта функция используется для теста ожидания завершения таймаута
 * \arg arg - (etimer*) инспектируемое событие
 * \return  true - если событие сработало
 * */
//#ifndef INLINE_ETIMER
//#define INLINE_ETIMER extern inline
//#endif
INLINE
bool_t etimer_is_timeout(void* arg){
    etimer* t = (etimer*)arg;
    return (t->cur_time <= 0);
}

/** \~russian
 * 
 * */
INLINE bool_t etimer_mutex_lock(mutex_t* m, etimer* t){
    return mutex_lock_until(m, (etimer_is_timeout), (void*)t);
};
bool_t etimer_mutex_timedlock(mutex_t* m, etimer* t, etimer_time_t timeout);
bool_t mutex_etimedlock(mutex_t* m, etimer_time_t timeout);

INLINE bool_t etimer_mutex_wait(mutex_t* m, etimer* t){
    return mutex_wait_until(m, (etimer_is_timeout), (void*)t);
};
bool_t etimer_mutex_timedwait(mutex_t* m, etimer* t, etimer_time_t timeout);
bool_t mutex_etimedwait(mutex_t* m, etimer_time_t timeout);

//* выполняет ожидание таймаута usec
//* \arg sanity == 0 - ожидает до завершения таймаута
//*      sanity != 0 - ожидает до ближайшего просыпания нитки
//* \return = 0  - таймаут завершен
//* \return = -1 - таймаут незавершен
int etimer_uswait(unsigned usec, bool_t sanity);

INLINE
void etimer_usleep(unsigned usec){
    etimer_uswait(usec, 0);
};

#ifdef __cplusplus
}
#endif

#endif /* ETIMER_THREADS_H_ */
