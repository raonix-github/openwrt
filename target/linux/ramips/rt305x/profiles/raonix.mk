#
# Copyright (C) 2015 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

define Profile/R9-NEMOAHN
	NAME:=RAONIX NemoAhn
	PACKAGES:=\
		kmod-usb2 
endef

define Profile/R9-NEMOAHN/Description
	Package set for RAONIX NemoAhn board
endef
$(eval $(call Profile,R9-NEMOAHN))
