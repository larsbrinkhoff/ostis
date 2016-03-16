#ifndef DEV_H
#define DEV_H

#include "common.h"
#include "cpu.h"

struct mmu_state;

struct device {
  struct device *next;
  char id[4];
  const char *name;
  int verbosity;
  int (*state_collect)(struct mmu_state *);
  void (*state_restore)(struct mmu_state *);
  void (*diagnostics)();
  void (*interrupt)(struct cpu *);
};

#endif
