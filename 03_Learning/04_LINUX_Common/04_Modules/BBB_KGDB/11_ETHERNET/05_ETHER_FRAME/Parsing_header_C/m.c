#include "parse.h"
#include "ssh.h"
#include "ping.h"
#include "arp.h"

void main(){
    // parse_and_print_packet(ping_rx, sizeof(ping_rx) - 8, "ping_rx");
    // parse_and_print_packet(ping_tx, sizeof(ping_tx), "ping_tx");

    // parse_and_print_packet(ssh_rx1, sizeof(ssh_rx1) - 8, "ssh_rx1", 0, 0);
    // parse_and_print_packet(ssh_tx1, sizeof(ssh_tx1), "ssh_tx1", 0, 0);

    // parse_and_print_packet(ssh_rx2, sizeof(ssh_rx2) - 8, "ssh_rx2", 0, 0);
    // parse_and_print_packet(ssh_tx2, sizeof(ssh_tx2), "ssh_tx2", 0, 0);

    // parse_and_print_packet(ssh_rx3, sizeof(ssh_rx3) - 8, "ssh_rx3", 0, 0);
    // parse_and_print_packet(ssh_tx3, sizeof(ssh_tx3), "ssh_tx3", 0, 0);

    parse_and_print_packet(ssh_rx4, sizeof(ssh_rx4) - 8, "ssh_rx4", 0, 0);
    //parse_and_print_packet(ssh_tx3, sizeof(ssh_tx3), "ssh_tx3", 0, 0);
}
