include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=crypto-hw-eip93-mt7621
PKG_RELEASE:=1

PKG_SOURCE_PROTO:=git
PKG_SOURCE_URL:=https://github.com/denisyuriev/crypto-hw-eip93-mt7621.git
PKG_SOURCE_DATE:=2026-11-05
PKG_SOURCE_VERSION:=7de663c76dd500343c970a60121b4e88ae7d8cdf
PKG_MIRROR_HASH:=skip

PKG_LICENSE:=GPL-2.0
PKG_BUILD_PARALLEL:=1

include $(INCLUDE_DIR)/package.mk

define KernelPackage/crypto-hw-eip93-mt7621
  SUBMENU:=Cryptographic API modules
  TITLE:=MTK EIP93 crypto engine (MT7621)
  DEPENDS:=@TARGET_ramips_mt7621 \
	+kmod-crypto-authenc \
	+kmod-crypto-des \
	+kmod-crypto-md5 \
	+kmod-crypto-sha1 \
	+kmod-crypto-sha256
  FILES:=$(PKG_BUILD_DIR)/crypto-hw-eip93.ko
  AUTOLOAD:=$(call AutoLoad,09,crypto-hw-eip93)
  $(call AddDepends/crypto)
endef

define KernelPackage/crypto-hw-eip93-mt7621/description
  Hardware crypto engine EIP-93 found in the MediaTek MT7621 SoC.
  DES/3DES/AES in ECB/CBC modes, MD5/SHA1/SHA2 hashes and HMAC.
endef

NOSTDINC_FLAGS = \
	-I$(PKG_BUILD_DIR) \
	-I$(STAGING_DIR)/usr/include/mac80211-backport/uapi

define Build/Compile
	$(KERNEL_MAKE) $(PKG_JOBS) \
		M="$(PKG_BUILD_DIR)" \
		CONFIG_CRYPTO_DEV_EIP93=m \
		modules
endef

$(eval $(call KernelPackage,crypto-hw-eip93-mt7621))
