#!/usr/bin/env bash
# =============================================
# Wi-Fi connection and internet test script
# Network: NGOC VY
# Password: hardcoded
# Interface: wlan0
# =============================================

set -euo pipefail

INTERFACE="wlan0"
SSID="NGOC VY"
PASSWORD="28012015"

echo "Wi-Fi Connection Script"
echo "────────────────────────"
echo "Target network : $SSID"
echo "Interface      : $INTERFACE"
echo ""

# ───────────────────────────────────────────────
# Check prerequisites
# ───────────────────────────────────────────────
if ! command -v nmcli >/dev/null 2>&1; then
    echo "Error: nmcli is not installed."
    echo "This script requires NetworkManager."
    exit 1
fi

if ! ip link show "$INTERFACE" &>/dev/null; then
    echo "Error: Interface '$INTERFACE' not found."
    echo "Available interfaces:"
    ip link show | grep -i -E 'wlan|wifi'
    exit 1
fi

# ───────────────────────────────────────────────
# Step 1: Show available networks
# ───────────────────────────────────────────────
echo "Scanning for Wi-Fi networks..."
echo "───────────────────────────────"
#nmcli -f SSID,SIGNAL,SECURITY,BARS device wifi list --rescan yes
echo ""

if ! nmcli device wifi list | grep -q "$SSID"; then
    echo "Warning: '$SSID' was not found in the scan."
    echo "Make sure the network is in range and broadcasting."
    echo ""
fi

# ───────────────────────────────────────────────
# Step 2: Connect to the network (password is hardcoded)
# ───────────────────────────────────────────────
echo "Connecting to '$SSID'..."
echo ""

if sudo nmcli device wifi connect "$SSID" \
    password "$PASSWORD" \
    ifname "$INTERFACE"; then
    
    echo ""
    echo "✓ Connection attempt completed."
else
    echo ""
    echo "✗ Connection failed."
    echo "Possible reasons: wrong password, network not in range, authentication type mismatch, etc."
    echo ""
    exit 1
fi

# Give DHCP some time to complete
sleep 2

# ───────────────────────────────────────────────
# Step 3: Show connection status
# ───────────────────────────────────────────────
echo ""
echo "Current connection status:"
nmcli -c no connection show --active | grep -E 'NAME|DEVICE|TYPE|UUID' || true
echo ""

ip -4 addr show "$INTERFACE" | grep -E 'inet |valid_lft' || true
echo ""

# ───────────────────────────────────────────────
# Step 4: Test internet connectivity
# ───────────────────────────────────────────────
echo "Testing internet connection via $INTERFACE..."
echo "──────────────────────────────────────────"

# Test 1: basic IP connectivity
if sudo ping -c 4 -W 5 -I "$INTERFACE" 8.8.8.8 >/dev/null 2>&1; then
    echo "✓ Can reach 8.8.8.8 (IP connectivity OK)"
else
    echo "✗ Cannot reach 8.8.8.8"
fi

# Test 2: DNS + real website
echo -n "Testing DNS and real website... "
if sudo ping -c 3 -W 5 -I "$INTERFACE" google.com >/dev/null 2>&1; then
    echo "✓ OK (google.com resolved and reachable)"
else
    echo "✗ Failed (DNS or internet issue)"
fi

echo ""
echo "Final check summary:"
if sudo ping -c 1 -W 4 -I "$INTERFACE" google.com >/dev/null 2>&1; then
    echo "→ Internet appears to be WORKING"
else
    echo "→ No internet connectivity detected"
    echo ""
    echo "Troubleshooting commands you can try:"
    echo "  nmcli device status"
    echo "  nmcli connection show \"$SSID\""
    echo "  journalctl -u NetworkManager -n 60 --no-pager"
fi

echo ""
echo "Done."
