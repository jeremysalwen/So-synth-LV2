#
# Regular cron jobs for the so-666 package
#
0 4	* * *	root	[ -x /usr/bin/so-666_maintenance ] && /usr/bin/so-666_maintenance
