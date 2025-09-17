cmd_drivers/usb/musb/modules.order := {   echo drivers/usb/musb/musb_hdrc.ko;   echo drivers/usb/musb/musb_dsps.ko; :; } | awk '!x[$$0]++' - > drivers/usb/musb/modules.order
