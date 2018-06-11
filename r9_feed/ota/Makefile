# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.

include $(TOPDIR)/rules.mk

PKG_NAME:=ota
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/ota
  TITLE:=RAONIX DJ300 OTA
  DEPENDS:=+jansson libopenssl libuci
  DEFAULT:=y
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(EXTRA_CFLAGS) -I$(STAGING_DIR)/usr/include -o $(PKG_BUILD_DIR)/ota src/ota.c -luci -lssl -ljansson -lcrypto
endef

define Package/ota/install
	$(INSTALL_DIR) $(1)/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/ota $(1)/bin/ota
endef

$(eval $(call BuildPackage,ota))
