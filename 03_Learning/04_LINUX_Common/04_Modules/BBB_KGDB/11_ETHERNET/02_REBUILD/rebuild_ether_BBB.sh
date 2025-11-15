echo "cp *.ko\n"
sudo cp ether_modules/*.ko /lib/modules/5.15.177+/kernel/drivers/net/ethernet/ti/

echo "update initramfs for ether\n"
sudo update-initramfs -u -k $(uname -r)

echo "Done -> restart BBB\n"
