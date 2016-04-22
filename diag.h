// Include this in every MMU-connected device to enable diagnostics.
//
// Add HANDLE_DIAGNOSTICS near the top of the device source file to
// insert support code for diagnostics.  It will create a function
// which implements a callback to control the verbosity level.  This
// function must be included in the struct passed to mmu_register.
//
// The syntax for diagnostic output is this:
//   LEVEL(FORMAT [, ARGS])
// where LEVEL is one of the six levels below,
// FORMAT is a printf-style format string, and
// ARGS are arguments to the format string.

#include "mmu.h"

extern void diag_set_module_levels(char *);
extern void print_diagnostic(int, struct mmu *, const char *, ...);

#define LEVEL_FATAL  1
#define LEVEL_ERROR  2
#define LEVEL_WARN   3
#define LEVEL_INFO   4
#define LEVEL_DEBUG  5
#define LEVEL_TRACE  6
#define LEVEL_CLOCK  7

#define PRINT_DIAG(LEVEL, FORMAT, ...) \
  print_diagnostic(LEVEL_##LEVEL, mmu_device, FORMAT, ##__VA_ARGS__)

// Panic, we're about to die!
#define FATAL(FORMAT, ...)  PRINT_DIAG(FATAL, FORMAT, ##__VA_ARGS__)
// Serous error, but we can proceed.
#define ERROR(FORMAT, ...)  PRINT_DIAG(ERROR, FORMAT, ##__VA_ARGS__)
// Problem, alert the user.
#define WARN(FORMAT, ...)   PRINT_DIAG(WARN, FORMAT, ##__VA_ARGS__)
// No problem, but user may want to know.
#define INFO(FORMAT, ...)   PRINT_DIAG(INFO, FORMAT, ##__VA_ARGS__)
// Step-by-step description of internal mechanisms.
#define DEBUG(FORMAT, ...)  PRINT_DIAG(DEBUG, FORMAT, ##__VA_ARGS__)

#ifdef DBG
// Feel free to call this just about CPU instruction.
#define TRACE(FORMAT, ...)  PRINT_DIAG(TRACE, FORMAT, ##__VA_ARGS__)
// Feel free to call this just about every clock cycle.
#define CLOCK(FORMAT, ...)  PRINT_DIAG(CLOCK, FORMAT, ##__VA_ARGS__)

#define ASSERT(CONDITION)				\
  do {							\
    if(!(CONDITION))					\
      FATAL("Assertion failed: %s", #CONDITION);	\
  } while(0)
#else
#define ASSERT(CONDITION)
#define TRACE(FORMAT, ...)
#define CLOCK(FORMAT, ...)
#endif

#define HANDLE_DIAGNOSTICS(device)				\
static struct mmu *mmu_device = NULL;				\
static void device ## _diagnostics(struct mmu *device, int n)	\
{								\
  mmu_device = device;						\
  device->dev.verbosity = n;					\
}

#define HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(device, device_id)    \
  do {                                                          \
    extern void diagnostics_level(struct mmu *, int);           \
    extern int verbosity;                                       \
    struct mmu *dummy_device;                                   \
    dummy_device = (struct mmu *)malloc(sizeof(struct mmu));    \
    dummy_device->dev.diagnostics = device ## _diagnostics;     \
    memcpy(dummy_device->dev.id, device_id, 4);                 \
    diagnostics_level(dummy_device, verbosity);                 \
  } while(0)
