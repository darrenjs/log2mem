
#include <iostream>

#include <log2mem/log2mem.h>

using namespace std;

int main(int argc, char** argv)
{
  cout << "hello world\n";

  // obtain a logging row, and write a simple message
  void * rowptr = \xk
log2mem_get_row(MEMLOGFL);
  log2mem_append_str2(rowptr, "hello world");
  log2mem_append_int(rowptr, 100);

  cout << "done\n";
  return 0;
}

