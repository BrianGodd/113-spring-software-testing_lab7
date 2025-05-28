#ifdef __ANDROID__
  #include "android-ashmem.h"
#endif
#include "config.h"
#include "types.h"
#include "cmplog.h"
#include "llvm-alternative-coverage.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <errno.h>

#include <sys/mman.h>
#ifndef __HAIKU__
  #include <sys/syscall.h>
#endif
#ifndef USEMMAP
  #include <sys/shm.h>
#endif
#include <sys/wait.h>
#include <sys/types.h>

#if !__GNUC__
  #include "llvm/Config/llvm-config.h"
#endif

#ifdef __linux__
  #include "snapshot-inl.h"
#endif

void __demo_crash() {

  fprintf(stderr, "system found!\n");
  abort();

}