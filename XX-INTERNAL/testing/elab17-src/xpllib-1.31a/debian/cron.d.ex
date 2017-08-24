#
# Regular cron jobs for the xpllib package
#
0 4	* * *	root	[ -x /usr/bin/xpllib_maintenance ] && /usr/bin/xpllib_maintenance
