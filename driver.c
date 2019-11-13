#include <stdio.h> 
#include "bvfs.h"

int main(int argc, char** argv) {
  const char *f = "test";
  bv_destroy();
  bv_init(f);
  bv_ls();
  bv_destroy();
}
