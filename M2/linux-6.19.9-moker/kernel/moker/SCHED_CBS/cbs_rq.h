#ifndef __CBS_RQ_H_
#define __CBS_RQ_H_

#include <linux/sched.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/spinlock.h>
#include <linux/types.h>

struct task_struct;
struct sched_cbs_entity;

/*
 * Per-CPU CBS sub-runqueue.
 *
 * Tasks are ordered by their current CBS deadline (dk) in tasks_timeline,
 * so that pick_task_cbs() simply returns the leftmost entity (EDF).
 */
struct cbs_rq {
	struct rb_root_cached tasks_timeline;   /* EDF tree keyed on dk          */
	unsigned int          nr_running;       /* number of CBS tasks queued    */
	u64                   total_bw;         /* sum of Qs/Ts (fixed-point)    */
	u64                   earliest_dl;      /* dk of leftmost entity (cache) */
};

/*
 * Sets the rq to an empty state.
 * Called once per CPU at boot from sched_init().
 */
void init_cbs_rq(struct cbs_rq *rq);

/* rb-tree management (keyed on entity->deadline) */
void cbs_rq_insert(struct cbs_rq *rq, struct sched_cbs_entity *cbs_se);
void cbs_rq_erase(struct cbs_rq *rq, struct sched_cbs_entity *cbs_se);
struct task_struct *cbs_rq_first(struct cbs_rq *rq);

/*
 * AB98 Rule 1 (job arrival / enqueue):
 *   true  -> regenerate: dk = now + Ts, cs = Qs
 *   false -> keep current dk, cs
 *
 * Equivalent (overflow-safe) form of  cs >= (dk - now) * Us  with Us = Qs/Ts:
 *   cs * Ts >= (dk - now) * Qs
 * If (dk - now) <= 0 the inequality is trivially true.
 */
bool cbs_wake_needs_new_dl(struct sched_cbs_entity *cbs_se, u64 now);

/*
 * AB98 Rule 3 (budget exhaustion, runs while cs <= 0):
 *   dk += Ts; cs += Qs;
 * The += form (vs the paper's `cs = Qs`) absorbs overshoot when a tick
 * straddles multiple server periods.
 */
void cbs_replenish(struct sched_cbs_entity *cbs_se);

#endif
