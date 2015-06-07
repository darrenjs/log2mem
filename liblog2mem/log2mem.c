/*
    Copyright 2013, Darren Smith

    This file is part of log2mem, a library for providing high performance
    logging, timing and debugging to applications.

    log2mem is free software: you can redistribute it and/or modify it under the
    terms of the GNU General Public License as published by the Free Software
    Foundation, either version 3 of the License, or (at your option) any later
    version.

    log2mem is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with log2mem.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "log2mem/log2mem_dev.h"

#include "log2mem_atomic.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>

#include <sys/time.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>



#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h> /* for getopt_long; standard getopt is in unistd.h */
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <sys/prctl.h>




/* */
static struct log2mem_handle * g_log2mem = 0;

/* */
static int g_log2mem_errno = 0;

/* */
static volatile int g_log2mem_lock = 0;


static volatile int g_log2mem_dummytotal = 0;

/* These rdtsc implementations are based on code in ACE 6.1.0.  The "memory"
 * clobber is probably present as an attempt to serialise the rdtsc. */
#if defined(__i386__)
static inline uint64_t read_rdtsc(void)
{
  uint64_t edxeax;
  asm volatile ("rdtsc" : "=A" (edxeax) : : "memory");
  return edxeax;
}
#elif defined (__amd64__) || defined(__x86_64__)
static inline uint64_t read_rdtsc(void)
{
  uint32_t edx, eax;
  asm volatile ("rdtsc" : "=a"(eax), "=d"(edx) : : "memory");
  return ( (uint64_t)eax) |( ((uint64_t)edx)<<32 );
}
#endif


// forwards
static char*       __default_filename();
static void        __default_log2mem_init();


#define MEMLOG_PREPARE_PTR {                                    \
    if (g_log2mem == 0)                                          \
    {                                                           \
      __default_log2mem_init();                                  \
    }                                                           \
  }


#define MEMLOG_PUT_HDR(PTR, DATATYPE, DATALEN) do {                   \
    struct log2mem_data_hdr* __hdr = (struct log2mem_data_hdr*)(PTR);   \
    __hdr->type = DATATYPE;                                           \
    __hdr->len  = sizeof(struct log2mem_data_hdr)+DATALEN;             \
    PTR += sizeof(struct log2mem_data_hdr);                            \
  } while (0);

/*
Round up v so to the smallest number n = 2^m which is >= v

Eg.

0 --> 0
1 --> 1
2 --> 2
3 --> 4
4 --> 4
5 --> 8

http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2

*/

inline unsigned int round_up_next_power_of_2_uint(unsigned int v)
{
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v++;

  return v;
}

static
int __log2mem_valid()
{
  return (g_log2mem != 0);
}

static
void __log2mem_error(int errcode, const char* errmsg)
{
  g_log2mem_errno = errcode;
  // TODO
}

#define MEMLOG_ERROR_OK 0
#define MEMLOG_ERROR_INCOMPATIBLE_ARCH 1

const char* log2mem_strerr()
{
  switch(g_log2mem_errno)
  {
    case MEMLOG_ERROR_OK:return "no error";
    case MEMLOG_ERROR_INCOMPATIBLE_ARCH:
      return "memmap 32/64 bits architecture differs to program";
    default: return "unknown error";
  };
}

//----------------------------------------------------------------------

void log2mem_calculate_layout(const struct log2mem_config * cfg,
                           struct log2mem_layout * layout)
{
  layout->counters_off = MEMLOG_MM_HEADER_SIZE;

  /* place at 8 byte boundary */
  layout->rows_off     = layout->counters_off +
    ((cfg->num_counters * sizeof(struct log2mem_counter) + 8 -1) & ~(8-1));

  /* place at 8 byte boundary */
  layout->counter_labels_off = layout->rows_off +
    ((cfg->num_rows * calc_row_total_size(cfg) + 8-1) & ~(8-1));

  layout->double_vars_off = layout->counter_labels_off +
    ((cfg->num_counters * sizeof(struct log2mem_label) + 8 -1) & ~(8-1));

