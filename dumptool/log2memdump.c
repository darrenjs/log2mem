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
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

const char* g_delim = " ";
int   g_showdelim = 1;
int   g_terse = 0;

/* struct set_var_double */
/* { */
/*     char type; */
/*     unsigned int idx; */
/*     double value; */
/* }; */

/* struct node */
/* { */
/*     void * data; */
/*     struct node * next; */
/*     struct node * prev; */
/* }; */


/* struct nodelist */
/* { */
/*     struct node* begin; */
/*     struct node* end; /\* immutable *\/ */
/* }; */


/* void list_init(struct nodelist * l) */
/* { */
/*   l->begin = (struct node*) malloc(sizeof(struct node)); */
/*   l->end   = l->begin; */

/*   memset(l->end, 0, sizeof(struct node)); */
/*   l->end->next = l->end; */
/*   l->end->prev = l->prev; */
/* } */

/* void list_push_back(struct nodelist * nl, void* data) */
/* { */
/*   // create new node */
/*   struct node * nodeptr = (struct node*) malloc(sizeof(struct node)); */
/*   memset(nodeptr, 0, sizeof(struct node)); */
/*   nodeptr->data = data; */

/*   // insert into list */
/*   nodeptr->next = l->end; */
/*   nodeptr->prev = end->prev; */
/* } */

/* struct nodelist g_list; */



#undef  PFSIZET
#if defined __x86_64__ || defined __LP64__
#define PFSIZET "%lu"
#else
#define PFSIZET "%u"
#endif

#define OPT_DOUBLE "--double"
#define OPT_STRING "--string"
#define OPT_INT "--int"

//----------------------------------------------------------------------
void help()
{
  printf("usage: log2mem-dump [MEMMAP-FILE] [OPTIONS]\n");
  printf("\n");
  printf("Options\n");
  printf("  -s\t\t\tshow memmap summary\n");
  printf("  -c\t\t\tshow counters\n");
  printf("  -r\t\t\tshow rows\n");
  printf("  -v\t\t\tshow variables\n");
  printf("  -a\t\t\tshow all data sections\n");
  printf("  -F DELIM \t\tuse DELIM as output delimiter for data values\n");
  printf("  -z\t\t\tsuppress delimiter\n");
  printf("  -t\t\t\tterse, only print data values\n");
  printf("  " OPT_DOUBLE "[I]=N\t\tset var double[I] to value N\n");
  printf("  " OPT_INT "[I]=N\t\tset var int[I] to value N\n");
  printf("  --NAME=N\t\tset var with name NAME to value N\n");
  printf("  --version\t\tversion info\n");
  exit(0);
}
//----------------------------------------------------------------------
void die(const char* msg, const char * msg2)
{
  if (msg2==0) msg2="";
  printf("%s%s\n", msg,msg2);
  exit(1);
}

