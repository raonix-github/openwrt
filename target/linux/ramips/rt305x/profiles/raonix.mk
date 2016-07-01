#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

# 2016.07.01 bygomma : add r9-nemoahn
define Profile/R9-NEMOAHN
	NAME:=RAONIX NEMOAHN
	PACKAGES:=\
		kmod-usb2 
endef

define Profile/R9-NEMOAHN/Description
	Package set for RAONIX NEMOAHN board
endef
$(eval $(call Profile,R9-NEMOAHN))
