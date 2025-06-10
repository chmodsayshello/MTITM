#pragma once
#include "genericpacket.h"
#include <stdbool.h>

// See https://github.com/luanti-org/luanti/blob/0695541bf59b305cc6661deb5d57a30e92cb9c6c/src/network/networkprotocol.h
typedef enum ClientOpCodes {
    ClientOp_Request_Peer_Id = 0x01,
    ClientOp_ToServer_Init = 0x02,
    ClientOp_ToServer_SRP_A = 0x51,
    ClientOp_ToServer_SRP_M = 0x52,
    ClientOp_ToServer_Init2 = 0x11,
    ClientOp_ToServer_Ready = 0x43,
    ClientOp_ToServer_SRP_First = 0x50,
    ClientOp_ToServer_Disco = 0x03
} ClientOpCodes;

PKTSTRUCT TOSERVER_REQUEST_PEERID {
    generic_header header;
    generic_reliable_header reliable_header;
    uint8_t cmd;
} TOSERVER_REQUEST_PEERID;

PKTSTRUCT TOSERVER_INIT {
    generic_header header; // type original
    uint16_t command;
    uint8_t ser_ver;
    uint16_t network_compression; // dummy
    uint16_t min_ver;
    uint16_t max_ver;
    uint16_t name_len;
    // char*
} TOSERVER_INIT;

PKTSTRUCT TOSERVER_SRP_BYTES_A {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;

    uint16_t len;
    // raw bytes;
    uint8_t base;
} TOSERVER_SRP_BYTES_A;

PKTSTRUCT TOSERVER_SRP_BYTES_M {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;

    uint16_t len;
    // raw bytes
} TOSERVER_SRP_BYTES_M;

PKTSTRUCT TOSERVER_INIT2 { // simply a specific ACK-Packet for Auth_Accept
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;
} TOSERVER_INIT2;

PKTSTRUCT TOSERVER_CLIENT_READY {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;

    uint8_t major_version;
    uint8_t minor_version;
    uint8_t patch_version;
    uint8_t reserved_version_space;

    uint16_t version_string_len;
    // raw version string bytes, let's make life easy and say ""
} TOSERVER_CLIENT_READY;

PKTSTRUCT TOSERVER_SRP_FIRST {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;

    uint16_t srp_salt_len;
    // raw salt bytes!
    uint16_t srp_key_len;
    // raw key bytes!
    bool empty;
} TOSERVER_SRP_FIRST;
#define SRP_FIRST_SALT_OFFSET sizeof(generic_seqnum_peek_header) + sizeof(uint16_t)

PKTSTRUCT TOSERVER_DISCO {
	generic_header header;
	generic_control_header control;
} TOSERVER_DISCO;

// EOF
