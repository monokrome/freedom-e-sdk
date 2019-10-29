/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */
#include <metal/cache.h>
#include <metal/cpu.h>
#include <stdio.h>
#include <stdlib.h>

#include "latency_test.h"

struct test_param {
  size_t size;
  size_t stride;
};

struct test_param tests[] = {{4, 64},       {8, 128},      {16, 256},
                             {32, 512},     {64, 1024},    {128, 2048},
                             {256, 4096},   {512, 8192},   {1024, 16384},
                             {2048, 32768}, {4096, 65536}, {8192, 131072}};
extern char metal_segment_heap_target_start;
extern char metal_segment_heap_target_end;

char *heap_start_location = &metal_segment_heap_target_start;
char *heap_end_location = &metal_segment_heap_target_end;

#define ONE p = (char **)*p;
#define FIVE ONE ONE ONE ONE ONE
#define TEN FIVE FIVE
#define FIFITY TEN TEN TEN TEN TEN
#define HUNDRED FIFITY FIFITY
#define FHUNDRED HUNDRED HUNDRED HUNDRED HUNDRED HUNDRED
#define THOUSAND FHUNDRED FHUNDRED

#define K_TO_BYTE(x) (x * 1024)

void benchmark_latency(void *test_setting, size_t min_loop) {
  struct test_info *setting = (struct test_info *)test_setting;
  register char **p = (char **)setting->p;
  register size_t i;
  register size_t loop = min_loop;

  asm volatile("sltu zero, x1, x2"); // For RTLsim log marker
  for (i = 0; i < loop; ++i) {
    THOUSAND;
  }
  asm volatile("sltu zero, x1, x2"); // For RTLsim log marker

  setting->p = (char *)p;
}

int main() {
  size_t heap_size = (size_t)(heap_end_location - heap_start_location);
  struct test_info setting = {};
  uint32_t test_count = 0;
  uint64_t cycle_count = 0;
  uint32_t i = 0, loop = 1, warmup_loop = 1;
  uint32_t all_test = sizeof(tests) / sizeof(tests[0]);

  printf("heap size: %u K\n", (size_t)heap_size / 1024);

  while ((test_count < all_test) &&
         (K_TO_BYTE(tests[test_count].size) < heap_size))
    test_count++;

  if (test_count == 0) {
    printf("heap too small to run any test");
    return 1;
  }

  setting.max_range = K_TO_BYTE(tests[test_count - 1].size);

  // iterate through all tests
  for (i = 0; i < test_count; i++) {
    uint64_t start = 0, end = 0;

    // init all data structure
    setting.line = tests[i].stride;
    setting.cur_range = K_TO_BYTE(tests[i].size);
    rnd_init(&setting);

    // warm-up get cycle
    start = get_cycle();
    start = get_cycle();
    // warm-up run
    benchmark_latency(&setting, warmup_loop);

    // benchmark
    start = get_cycle();
    benchmark_latency(&setting, loop);
    end = get_cycle();

    cycle_count = end - start;
    printf("%5u K : %u  cycle\n", tests[i].size,
           cycle_count / (loop * 1000));
  }
  printf("---test end---\n");
  exit(0);
}