//----------------------------------------------------------------------
void print_summary(struct log2mem_handle * handle)
{
  if (!handle) return;

  struct log2mem_layout layout;
  log2mem_calculate_layout(&handle->header->config, &layout);

  printf("Memmap configuration\n--------------------\n");

  printf("created: %s\n", handle->header->datecreated);
  printf("options: ");
  const char* delim="";
  if (handle->header->config.options & MEMLOG_FILELINE)
  {
    printf("%s%s", delim, "MEMLOG_FILE_LINE");
    delim = ",";
  }
  if (handle->header->config.options & MEMLOG_THREADID)
  {
    printf("%s%s", delim, "MEMLOG_THREADID");
    delim = ",";
  }

  /* time */
  switch(handle->header->config.options & MEMLOG_CLOCKSOURCE_MASK)
  {
    case MEMLOG_RDTSC : {
      printf("%s%s", delim, "MEMLOG_RDTSC");
      delim = ",";
      break;
    }
    case MEMLOG_GETTIMEOFDAY : {
      printf("%s%s", delim, "MEMLOG_GETTIMEOFDAY");
      delim = ",";
      break;
    };
    case  MEMLOG_CLOCKGETTIME : {
      printf("%s%s", delim, "MEMLOG_CLOCKGETTIME");
      delim = ",";
      break;
    };
  };

  printf("\n");

  printf("number of counters: %d\n", handle->header->config.num_counters);
  printf("number of rows: %d\n", handle->header->config.num_rows);
  printf("number of vars %d\n", handle->header->config.num_vars);
  printf("row data size: %d\n", handle->header->config.row_len);
  printf("row total size: " PFSIZET "\n",
         calc_row_total_size(&handle->header->config));
  printf("next rowid: %d\n", handle->header->nextrow);

  printf("\nMemmap architecture\n-------------------\n");

  printf("version: %d\n", handle->header->version);
  printf("arch: %d\n", handle->header->arch);
  printf("endian: %s\n", ((handle->header->flags & 0x1)? "little":"big"));

  printf("\nSection offsets & size\n----------------------\n");
  printf("header: %u\n", 0);
  printf("counters: %u\n", layout.counters_off);
  printf("rows: %u\n", layout.rows_off);
  printf("labels: %u\n", layout.counter_labels_off);
  printf("double_vars: %u\n", layout.double_vars_off);
  printf("int_vars: %u\n", layout.int_vars_off);
  printf("memmap size: %u\n", layout.size);

  printf("\nFixed structure sizes\n---------------------\n");
  printf("MEMLOG_MM_HEADER_SIZE: %d\n", MEMLOG_MM_HEADER_SIZE);
  printf("MEMLOG_LABEL_LEN: %d\n", MEMLOG_LABEL_LEN);
  printf("sizeof(log2mem_mm_header): " PFSIZET  "\n", sizeof(struct log2mem_mm_header));
  printf("sizeof(log2mem_data_hdr): " PFSIZET "\n", sizeof(struct log2mem_data_hdr));
  printf("sizeof(log2mem_row): " PFSIZET "\n", sizeof(struct log2mem_row));
  printf("sizeof(log2mem_counter): " PFSIZET  "\n", sizeof(struct log2mem_counter));
  printf("sizeof(log2mem_var_double): " PFSIZET "\n", sizeof(struct log2mem_var_double));
  printf("sizeof(log2mem_var_int): " PFSIZET "\n", sizeof(struct log2mem_var_int));
}
//----------------------------------------------------------------------
void print_counters(struct log2mem_handle * handle)
{
  if (!handle) return;

  char label[ MEMLOG_LABEL_LEN +1];

  struct log2mem_counter * c = handle->counters;
  struct log2mem_label * l = handle->counter_labels;

  const int num_counters = handle->header->config.num_counters;
  int i;
  for (i=0; i < num_counters; ++i, ++c, ++l)
  {
    if (c->used)
    {
      // reset the label
      memset(label, 0, sizeof(label));
      strcpy(label, "noname");
      if (l) strncpy(label, l->text, MEMLOG_LABEL_LEN);

      printf("counter[%d] %s : %d\n", i, label, c->value.v);
    }
  }

}

