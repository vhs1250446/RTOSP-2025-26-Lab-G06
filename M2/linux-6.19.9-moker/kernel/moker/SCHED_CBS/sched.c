// SPDX-License-Identifier: GPL-2.0
/*
 * SCHED_CBS scheduling class.
 *
 * Implements the Constant Bandwidth Server of Abeni & Buttazzo (RTSS 1998)
 * as a per-task EDF server. One enqueue->dequeue cycle of a SCHED_CBS task
 * corresponds to a single AB98 "job" (so the paper's pending-request counter
 * n collapses to the on_rq bit).
 *
 *   On arrival  (enqueue): apply Rule 1.
 *   On execution (update_curr/tick): debit cs; if exhausted, apply Rule 3.
 *   On termination (dequeue): persist cs and dk; Rule 1 will re-evaluate
 *                              them at the next arrival.
 *   Pick: leftmost of the per-CPU EDF rb-tree on dk.
 */
#include "../../sched/sched.h"

#include <uapi/linux/sched/types.h> /* where sched_attr is defined */

#include "cbs_rq.h"
#include "cbs_task.h"


#define QMASK 8 /* Between DL and RT (DL -> SCHED_CBS -> RT -> ... */

/* Time-wrap-safe deadline comparison (mirrors cbs_rq.c). */
static inline bool cbs_dl_before(u64 a, u64 b)
{
	return (s64)(a - b) < 0;
}

/*
 * Reposition an entity in the EDF tree after its dk changed.
 * The rq is locked; nr_running is unchanged across the pair.
 */
static void cbs_rq_requeue(struct cbs_rq *cbs_rq, struct sched_cbs_entity *se)
{
	if (!se->on_rq)
		return;
	cbs_rq_erase(cbs_rq, se);
	cbs_rq_insert(cbs_rq, se);
}

/*
 * Runtime accounting for the currently-running CBS task (rq->donor).
 *
 *   delta = now - exec_start
 *   cs   -= delta
 *   if cs <= 0:  AB98 Rule 3 (replenish), then reposition + resched.
 */
static void update_curr_cbs(struct rq *rq)
{
	struct task_struct *curr = rq->donor;
	struct sched_cbs_entity *cbs_se;
	u64 now;
	s64 delta;

	if (!curr || curr->sched_class != &cbs_sched_class)
		return;

	cbs_se = &curr->cbs;
	now    = rq_clock_task(rq);
	delta  = (s64)(now - cbs_se->exec_start);
	if (delta <= 0)
		return;

	cbs_se->exec_start = now;
	cbs_se->runtime   -= (u64)delta;     /* may wrap "negative" in u64 */

	if ((s64)cbs_se->runtime <= 0) {
		cbs_replenish(cbs_se);            /* dk += Ts; cs += Qs (looped) */
		cbs_rq_requeue(&rq->cbs, cbs_se);
		resched_curr(rq);
	}
}

/*
 * AB98 job arrival: every enqueue is the start of a new job.
 * Rule 1 decides whether the previous (cs, dk) can carry over or must
 * be regenerated against the wakeup time r = rq_clock_task(rq).
 */
static void enqueue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	struct sched_cbs_entity *cbs_se = &p->cbs;
	u64 arrival = rq_clock_task(rq);

	if (cbs_wake_needs_new_dl(cbs_se, arrival)) {
		cbs_se->deadline = arrival + cbs_se->period;
		cbs_se->runtime  = cbs_se->max_runtime;
	}

	cbs_rq_insert(&rq->cbs, cbs_se);
	add_nr_running(rq, 1);

#ifdef CONFIG_MOKER_TRACING
	moker_trace(ENQUEUE_RQ, p, -1);
#endif
}

/*
 * AB98 job termination. cs and dk are intentionally preserved so the
 * next enqueue_task_cbs() can evaluate Rule 1 against them.
 */
static bool dequeue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	update_curr_cbs(rq);

	cbs_rq_erase(&rq->cbs, &p->cbs);
	sub_nr_running(rq, 1);

#ifdef CONFIG_MOKER_TRACING
	moker_trace(DEQUEUE_RQ, p, -1);
#endif
	return true;
}

/*
 * Called when the current task is also SCHED_CBS (same class) and another
 * CBS task wakes up. Higher-class wakeups (DL above us) are handled by the
 * core scheduler via class ordering, so they never reach this hook.
 */
static void wakeup_preempt_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	switch (rq->donor->policy) {
	case SCHED_DEADLINE:
		break;
	case SCHED_FIFO:
	case SCHED_RR:
	case SCHED_NORMAL:
	case SCHED_BATCH:
	case SCHED_IDLE:
		/* CBS class is above these; core has already requested resched. */
		break;
	case SCHED_CBS:
		if (cbs_dl_before(p->cbs.deadline, rq->donor->cbs.deadline))
			resched_curr(rq);
		break;
	}
}

static struct task_struct *pick_task_cbs(struct rq *rq, struct rq_flags *rf)
{
	if (!rq->cbs.nr_running)
		return NULL;
	return cbs_rq_first(&rq->cbs);
}

static void put_prev_task_cbs(struct rq *rq, struct task_struct *p,
			      struct task_struct *next)
{
	update_curr_cbs(rq);
}

static void set_next_task_cbs(struct rq *rq, struct task_struct *p, bool first)
{
	p->cbs.exec_start = rq_clock_task(rq);
}

