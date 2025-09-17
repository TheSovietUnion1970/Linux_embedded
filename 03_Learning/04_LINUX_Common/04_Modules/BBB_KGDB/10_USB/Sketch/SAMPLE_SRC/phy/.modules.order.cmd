cmd_drivers/usb/phy/modules.order := {   echo drivers/usb/phy/phy-am335x-control.ko;   echo drivers/usb/phy/phy-am335x.ko; :; } | awk '!x[$$0]++' - > drivers/usb/phy/modules.order
