#
# Regular cron jobs for the xpleibd package
#
0 4	* * *	root	[ -x /usr/bin/xpleibd_maintenance ] && /usr/bin/xpleibd_maintenance
