#include <stdlib.h>
#include <xs.h> // don't forget to link with libxenstore.so

int main(int argc, char** argv)
{
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

printf("daemon open\n");
/* Get a connection to the daemon */
xs = xs_daemon_open();
if ( xs == NULL ) xs = xs_domain_open();
if ( xs == NULL ) error();
printf("ok\n");
/* Get the local domain path */
path = xs_get_domain_path(xs, 0); // replace "domid" with a valid domain ID (or one which will become valid)
if ( path == NULL ) error();
/* Make space for our node on the path */
path = realloc(path, strlen(path) + strlen("/mynode") + 1);
if ( path == NULL ) error();
strcat(path, "/mynode");
/* Create a watch on /local/domain/%d/mynode. */
printf("path: %s\n", path);
er = xs_watch(xs, path, "mytoken");
if ( er == 0 ) error();
/* We are notified of read availability on the watch via the
 * file descriptor.
 */
fd = xs_fileno(xs);
while (1)
{
    /* TODO (TimPost), show a simpler example with poll()
     * in a modular style, using a simple callback. Most
     * people think 'inotify' when they see 'watches'. */
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
        /* Prepare a transaction and do a write. */
        //th = xs_transaction_start(xs);
        //er = xs_write(xs, th, path, "somestuff", strlen("somestuff"));
        //xs_transaction_end(xs, th, false);
        //if ( er == 0 ) error();
    }
}
/* Cleanup */
close(fd);
xs_daemon_close(xs);
free(path);
}
