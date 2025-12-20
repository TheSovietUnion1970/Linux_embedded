#include "parse.h"
#include "ssh.h"
#include "ping.h"
#include "arp.h"
#include <stdio.h>

void main(){
    // /* ======================== PING ========================== */
    // parse_and_print_packet(ping_rx, sizeof(ping_rx) - 8, "ping_rx");
    // parse_and_print_packet(ping_tx, sizeof(ping_tx), "ping_tx");

    // /* ======================== SSH ========================== */
    // /* SYN -> SYN + ACK: Establish TCP connection */
    // parse_and_print_packet(ssh_rx1, sizeof(ssh_rx1) - 8, "ssh_rx1", 0, 0);
    // parse_and_print_packet(ssh_tx1, sizeof(ssh_tx1), "ssh_tx1", 0, 0);

    // /* PSH -> PSH + ACK: Id string exchange */
    // parse_and_print_packet(ssh_rx2, sizeof(ssh_rx2) - 8, "ssh_rx2", 0, 0);
    // parse_and_print_packet(ssh_tx2, sizeof(ssh_tx2), "ssh_tx2", 0, 0);

    // /* PSH -> PSH + ACK: Id string exchange */
    // parse_and_print_packet(ssh_rx3, sizeof(ssh_rx3) - 8, "ssh_rx3", 0, 0);
    // parse_and_print_packet(ssh_tx3, sizeof(ssh_tx3), "ssh_tx3", 0, 0);


    // /* SSH_MSG_KEXINIT: algorithms negotiation */
    // parse_and_print_packet(ssh_tx4, sizeof(ssh_tx4), "ssh_tx4", 0, 0);
    // parse_and_print_ssh_kexinit(ssh_tx4, sizeof(ssh_tx4), "ssh_tx4");
    // parse_and_print_packet(ssh_rx4, sizeof(ssh_rx4) - 8, "ssh_rx4", 0, 0);
    // parse_and_print_ssh_kexinit(ssh_rx4, sizeof(ssh_rx4) - 8, "ssh_rx4");

    // /* SSH_MSG_KEXINIT ACK */
    // parse_and_print_packet(ssh_tx5, sizeof(ssh_tx5), "ssh_tx5", 0, 0);


    parse_and_print_packet(ssh_tx8, sizeof(ssh_tx8), "ssh_tx8", 0, 0);
    parse_ssh_kex_reply(ssh_tx8 + 71, 188 - 5);


}

