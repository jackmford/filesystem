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
  short num = 2;
  const void *ptr = &num;
  bv_write(addr, &num, 2);
  bv_ls();
  printf("Driver %d\n", addr);
  bv_close(addr);

  const char *h = "tylersfile";
  short newaddr = bv_open(h, 1);
  int nums[16384];
  for (int i = 0; i < 16384; i++)
      nums[i] = i*2;
  printf("Size of nums %ld\n", sizeof(nums));
  bv_write(newaddr, &nums, sizeof(nums));
//  bv_unlink(h);
  bv_ls();
}
