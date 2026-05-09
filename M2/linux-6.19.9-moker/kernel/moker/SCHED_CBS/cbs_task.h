#ifndef __CBS_TASK_H_
#define __CBS_TASK_H_


#include <linux/rbtree_types.h>
#include <linux/types.h>

/*
 * Per-task CBS server state (Abeni & Buttazzo, 1998).
 *
 * One enqueue->dequeue cycle of the task corresponds to a single AB98 "job".
 * Static server parameters (Qs, Ts) are set from sched_attr; the dynamic
 * state (cs, dk) is preserved across jobs so that AB98 Rule 1 can be
 * evaluated on every arrival (every enqueue_task_cbs).
 */
struct sched_cbs_entity {
	struct rb_node rb_node;       /* node in cbs_rq.tasks_timeline (key: dk) */

	/* static server parameters */
	u64 max_runtime;              /* Qs: server max budget            (ns) */
	u64 period;                   /* Ts: server period                (ns) */

	/* dynamic server state (persists across jobs) */
	u64 runtime;                  /* cs: remaining budget        (ns)      */
	u64 deadline;                 /* dk: current server deadline (ns)      */

	/* runtime accounting */
	u64 exec_start;               /* rq_clock_task at last update_curr_cbs */

	int on_rq;                    /* 1 iff this entity is in the rb-tree   */
};


#endif
