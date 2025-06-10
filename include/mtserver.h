#pragma once
#include <stdlib.h>
#include "clientpacket.h"
#include "proxy.h"

// All the magic is about to happen in here
void handle_clientpacket(void* raw_pkt, size_t size, Proxy* proxy);
void ack_clientpacket(uint16_t seqnum, Proxy* proxy);

void handle_toserver_request_peer_id(TOSERVER_REQUEST_PEERID* pkt, uint16_t* seqnum, Proxy* proxy);
void handle_toserver_init(TOSERVER_INIT* pkt, size_t size, uint16_t* seqnum, Proxy* proxy);
void handle_toserver_srp_bytes_a(TOSERVER_SRP_BYTES_A* pkt, size_t size, uint16_t* seqnum, Proxy* proxy);
void handle_toserver_srp_bytes_m(TOSERVER_SRP_BYTES_M* pkt, size_t size, uint16_t* seqnum, Proxy* proxy);
void kick_client(Proxy* proxy, uint16_t* seqnum);
// EOF