  layout->int_vars_off   = layout->double_vars_off +
    ((cfg->num_vars * sizeof(struct log2mem_var_double) + 8 -1) & ~(8-1));

  /* determine size based on the highest offset */
  layout->size = layout->int_vars_off +
    cfg->num_vars * sizeof(struct log2mem_var_int);


  /* we want the full memmap size to be a multiple of the system page size */
  const int ps = getpagesize();
  layout->size  = (layout->size + ps - 1) & ~(ps - 1);
}


//----------------------------------------------------------------------
static
void __log2mem_calculate_section_pointers(void * mem,
                                       struct log2mem_handle * handle)
{
  struct log2mem_mm_header * mmhdr = mem;

  struct log2mem_layout layout;
  log2mem_calculate_layout(&(mmhdr->config), &layout);

  memset((void*)handle, 0, sizeof(struct log2mem_handle));  // TODO: pass handle size in
  handle->mem    = mem;
  handle->header = mem;
  handle->counters = mem+layout.counters_off;
  handle->rows = mem+layout.rows_off;
  handle->counter_labels = mem+layout.counter_labels_off;
  handle->double_vars = mem+layout.double_vars_off;
  handle->int_vars    = mem+layout.int_vars_off;
}

//----------------------------------------------------------------------

/* Attach to an already existing memmap */
struct log2mem_handle * log2mem_attach (const char* filename)
{
  /* to access memmap, both the filehandle and mmap permissions must be set
   * appropriately */

  int readonly = 0;
  int fdperms = (readonly)? O_RDONLY  : O_RDWR;
  int mmperms = (readonly)? PROT_READ : PROT_READ | PROT_WRITE;

  /* Open the file.  Note: we do not care about the current contents of the
   * file, hence we provide the TRUNC flag. */
  int fd = open(filename, fdperms);

  struct stat filestat;
  if (fstat( fd, &filestat) < 0)
  {
    // TODO: handle err
    return 0;
  }

  /* mmap - map the file into memory */
  void * addr = mmap(0, filestat.st_size, mmperms, MAP_SHARED, fd, 0);

  if( MAP_FAILED == addr)
  {
    int __errno = errno;
    printf("mmap failed, error %i\n", __errno);
    return 0;

    // TODO: cleanup();
  }

  // read from memory
  size_t i;
  const char* chptr = (char*) addr;
  for (i=0; i < filestat.st_size; i++, chptr++)
    g_log2mem_dummytotal += *chptr;


  /* set up our pointers the the mm sections */

  g_log2mem = malloc(sizeof(struct log2mem_handle));
  __log2mem_calculate_section_pointers(addr, g_log2mem);

  if (!g_log2mem) return 0;

  /* check map architecture */
  int myarch = (sizeof(long) * 8);
  if (myarch != g_log2mem->header->arch)
  {
    __log2mem_error(MEMLOG_ERROR_INCOMPATIBLE_ARCH, "incompatible architecture");
    g_log2mem = NULL;
    return g_log2mem;
  }

  // TODO: perform a size check .. ie, the size of the memmap should match
  // thelayout

  return g_log2mem;
}

//----------------------------------------------------------------------

/*
Create and configure an instance of log2mem_handle, backing it with a
memory-mapped file.
*/

static
struct log2mem_handle * __log2mem_create(const char* fn,
                                       struct log2mem_config config)
{
  struct log2mem_handle * ptr = NULL;

  ptr = malloc(sizeof(struct log2mem_handle));
  memset((void*)ptr, 0, sizeof(struct log2mem_handle));

  /* copy or create a filename */
  if (fn)
    ptr->filename = strdup(fn);
  else
    ptr->filename = __default_filename();

  // round up to power-of-2
  config.num_counters = round_up_next_power_of_2_uint(config.num_counters);
  config.num_counters += (config.num_counters == 0);
  config.num_rows = round_up_next_power_of_2_uint(config.num_rows);
  config.num_rows += (config.num_rows == 0);
  config.num_vars = round_up_next_power_of_2_uint(config.num_vars);

