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

  const char *h = "tylersfile";
  addr = bv_open(h, 2);
  int nums[34000];
  for (int i = 0; i < 34000; i++)
      nums[i] = i*2;
  bv_write(addr, &nums, sizeof(nums));
  bv_ls();
}
