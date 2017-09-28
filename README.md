3proxy running on Openwrt/LEDE
===

Брвы/Compile
---

```bash
cd openwrt
git clone https://github.com/muziling/3proxy-openwrt.git feeds/packages/net/3proxy
rm -rf tmp/

./scripts/feeds update -a
./scripts/feeds install -a

make menuconfig
make package/3proxy/compile
```
