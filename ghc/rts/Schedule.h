/* -----------------------------------------------------------------------------
 * $Id: Schedule.h,v 1.16 2000/03/16 17:27:13 simonmar Exp $
 *
 * (c) The GHC Team 1998-1999
 *
 * Prototypes for functions in Schedule.c 
 * (RTS internal scheduler interface)
 *
 * -------------------------------------------------------------------------*/

//@menu
//* Scheduler Functions::	
//* Scheduler Vars and Data Types::  
//* Some convenient macros::	
//* Index::			
//@end menu

//@node Scheduler Functions, Scheduler Vars and Data Types
//@subsection Scheduler Functions

//@cindex initScheduler
//@cindex exitScheduler
//@cindex startTasks
/* initScheduler(), exitScheduler(), startTasks()
 * 
 * Called from STG :  no
 * Locks assumed   :  none
 */
void initScheduler( void );
void exitScheduler( void );
#ifdef SMP
void startTasks( void );
#endif

//@cindex awakenBlockedQueue
/* awakenBlockedQueue()
 *
 * Takes a pointer to the beginning of a blocked TSO queue, and
 * wakes up the entire queue.
 *
 * Called from STG :  yes
 * Locks assumed   :  none
 */
#if defined(GRAN)
void awakenBlockedQueue(StgBlockingQueueElement *q, StgClosure *node);
#elif defined(PAR)
void awakenBlockedQueue(StgBlockingQueueElement *q, StgClosure *node);
#else
void awakenBlockedQueue(StgTSO *tso);
#endif

//@cindex unblockOne
/* unblockOne()
 *
 * Takes a pointer to the beginning of a blocked TSO queue, and
 * removes the first thread, placing it on the runnable queue.
 *
 * Called from STG : yes
 * Locks assumed   : none
 */
#if defined(GRAN)
StgTSO *unblockOne(StgTSO *tso, StgClosure *node);
#elif defined(PAR)
StgTSO *unblockOne(StgTSO *tso, StgClosure *node);
#else
StgTSO *unblockOne(StgTSO *tso);
#endif

//@cindex raiseAsync
/* raiseAsync()
 *
 * Raises an exception asynchronously in the specified thread.
 *
 * Called from STG :  yes
 * Locks assumed   :  none
 */
void raiseAsync(StgTSO *tso, StgClosure *exception);

//@cindex awaitEvent
/* awaitEvent()
 *
 * Raises an exception asynchronously in the specified thread.
 *
 * Called from STG :  NO
 * Locks assumed   :  sched_mutex
 */
void awaitEvent(rtsBool wait);  /* In Select.c */

// ToDo: check whether all fcts below are used in the SMP version, too
//@cindex awaken_blocked_queue
#if defined(GRAN)
void    awaken_blocked_queue(StgBlockingQueueElement *q, StgClosure *node);
void    unlink_from_bq(StgTSO* tso, StgClosure* node);
void    initThread(StgTSO *tso, nat stack_size, StgInt pri);
#elif defined(PAR)
nat     run_queue_len(void);
void    awaken_blocked_queue(StgBlockingQueueElement *q, StgClosure *node);
void    initThread(StgTSO *tso, nat stack_size);
#else
char   *info_type(StgClosure *closure);    // dummy
char   *info_type_by_ip(StgInfoTable *ip); // dummy
void    awaken_blocked_queue(StgTSO *q);
void    initThread(StgTSO *tso, nat stack_size);
#endif

//@node Scheduler Vars and Data Types, Some convenient macros, Scheduler Functions
//@subsection Scheduler Vars and Data Types

//@cindex context_switch
/* Context switch flag.
 * Locks required  : sched_mutex
 */
extern nat context_switch;
extern rtsBool interrupted;

extern  nat ticks_since_select;

//@cindex Capability
/* Capability type
 */
typedef StgRegTable Capability;

/* Free capability list.
 * Locks required: sched_mutex.
 */
#ifdef SMP
extern Capability *free_capabilities;
extern nat n_free_capabilities;
#else
extern Capability MainRegTable;
#endif

/* Thread queues.
 * Locks required  : sched_mutex
 */
extern  StgTSO *run_queue_hd, *run_queue_tl;
extern  StgTSO *blocked_queue_hd, *blocked_queue_tl;
extern  StgTSO *all_threads;

#ifdef SMP
//@cindex sched_mutex
//@cindex thread_ready_cond
//@cindex gc_pending_cond
extern pthread_mutex_t sched_mutex;
extern pthread_cond_t  thread_ready_cond;
extern pthread_cond_t  gc_pending_cond;
#endif

//@cindex task_info
#ifdef SMP
typedef struct {
  pthread_t id;
  double    elapsedtimestart;
  double    mut_time;
  double    mut_etime;
  double    gc_time;
  double    gc_etime;
} task_info;

extern task_info *task_ids;
#endif

