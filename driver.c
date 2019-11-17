#include <stdio.h> 
#include "bvfs.h"

int main(int argc, char** argv) {
  const char *f = "test";
  bv_init(f);
  bv_ls();
  const char *t = "testingopen";
  const char *tu = "testingopen";
  short addr = bv_open(t, 2);
  printf("Driver %d\n", addr);
  short writethis[10];
  writethis[0] = 200;
  short num = 2;
  const void *ptr = &num;
  //bv_ls();
  //bv_unlink(tu);
  //bv_unlink(t);
  //bv_ls();
  bv_write(addr, ptr, 2);
//  bv_destroy();
//  bv_ls();
}