//----------------------------------------------------------------------
void print_vars(struct log2mem_handle * handle)
{
  if (!handle) return;

  struct log2mem_var_double * var_double = handle->double_vars;

  int i;
  for (i=0; i < handle->header->config.num_vars; ++i, ++var_double)
  {
    if (var_double->flags & log2mem_eUsed)
    {
      printf("double[%d] %s%s: %f\n", i, var_double->text,
             (strlen(var_double->text)? " ": ""), var_double->value);
    }
  }

  struct log2mem_var_int * var_int = handle->int_vars;
  for (i=0; i < handle->header->config.num_vars; ++i, ++var_int)
  {
    if (var_int->flags & log2mem_eUsed)
    {
      printf("int[%d] %s%s: %i\n", i, var_int->text,
             (strlen(var_int->text)? " ": ""), var_int->value);
    }
  }

}
//----------------------------------------------------------------------
void print_rows(struct log2mem_handle * handle)
{
  if (!handle) return;

  const int num_rows = handle->header->config.num_rows;
  const int row_total_size = calc_row_total_size(&handle->header->config);
  char* rowstart = (char*) handle->rows;
  unsigned int i;

  /* scan all the rows, looking for the lowest rowid */
  unsigned int lowest_rowid  = ~0;
  for (i = 0; i < num_rows; ++i)
  {
    struct log2mem_row * r = (struct log2mem_row *)
      (rowstart+i*row_total_size);

    if (r->id <= lowest_rowid) lowest_rowid = r->id;
  }

  /* print all rows, starting at the row with lowest rowid */
  unsigned int next_rowid = lowest_rowid;
  for (i = 0; i < num_rows; ++i, ++next_rowid)
  {
    unsigned int rowidx = next_rowid & (handle->header->config.num_rows-1);
    struct log2mem_row * r = (struct log2mem_row *)
      (rowstart+rowidx*row_total_size);

    if (r->datalen == 0) continue;


    if (!g_terse)
    {
      printf("%i ",  r->id);


      switch(handle->header->config.options & MEMLOG_CLOCKSOURCE_MASK)
      {
        case MEMLOG_RDTSC : {
          // TODO: how to convert from tsc to cpu resolution?
          uint64_t tsc = r->ts;
          printf("%lu ", tsc);
          break;
        }
        case MEMLOG_GETTIMEOFDAY : {
          struct timeval __tval;
          log2mem_ts_to_timeval(r->ts, &__tval);

          struct tm _tm;
          localtime_r(&__tval.tv_sec, &_tm);

          printf("%02d:%02d:%02d.%06lu ", _tm.tm_hour, _tm.tm_min, _tm.tm_sec,
                 __tval.tv_usec);
          break;
        };
        case  MEMLOG_CLOCKGETTIME : {
          struct timespec __timespec;
          log2mem_ts_to_timespec(r->ts, &__timespec);

          struct tm _tm;
          localtime_r(&__timespec.tv_sec, &_tm);

          printf("%02d:%02d:%02d.%09lu ", _tm.tm_hour, _tm.tm_min, _tm.tm_sec,
                 __timespec.tv_nsec);
          break;
        };
      };

      if (handle->header->config.options & MEMLOG_THREADID)
        printf("[%i] ", r->tid);
    }

    int i = 0;
    int wantdelim = 0;

    const char* rowdata = log2mem_row_payload(r);
    while (i < r->datalen)
    {
      struct log2mem_data_hdr * hdr = (struct log2mem_data_hdr *) (rowdata+i);
      const char* data = rowdata + i + sizeof(struct log2mem_data_hdr);

      // printf("[type %d, datalen %d]", hdr->type, hdr->len);

      if (g_showdelim && wantdelim)
        printf("%s", g_delim);
      else
        wantdelim=1;

      // TODO: only show this for debug runs
      //printf("[hdr type=%d, len=%d] ", hdr->type, hdr->len);

      switch (hdr->type)
      {
        case MEMLOG_DATATYPE_FILENAME:
        {
          if (!g_terse)
          {
            const char * s = (const char *) data;
            printf("(%s:", s);
          }
          wantdelim=0; /* suppress next delimiter */
          break;
        }
        case MEMLOG_DATATYPE_LINENO:
        {
          if (!g_terse)
          {
            const int * d = (const int *) data;
            printf("%d) ", *d);
          }
          wantdelim=0; /* suppress next delimiter */
          break;
        }
        case MEMLOG_DATATYPE_STRING:
        {
          const char * s = (const char *) data;
          printf("%s", s);
          break;
        }
        case MEMLOG_DATATYPE_UINT:
        {
          const unsigned int * d = (const unsigned int *) data;
          printf("%u", *d);
          break;
        }
        case MEMLOG_DATATYPE_INT:
        {
          const int * d = (const int *) data;
          printf("%d", *d);
          break;
        }
        case MEMLOG_DATATYPE_ULONG:
        {
          const unsigned long * d = (const unsigned long *) data;
          printf("%lu", *d);
          break;
        }
        case MEMLOG_DATATYPE_LONG:
        {
          const long * d = (const long *) data;
          printf("%ld", *d);
          break;
        }

        case MEMLOG_DATATYPE_FLOAT:
        {
          const float * d = (const float *) data;
          printf("%f", *d);
          break;
        }
        case MEMLOG_DATATYPE_DOUBLE:
        {
          const double * d = (const double *) data;
          printf("%f", *d);
          break;
        }
        case MEMLOG_DATATYPE_CHAR:
        {
          const char * d = (const char *) data;
          printf("%c", *d);
          break;
        }
      }
      i += hdr->len;
    }



    printf("\n");
  }


}

