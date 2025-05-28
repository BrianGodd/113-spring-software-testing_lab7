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

/*
  Implement __afl_check_cmd_injection to detect metacharacters in a command string.
  If a metacharacter is found, it prints an error message to stderr and calls abort().
*/
void __afl_check_cmd_injection(const char *command_string) {

  if (command_string == NULL) { return; }

  // Metacharacters that could indicate command injection vulnerabilities.
  // Note: '&' can appear in legitimate URLs. This is a known trade-off.
  const char *metachars = ";|&`$()<>";

  // Iterate through the command_string to check for any metacharacters.
  // Using strpbrk for efficiency to find any character from metachars in command_string.
  const char *found_char = strpbrk(command_string, metachars);

  if (found_char != NULL) {
    // A metacharacter was found.
    fprintf(stderr,
            "Potential command injection detected! Metacharacter '%c' found in command: %s\n",
            *found_char, command_string);
    abort(); // Terminate the program, allowing AFL++ to detect the crash.
  }

  // No metacharacters found, function returns normally.
}