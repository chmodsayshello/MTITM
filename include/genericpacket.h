#pragma once
#include <stdint.h>
#include <stddef.h>

#define PROTOCOL_ID 0x4f457403
#define SEQNUM_INITAL 65500

#define BIGE __attribute__((scalar_storage_order("big-endian")))

#define PACK __attribute__((__packed__))

/* Tell GCC that weird shit with pointers related to Endianess will happen and I'm all for it! */
#pragma GCC diagnostic ignored "-Wscalar-storage-order"

#define PKTSTRUCT typedef struct BIGE PACK

#define TYPE_RELIABLE 3
#define TYPE_ORIGINAL 1
#define TYPE_CONTROL 0

#define CONTROLTYPE_ACK 0

#define PEER_ID_INEXISTENT 0
#define PEER_ID_SERVER 1

PKTSTRUCT generic_header {
    uint32_t protocol_id;
    uint16_t sender_peer_id;
    uint8_t channel;
    uint8_t nextheader_type;
} generic_header;

PKTSTRUCT generic_control_header {
    uint8_t controltype;
} generic_control_header;

PKTSTRUCT generic_reliable_header {
    uint16_t seqnum;
    uint8_t nextheader_type;
} generic_reliable_header;

PKTSTRUCT generic_ack_packet {
    generic_header header;
    generic_control_header controlheader;
    uint16_t seqnum;
} generic_ack_packet;

PKTSTRUCT generic_seqnum_peek_header {
    generic_header header;
    generic_reliable_header reliable_header;
    uint16_t command; // only if nexttype = TYPE_ORIGINAL!
} generic_seqnum_peek_header;

void prepare_ack_packet(uint16_t peer_id, uint16_t seqnum, generic_ack_packet* pkt);
void prepare_unreliable_packet(uint16_t peer_id, generic_header* pktheader);
void prepare_reliable_header(generic_reliable_header* rel_header, uint16_t* seqnum, uint8_t nextheader);
uint16_t get_pkt_command(void* pkt, size_t size); // returns controltype for control packets!
// EOF