

#include "log2mem/log2mem.h"

#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>

#define MEMMAPFILE_SWITCH "--memmapfile"
#define HELP_SWITCH "--help"


const char* timestamp()
{
  static char lbuf[100] = {'\0', '\0'};

  // get current time
  struct timeval now;
  struct timezone * const tz = NULL; /* not used on Linux */
  gettimeofday(&now, tz);

  // break time down into parts
  struct tm _tm;
  localtime_r(&now.tv_sec, &_tm);


  snprintf(lbuf, sizeof(lbuf)-1, "gettimeofday  %02li.%09li",
           now.tv_sec, now.tv_usec*1000);

  return lbuf;
}

const char* posix_timestamp(clockid_t clk_id, const char* s)
{
  static char lbuf[100] = {'\0', '\0'};
  struct timespec tp;

  if (!clock_gettime(clk_id, &tp))
  {
    snprintf(lbuf, sizeof(lbuf)-1, "clock_gettime %s %02li.%09li", s,
             tp.tv_sec,
             tp.tv_nsec);
  }
  return lbuf;
}


void loop(const char* s)
{
  log2mem_write("starting loop", __FILE__, __LINE__);

  /* void * rowptr = log2mem_get_row(__FILE__, __LINE__); */
  /* log2mem_append_str(rowptr, "loop name:", 10); */
  /* log2mem_append_str(rowptr, s, strlen(s)); */

  float  f = 1.1;
  int i = 0;

  int iterations = 0;
  for (i = 0; i < 50; ++i)
  {
    f += 1.12;
//    log2mem_write(s,__FILE__, __LINE__);
    struct timeval tv;
    gettimeofday(&tv, 0);

    long l =  tv.tv_sec*1000000+tv.tv_usec;

    void* rowptr = log2mem_get_row(MEMLOGFL);
    log2mem_append_long(rowptr, l);

    log2mem_var_double_set(i & 0x7, f);

    log2mem_counter_incr(0);

    iterations++;
  }
  log2mem_write("ending loop", MEMLOGFL);


  sleep(4);
  void * rowptr = log2mem_get_row(__FILE__, __LINE__);
  log2mem_append_str(rowptr, "loop name:");
  log2mem_append_str(rowptr, s);
  log2mem_append_str(rowptr, "completed after");
  log2mem_append_int(rowptr, iterations);
  log2mem_append_str(rowptr, "iterations");

}



void flow1(int idoffset)
{
  int i = 0;
  for (; i < 1000; ++i)
  {
    int id = idoffset + i*10;

    log2mem_ts t0 = log2mem_time();
    log2mem_ts t1 = log2mem_time();
    log2mem_ts t2 = log2mem_time();
    log2mem_ts t3 = log2mem_time();

    /* time logging of an initial pre-sequence */
    log2mem_time_event3(0, id, "preseq_beg");
    log2mem_time_event2(0, id);
    log2mem_time_event2(0, id);
    log2mem_time_event3(0, id, "preseq_end");

    /* pre-sequence now gives way to the loop of sub-sequences. */
    int j;
    for (j=0; j < 3; ++j)
    {

      log2mem_counter_incr(0);

      void* row = log2mem_time_event3(1, id+j, "subseq_beg");

      /* log the ID of the preseq*/
      log2mem_append_str(row, "preseqid");
      log2mem_append_str(row, "XXYYZZ");
      log2mem_append_int(row, id);

      log2mem_time_event2(1, id+j);
      log2mem_time_event2(1, id+j);
      log2mem_time_event2(1, id+j);
      log2mem_time_event3(1, id+j, "subseq_end");
    }

    log2mem_time_event3(0, id, "fullseq_end");

    log2mem_set_ts( log2mem_time_event3(2, 0, "t0"), t0 );
    log2mem_set_ts( log2mem_time_event3(2, 1, "t1"), t1 );
    log2mem_set_ts( log2mem_time_event3(2, 2, "t2"), t2 );
    log2mem_set_ts( log2mem_time_event3(2, 3, "t3"), t3 );
    log2mem_set_ts( log2mem_time_event3(2, 3, "t3"), t3 );

    int k = 0;
    for (k = 0; k < 3; ++k)
    {
      void* r = log2mem_time_event3(0, id, "TEST");

      log2mem_append_str(r, "example_string");
      log2mem_append_str2(r,  "str2", 4);
      log2mem_append_int(r,   -1024);
      log2mem_append_uint(r,  +1024);
      log2mem_append_long(r,  -92233720368547758UL);
      log2mem_append_ulong(r, 9223372036854775807);
      log2mem_append_float(r, 1.1);
      log2mem_append_double(r, 1234.0000001);
      log2mem_append_char(r,  'x');
      log2mem_time_event3(0, id, "POST");
    }

//    usleep(500000 + idoffset);
  }
}

//----------------------------------------------------------------------

void *THREAD_A_main(void *arg)
{
  loop("A");
  return NULL;
}
void *THREAD_B_main(void *arg)
{
  loop("B");
  return NULL;
}
void *THREAD_C_main(void *arg)
{
  flow1(200000);
  return NULL;
}
void *THREAD_D_main(void *arg)
{
  flow1(10000);
  return NULL;
}



