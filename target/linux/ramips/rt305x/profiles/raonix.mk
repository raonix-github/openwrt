#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

# 2016.07.01 bygomma : add r9-nemoahn
define Profile/R9-NEMOAHN
	NAME:=RAONIX R9-NEMOAHN
	PACKAGES:=\
		kmod-usb2 
endef

define Profile/R9-NEMOAHN/Description
	Package set for RAONIX NEMOAHN board
endef
$(eval $(call Profile,R9-NEMOAHN))

define Profile/R9-DJ270
	NAME:=RAONIX R9-DJ270
	PACKAGES:=\
		kmod-usb2 
endef

define Profile/R9-DJ270/Description
	Package set for RAONIX DJ270 board
endef
$(eval $(call Profile,R9-DJ270))

define Profile/R9-DJ300
	NAME:=RAONIX R9-DJ300
	PACKAGES:=\
		kmod-usb2 
endef

define Profile/R9-DJ300/Description
	Package set for RAONIX DJ300 board
endef
$(eval $(call Profile,R9-DJ300))
