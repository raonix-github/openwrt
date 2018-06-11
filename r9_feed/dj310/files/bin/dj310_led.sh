#!/bin/sh

. /lib/functions.sh
. /lib/functions/leds.sh

run_led="r9-dj310:green:run"
iot_led="r9-dj310:green:iot"
xmpp_led="r9-dj310:red:iot"

# fs : start friend
# fe : end friend
# is : start iot
# ie : end iot
# us : start upgrade
# ue : end upgrade

case "$1" in
	-xc)
		led_on $xmpp_led
		;;
	-xd)
		led_off $xmpp_led
		;;
	-fs)
		led_timer $xmpp_led 100 100
		;;
	-fe)
		led_on $xmpp_led
		;;
	-is)
		led_timer $iot_led 100 100
		;;
 	-ie)
		led_on $iot_led
		;;
	-us)
		led_timer $run_led 100 100
		;;
	-ue)
		led_on $run_led
		;;
esac
