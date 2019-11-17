#include <stdio.h> 
#include "bvfs.h"

int main(int argc, char** argv) {
  const char *f = "test";
  bv_init(f);
  bv_ls();
//  bv_destroy();
  bv_ls();
  const char *t = "testingopen";
  short addr = bv_open(t, 2);
  printf("Driver %d\n", addr);
  short num = 2;
  const void *ptr = &num;
  bv_write(addr, &num, 2);
  bv_ls();
}
