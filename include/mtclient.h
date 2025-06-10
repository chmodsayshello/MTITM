#pragma once
#include <stddef.h>
#include <stdint.h>
#include "serverpacket.h"
#include "clientpacket.h"
#include <stdbool.h>

#define MIN_PROTOCOL_VERSION 0x25
#define MAX_PROTOCOL_VERSION 0x2a
#define SER_VER 0x1d

// The Server desperately wants to know the client's version
// This is everything but a proper minetest client, so let's just play pretend
#define MAJOR_VERSION 5
#define MINOR_VERSION 10
#define PATCH_VERSION 11

typedef struct MinetestClient {
    char* username;
    size_t username_len;
    int remote_fd;
    uint16_t seqnum;
    uint16_t peer_id;
    uint32_t sudo_methods;
} MinetestClient;

// Writes TOCLIENT_HELLO into that Pointer!
void client_verbindung_vorbereiten(MinetestClient* client, TOCLIENT_HELLO* toclient_hello_pkt);

// The raw bytes have to be RIGHT behind that struct
// Returns pointer to TOCLIENT_SRP_BYTES_S_B response
void* client_srp_A(MinetestClient* client, TOSERVER_SRP_BYTES_A* srp_a, size_t* size);

// Whether or not the server did accept auth
// The raw bytes have to be RIGHT behind that struct
bool client_srp_M(MinetestClient* client, TOSERVER_SRP_BYTES_M* srp_m);
void client_ack_incomming(MinetestClient* client);

void client_send_init2(MinetestClient* client);
void client_send_ready(MinetestClient* client);
void client_change_password(MinetestClient* client, uint16_t* seqnum, char* new_password);
void client_disconnect(MinetestClient* client);
// EOF