static int select_task_rq_cbs(struct task_struct *p, int cpu, int flags)
{
	/* AB98 is single-CPU; keep affinity stable for now. */
	return task_cpu(p);
}

static void task_tick_cbs(struct rq *rq, struct task_struct *p, int queued)
{
	update_curr_cbs(rq);
}

static void prio_changed_cbs(struct rq *rq, struct task_struct *p, u64 oldprio)
{
	if (!task_on_rq_queued(p))
		return;
	if (rq->donor == p)
		return;
	if (rq->donor->sched_class != &cbs_sched_class ||
	    cbs_dl_before(p->cbs.deadline, rq->donor->cbs.deadline))
		resched_curr(rq);
}

/*
 * Switching into SCHED_CBS. Static parameters (Qs=max_runtime, Ts=period)
 * are expected to have been installed by the syscall path; cs and dk are
 * left as-is so the very first enqueue_task_cbs() triggers Rule 1.
 */
static void switched_to_cbs(struct rq *rq, struct task_struct *p)
{
	if (!task_on_rq_queued(p))
		return;
	if (rq->donor == p)
		return;
	if (rq->donor->sched_class != &cbs_sched_class ||
	    cbs_dl_before(p->cbs.deadline, rq->donor->cbs.deadline))
		resched_curr(rq);
}


/*
 * ----------------------------------------------------------------------------
 * sched_attr / setscheduler plumbing.
 *
 * sched_attr fields used:
 *   sched_runtime  -> Qs  (server max budget,  ns)
 *   sched_period   -> Ts  (server period,      ns)
 *   sched_deadline -> ignored (CBS uses dk derived from r + Ts)
 *
 * On parameter (re)set the dynamic state (cs, dk) is reset to zero so that
 * cbs_wake_needs_new_dl() at the next enqueue trivially regenerates them
 * via Rule 1.
 * ----------------------------------------------------------------------------
 */

bool __checkparam_cbs(const struct sched_attr *attr)
{
	/* Qs > 0, Ts > 0, Qs <= Ts (utilization in (0, 1]). */
	if (!attr->sched_runtime || !attr->sched_period)
		return false;
	if (attr->sched_runtime > attr->sched_period)
		return false;
	return true;
}

void __setparam_cbs(struct task_struct *p, const struct sched_attr *attr)
{
	struct sched_cbs_entity *cbs_se = &p->cbs;

	cbs_se->max_runtime = attr->sched_runtime;
	cbs_se->period      = attr->sched_period;

	cbs_se->runtime  = 0;
	cbs_se->deadline = 0;
}

void __getparam_cbs(struct task_struct *p, struct sched_attr *attr)
{
	attr->sched_runtime = p->cbs.max_runtime;
	attr->sched_period  = p->cbs.period;
}

bool cbs_param_changed(struct task_struct *p, const struct sched_attr *attr)
{
	return p->cbs.max_runtime != attr->sched_runtime ||
	       p->cbs.period      != attr->sched_period;
}

/*
 * AB98 Theorem 1 admission control: the periodic-hard set plus all CBS
 * servers is EDF-schedulable iff Up + Us <= 1.
 *
 * A complete implementation needs per-root-domain CBS bandwidth bookkeeping
 * (mirroring rq->rd->dl_bw) and a switched_from_cbs hook to release it.
 * Until that is in place we accept all parameters; misconfiguration can
 * still only damage CBS tasks themselves because the class sits below DL.
 */
int sched_cbs_overflow(struct task_struct *p, int policy,
		       const struct sched_attr *attr)
{
	return 0;
}

/*
  1- switched_to
  2- enqueue	  <---------------------------------------------<-
  3- pick_task    <------------------------------------<-	 |
  ++ SWITCHED_TO is traced                              |	 |
  4- set_next_task                                      |	 |
  ++ from here task executes in CPU                     |	 |
  5- task_tick                                          |	 |
  ++ it might get preempted or it continues to run      |	 |
  6- wakeup_preempt                                     |	 |
  7- put_prev_task ++ leaves CPU but continues on rq    |	 |
  ++ SWITCHED_AWAY is traced                            |	 |
  8- pick_task  >---------------------------------------|	 |
       |							 |
       | ++ userspace calls clock_nanosleep			 |
       v							 |
  9- dequeue_task						 |
       |							 |
       |							 |
       >---------------------------------------------------------|
 */

DEFINE_SCHED_CLASS(cbs) = {
	.queue_mask        = QMASK,
	.enqueue_task      = enqueue_task_cbs, /*task goes into CBS rq*/
	.dequeue_task      = dequeue_task_cbs, /*task goes out of CBS rq*/
	.wakeup_preempt    = wakeup_preempt_cbs, /*choose between two CBS tasks*/
	.pick_task         = pick_task_cbs, /*scheduler picks next task to run (leftmost)*/
	.put_prev_task     = put_prev_task_cbs, /*tasks continues to run */
	.set_next_task     = set_next_task_cbs, /**/
	.select_task_rq    = select_task_rq_cbs, /**/
	.set_cpus_allowed  = set_cpus_allowed_common, /**/
	.task_tick         = task_tick_cbs, /*1 execution step*/
	.prio_changed      = prio_changed_cbs, /**/
	.switched_to       = switched_to_cbs, /*inits the task when it becomes SCHED_CBS*/
	.update_curr       = update_curr_cbs, /**/
};
