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
  bv_close(addr);

  const char *h = "tylersfile";
  addr = bv_open(h, 2);
  int nums[11000];
  for (int i = 0; i < 11000; i++)
      nums[i] = i*2;
  bv_write(addr, &nums, sizeof(nums));
  printf("You shoudl see memememe\n");
  bv_ls();
  printf("About to close %s\n",h);
  bv_close(addr);
  printf("closed? %s\n",h);
  short nf = bv_open(h, 0);
  printf("Got fd of :%d from bvopen\n", nf);
  int arr[11000];
  bv_read(nf,(void*)arr, sizeof(arr));
 /// for (int i = 0; i < 11000; i++)
   //   printf("received back from bvfs: %d\n",arr[i]);
  bv_unlink(h);
  bv_ls();

  const char *j = "tsmalll";
  addr = bv_open(j, 2);
  char ns[11000];
  for (int i = 0; i < 11000; i++)
      ns[i] = 'T';
  bv_write(addr, &ns, sizeof(ns));
  bv_ls();
  bv_unlink(j);
  bv_ls();
}
