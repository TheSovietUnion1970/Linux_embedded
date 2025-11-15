echo "cp *.ko\n"
sudo cp ether_modules/cpsw-common.ko /lib/modules/5.15.177+/kernel/drivers/net/ethernet/ti/
sudo cp ether_modules/davinci_mdio.ko /lib/modules/5.15.177+/kernel/drivers/net/ethernet/ti/
sudo cp ether_modules/ti_cpsw_new.ko /lib/modules/5.15.177+/kernel/drivers/net/ethernet/ti/
sudo cp ether_modules/phy-gmii-sel.ko /lib/modules/5.15.177+/kernel/drivers/phy/ti/

echo "update initramfs for ether\n"
sudo update-initramfs -u -k $(uname -r)

echo "Done -> restart BBB\n"
