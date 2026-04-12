#!/bin/bash
for dir in wlcore wl18xx; do
    echo "=== Building $dir ==="
    (cd "$dir" && make -j$(nproc)) || { echo "Failed to build $dir"; exit 1; }
done
echo "All drivers built successfully!"

sudo scp ./wl18xx/wl18.ko ./wlcore/wlc.ko debian@192.168.137.2:/home/debian/
echo "Done!"