  /* determine the memmap layout */
  struct log2mem_layout layout;
  log2mem_calculate_layout(&config, &layout);

  /* obtain filename */

  /* Open the file.  Note: we do not care about the current contents of the
   * file, hence we provide the TRUNC flag. */
  int fd = open(ptr->filename, O_RDWR | O_CREAT, 0640);

  /* Enlarge the file to the length required for our data, and also that it
   * has a size which is a multiple of the system page size */
  if (ftruncate(fd, layout.size) != 0)
  {
    int __errno = errno;
    printf("ftruncate failed, error %i\n", __errno);

    // TODO: need a strategy for dealing with log2mem errors
    exit(1); // TODO: remove this
    /* TODO: check for error */
  }

  /* TODO: fstat the file, to check its size */


  /* mmap - map the file into memory */
  ptr->mem = mmap(0, layout.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

  if( MAP_FAILED == ptr->mem)
  {
    // TODO: need to get system error string here.
    int __errno = errno;
    printf("mmap failed, error %i\n", __errno);
    exit(1); // TODO: remove this
  /*   __errno = errno; */
  /*   if (m_logsvc and m_logsvc->want_error()) */
  /*   { */
  /*     std::ostringstream os; */
  /*     os << "Failed to perform initial mmap. errno=" << __errno; */
  /*     m_logsvc->error( os.str(), true ); */
  /*   } */
  /*   cleanup(); */
  /*   return -1; */
  }

  memset(ptr->mem, 0, layout.size);

  /* TODO: there is a function for calculating the pointers based on offsets
     ... why is that not being used here?  */

  /* set up our pointers the the mm sections */
  ptr->header = (struct log2mem_mm_header*) ptr->mem;
  ptr->counters = (struct log2mem_counter*) (ptr->mem + layout.counters_off);
  ptr->rows = (struct log2mem_row*) (ptr->mem + layout.rows_off);
  ptr->counter_labels = (struct log2mem_label*) (ptr->mem + layout.counter_labels_off);
  ptr->double_vars = (struct log2mem_var_double*) (ptr->mem + layout.double_vars_off);
  ptr->int_vars = (struct log2mem_var_int*) (ptr->mem + layout.int_vars_off);

  /* populate the memmap */

  snprintf(ptr->header->desc, MEMLOG_DESC_LEN,
           "log2mem memmap version %d", MEMLOG_VERSION);
  ptr->header->desc[MEMLOG_DESC_LEN-1] = '\0';

  ptr->header->version = MEMLOG_VERSION;

  long int endian = 1; char *p = (char *) &endian;

  ptr->header->flags = p[0]? 0x1 : 0;
  ptr->header->arch  = (sizeof(long) * 8);

  ptr->header->config = config;
  ptr->header->nextrow = 0;

  time_t clock = time(0);
  struct tm _tm;
  localtime_r(&clock, &_tm);
  memset(ptr->header->datecreated, 0, MEMLOG_TIMESTR_LEN);
  snprintf(ptr->header->datecreated, MEMLOG_TIMESTR_LEN-1,
           "%04i/%02d/%02d %02i:%02i:%02i",
           _tm.tm_year+1900, _tm.tm_mon+1, _tm.tm_mday,
           _tm.tm_hour, _tm.tm_min, _tm.tm_sec);
  ptr->header->datecreated[MEMLOG_TIMESTR_LEN-1] = '\0';

  return ptr;
}

//----------------------------------------------------------------------
void log2mem_init(const char* filename,
               const struct log2mem_config* userconfig,
               int configsize)
{
  /* copy the user config to a local object, so that we can deal with a client
   * applicaiton that might be compiled to an older version of the config
   * struct.
   */
  struct log2mem_config config;
  memset(&config, 0, sizeof(struct log2mem_config));
  memcpy(&config, userconfig, configsize);

  /* double checked locking */
  if (g_log2mem == 0)
  {
    /* TODO: review, does this really need to be a spinlock?  I think a
       pthread would be okay here?  */
    log2mem_atomic_spinlock(&g_log2mem_lock);

    if (g_log2mem == 0)
    {
      g_log2mem = __log2mem_create(filename, config);
    }

    log2mem_atomic_spinunlock(&g_log2mem_lock);
  }

}

//----------------------------------------------------------------------
void  log2mem_counter_incr(int counterid)
{
  log2mem_counter_add(counterid, 1);
}

//----------------------------------------------------------------------
void log2mem_counter_add(int counterid, int valueToAdd)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return;

