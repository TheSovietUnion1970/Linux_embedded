sudo rmmod wl18 wlc
sudo rmmod mac80211 cfg80211 libarc4 

sudo insmod libarc4.ko 
sudo insmod cfg80211.ko
sudo insmod mac80211.ko
sudo insmod wlc.ko 
sudo insmod wl18.ko

