/*
 * unix_process.c - Get information on processes for various UNIX systems.
 * Copyright (c) 2009 Michael B. Trausch <mike@trausch.us>
 * License: GNU LGPL v3.0 as published by the Free Software Foundation
 *
 * This is a weird file.  I don't know of a better way to do this, so
 * if you know of one, please re-do it that way.  Basically,
 * system-specific files are in the "unix_process" directory contained
 * in this directory, and contain files named
 * "${PLATFORM}_process_info.c", which are included in this file
 * through the magic of preprocessor directives.
 *
 * See unix_process/linux_process_info.c for what needs to be in the
 * file, and update get_process_info below (and the build system) when
 * adding new implementations for different systems.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <config.h>
#include <unix_process.h>

static void unsupported_operating_system(void);
static char *get_nth_token(FILE *from, int which, char delim);

#ifdef LINUX
#include "unix_process/linux_process_info.c"
#endif /* LINUX */

#ifdef FREEBSD
#include "unix_process/freebsd_process_info.c"
#endif /* FREEBSD */

process_info_t *
get_process_info(pid_t pid) {
  #ifdef LINUX
  return(linux_get_process_info(pid));
  #endif /* LINUX */

  #ifdef FREEBSD
  return(freebsd_get_process_info(pid));
  #endif /* FREEBSD */

  unsupported_operating_system();
}

static void
unsupported_operating_system() {
  fprintf(stderr, "error: I don't know how to get process info on your "
	  "operating system.\n");
  fprintf(stderr, "Please file a bug at %s\n", PACKAGE_BUGREPORT);
  fprintf(stderr, "to request a fix for this.  Include your operating system "
	  "name and version,\n");
  fprintf(stderr, "if possible.\n");

  fflush(stderr);

  abort();
}

