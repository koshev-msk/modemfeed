#
# Copyright (C) 2006-2009 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=can-mcp251xfd
PKG_RELEASE:=1

PKG_SOURCE_URL:=https://github.com/tibbotech/mcp251xfd
PKG_SOURCE_PROTO:=git
PKG_SOURCE_DATE:=2021-03-11
PKG_SOURCE_VERSION:=e1dd938fbdc4e9c5ec0b1b009bd0a7a091dad8ff
PKG_MIRROR_HASH:=4473f358269f34b3907d253e95231c68e7aba849cfdcf482c22bf40d3fe83324

include $(INCLUDE_DIR)/package.mk

define KernelPackage/can-mcp251xfd
  SUBMENU:=CAN Support
  DEPENDS:= kmod-can
  TITLE:=Microchip MCP251xFD SPI CAN controllers
  PKG_MAKE_FLAGS += CONFIG_CAN_MCP251XFD=m
  FILES:=$(PKG_BUILD_DIR)/mcp251xfd/mcp251xfd.ko
  AUTOLOAD:=$(call AutoProbe,mcp251xfd)
  $(call AddDepends/can)
endef

define Build/Compile
	+$(MAKE) $(PKG_JOBS) -C "$(LINUX_DIR)" \
		$(KERNEL_MAKE_FLAGS) \
		$(PKG_MAKE_FLAGS) \
		M="$(PKG_BUILD_DIR)/mcp251xfd" \
		modules
endef

$(eval $(call KernelPackage,can-mcp251xfd))
