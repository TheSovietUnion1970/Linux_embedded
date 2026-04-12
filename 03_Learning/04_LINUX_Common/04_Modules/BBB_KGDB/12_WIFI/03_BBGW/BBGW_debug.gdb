set serial baud 115200
target remote /dev/pts/3

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/libarc4.ko 0xbf059000
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/cfg80211.ko 0xbf05e000
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/mac80211.ko 0xbf12d000

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/wlc.ko 0xbf0ec000
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/wl18.ko 0xbf1f4000


