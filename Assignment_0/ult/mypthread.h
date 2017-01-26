#ifndef H_MYPTHREAD
#define H_MYPTHREAD

// Types
 typedef enum {
      UNUSED,
      READY,
      RUNNING,
      BLOCKED,
      ZOMBIE
   } thread_state;


typedef struct mypthread_t mypthread_t;

struct mypthread_t
{
   // Fields for each thread 
   ucontext_t context;
   int pid;
};

typedef struct {
   // Left empty
}mypthread_attr_t;

typedef struct {
   // Data Structure tracking threads
   int tid[512];
   int previous_tid[512];
   thread_state state[512];
   mypthread_t* thread[512];
} threadpool;

// Functions
int mypthread_create(mypthread_t *thread, const mypthread_attr_t *attr,
         void *(*start_routine) (void *), void *arg);

void mypthread_exit(void *retval);

int mypthread_yield(void);

int mypthread_join(mypthread_t thread, void **retval);

void panic(char *reason);

void create_main_thread();

void init_thread_pool();


/* Don't touch anything after this line.
 *
 * This is included just to make the mtsort.c program compatible
 * with both your ULT implementation as well as the system pthreads
 * implementation. The key idea is that mutexes are essentially
 * useless in a cooperative implementation, but are necessary in
 * a preemptive implementation.
 */

typedef int mypthread_mutex_t;
typedef int mypthread_mutexattr_t;

static inline int mypthread_mutex_init(mypthread_mutex_t *mutex,
         const mypthread_mutexattr_t *attr) { return 0; }

static inline int mypthread_mutex_destroy(mypthread_mutex_t *mutex) { return 0; }

static inline int mypthread_mutex_lock(mypthread_mutex_t *mutex) { return 0; }

static inline int mypthread_mutex_trylock(mypthread_mutex_t *mutex) { return 0; }

static inline int mypthread_mutex_unlock(mypthread_mutex_t *mutex) { return 0; }

#endif
