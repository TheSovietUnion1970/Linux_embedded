#!/bin/bash
# Usage: ./make_gdb_script.sh X
# Example: ./make_gdb_script.sh 4

if [ $# -ne 1 ]; then
    echo "Usage: $0 X"
    exit 1
fi

X=$1

# Read symbol addresses directly from /sys/module
a1=$(sudo cat /sys/module/ti_cpsw_new/sections/.text)
a2=$(sudo cat /sys/module/davinci_mdio/sections/.text)
a3=$(sudo cat /sys/module/cpsw_common/sections/.text)

b1=$(sudo cat /sys/module/phy_gmii_sel/sections/.text)

a1=$(sudo cat /sys/module/ti_cpsw_new/sections/.text)
a2=$(sudo cat /sys/module/davinci_mdio/sections/.text)
a3=$(sudo cat /sys/module/cpsw_common/sections/.text)

cat > BBB_debug.gdb <<EOF
set serial baud 115200
target remote /dev/pts/${X}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/ethernet/ti/ti_cpsw_new.ko ${a1}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/ethernet/ti/davinci_mdio.ko ${a2}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/ethernet/ti/cpsw-common.ko ${a3}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/phy/ti/phy-gmii-sel.ko ${b1}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/phy/phylink.ko ${c1}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/phy/microchip.ko ${c2}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/phy/dp83867.ko ${c3}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/phy/ax88796b.ko ${c4}


EOF

echo "BBB_debug.gdb created with live module addresses."

echo "Cpy back to PC."
sudo scp BBB_debug.gdb vinh@192.168.137.1:/home/vinh/