//----------------------------------------------------------------------

/*
  argc,argv : command line args
  argi      : index of next arg, can be modified by this function
  idx,value : var index & value
*/
void process_set_var_arg(int argc, const char** argv, int * argi,
                         unsigned int* idx, const char** value,
                         const char* optionprefix)
{
  const char* firstarg = argv[*argi];

  *idx = 0;
  const char * ptr = firstarg+strlen(optionprefix);

  if (*ptr == '[') ptr++; /* optional bracket */

  while ( *ptr && (*ptr >= '0') && (*ptr <= '9') )
  {
    *idx = (10 * *idx) + (*ptr - '0');
    ptr++;
  }

  if (*ptr == ']') ptr++; /* optional bracket */

  if (*ptr == '=')
  {
    ptr++;
  }
  else if (*ptr != '\0')
  {
    die("expected numeric digits, ", firstarg);
  }
  else
  {
    if (++*argi >= argc) die("missing argument to ", firstarg);
    ptr = argv[*argi];
  }

  *value = ptr;
}

//----------------------------------------------------------------------
void set_var_string(int argc, const char** argv, int * i,
                    struct log2mem_handle * handle)
{
  unsigned int idx = 0;
  const char * ptr = 0;
  process_set_var_arg(argc, argv, i, &idx, &ptr, OPT_STRING);

  if (!handle) return;
  /*
  double val;
  sscanf(ptr, "%lf", &val);
  printf("setting double[%u]=%lf\n", idx, val);
  log2mem_var_set_double(idx, val);
  */
}

//----------------------------------------------------------------------

void set_var_double(int argc, const char** argv, int * i,
                    struct log2mem_handle * handle)
{
  unsigned int idx = 0;
  const char * ptr = 0;
  process_set_var_arg(argc, argv, i, &idx, &ptr, OPT_DOUBLE);

  if (!handle) return;

  double val;
  sscanf(ptr, "%lf", &val);
  log2mem_var_double_set(idx, val);
}

//----------------------------------------------------------------------

void set_var_int(int argc, const char** argv, int * i,
                 struct log2mem_handle * handle)
{
  unsigned int idx = 0;
  const char * ptr = 0;
  process_set_var_arg(argc, argv, i, &idx, &ptr, OPT_INT);

  if (!handle) return;

  int val;
  sscanf(ptr, "%i", &val);
  log2mem_var_int_set(idx, val);
}

//----------------------------------------------------------------------

void set_var_by_name(int argc, const char** argv, int * argi,
                     struct log2mem_handle * handle)
{
  const char* firstptr = argv[*argi];
  const char* firstarg = argv[*argi]+2; /* skip the '--' */

  char * argdup = 0;
  const char * label;
  const char* p = strchr(firstarg, '=');

  if (p)
  {
    argdup = strdup(firstarg);
    argdup[p - firstarg] = '\0';
    label=argdup;
    p++;
  }
  else
  {
    ++*argi;
    if (*argi >= argc)
      die("missing argument after ", firstptr);

    label=firstarg;
    p=argv[*argi];
  }

  /* don't proceed past argument parsing, in parse-only mode*/
  if (!handle) return;

  //printf("trying to set label[%s] to [%s]\n", label, p);

  if (label[0] == '\0') return;

  int i;
  int found=0;
  /* search doubles */
  struct log2mem_var_double * var_double = handle->double_vars;
  for (i=0; i < handle->header->config.num_vars; ++i, ++var_double)
  {
    if ((var_double->flags & log2mem_eUsed) &&
        (strcmp(label, var_double->text)==0))
    {
      sscanf(p, "%lf", &var_double->value);
      found=1;
    }
  }

