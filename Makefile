include $(TOPDIR)/rules.mk

# place this file one folder upper than all the sources, like this:
#
#    xinfcfeed
#    └── xinfc
#        ├── Makefile
#        └── src
#        	├── LICENSE
#...
#        	├── README
#        	├── src
#        	└── include
#
# add line 'src-link xinfcfeed /path/to/xinfcfeed' to your feeds.conf
# next, as usual perform ./scripts/feeds update -a && ./scripts/feeds install -a
# next perform config (e.g. make menuconfig) and enable xinfc build
# finally make package/xinfc/compile will produce package for you.

PKG_NAME:=xinfc
PKG_VERSION:=1.0.0
PKG_RELEASE:=$(AUTORELEASE)

PKG_MAINTAINER:=Alexey N. Vinogradov <a.n.vinogradov@gmail.com>
PKG_BUILD_DIR:=$(BUILD_DIR)/xinfc-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

define Package/xinfc
  SECTION:=examples
  CATEGORY:=Examples
  TITLE:=xinfc set nfc wifi tag on xiaomi router
endef

define Package/xinfc/description
  tools for interfacing with the NFC chip on Xiaomi routers.
endef

define Package/xinfc/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/xinfc-wsc $(1)/usr/bin/
endef

$(eval $(call BuildPackage,xinfc))
