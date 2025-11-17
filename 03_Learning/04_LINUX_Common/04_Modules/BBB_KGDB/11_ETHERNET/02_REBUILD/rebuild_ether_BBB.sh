echo "cp *.ko\n"
sudo cp ether_modules/net_ethernet_ti/*.ko /lib/modules/5.15.177+/kernel/drivers/net/ethernet/ti/
sudo cp ether_modules/phy_ti/*.ko /lib/modules/5.15.177+/kernel/drivers/phy/ti/
sudo cp ether_modules/net_phy/*.ko /lib/modules/5.15.177+/kernel/drivers/net/phy/

echo "update initramfs for ether\n"
sudo update-initramfs -u -k $(uname -r)

echo "Done -> restart BBB\n"
