#ifndef MAIN_H
#define MAIN_H

#define PRINT_DEBUG 0

/* Debug init */
#define PRINT_DEBUG_INIT 0

/* Debug for tx, rx rate */
#define PRINT_DEBUG_RATE 1

/* Debug for association process */
#define PRINT_DEBUG_ROC 0
/*
Only happens if connected to wifi hotspot

==================== wlan0: authenticate with f4:27:56:13:90:d8 ===============
[BSS_STATE] - 1					    => BSS_CHANGED_IDLE
[BSS_STATE] - 3, sta = 0x0			=> X
[BSS_STATE] - 4 - SET				=> wificore_set_bssid, wifi_cmd_role_start_sta

	[STA_STATE] - 6				    => ROC -> sta_no_exist -> sta_none
	
==================== wlan0: send auth to f4:27:56:13:90:d8 (try 1/3) 
		     wlan0: authenticated
		     wlan0: associate with f4:27:56:13:90:d8 (try 1/3)
		     wlan0: RX AssocResp from f4:27:56:13:90:d8 (capab=0x1431 status=0 aid=1) ===============
		     
	[STA_STATE] - 4				    => auth -> assoc
	
[BSS_STATE] - 3, sta = 0xca0077a0	=> Get supported rates from AP
[BSS_STATE] - 6					    => wificore_set_assoc
[BSS_STATE] - 7 - DISABLE			=> DISBALE arp filter

==================== wlan0: associated ===============

[BSS_STATE] - 3, sta = 0xca0077a0	=> Get supported rates from AP
[BSS_STATE] - 5					    => enable beacon filtering

==================== IPv6: ADDRCONF(NETDEV_CHANGE): wlan0: link becomes ready ==========

[BSS_STATE] - 2					    => Connection Quality Monitor (for rssi)

	[STA_STATE] - 1 			    => wifi_set_authorized
	
==================== wifi0: Association completed. ============================
	
	[STA_STATE] - 5				    => X ROC -> authrized
	
[BSS_STATE] - 7 - ENABLE			=> ENABLE arp filter





==================== wlan0: deauthenticating from f4:27:56:13:90:d8 ===============
	[STA_STATE] - 2				    => AUTHORIZED -> ASSOC
	[STA_STATE] - 3				    => ASSOC      -> AUTH
	[STA_STATE] - 5				    => AUTH       -> NOTEXIST
	
[BSS_STATE] - 3, sta = 0x0			=> X
[BSS_STATE] - 4 - CLEAR				=> wificore_clear_bssid, wifi_cmd_role_stop_sta
[BSS_STATE] - 6					    => wificore_set_unssoc
[BSS_STATE] - 7 - DISABLE
[BSS_STATE] - 1                     => => BSS_CHANGED_IDLE
*/

/* wifi_op_hw_scan is triggered periodically if no connection */
#define PRINT_DEBUG_SCAN 0

/* config rx frame filtering */
#define PRINT_DEBUG_CONFIG_FILTER 0

/* Print tx and rx frame */
#define PRINT_DEBUG_DATA_FRAME 0
#define RX_LIMIT 100 /* limit rx frame up to RX_LIMIT */
#define TX_LIMIT 100 /* limit tx frame up to TX_LIMIT */

/* Debug watchdog TX */
#define PRINT_DEBUG_TX_WATCHDOG 0
/*
Ex:
[ 8475.832158] Allocated blks for transmit: 2
[ 8475.839571] Free blks for transmit: 2

[ 8477.581393] Allocated blks for transmit: 3
[ 8477.585674] Allocated blks for transmit: 2
[ 8477.593452] Free blks for transmit: 5

	-> no watchdog tx stuck

[ 8475.832158] Allocated blks for transmit: 3
[ 8475.839571] Free blks for transmit: 2
	-> watchdog tx struct

*/

#define APPLY_EXTERNAL_CONFIG 0

#endif