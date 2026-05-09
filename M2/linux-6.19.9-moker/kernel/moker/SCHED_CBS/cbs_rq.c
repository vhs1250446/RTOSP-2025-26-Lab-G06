// SPDX-License-Identifier: GPL-2.0
/*
 * Per-CPU CBS sub-runqueue plus AB98 algorithmic helpers.
 *
 * Reference: L. Abeni and G. Buttazzo, "Integrating Multimedia Applications
 * in Hard Real-Time Systems", RTSS 1998 (AB98), Section 4.1.
 */

#include <linux/kernel.h>
#include <linux/rbtree.h>
#include <linux/sched.h>
#include <linux/types.h>

#include "cbs_rq.h"
#include "cbs_task.h"

#define entity_of_node(n)  rb_entry((n), struct sched_cbs_entity, rb_node)
#define task_of_entity(e)  container_of((e), struct task_struct, cbs)

/* Time-wrap-safe deadline comparison (cf. dl_time_before in deadline.c). */
static inline bool cbs_dl_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

static void cbs_update_earliest(struct cbs_rq *rq)
{
	struct rb_node *leftmost = rb_first_cached(&rq->tasks_timeline);

	rq->earliest_dl = leftmost ? entity_of_node(leftmost)->deadline : 0;
}

void init_cbs_rq(struct cbs_rq *rq)
{
	rq->tasks_timeline = RB_ROOT_CACHED;
	rq->nr_running    = 0;
	rq->total_bw      = 0;
	rq->earliest_dl   = 0;
}

void cbs_rq_insert(struct cbs_rq *rq, struct sched_cbs_entity *cbs_se)
{
	struct rb_node **link = &rq->tasks_timeline.rb_root.rb_node;
	struct rb_node  *parent = NULL;
	bool leftmost = true;

	while (*link) {
		struct sched_cbs_entity *entry;

		parent = *link;
		entry  = entity_of_node(parent);

		if (cbs_dl_before(cbs_se->deadline, entry->deadline)) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = false;
		}
	}

	rb_link_node(&cbs_se->rb_node, parent, link);
	rb_insert_color_cached(&cbs_se->rb_node, &rq->tasks_timeline, leftmost);

	cbs_se->on_rq = 1;
	rq->nr_running++;
	if (leftmost)
		rq->earliest_dl = cbs_se->deadline;
}

void cbs_rq_erase(struct cbs_rq *rq, struct sched_cbs_entity *cbs_se)
{
	if (!cbs_se->on_rq)
		return;

	rb_erase_cached(&cbs_se->rb_node, &rq->tasks_timeline);
	RB_CLEAR_NODE(&cbs_se->rb_node);

	cbs_se->on_rq = 0;
	rq->nr_running--;
	cbs_update_earliest(rq);
}

struct task_struct *cbs_rq_first(struct cbs_rq *rq)
{
	struct rb_node *leftmost = rb_first_cached(&rq->tasks_timeline);

	if (!leftmost)
		return NULL;

	return task_of_entity(entity_of_node(leftmost));
}

/*
 * AB98 Rule 1 (server idle, job arrival at time r):
 *
 *     if  cs >= (dk - r) * Us       with Us = Qs / Ts
 *         => deadline cannot accommodate the residual budget,
 *            regenerate: dk = r + Ts, cs = Qs
 *     else
 *         => keep current dk, cs
 *
 * Multiplied form (no division, integer arithmetic):
 *     cs * Ts >= (dk - r) * Qs
 *
 * If (dk - r) <= 0 the deadline is already in the past, which trivially
 * satisfies the condition; treat it as "regenerate".
 *
 * Overflow note: for sane parameters (Qs, Ts, cs all <= a few seconds in
 * nanoseconds, i.e. < 2^32) both products fit in u64.
 */
bool cbs_wake_needs_new_dl(struct sched_cbs_entity *cbs_se, u64 now)
{
	s64 slack = (s64)cbs_se->deadline - (s64)now;
	u64 lhs, rhs;

	if (slack <= 0)
		return true;

	lhs = cbs_se->runtime * cbs_se->period;          /* cs * Ts */
	rhs = (u64)slack       * cbs_se->max_runtime;    /* (dk - r) * Qs */

	return lhs >= rhs;
}

/*
 * AB98 Rule 3 (budget exhaustion):
 *     dk = dk + Ts;  cs = cs + Qs;
 *
 * Looped to absorb the case where a single accounting step accumulated
 * more overrun than one server period (cs interpreted as signed via cast,
 * since update_curr_cbs may have driven runtime "negative" in u64).
 */
void cbs_replenish(struct sched_cbs_entity *cbs_se)
{
	while ((s64)cbs_se->runtime <= 0) {
		cbs_se->deadline += cbs_se->period;
		cbs_se->runtime  += cbs_se->max_runtime;
	}
}