//----------------------------------------------------------------------
void help()
{
  printf("log2mem example progam\n\n");
  printf("Options\n");
  printf("  " MEMMAPFILE_SWITCH " - memory mapped file to create. If not provided, will use default name.\n");
  printf("  " HELP_SWITCH " - display help\n");
  exit(0);
}

//----------------------------------------------------------------------
void die(const char* arg)
{
  printf("%s\n", arg);
  exit(1);
}

//----------------------------------------------------------------------
int main(int argc, char** argv)
{
  const char* user_filename = 0;
  int i;

  printf ("argc: %d\n", argc);

  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i], MEMMAPFILE_SWITCH)==0)
    {
      i++;
      if (i>=argc) die("missing arg for option " MEMMAPFILE_SWITCH);
      user_filename = argv[i];
    }
    else if (strcmp(argv[i], HELP_SWITCH)==0)
    {
      help();
    }
    else
    {
      die("unknown option");
    }
  }

  printf("=== log2mem example program ===\n");


  if (user_filename)
  {
    struct log2mem_config config;
    memset(&config, 0, sizeof(struct log2mem_config));

    // add MEMLOG_THREADID to collect thread-id
    config.options |= MEMLOG_THREADID;

    // add MEMLOG_FILELINE to collect the file and line
    config.options |= MEMLOG_FILELINE;

    //config.options |= MEMLOG_GETTIMEOFDAY;
    //config.options |= MEMLOG_RDTSC;
    config.options |= MEMLOG_CLOCKGETTIME;

    config.num_counters=8;
    config.num_rows=1020;
    config.row_len=160;
    config.num_vars=10;

    printf("initialising log2mem memmap '%s'\n", user_filename);
    log2mem_init(user_filename, &config, sizeof(config));
  }
  else
  {
    printf("using memmap default initialisation (instead of explicit initialisation)\n");
  }

  printf("log2mem status: %s\n", (log2mem_valid()? "ok":"bad"));
  if (!log2mem_valid())
  {
    printf("log2mem could not be initialised ... exiting\n");
    exit(1);
  }

  printf("log2mem memmap filename: '%s'\n", log2mem_filename());

  printf("logging some rows\n");

  log2mem_write("log2mem starting", __FILE__, __LINE__);

  void* rowptr = log2mem_get_row(__FILE__,__LINE__);
  log2mem_append_int(rowptr, 100);
  log2mem_append_int(rowptr, 200);
  log2mem_append_int(rowptr, 300);

  printf("using some counters\n");
  log2mem_counter_label(0, "loops_made");
  log2mem_counter_label(1, "misc");

  log2mem_counter_incr(1);
  log2mem_counter_incr(1);
  log2mem_counter_incr(1);
  log2mem_counter_incr(1);

  printf("spawning separate threads, each will log\n");
  pthread_attr_t threadAttr;
  pthread_attr_init(&threadAttr);

  pthread_t threadA;
  pthread_create(&threadA, &threadAttr, THREAD_A_main, 0);

  pthread_t threadB;
  pthread_create(&threadB, &threadAttr, THREAD_B_main, 0);

  pthread_t threadC;
  pthread_create(&threadC, &threadAttr, THREAD_C_main, 0);

  pthread_t threadD;
  pthread_create(&threadD, &threadAttr, THREAD_D_main, 0);


  printf("waiting for threads to complete ... ");
  pthread_join(threadA, 0);
  pthread_join(threadB, 0);
  pthread_join(threadC, 0);
  pthread_join(threadD, 0);
  printf("ok\n");


  int          stopflag_variable_id   = 1;
  const char*  stopflag_variable_name = "stopflag";
  printf("creating a log2mem variable, id=%d ...\n", stopflag_variable_id);

  log2mem_var_int_init(stopflag_variable_id, 0, stopflag_variable_name);

  printf("waiting until variable, id=%d, is set to 1\n",
         stopflag_variable_id);

  printf("==> to trigger loop exit, use the log2memdump tool, eg:\n");
  printf("\n\t./log2mem-dump   %s --int[%d]=1\n",log2mem_filename(),stopflag_variable_id);



  int j = 0;
  void * rptr;
  for (j =0 ; j < 1000; j++)
  {
//    MEMLOGWRITE("function entered");  // 450 nanosec

// 130 nanosec on 2.5 GHz Core i5 2.50GHz


    rptr = log2mem_get_row(__FILE__, __LINE__);
    log2mem_append_str(rptr,  "function is:");
    log2mem_append_double(rptr,  j*17.0);
    log2mem_append_long(rptr,  j*17);
  }

  while (1)
  {
    sleep(1);

    /* read a value from the memmap -- this allows the user to interact with the
     * program from outside. */
    int stopflag = log2mem_var_int_get(stopflag_variable_id);
    if (stopflag)
    {
      printf("detected stopflag==1\n");
      break;
    }

  }

  rowptr = log2mem_get_row(__FILE__,__LINE__);
  log2mem_append_str(rowptr, "application ending");
  log2mem_append_str(rowptr, "normally");

  return 0;
}
