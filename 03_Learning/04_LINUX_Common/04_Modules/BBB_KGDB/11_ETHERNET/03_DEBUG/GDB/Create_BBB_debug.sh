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

c1=$(sudo cat /sys/module/libphy/sections/.text)
c2=$(sudo cat /sys/module/mdio_devres/sections/.text)
c3=$(sudo cat /sys/module/smsc/sections/.text)
c4=$(sudo cat /sys/module/fixed_phy/sections/.text)

d1=$(sudo cat /sys/module/of_mdio/sections/.text)
d2=$(sudo cat /sys/module/mdio_bitbang/sections/.text)
d3=$(sudo cat /sys/module/fwnode_mdio/sections/.text)

cat > BBB_debug.gdb <<EOF
set serial baud 115200
target remote /dev/pts/${X}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/ethernet/ti/ti_cpsw_new.ko ${a1}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/ethernet/ti/davinci_mdio.ko ${a2}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/ethernet/ti/cpsw-common.ko ${a3}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/phy/ti/phy-gmii-sel.ko ${b1}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/phy/libphy.ko ${c1}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/phy/mdio_devres.ko ${c2}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/phy/smsc.ko ${c3}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/phy/fixed_phy.ko ${c4}

add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/mdio/of_mdio.ko ${d1}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/mdio/mdio-bitbang.ko ${d2}
add-symbol-file /home/vinh/build_BBB_custom/linux-stable-rcn-ee/drivers/net/mdio/fwnode_mdio.ko ${d3}


EOF

echo "BBB_debug.gdb created with live module addresses."

echo "Cpy back to PC."
sudo scp BBB_debug.gdb vinh@192.168.137.1:/home/vinh/

