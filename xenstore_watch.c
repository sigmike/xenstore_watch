/* 
   xenstore_watch - Watches changes in XenStore

   Copyright (C) 2010 Michael Witrant

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  

#include <termios.h>
#include <grp.h>
#include <pwd.h>
*/

#include <stdio.h>
#include <sys/types.h>
#include <getopt.h>
#include "system.h"
#include <xs.h>

#define EXIT_FAILURE 1

char *xmalloc ();
char *xrealloc ();
char *xstrdup ();


static void usage (int status);

/* The name the program was run with, stripped of any leading path. */
char *program_name;

/* getopt_long return codes */
enum {DUMMY_CODE=129
};

/* Option flags and variables */

int want_verbose;		/* --verbose */

static struct option const long_options[] =
{
  {"verbose", no_argument, 0, 'v'},
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {NULL, 0, NULL, 0}
};

static int decode_switches (int argc, char **argv);

int
main (int argc, char **argv)
{
  int i;
  struct xs_handle *xs;
  xs_transaction_t th;
  char *path;
  int fd;
  fd_set set;
  int er;
  struct timeval tv = {.tv_sec = 0, .tv_usec = 0 };
  char **vec;
  unsigned int num_strings;
  char * buf;
  unsigned int len;

  program_name = argv[0];

  i = decode_switches (argc, argv);

  /* Get a connection to the daemon */
  xs = xs_daemon_open();
  if ( xs == NULL ) xs = xs_domain_open();
  if ( xs == NULL ) error();

  /* Get the local domain 0 path */
  path = xs_get_domain_path(xs, 0);
  if ( path == NULL ) error();

  /* Make space for our node on the path */
  path = realloc(path, strlen(path) + strlen("/mynode") + 1);
  if ( path == NULL ) error();
  strcat(path, "/mynode");

  /* Create a watch on /local/domain/0/mynode. */
  er = xs_watch(xs, path, "mytoken");
  if ( er == 0 ) error();

  /* We are notified of read availability on the watch via the
   * file descriptor.
   */
  fd = xs_fileno(xs);
  while (1)
  {
    FD_ZERO(&set);
    FD_SET(fd, &set);
    /* Poll for data. */
    if ( select(fd + 1, &set, NULL, NULL, &tv) > 0
         && FD_ISSET(fd, &set))
    {
      /* num_strings will be set to the number of elements in vec
       * (typically, 2 - the watched path and the token) */
      vec = xs_read_watch(xs, &num_strings);
      if ( !vec ) error();
      printf("vec contents: %s|%s\n", vec[XS_WATCH_PATH],
                                      vec[XS_WATCH_TOKEN]);
      /* Prepare a transaction and do a read. */
      th = xs_transaction_start(xs);
      buf = xs_read(xs, th, vec[XS_WATCH_PATH], &len);
      xs_transaction_end(xs, th, false);
      if ( buf )
      {
          printf("buflen: %d\nbuf: %s\n", len, buf);
      }
    }
  }
  /* Cleanup */
  close(fd);
  xs_daemon_close(xs);
  free(path);

  exit (0);
}

/* Set all the option flags according to the switches specified.
   Return the index of the first non-option argument.  */

static int
decode_switches (int argc, char **argv)
{
  int c;


  while ((c = getopt_long (argc, argv, 
			   "v"	/* verbose */
			   "h"	/* help */
			   "V",	/* version */
			   long_options, (int *) 0)) != EOF)
    {
      switch (c)
	{
	case 'v':		/* --verbose */
	  want_verbose = 1;
	  break;
	case 'V':
	  printf ("xenstore_watch %s\n", VERSION);
	  exit (0);

	case 'h':
	  usage (0);

	default:
	  usage (EXIT_FAILURE);
	}
    }

  return optind;
}


static void
usage (int status)
{
  printf (_("%s - \
Watches changes in XenStore\n"), program_name);
  printf (_("Usage: %s [OPTION]... [FILE]...\n"), program_name);
  printf (_("\
Options:\n\
  --verbose                  print more information\n\
  -h, --help                 display this help and exit\n\
  -V, --version              output version information and exit\n\
"));
  exit (status);
}