  int idx = counterid & (g_log2mem->header->config.num_counters-1);
  struct log2mem_counter* counter = &g_log2mem->counters[idx];
  log2mem_atomic_incr_fetch_int( (int*) &(counter->value.v) , valueToAdd);
}

//----------------------------------------------------------------------
void log2mem_counter_label(int counterid, const char* src)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return;

  int idx = counterid & (g_log2mem->header->config.num_counters-1);
  strncpy(g_log2mem->counter_labels[idx].text, src, MEMLOG_LABEL_LEN-1);
  g_log2mem->counters[idx].used = 1;
}

//----------------------------------------------------------------------

/*
 * NOTE: this push function must not be used for pushing c-string.
 */
static inline
int __log2mem_push(struct log2mem_row* row,
                 int datatype,
                 int datalen,
                 const void* data)
{
  /* g_log2mem should already be initialised by now */
  const int remain = g_log2mem->header->config.row_len - row->datalen;
  const int totlen = sizeof(struct log2mem_data_hdr) + datalen;

  if (totlen <= remain)
  {
    char* ptr = (char*) log2mem_row_payload(row) + row->datalen;
    MEMLOG_PUT_HDR(ptr, datatype, datalen);

    memcpy(ptr, data, datalen);

    row->datalen += totlen;
  }
}

static inline
int __log2mem_push_str(struct log2mem_row* row,
                    int datatype,
                    int len,
                    const void* str)
{
  /* g_log2mem should already be initialised by now */
  const int remain = g_log2mem->header->config.row_len - row->datalen;
  const int totlen = sizeof(struct log2mem_data_hdr) + (len+1);

  if (totlen <= remain)
  {
    char* ptr = (char*) log2mem_row_payload(row) + row->datalen;
    MEMLOG_PUT_HDR(ptr, datatype, (len+1));

    memcpy(ptr, str, len);
    ptr+=len;
    *ptr='\0'; ptr++; /* push a null char */

    row->datalen += totlen;
  }
}
//----------------------------------------------------------------------
log2mem_ts log2mem_time()
{
  MEMLOG_PREPARE_PTR;
  if (__log2mem_valid())
  {
    /* get time */
    switch(g_log2mem->header->config.options & MEMLOG_CLOCKSOURCE_MASK)
    {
      case MEMLOG_RDTSC : {
        return read_rdtsc();
        break;
      }
      case MEMLOG_GETTIMEOFDAY : {
        struct timeval tp;
        gettimeofday(&tp, 0);
        // normalise to nanosec
        return (tp.tv_sec << MEMLOG_BITS_NANO) + (tp.tv_usec*1000);
        break;
      };
      case  MEMLOG_CLOCKGETTIME : {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        return (ts.tv_sec << MEMLOG_BITS_NANO) + ts.tv_nsec;
        break;
      };
      default: return 0;
    }
  }
}
//----------------------------------------------------------------------
static
struct log2mem_row * __log2mem_init_row(const char* filename, int lineno)
{
  MEMLOG_PREPARE_PTR;
  if (__log2mem_valid())
  {
    /* obtain a row id */
    unsigned int rowid = log2mem_atomic_incr_fetch_uint(
      (unsigned int*) &g_log2mem->header->nextrow,1)-1;

    const int rowidx = rowid & (g_log2mem->header->config.num_rows-1);

    /* obtain a row pointer */
    const int row_total_size = calc_row_total_size(&g_log2mem->header->config);
    struct log2mem_row * r = (struct log2mem_row *) ((char*)g_log2mem->rows
                                             + (rowidx * row_total_size));

    if (g_log2mem->header->config.options & MEMLOG_THREADID)
      r->tid = syscall(SYS_gettid);

    /* get time */
    switch(g_log2mem->header->config.options & MEMLOG_CLOCKSOURCE_MASK)
    {
      case MEMLOG_RDTSC : {
        r->ts = read_rdtsc();
        break;
      }
      case MEMLOG_GETTIMEOFDAY : {
        struct timeval tp;
        gettimeofday(&tp, 0);
        // normalise to nanosec
        r->ts = (tp.tv_sec << MEMLOG_BITS_NANO) + (tp.tv_usec*1000);
        break;
      };
      case  MEMLOG_CLOCKGETTIME : {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        r->ts = (ts.tv_sec << MEMLOG_BITS_NANO) + ts.tv_nsec;
        break;
      };
    }

    r->id       = rowid;
    r->datalen  = 0;

    if ((g_log2mem->header->config.options & MEMLOG_FILELINE) && filename)
    {
      __log2mem_push_str(r, MEMLOG_DATATYPE_FILENAME, strlen(filename), filename);
      __log2mem_push(r, MEMLOG_DATATYPE_LINENO, sizeof(lineno), &lineno);
    }

    return r;
  }
  else
  {
    return 0;
  }

}

