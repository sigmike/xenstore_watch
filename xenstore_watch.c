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
#include <sys/types.h>
#include <sys/wait.h>
              

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
static void error(char *message);

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
  char **vec;
  unsigned int num_strings;
  char * buf;
  unsigned int len;
  char *program;
  char **arguments;
  int arguments_count;
  pid_t pid;
  int j;
  char *last_value = NULL;
  int status;

  program_name = argv[0];

  i = decode_switches (argc, argv);
  
  if (argc - i < 1)
    usage(1);
  
  path = argv[i++];
  if (argc - i > 0)
  {
    program = argv[i++];
    arguments_count = argc - i;
    
    arguments = malloc(sizeof(char*) * (argc - i + 2));
    arguments[0] = program;
    for (j=0; j<arguments_count; j++)
      arguments[j + 1] = argv[i + j];
    arguments[j + 1] = NULL;
  } else
  {
    program = NULL;
    arguments = NULL;
    arguments_count = 0;
  }
  
  if (want_verbose) {
    printf("Path: %s\n", path);
    if (program)
    {
      printf("Program: %s", program);
      for (i=1; i<arguments_count + 1; i++)
        printf(" %s", arguments[i]);
      printf("\n");
    }
  }
  
  /* Get a connection to the daemon */
  xs = xs_daemon_open();
  if ( xs == NULL ) xs = xs_domain_open();
  if ( xs == NULL ) {
    error("Unable to connect to XenStore");
    exit(1);
  }

  /* Create a watch on /local/domain/0/mynode. */
  er = xs_watch(xs, path, "token");
  if ( er == 0 ) {
    error("Unable to create watch");
    exit(1);
  }

  /* We are notified of read availability on the watch via the
   * file descriptor.
   */
  fd = xs_fileno(xs);
  while (1)
  {
    FD_ZERO(&set);
    FD_SET(fd, &set);
    /* Poll for data. */
    if ( select(fd + 1, &set, NULL, NULL, NULL) > 0
         && FD_ISSET(fd, &set))
    {
      /* num_strings will be set to the number of elements in vec
       * (typically, 2 - the watched path and the token) */
      vec = xs_read_watch(xs, &num_strings);
      if ( !vec ) {
        error("Unable to read watch");
        continue;
      }
      if (want_verbose)
        printf("Path changed: %s\n", vec[XS_WATCH_PATH]);
      /* Prepare a transaction and do a read. */
      th = xs_transaction_start(xs);
      buf = xs_read(xs, th, vec[XS_WATCH_PATH], &len);
      xs_transaction_end(xs, th, false);
      if (buf)
      {
        if (last_value && strcmp(buf, last_value) == 0) {
          if (want_verbose)
            printf("Value did not change\n");
          continue;
        }
      
        if (want_verbose)
          printf("New value: %s\n", buf);
          
        if (program) {
          pid = fork();
          if (pid == 0) {
            setenv("XENSTORE_WATCH_PATH", vec[XS_WATCH_PATH], 1);
            setenv("XENSTORE_WATCH_VALUE", buf, 1);
            for (i=0; arguments[i]; i++) {
              if (strcmp(arguments[i], "%v") == 0)
                arguments[i] = buf;
              else if (strcmp(arguments[i], "%p") == 0)
                arguments[i] = vec[XS_WATCH_PATH];
            }
            execvp(program, arguments);
            error("Unable to start program");
            exit(1);
          } else {
            waitpid(pid, &status, 0);
          }
        } else {
          if (!want_verbose)
            printf("%s\n", buf);
        }
        
        if (last_value)
          free(last_value);
        last_value = buf;
      }
    }
  }
  /* Cleanup */
  close(fd);
  xs_daemon_close(xs);
  free(path);

  exit(0);
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
  printf (_("Usage: %s [OPTION]... PATH [--] [PROGRAM [ARGUMENT]...]\n"), program_name);
  printf (_("\
Options:\n\
  --verbose                  print more information\n\
  -h, --help                 display this help and exit\n\
  -V, --version              output version information and exit\n\
"));
  exit (status);
}

static void error(char *message)
{
  printf("Error: %s\n", message);
}
