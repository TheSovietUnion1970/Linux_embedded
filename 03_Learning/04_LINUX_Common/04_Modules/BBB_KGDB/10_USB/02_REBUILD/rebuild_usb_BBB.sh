echo "cp *.ko\n"
sudo cp usb_modules/musb_dsps.ko /lib/modules/5.15.177+/kernel/drivers/usb/musb/
sudo cp usb_modules/musb_hdrc.ko /lib/modules/5.15.177+/kernel/drivers/usb/musb/

sudo cp usb_modules/phy-am335x-control.ko /lib/modules/5.15.177+/kernel/drivers/usb/phy/
sudo cp usb_modules/phy-am335x.ko /lib/modules/5.15.177+/kernel/drivers/usb/phy/

sudo cp usb_modules/ledtrig-usbport.ko /lib/modules/5.15.177+/kernel/drivers/usb/core/
sudo cp usb_modules/usbcore.ko /lib/modules/5.15.177+/kernel/drivers/usb/core/

echo "update initramfs\n"
sudo update-initramfs -u -k $(uname -r)

echo "Done -> restart BBB\n"