//----------------------------------------------------------------------
void* log2mem_write(const char* str, const char* filename, int lineno)
{
  struct log2mem_row * r = __log2mem_init_row(filename, lineno);
  if (r && str)
  {
    __log2mem_push_str(r, MEMLOG_DATATYPE_STRING, strlen(str), str);
  }

  return r;
}

//----------------------------------------------------------------------
void* log2mem_get_row(const char* filename, int lineno)
{
  return __log2mem_init_row(filename, lineno);
}
//----------------------------------------------------------------------
void log2mem_append_uint(void* r, unsigned int d)
{
  if (r)
  {
    __log2mem_push(r, MEMLOG_DATATYPE_UINT, sizeof(d), &d);
  }
}

//----------------------------------------------------------------------
void log2mem_append_int(void* r, int d)
{
  if (r)
  {
    __log2mem_push(r, MEMLOG_DATATYPE_INT, sizeof(d), &d);
  }
}

//----------------------------------------------------------------------
void log2mem_append_str3(void* r, const char* str, int len)
{
  if (r && str)
  {
    __log2mem_push_str(r, MEMLOG_DATATYPE_STRING, len, str);
  }
}

//----------------------------------------------------------------------
void log2mem_append_str2(void* r, const char* str)
{
  if (r && str)
  {
    __log2mem_push_str(r, MEMLOG_DATATYPE_STRING, strlen(str), str);
  }
}

//----------------------------------------------------------------------
void log2mem_append_ulong(void* r, unsigned long d)
{
  if (r)
  {
    __log2mem_push(r, MEMLOG_DATATYPE_ULONG, sizeof(d), &d);
  }
}

//----------------------------------------------------------------------
void log2mem_append_long(void* r, long d)
{
  if (r)
  {
    __log2mem_push(r, MEMLOG_DATATYPE_LONG, sizeof(d), &d);
  }
}
//----------------------------------------------------------------------
void log2mem_append_float(void* r, float d)
{
  if (r)
  {
    __log2mem_push(r, MEMLOG_DATATYPE_FLOAT, sizeof(d), &d);
  }
}

//----------------------------------------------------------------------
void log2mem_append_double(void* r, double d)
{
  if (r)
  {
    __log2mem_push(r, MEMLOG_DATATYPE_DOUBLE, sizeof(d), &d);
  }
}

//----------------------------------------------------------------------
void log2mem_append_char(void* r, char d)
{
  if (r)
  {
    __log2mem_push(r, MEMLOG_DATATYPE_CHAR, sizeof(d), &d);
  }
}

