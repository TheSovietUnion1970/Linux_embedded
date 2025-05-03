
sudo insmod /lib/modules/5.15.177/kernel/drivers/usb/gadget/legacy/g_ether.ko
sudo ip addr add 192.168.137.2/24 dev usb0
sudo ip link set usb0 up

echo "Checking /etc/resolv.conf: "
cat /etc/resolv.conf

echo "Checking ip route"
sudo ip route add default via 192.168.137.1 dev usb0
ip route

echo "Restart networking..."
sudo service networking restart

echo "Checing networking status..."
systemctl status networking

echo "Install libatomic1 for handling ~/.vscode-server..."
sudo apt update
sudo apt install libatomic1