#!/bin/bash
(make -j$(nproc)) || { echo "Failed to build $dir"; exit 1; }
echo "All drivers built successfully!"

sudo scp sdio.ko wifi0.ko debian@192.168.137.2:/home/debian/
echo "Done!"