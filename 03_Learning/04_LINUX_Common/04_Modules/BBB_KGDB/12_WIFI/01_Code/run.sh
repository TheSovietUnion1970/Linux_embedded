# PC:
./wlcore/make
./wl18xx/make

sudo scp ./wl18xx/wl18.ko ./wlcore/wlc.ko debian@192.168.137.2:/home/debian/

# BBGW
sudo rmmod wl18xx wlcore_sdio
sudo rmmod wlcore

sudo insmod wlc.ko
sudo insmod wl18.ko
