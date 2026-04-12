#!/bin/bash
# Usage: ./make_gdb_script.sh X
# Example: ./make_gdb_script.sh 4

if [ $# -ne 1 ]; then
    echo "Usage: $0 X"
    exit 1
fi

X=$1

# Read symbol addresses directly from /sys/module
a1=$(sudo cat /sys/module/wl18/sections/.text)
a2=$(sudo cat /sys/module/wlc/sections/.text)

b1=$(sudo cat /sys/module/libarc4/sections/.text)
b2=$(sudo cat /sys/module/mac80211/sections/.text)
b3=$(sudo cat /sys/module/cfg80211/sections/.text)

cat > BBGW_debug.gdb <<EOF
set serial baud 115200
target remote /dev/pts/${X}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/libarc4.ko ${b1}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/cfg80211.ko ${b3}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/mac80211.ko ${b2}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/wlcore/wlc.ko ${a2}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/wl/wl18xx/wl18.ko ${a1}


EOF

echo "BBGW_debug.gdb created with live module addresses."

echo "Cpy back to PC."
sudo scp BBGW_debug.gdb vinh@192.168.137.1:/home/vinh/