  /* search ints  */
  struct log2mem_var_int * var_int = handle->int_vars;
  for (i=0; i < handle->header->config.num_vars; ++i, ++var_int)
  {
    if ((var_int->flags & log2mem_eUsed) &&
        (strcmp(label, var_int->text)==0))
    {
      sscanf(p, "%i", &var_int->value);
      found=1;
    }
  }

  if (!found) printf("warning, no variable found with name \"%s\"\n", label);

  free(argdup);
}

//----------------------------------------------------------------------
const char* parse_args(int argc, const char** argv, int parseonly,
                       struct log2mem_handle * handle)
{
  int i = 1;
  const char* filename = 0;
  int defaultshow = 1;

  enum showbits
  {
    eSummary  = 0x01,
    eCounters = 0x02,
    eRows     = 0x04,
    eVars     = 0x08
  };
  int toshow = 0;

  for (;i < argc; i++)
  {
    if ( strcmp(argv[i], "-h")==0 || strcmp(argv[i],"--help")==0 ) help();
    else if (strcmp(argv[i], "-s")==0)
    {
      toshow |= eSummary;
    }
    else if (strcmp(argv[i], "-c")==0)
    {
      toshow |= eCounters;
    }
    else if (strcmp(argv[i], "-r")==0)
    {
      toshow |= eRows;
    }
    else if (strcmp(argv[i], "-v")==0)
    {
      toshow |= eVars;
    }
    else if (strcmp(argv[i], "-a")==0)
    {
      toshow |= ~eSummary;
    }
    else if (strcmp(argv[i], "-F")==0)
    {
      if (++i >= argc) die("missing argument to -F", NULL);
      g_delim = argv[i];
      g_showdelim = 1;
    }
    else if (strcmp(argv[i], "-z")==0)
    {
      g_showdelim = 0;
    }
    else if (strcmp(argv[i], "-t")==0)
    {
      g_terse = 1;
    }
    else if (strcmp(argv[i], "--version")==0)
    {
      printf("log2mem version: " PACKAGE_VERSION "\n");
      exit(1);
    }
    else if (strncmp(argv[i], OPT_DOUBLE, strlen(OPT_DOUBLE))==0)
    {
      set_var_double(argc, argv, &i, handle);
      defaultshow = 0;
    }
    else if (strncmp(argv[i], OPT_INT, strlen(OPT_INT))==0)
    {
      set_var_int(argc, argv, &i, handle);
      defaultshow = 0;
    }
    else if (strncmp(argv[i], OPT_STRING, strlen(OPT_STRING))==0)
    {
      set_var_string(argc, argv, &i, handle);
      defaultshow = 0;
    }
    else if ((strncmp(argv[i], "--",2)==0) && strlen(argv[i])>2 )
    {
      set_var_by_name(argc, argv, &i, handle);
      defaultshow = 0;
    }
    else
    {
      if (filename == 0)
        filename = argv[i];
      else
      {
        die("unexpected argument: ", argv[i]);
      }
    }
  }

  if (defaultshow && !toshow) toshow = 0xFFFF;

  if (!parseonly)
  {
    //if (toshow & eSummary)  print_summary(handle);
    if (toshow & eRows)     print_rows(handle);
    if (toshow & eCounters) print_counters(handle);
    if (toshow & eVars)     print_vars(handle);
  }

  return filename;
}
//----------------------------------------------------------------------
int main(int argc, const char** argv)
{

  struct log2mem_handle * handle = 0;

  const char* filename = parse_args(argc, argv, 1, handle);

  if (filename == 0) die("please specify filename of log2mem memmap", 0);

  handle = log2mem_attach(filename);
  if (!handle)
  {
    printf("failed to attach to memmap file '%s', error: %s\n", filename,
           log2mem_strerr());
    return 1;
  }

  parse_args(argc, argv, 0, handle);

  return 0;
}