//----------------------------------------------------------------------
void* log2mem_time_event4(int flowid, int msgref, const char* str, int len)
{
  struct log2mem_row * row = __log2mem_init_row(0, 0);

  if (row)
  {
    int len_needed = 2*sizeof(struct log2mem_data_hdr)
      + 2* sizeof(int);

    if (str && len>0)
      len_needed += sizeof(struct log2mem_data_hdr) + (len+1);

    const int avail = g_log2mem->header->config.row_len - row->datalen;

    if (len_needed <= avail)
    {
      const int row_datalen = 0; /* there is no data already in the row */
      char* ptr = (char*) (log2mem_row_payload(row) + row_datalen);
      int* intptr;

      // push record
      MEMLOG_PUT_HDR(ptr, MEMLOG_DATATYPE_INT, sizeof(flowid));
      intptr = (int*) ptr;
      *intptr =  flowid;
      ptr += sizeof(flowid);

      // push record
      MEMLOG_PUT_HDR(ptr, MEMLOG_DATATYPE_INT, sizeof(msgref));
      intptr = (int*) ptr;
      *intptr =  msgref;
      ptr += sizeof(msgref);

      // push record
      if (str && len>0)
      {
        MEMLOG_PUT_HDR(ptr, MEMLOG_DATATYPE_STRING, (len+1));

        memcpy(ptr, str, len);
        ptr+=len;
        *ptr='\0'; ptr++; /* push a null char */
      }

      row->datalen += len_needed;
    }
  }

  return row;
}
//----------------------------------------------------------------------

void* log2mem_time_event2(int flowid, int msgid)
{
  return log2mem_time_event4(flowid, msgid, 0, 0);
}
void* log2mem_time_event3(int flowid, int msgid, const char* msgstr)
{
  return log2mem_time_event4(flowid, msgid, msgstr, strlen(msgstr));
}

void* log2mem_set_ts(void* r, log2mem_ts ts)
{
  if (r)
  {
    struct log2mem_row * row = (struct log2mem_row *)r;
    row->ts = ts;
  }
}

//----------------------------------------------------------------------
static void __default_log2mem_init()
{
  struct log2mem_config cfg;

  memset(&cfg, 0, sizeof(cfg));

  cfg.options      = MEMLOG_FILELINE|MEMLOG_THREADID;
  cfg.num_counters = 16;
  cfg.num_rows     = 1<<16;
  cfg.row_len      = 256;
  cfg.num_vars     = 10;

  // TODO: also provide a pattern, eg, MEMLOG_FILEPATTERN, support things like
  // %P for pid etc.,  %H etc.

  char * s = getenv("MEMLOG_FILENAME");
  log2mem_init(s, &cfg, sizeof(struct log2mem_config));
}

//----------------------------------------------------------------------

static char* __build_user_id()
{
  size_t buflen = 256;
  char* buf     = malloc( buflen );
  memset(buf, 0, sizeof(buf));

  // try, using getlogin_r
  if (getlogin_r(buf, buflen) != 0)
  {
    buf[0]='\0';
  }

  // or, try cuserid
  if (buf[0]=='\0')
  {
    char buf_cuserid[1024*2];
    memset(buf_cuserid, 0, sizeof(buf_cuserid));
    cuserid(buf);
    if (buf_cuserid[0] != '\0')
    {
      // TODO: u tback in
//      memcpy(username, buf_cuserid, std::min(sizeof(username), sizeof(buf_cuserid)));
    }
  }

  // or, try LOGNAME
  if (buf[0]=='\0')
  {
    char* s = getenv("LOGNAME");
    if (s) strncpy(buf, s, buflen);
  }

  // or, try USER
  if (buf[0]=='\0')
  {
    char* s = getenv("USER");
    if (s) strncpy(buf, s, buflen);
  }

  buf[buflen-1] = '\0';

  return buf;
}


//----------------------------------------------------------------------
static char* __default_filename()
{
  int    buflen= 1024*4;
  char * buf = malloc(buflen);
  memset(buf, 0, buflen);

  char * username = __build_user_id();

  snprintf(buf, buflen, "/var/tmp/log2mem-%s.dat", username);

  free(username);

  return buf;
}

