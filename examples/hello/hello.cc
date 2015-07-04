#include <iostream>
#include <log2mem/log2mem.h>

int main(int, char**)
{
  std::cout << "hello world" << std::endl;

  // obtain a logging row, and write a simple message & va
  void * rowptr = log2mem_get_row(MEMLOGFL);
  log2mem_append_str(rowptr, "hello world");
  log2mem_append_int(rowptr, 100);

  std::cout << "log2mem memmap: " << log2mem_filename() << std::endl;
  return 0;
}
