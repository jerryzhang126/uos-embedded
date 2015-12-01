/*
 * Copyright (C) 2000-2005 Serge Vakulenko, <vak@cronyx.ru>
 *
 * This file is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.
 *
 * You can redistribute this file and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software Foundation;
 * either version 2 of the License, or (at your discretion) any later version.
 * See the accompanying file "COPYING.txt" for more details.
 *
 * As a special exception to the GPL, permission is granted for additional
 * uses of the text contained in this file.  See the accompanying file
 * "COPY-UOS.txt" for details.
 */
#include <runtime/lib.h>
#include <kernel/uos.h>
#include <kernel/internal.h>

list_t task_active;			/* list of tasks ready to run */
task_t *task_current;			/* current running task */
task_t *task_idle;			/* background system task */
mutex_irq_t mutex_irq [ARCH_INTERRUPTS]; /* interrupt handlers */

#ifndef IDLE_TASK_STACKSZ
#define IDLE_TASK_STACKSZ   (256+MIPS_FSPACE)

#endif

#define ALIGNED_IDLE_TASK_STACKSZ ((IDLE_TASK_STACKSZ + UOS_STACK_ALIGN - 1) & ~(UOS_STACK_ALIGN - 1))

static ARRAY (task_idle_data, sizeof(task_t) + ALIGNED_IDLE_TASK_STACKSZ - UOS_STACK_ALIGN);
bool_t task_need_schedule;

/*
 * Switch to most priority task if needed.
 */
CODE_ISR 
void task_schedule ()
{
	task_t *new_task;

	task_need_schedule = 0;
	new_task = task_policy ();
	if (new_task != task_current) {
		new_task->ticks++;
		arch_task_switch (new_task);
	}
}

/*
 * Activate all waiters of the lock.
 */
CODE_ISR 
void 
mutex_activate (mutex_t *m, void *message)
{
	task_t *t;
	mutex_slot_t *s;

	assert (m != 0);
	if (! m->item.next)
		mutex_init (m);

	while (! list_is_empty (&m->waiters)) {
		t = (task_t*) list_first (&m->waiters);
		assert (t->wait == m);
		t->wait = 0;
		t->message = message;
		task_activate (t);
	}

	/* Activate groups. */
	list_iterate (s, &m->groups) {
		assert (s->lock == m);
		s->message = message;
		s->active = 1;
		t = s->group->waiter;
		if (t) {
		    //group_lockwait - use groun in paralel with 
		    //  lock operation
			//assert (list_is_empty (&t->item));
			s->group->waiter = 0;
			task_activate (t);
		}
	}
}

/*
 * Additional machine-dependent initialization after
 * user initialization. Needed for some processor 
 * architectures.
 */
void __attribute__ ((weak, noinline))
uos_post_init ()
{
}

/*
 * Call user initialization routine uos_init(),
 * then create the idle task, and run the OS.
 * The idle task uses the default system stack.
 */
int __attribute__ ((weak))
__attribute ((noreturn))
main (void)
{
	/* Create the idle task. */
	task_idle = (task_t*) task_idle_data;
	memset (task_idle->stack, STACK_MAGIC, ALIGNED_IDLE_TASK_STACKSZ);
	assert (STACK_GUARD (task_idle));
	
	/* Move stack pointer to task_idle stack area */
    unsigned sp = (unsigned)(&task_idle->stack[ALIGNED_IDLE_TASK_STACKSZ]);
    /* stack pointer should align to doubles */
    set_stack_pointer ((void *)( sp & ~(sizeof(double)-1) ));

	task_idle->name = "idle";
	list_init (&task_idle->item);
	list_init (&task_idle->slaves);
#ifdef ARCH_HAVE_FPU
	/* Default state of float point unit. */
	task_idle->fpu_state = ARCH_FPU_STATE;
#endif

	/* Make list of active tasks. */
	list_init (&task_active);
	task_current = task_idle;
	task_activate (task_idle);

	/* Create user tasks. */
	uos_init ();
	
	/* Additional machine-dependent initialization */
	uos_post_init ();

	/* Switch to the most priority task. */
	assert (task_current == task_idle);

    arch_state_t x;
    arch_intr_disable (&x);
	task_schedule ();
	arch_intr_restore(x);

	/* Idle task activity. */
	for (;;) {
		arch_idle ();
	}
}
