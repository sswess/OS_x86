/*----- Includes without dependencies -----*/

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include "mypthread.h"

/*----- Define constants -----*/

#define true 1
#define false 0
#define STACK sizeof(mypthread_t) * 512
#define UCSTACK 16384 

/*----- Global Variables -----*/

//int for initialization
static int thread_id = 0;

//Flow control ints
static int current_thread = 0;
static int next_thread = -1;

//Global structs
static threadpool thread_pool;
static mypthread_attr_t empty_attr;


/*----- Function Implementations -----*/

/*-- Helper Functions --*/
void create_main_thread(){
   printf("createl\n");
   current_thread = 0;

   mypthread_t* main;
   main = (mypthread_t*) malloc(sizeof(mypthread_t));
   thread_pool.thread[0] = main; // &main
   thread_pool.thread[0]->pid = 0;
   thread_pool.state[0] = RUNNING;

   // Prevent Segfault
   getcontext(&thread_pool.thread[0]->context);

   //Malloc context stack
   thread_pool.thread[0]->context.uc_stack.ss_sp = malloc(UCSTACK); 
   thread_pool.thread[0]->context.uc_stack.ss_size = UCSTACK;
}

void init_thread_pool(){
    printf("init_thread_pool\n");
    int i;
    for(i = 0;i<512;i++){
      // Initialize Variables in thread_pool
      thread_pool.tid[i] = i;
      thread_pool.previous_tid[i] = 0;
      thread_pool.state[i] = UNUSED;
   }
}

/*-- Required Functions --*/
int mypthread_create(mypthread_t *thread, const mypthread_attr_t *attr,
         void *(*start_routine) (void *), void *arg)
{
   //printf("mypthread_create\n");

   //Push initial thread and malloc parts of main struct
   if(thread_id == 0)
   {
      printf("Initial thread created.\n");
      create_main_thread();
      init_thread_pool();
      thread_id = 1;
   }

   // Edge case failure - Too many threads.
   if(thread_id == 512)
   {
     printf("Too many threads.\n");
     return -1; 
   } 

   // Create + malloc thread and make context
   thread->pid = thread_id;

   getcontext(&thread->context);
         thread->context.uc_stack.ss_sp = malloc(UCSTACK *sizeof(char)); 
         thread->context.uc_stack.ss_size = UCSTACK;
         thread->context.uc_stack.ss_flags = 0;
         thread->context.uc_link = NULL; // &thread_pool.thread[0]->context;
   
   thread_pool.thread[thread_id] = thread;
   thread_pool.tid[thread_id] = thread_id;
   thread_pool.state[thread_id] = READY;

   makecontext(&thread->context, (void (*) (void)) start_routine, 1, arg);
   
   thread_id++;
   return 0;
}

void mypthread_exit(void *retval)
{
  // Exits the thread.
  // Return value is whether its joinable. (Exit status of the thread)

   //  printf("mypthread_exit\n");

   //If main thread tries to exit, return
   if(current_thread == 0)
      return;

   int temp = -1;

   //Modify current thread state
   thread_pool.state[current_thread] = ZOMBIE;

   //Save parent thread (from join) and set parent to main
   temp = thread_pool.previous_tid[current_thread];
   thread_pool.previous_tid[current_thread] = 0;

   //Modify target thread state and switch
   current_thread = thread_pool.thread[temp]->pid;
   thread_pool.state[current_thread] = READY;

   setcontext(&thread_pool.thread[temp]->context);

}

int mypthread_yield(void)
{
   //printf("mypthread_yield\n");

   // Yield runs the next guy ready.

   //Control variables
   int i;
   int readyIndex = -1;

   //Special emergency edge case
   if(thread_pool.state[current_thread] == UNUSED)
      {current_thread = 1; next_thread = 1;}

   //Modify global variable to yield next available thread
   if(next_thread == -1)
      next_thread = current_thread;

   //Find next ready thread after current.
   for(i = next_thread; i < 512; i++)
   {
      if(thread_pool.state[i] == READY)
      {
         readyIndex = i;
         break;
      }
   }

   //If nothing ready after current, check from start
   if(readyIndex == -1)
   {
      for(i = 0; i < next_thread; i++)
      {
         if(thread_pool.state[i] == READY)
         {
            readyIndex = i;
            break;
         }
      }
   }

   //If ready thread found
   if(readyIndex != -1)
   {
      int tmp = current_thread;
      next_thread = readyIndex +1;
      current_thread = readyIndex;


      thread_pool.state[tmp] = READY;
      setcontext(&thread_pool.thread[readyIndex]->context);
      
      //Think these are neccesary
      thread_pool.state[readyIndex] = READY;
      current_thread = tmp;// + 1;
      
      return 0;
   }
   else
   {
      //No thread to yield
      return 0;
   }

   panic("Reached end of yield \n");
   return 1;


}

int mypthread_join(mypthread_t thread, void **retval)
{
   //TODO
   // ADD check to join to make sure it is ready

   //printf("pthread_join, current thread %d\n", current_thread);
  
   // Return value is 0 on success, err%orcode on failure.
   //the thread(a) that calls gives execution to thread(b)

   int index = thread.pid;    

   //Establish link to join target
   thread_pool.previous_tid[index] = thread_pool.thread[current_thread]->pid;
   thread_pool.thread[index]->context.uc_link = &thread_pool.thread[0]->context;

   //Change state of join'er and join'ee
   thread_pool.state[current_thread] = BLOCKED;
   thread_pool.state[index] = READY;

   //Control
      //int original_thread = current_thread;
   current_thread = index;

   swapcontext(&thread_pool.thread[0]->context, &thread_pool.thread[current_thread]->context);
   
   //printf("End join.\n");
   return 0;
}

// Function is responsible for reporting a fatal error and halting execution.
void panic(char *reason) {
    fprintf(stderr, "Sorry, the program has encountered an unrecoverable error. Given reason was: %s\n", reason);
    exit(EXIT_FAILURE);
}
