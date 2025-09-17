cmd_drivers/usb/phy/Module.symvers := sed 's/\.ko$$/\.o/' drivers/usb/phy/modules.order | scripts/mod/modpost -m   -o drivers/usb/phy/Module.symvers -e -i Module.symvers   -T -
