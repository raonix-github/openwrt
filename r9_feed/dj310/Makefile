# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=dj310
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/dj310
  TITLE:=RAONIX DJ310 board support package
  DEFAULT:=y
  DEPENDS:=libblobmsg-json libjson-c libstrophe libubox libubus libstdcpp
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS) -I$(STAGING_DIR)/usr/include -Wall -o $(PKG_BUILD_DIR)/bdtest src/bdtest.c src/flash_api.c src/loopback.c src/figlet.c
endef

define Package/dj310/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/bdtest $(1)/bin/bdtest

	$(CP) ./files/* $(1)
endef

$(eval $(call BuildPackage,dj310))
