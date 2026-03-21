# build-vinh.bb - Custom image with your U-Boot and kernel

SUMMARY = "Custom image for Vinh's BeagleBone project"
DESCRIPTION = "Minimal image with custom U-Boot and kernel"

# Force your providers first
#PREFERRED_PROVIDER_virtual/bootloader = "u-boot-v"
# PREFERRED_PROVIDER_virtual/kernel = "linux-stable-rcn-ee_5.15"

inherit core-image

IMAGE_INSTALL += " \
    kernel-devicetree \
    kernel-modules \

"

IMAGE_FEATURES += "ssh-server-dropbear"

IMAGE_FSTYPES += "tar.bz2 wic wic.bmap"
