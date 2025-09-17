cmd_drivers/usb/musb/Module.symvers := sed 's/\.ko$$/\.o/' drivers/usb/musb/modules.order | scripts/mod/modpost -m   -o drivers/usb/musb/Module.symvers -e -i Module.symvers   -T -
