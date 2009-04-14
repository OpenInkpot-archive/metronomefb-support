#! /bin/sh

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DESC="metronomefb support"
NAME="metronomefb-support"

[ -f /etc/default/metronomefb-support ] && . /etc/default/metronomefb-support

case "$1" in
	start)
		echo -n "Loading metronomefb module... "
		temp=`cat $TEMPERATURE_SOURCE`
		temp_deg=`expr $temp / $TEMPERATURE_DIVIDER`
		modprobe metronomefb temp=$temp_deg
		modprobe $BOARD_MODULE
		echo $DEFIO_DELAY > /sys/class/graphics/fb$FB_NUM/defio_delay 
		echo "done."
		;;
	stop)
		echo -n "Stopping $DESC: "
		echo "$NAME."
		;;
	*)
		N=/etc/init.d/$NAME
		echo "Usage: $N {start|stop}" >&2
		exit 1
	;;
esac

exit 0
