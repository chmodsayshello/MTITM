#pragma once
#include "genericpacket.h"
#include <stdbool.h>

// https://github.com/luanti-org/luanti/blob/0695541bf59b305cc6661deb5d57a30e92cb9c6c/src/network/networkprotocol.h
typedef enum ServerOpCodes {
    ServerOp_Set_Peer_Id = 0x01,
    ServerOp_ToClient_Hello = 0x02,
    ServerOp_ToClient_SRP_S_B = 0x60,
    ServerOp_ToClient_Auth_Accept = 0x03,
    ServerOpt_ToClietn_Sudo_Accept = 0x04,
    ServerOp_ToClient_Access_Denied = 0x0a
} ServerOpCodes;

// More or less directly taken from minetest's sources. See src/network/networkprotocol.h
typedef enum AccessDeniedCode {
	SERVER_ACCESSDENIED_WRONG_PASSWORD,
	SERVER_ACCESSDENIED_UNEXPECTED_DATA,
	SERVER_ACCESSDENIED_SINGLEPLAYER,
	SERVER_ACCESSDENIED_WRONG_VERSION,
	SERVER_ACCESSDENIED_WRONG_CHARS_IN_NAME,
	SERVER_ACCESSDENIED_WRONG_NAME,
	SERVER_ACCESSDENIED_TOO_MANY_USERS,
	SERVER_ACCESSDENIED_EMPTY_PASSWORD,
	SERVER_ACCESSDENIED_ALREADY_CONNECTED,
	SERVER_ACCESSDENIED_SERVER_FAIL,
	SERVER_ACCESSDENIED_CUSTOM_STRING,
	SERVER_ACCESSDENIED_SHUTDOWN,
	SERVER_ACCESSDENIED_CRASH,
	SERVER_ACCESSDENIED_MAX,
} AccessDeniedCode;

PKTSTRUCT TOCLIENT_ASSIGN_PEER_ID {
    generic_header header;
    generic_reliable_header reliable_header;
    generic_control_header controlheader;
    uint16_t peer_id;
} TOCLIENT_ASSIGN_PEER_ID;

PKTSTRUCT TOCLIENT_HELLO {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;
    uint8_t ser_ver;
    uint16_t junk;
    uint16_t protocol_ver;
    uint32_t auth_methods;
    uint16_t junk2;
} TOCLIENT_HELLO;

PKTSTRUCT TOCLIENT_SRP_S_B {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;
    uint16_t s_len;
    uint8_t* s; // This will point somewhere onto the heap!
    uint16_t b_len;
    uint8_t* b; // This will point somewhere onto the heap!
} TOCLIENT_SRP_S_B;

PKTSTRUCT TOCLIENT_AUTH_ACCEPT {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;

    float junk[3]; // unused 
    uint64_t map_seed;
    float send_interval; // not all that interesting for this program
    uint32_t sudo_methods;
} TOCLIENT_AUTH_ACCEPT;

PKTSTRUCT TOCLIENT_ACCEPT_SUDO {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;

    uint32_t insudo_auth_mechs;
} TOCLIENT_ACCEPT_SUDO;

PKTSTRUCT TOCLIENT_ACCESS_DENIED {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command;

    uint8_t reason;
    uint16_t custom_reason_len;
    // if there's a custom reason, the bytes will be here!
    bool reconnect;
} TOCLIENT_ACCESS_DENIED;
#define TOCLIENT_ACCESS_DENIED_REASON_OFFSET sizeof(TOCLIENT_ACCESS_DENIED) - sizeof(bool)
// EOF