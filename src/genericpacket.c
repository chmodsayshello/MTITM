#include "genericpacket.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <assert.h>
#include "util.h"

void prepare_unreliable_packet(uint16_t peer_id, generic_header* pktheader) {
    pktheader->protocol_id = PROTOCOL_ID;
    pktheader->sender_peer_id = peer_id;
    pktheader->channel = 0;
}

void prepare_ack_packet(uint16_t peer_id, uint16_t seqnum, generic_ack_packet* pkt) {
    prepare_unreliable_packet(peer_id, &pkt->header);
    pkt->header.nextheader_type = TYPE_CONTROL;
    pkt->controlheader.controltype = CONTROLTYPE_ACK;
    pkt->seqnum = seqnum;
}

void prepare_reliable_header(generic_reliable_header* rel_header, uint16_t* seqnum, uint8_t nextheader) {
    rel_header->nextheader_type = nextheader;
    rel_header->seqnum = *seqnum;
    *seqnum = *seqnum + 1;
    // ++*seqnum or *seqnum++ won't work!!!
}

static uint16_t walk_header(void* header, uint8_t headertype, size_t size);
uint16_t get_pkt_command(void* pkt, size_t size) {
    assert(size > sizeof(generic_header));
    generic_header* header = (generic_header*) pkt;

    return walk_header(pkt + sizeof(generic_header), header->nextheader_type, size - sizeof(generic_header));
}

static uint16_t walk_header(void* header, uint8_t headertype, size_t size) {
    if (headertype == TYPE_CONTROL) {
        if (size < sizeof(generic_control_header)) {
            return 0;
        }
        const generic_control_header* control_header = (generic_control_header*) header;
        return (uint16_t) control_header->controltype;
    }
    if (headertype == TYPE_ORIGINAL) {
        if (size < sizeof(uint16_t)) {
            return 0;
        }
        const uint16_t* cmd = (uint16_t*) header;

        return ntohs(*cmd);
    }
    if (headertype == TYPE_RELIABLE) {
        if (size < sizeof(generic_reliable_header)) {
            return 0;
        }
        const generic_reliable_header* rel_header = (generic_reliable_header*) header;
        return walk_header(header + sizeof(generic_reliable_header), rel_header->nextheader_type, size - sizeof(generic_reliable_header));
    }
    printf("%s Ein unbekannter Headertyp %d soll geparst werden!%s\n", RED_BACK, headertype, RESET);
    return 0;
}
// EOF