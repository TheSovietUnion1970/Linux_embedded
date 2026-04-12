# PC:
./wlcore/make
./wl18xx/make

sudo scp ./wl18xx/wl18.ko ./wlcore/wlc.ko debian@192.168.137.2:/home/debian/

# BBGW
sudo rmmod wl18xx wlcore wlcore_sdio
sudo ./run1.sh

# For debug
echo wl18xx.1.auto | sudo tee /sys/bus/platform/drivers/wl18xx_driver/unbind
echo wl18xx.1.auto | sudo tee /sys/bus/platform/drivers/wl18xx_driver/bind

# -> BBB
sudo ./BBGW.sh 4
echo "ttyS0" | sudo tee /sys/module/kgdboc/parameters/kgdboc
echo "g" | sudo tee /proc/sysrq-trigger
# -> PC
gdb-multiarch ~/build_BBB_custom/linux-stable-rcn-ee/vmlinux -x BBGW_debug.gdb

# add breakpoint
break wl/wlcore/main.c:6802



# wifi userspace
nmcli device wifi list
sudo nmcli device wifi connect 64DVC --ask

nmcli connection show --active 
sudo nmcli connection down 64DVC


CONFIG_KGDB=y
CONFIG_KGDB_SERIAL_CONSOLE=y
CONFIG_DEBUG_INFO=y
CONFIG_FRAME_POINTER=y
CONFIG_KALLSYMS=y

setenv bootcmd 'mmc dev 0; echo "mmc dev 0 done"; setenv bootargs "console=ttyS0,115200n8 root=/dev/mmcblk0p1 ro rootfstype=ext4 rootwait";echo "=== Loading kernel ==="; ext4load mmc 0:1 ${kernel_addr_r} /boot/vmlinuz-5.15.177; echo "=== Loading dtb ==="; ext4load mmc 0:1 ${fdt_addr_r} /boot/dtbs/5.15.177/am335x-bonegreen-wireless.dtb; echo "=== Booting kernel ==="; bootz ${kernel_addr_r} - ${fdt_addr_r}'  