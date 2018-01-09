#!/bin/sh
NETIFD_MAIN_DIR=../../scripts
. $NETIFD_MAIN_DIR/netifd-wireless.sh

init_wireless_driver "$@"

drv_rt2860v2_init_device_config() {
	logger -t rt2860v2  "drv_rt2860v2_init_device_config"
}

drv_mac80211_init_iface_config() {
	logger -t rt2860v2  "drv_rt2860v2_init_iface_config"
}

drv_rt2860v2_setup() {
	logger -t rt2860v2  "drv_rt2860v2_setup"
}

drv_rt2860v2_cleanup() {
	logger -t rt2860v2  "drv_rt2860v2_cleanup"
}

drv_rt2860v2_teardown() {
	logger -t rt2860v2  "drv_rt2860v2_teardown"
}

add_driver rt2860v2
