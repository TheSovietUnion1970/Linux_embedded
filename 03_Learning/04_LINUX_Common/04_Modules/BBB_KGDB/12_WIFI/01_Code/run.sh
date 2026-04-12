# PC:
./wlcore/make
./wl18xx/make

sudo scp ./wl18xx/wl18.ko ./wlcore/wlc.ko debian@192.168.137.2:/home/debian/

# BBGW
sudo rmmod wl18xx wlcore_sdio
sudo rmmod wlcore

sudo insmod wlc.ko
sudo insmod wl18.ko


setenv bootcmd 'mmc dev 0; echo "mmc dev 0 done"; setenv bootargs "console=ttyS0,115200n8 root=/dev/mmcblk0p1 ro rootfstype=ext4 rootwait";echo "=== Loading kernel ==="; ext4load mmc 0:1 ${kernel_addr_r} /boot/vmlinuz-5.15.177; echo "=== Loading dtb ==="; ext4load mmc 0:1 ${fdt_addr_r} /boot/dtbs/5.15.177/am335x-bonegreen-wireless.dtb; echo "=== Booting kernel ==="; bootz ${kernel_addr_r} - ${fdt_addr_r}'  