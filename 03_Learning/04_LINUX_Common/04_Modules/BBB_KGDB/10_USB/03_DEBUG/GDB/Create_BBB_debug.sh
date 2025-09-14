#!/bin/bash
# Usage: ./make_gdb_script.sh X
# Example: ./make_gdb_script.sh 4

if [ $# -ne 1 ]; then
    echo "Usage: $0 X"
    exit 1
fi

X=$1

# Read symbol addresses directly from /sys/module
a1=$(sudo cat /sys/module/musb_dsps/sections/.text)
a2=$(sudo cat /sys/module/musb_hdrc/sections/.text)
b1=$(sudo cat /sys/module/phy_am335x_control/sections/.text)
b2=$(sudo cat /sys/module/phy_am335x/sections/.text)
c1=$(sudo cat /sys/module/usbcore/sections/.text)

cat > BBB_debug.gdb <<EOF
set serial baud 115200
target remote /dev/pts/${X}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/usb/musb/musb_dsps.ko ${a1}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/usb/musb/musb_hdrc.ko ${a2}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/usb/phy/phy-am335x_control.ko ${b1}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/usb/phy/phy-am335x.ko ${b2}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/usb/core/usbcore.ko ${c1}
EOF

echo "BBB_debug.gdb created with live module addresses."

echo "Cpy back to PC."
sudo scp BBB_debug.gdb vinh@192.168.137.1:/home/vinh/

