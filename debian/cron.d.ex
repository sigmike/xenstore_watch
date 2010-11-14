#
# Regular cron jobs for the xenstore-watch package
#
0 4	* * *	root	[ -x /usr/bin/xenstore-watch_maintenance ] && /usr/bin/xenstore-watch_maintenance
