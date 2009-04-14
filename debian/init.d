#! /bin/sh

PATH=/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin
DESC="metronome temperature daemon"
NAME="metronometempd"

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
		echo -n "Starting metronome temperature daemon: "
		start-stop-daemon --start --quiet --pidfile /var/run/metronometempd.pid --exec /usr/sbin/metronometempd -- $TEMPERATURE_SOURCE /sys/class/graphics/fb$FB_NUM/temp $TEMPERATURE_DIVIDER $PERIOD
		echo "metronometempd."
		;;
	stop)
		echo -n "Stopping $DESC: "
		start-stop-daemon --stop --quiet --pidfile /var/run/metronometempd.pid --oknodo
		echo "$NAME."
		;;
	restart)
		$0 stop
		sleep 1
		$0 start
		;;
	*)
		N=/etc/init.d/$NAME
		echo "Usage: $N {start|stop}" >&2
		exit 1
	;;
esac

exit 0
