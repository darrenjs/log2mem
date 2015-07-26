log2mem
=======

log2mem is a high-performance diagnostic C/C++ logger designed to serve as a
debugging tool when dealing with bugs and race conditions in multi-threaded or
low latency code.

Most developers are familer with the two basic approachs to debugging: execute
the code in a debugger; or litter the source with increasingly targeted and
detailed printf/std::cout statements.  However for multit-hreaded or low latency
code, both approaches often fail to work.

The problem is time. Whether or not a bug gets triggered can be dependent on not
just a program's inputs but also on its run-time speed and how important threads
interact with one another.  Stepping through the code with a debugger (such as
gdb) will significantly reduce execution speed.  So if a bug only arises when
its threads interact at normal speed, then slowing down the program typically
supresses the bug.  Similarly printf/std::cout statements have a cost; strings
are constructed and written to an output stream, and these too impede program
speed. The developer is left with the frustrating and not uncommon conundrum
that the act of debugging a bug causes it to temporarily disappear.

The excellent blog *Preshing on Programming* also identifies this problem of
debugging multi-threaded code, and log2mem is inspired by the solution described
there. [Lightweight In-Memory Logging, 22 May 2012,
http://preshing.com/20120522/lightweight-in-memory-logging]

Additionally for some classes of program typically characterised as low latency
or real-time, proper program execution is dependent on it running at normal
speed (e.g. software for robot contro or processing live finanical market data).
So any runtime slow introduced by debugging techniques might not even allow for
correct functionality, let alone tickling the bug into appearence.

Use of application specific asynchronous loggers is sometimes an option. However
they are often not designed or implemented with low latency in mind (e.g. they
typically build strings), and so again their usage can adveresly impact program
speed.  Such loggers might not be readily accesible at the code site being
examined, so even trying to use them can be painful.

What does log2mem offer?
------------------------

log2mem is a tool to use for these situations when a debugger and printf
somewhat sturggle.  It offers the rough equivalent of very high speed printf;
this can then be used to instrument the code without comprimsing runtime
performance. The program can be run at full speed, the bug can arise, and its
footprints can be logged.

log2mem acheives high performance (typically around 100 to 150 nanoseconds per
call) by writing log statements to a circular buffer held in shared memory.  So
whereas printf involves string construction and writing to some kind of stream
at the time of the call, log2mem instead writes the raw data values direct to
in-memory circular buffer. A separate tool, log2mem-util, is later used to
inspect the shared memory and convert the raw logging data to human readable
strings. log2mem additionally makes use of lock-free techniques to ensure
optimal performance.  Additionally, log2mem is easy to integrate in existing
C/C++ code.

Note that the ciruclar bufffer is pre-defined to store a fixed amount of data.
Once that limit is reached, new messages can continue to be stored, but they
will overwrite the oldest messages.  Given this limitation, log2mem is not
particularly suitable for general application logging.

Hello World
===========

Below is a basic "hello world" example of using log2mem.  Integration into
application code requires that the header file `log2mem.h` is included and the
library `liblog2mem` added to the link line.

```c++
#include <log2mem/log2mem.h>
#include <iostream>

int main(int, char**)
{
    std::cout << "hello world" << std::endl;

    // obtain a logging row, and write a simple message & value
    void * rowptr = log2mem_get_row(MEMLOGFL);
    log2mem_append_str2(rowptr, "hello world");
    log2mem_append_int(rowptr, 100);

    // print location of the memmap
    std::cout << "log2mem memmap: " << log2mem_filename() << std::endl;
    return 0;
}
```

This could be complied with the following kind of command, which needs to
specify the location where log2mem has been installed.

    g++ -I/tmp/log2mem/include  hello.cc  -o hello  -L/tmp/log2mem/lib -llog2mem

This assumes that, when log2mem was build, it was installed into /tmp/log2mem.
If different install location was used, then the compile command should be
changed appropriately.

The logging statements can be displayed by using the log2mem-util:

    $ util/log2mem-util /var/tmp/log2mem-darrens.dat
    0 [31224] (hello.cc:9) hello world 100


Variables & external interaction
================================

In addition to diagnositc logging, log2mem offers and additiional feature to
assist debugging. The API can be used to define variables that get stored in the
memmap, and which can be read and written both by the application code and by
the external log2mem-util.  This provides a quick & easy mechanism to enable a
developer to interact with a program while it is being debugged (e.g., to
temporarily alter its course of execution, or read & write program state). The
traditional approach to achieve this, outside of a debugger, is to use posix
signals.

The following code shows how to create a variable nd how to access it from
outside the debugged program:

    // create int variable, give it a name & id, and set to 0
    int          var_id   = 1;
    const char*  var_name = "stopflag";
    log2mem_var_int_init(var_id, 0, var_name);

    # externally set variable to 1
    $ log2mem-util   /tmp/log2mem.dat --int[1]=1