//----------------------------------------------------------------------
void log2mem_var_double_init(unsigned int id, double value, const char* src)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return;

  int idx = id & (g_log2mem->header->config.num_vars-1);

  if (g_log2mem->double_vars[idx].flags & log2mem_eInitialised) return;

  g_log2mem->double_vars[idx].value = value;
  g_log2mem->double_vars[idx].flags |= (log2mem_eUsed | log2mem_eInitialised);

  if (src)
  {
    strncpy(g_log2mem->double_vars[idx].text, src, MEMLOG_LABEL_LEN-1);
    g_log2mem->double_vars[idx].text[MEMLOG_LABEL_LEN-1] = '\0';
  }
}

//----------------------------------------------------------------------

void log2mem_var_double_set(unsigned int id, double d)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return;

  int idx = id & (g_log2mem->header->config.num_vars-1);
  g_log2mem->double_vars[idx].value = d;
  g_log2mem->double_vars[idx].flags |= log2mem_eUsed;
}

//----------------------------------------------------------------------

double log2mem_var_double_get(unsigned int id)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return 0.0;

  int idx = id & (g_log2mem->header->config.num_vars-1);
  return g_log2mem->double_vars[idx].value;
}

//----------------------------------------------------------------------

int log2mem_var_double_used(unsigned int id)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return 0;

  int idx = id & (g_log2mem->header->config.num_vars-1);
  return g_log2mem->double_vars[idx].flags & log2mem_eUsed;
}

//----------------------------------------------------------------------

void log2mem_var_int_init(unsigned int id, int value, const char* src)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return;

  int idx = id & (g_log2mem->header->config.num_vars-1);

  if (g_log2mem->int_vars[idx].flags & log2mem_eInitialised) return;

  g_log2mem->int_vars[idx].value = value;
  g_log2mem->int_vars[idx].flags |= (log2mem_eUsed | log2mem_eInitialised);

  if (src)
  {
    strncpy(g_log2mem->int_vars[idx].text, src, MEMLOG_LABEL_LEN-1);
    g_log2mem->int_vars[idx].text[MEMLOG_LABEL_LEN-1] = '\0';
  }
}
//----------------------------------------------------------------------

void log2mem_var_int_set(unsigned int id, int d)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return;

  int idx = id & (g_log2mem->header->config.num_vars-1);
  g_log2mem->int_vars[idx].value = d;
  g_log2mem->int_vars[idx].flags |= log2mem_eUsed;
}

//----------------------------------------------------------------------

int log2mem_var_int_get(unsigned int id)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return 0.0;

  int idx = id & (g_log2mem->header->config.num_vars-1);
  return g_log2mem->int_vars[idx].value;
}

//----------------------------------------------------------------------

int log2mem_var_int_used(unsigned int id)
{
  MEMLOG_PREPARE_PTR;
  if (!__log2mem_valid()) return 0;

  int idx = id & (g_log2mem->header->config.num_vars-1);
  return g_log2mem->int_vars[idx].flags & log2mem_eUsed;
}


//----------------------------------------------------------------------

int log2mem_valid()
{
  MEMLOG_PREPARE_PTR;
  if (__log2mem_valid())
    return 1;
  else
    return 0;
}


//----------------------------------------------------------------------

const char* log2mem_filename()
{
  MEMLOG_PREPARE_PTR;
  if (__log2mem_valid())
  {
    return g_log2mem->filename;
  }
  else
    return 0;
}


//----------------------------------------------------------------------



void log2mem_ts_to_timespec(log2mem_ts ts, struct timespec* tspec)
{
  // used for CLOCKGETTIME
  tspec->tv_sec  = (ts >> MEMLOG_BITS_NANO) & MEMLOG_MASK_SECS;
  tspec->tv_nsec = (ts & MEMLOG_MASK_NANO);
}

//----------------------------------------------------------------------

void log2mem_ts_to_timeval(log2mem_ts ts,struct timeval* tval)
{
  // used for GETTIMEOFDAY
  tval->tv_sec  = (ts >> MEMLOG_BITS_NANO) & MEMLOG_MASK_SECS;
  tval->tv_usec = (ts & MEMLOG_MASK_NANO) / 1000;
}

//----------------------------------------------------------------------
