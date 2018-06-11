#!/bin/sh

. /lib/functions.sh

buildcode="$(uci_get system.@system[0].firmware_buildcode)"

if [ -z "$buildcode" ]; then
	buildcode=98
fi

/bin/dj310_led.sh -us

ota "http://nms.raonix.co.kr/raonix/data/dj310/dj310-ota-$buildcode.json"

/bin/dj310_led.sh -ue
