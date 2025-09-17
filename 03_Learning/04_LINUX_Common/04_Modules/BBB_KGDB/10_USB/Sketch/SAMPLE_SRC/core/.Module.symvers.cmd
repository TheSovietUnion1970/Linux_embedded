cmd_drivers/usb/core/Module.symvers := sed 's/\.ko$$/\.o/' drivers/usb/core/modules.order | scripts/mod/modpost -m   -o drivers/usb/core/Module.symvers -e -i Module.symvers   -T -