#if !defined(GRAN)
extern  StgTSO *run_queue_hd, *run_queue_tl;
extern  StgTSO *blocked_queue_hd, *blocked_queue_tl;
#endif

/* Needed by Hugs.
 */
void interruptStgRts ( void );

// ?? needed -- HWL
void raiseAsync(StgTSO *tso, StgClosure *exception);
nat  run_queue_len(void);

void resurrectThreads( StgTSO * );

//@node Some convenient macros, Index, Scheduler Vars and Data Types
//@subsection Some convenient macros

/* debugging only 
 */
#ifdef DEBUG
void printThreadBlockage(StgTSO *tso);
void printThreadStatus(StgTSO *tso);
void printAllThreads(void);
#endif
void print_bq (StgClosure *node);

/* -----------------------------------------------------------------------------
 * Some convenient macros...
 */

#define END_TSO_QUEUE  ((StgTSO *)(void*)&END_TSO_QUEUE_closure)
#define END_CAF_LIST   ((StgCAF *)(void*)&END_TSO_QUEUE_closure)

//@cindex APPEND_TO_RUN_QUEUE
/* Add a thread to the end of the run queue.
 * NOTE: tso->link should be END_TSO_QUEUE before calling this macro.
 */
#define APPEND_TO_RUN_QUEUE(tso)		\
    ASSERT(tso->link == END_TSO_QUEUE);		\
    if (run_queue_hd == END_TSO_QUEUE) {	\
      run_queue_hd = tso;			\
    } else {					\
      run_queue_tl->link = tso;			\
    }						\
    run_queue_tl = tso;

//@cindex PUSH_ON_RUN_QUEUE
/* Push a thread on the beginning of the run queue.  Used for
 * newly awakened threads, so they get run as soon as possible.
 */
#define PUSH_ON_RUN_QUEUE(tso)			\
    tso->link = run_queue_hd;			\
      run_queue_hd = tso;			\
    if (run_queue_tl == END_TSO_QUEUE) {	\
      run_queue_tl = tso;			\
    }

//@cindex POP_RUN_QUEUE    
/* Pop the first thread off the runnable queue.
 */
#define POP_RUN_QUEUE()				\
  ({ StgTSO *t = run_queue_hd;			\
    if (t != END_TSO_QUEUE) {			\
      run_queue_hd = t->link;			\
      t->link = END_TSO_QUEUE;			\
      if (run_queue_hd == END_TSO_QUEUE) {	\
        run_queue_tl = END_TSO_QUEUE;		\
      }						\
    }						\
    t;						\
  })

//@cindex APPEND_TO_BLOCKED_QUEUE
/* Add a thread to the end of the blocked queue.
 */
#define APPEND_TO_BLOCKED_QUEUE(tso)		\
    ASSERT(tso->link == END_TSO_QUEUE);		\
    if (blocked_queue_hd == END_TSO_QUEUE) {    \
      blocked_queue_hd = tso;			\
    } else {					\
      blocked_queue_tl->link = tso;		\
    }						\
    blocked_queue_tl = tso;

//@cindex THREAD_RUNNABLE
/* Signal that a runnable thread has become available, in
 * case there are any waiting tasks to execute it.
 */
#ifdef SMP
#define THREAD_RUNNABLE()			\
  if (free_capabilities != NULL) {		\
     pthread_cond_signal(&thread_ready_cond);	\
  }						\
  context_switch = 1;
#else
#define THREAD_RUNNABLE()  /* nothing */
#endif

//@node Index,  , Some convenient macros
//@subsection Index

//@index
//* APPEND_TO_BLOCKED_QUEUE::  @cindex\s-+APPEND_TO_BLOCKED_QUEUE
//* APPEND_TO_RUN_QUEUE::  @cindex\s-+APPEND_TO_RUN_QUEUE
//* Capability::  @cindex\s-+Capability
//* POP_RUN_QUEUE    ::  @cindex\s-+POP_RUN_QUEUE    
//* PUSH_ON_RUN_QUEUE::  @cindex\s-+PUSH_ON_RUN_QUEUE
//* THREAD_RUNNABLE::  @cindex\s-+THREAD_RUNNABLE
//* awaitEvent::  @cindex\s-+awaitEvent
//* awakenBlockedQueue::  @cindex\s-+awakenBlockedQueue
//* awaken_blocked_queue::  @cindex\s-+awaken_blocked_queue
//* context_switch::  @cindex\s-+context_switch
//* exitScheduler::  @cindex\s-+exitScheduler
//* gc_pending_cond::  @cindex\s-+gc_pending_cond
//* initScheduler::  @cindex\s-+initScheduler
//* raiseAsync::  @cindex\s-+raiseAsync
//* sched_mutex::  @cindex\s-+sched_mutex
//* startTasks::  @cindex\s-+startTasks
//* task_info::  @cindex\s-+task_info
//* thread_ready_cond::  @cindex\s-+thread_ready_cond
//* unblockOne::  @cindex\s-+unblockOne
//@end index
