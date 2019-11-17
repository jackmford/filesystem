#include <stdio.h> 
#include "bvfs.h"

int main(int argc, char** argv) {
  const char *f = "test";
  bv_init(f);
  bv_ls();
  const char *y = "testinging";
  bv_open(y, 2);
  bv_destroy();
  bv_ls();
}
