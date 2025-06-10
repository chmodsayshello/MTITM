#include "mtclient.h"
#include "clientpacket.h"

#include <sys/socket.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

#include <srp.h>

static void ack_serverpacket(MinetestClient* client, uint16_t seqnum, uint8_t channel) {
    generic_ack_packet ack;
    prepare_ack_packet(client->peer_id, seqnum, &ack);
    ack.header.channel = channel;
    send(client->remote_fd, &ack, sizeof(ack), 0);
}

// TODO: TIMEOUT!!!
static size_t listen_until_pkt(MinetestClient* client, void* buff, size_t buffsize, ServerOpCodes code) {
    size_t size;
    do {
        size = recv(client->remote_fd, buff, buffsize, 0);
    } while (get_pkt_command(buff, buffsize) != code);
    return size;
}

static void peer_id_erlangen(MinetestClient* client) {
    TOSERVER_REQUEST_PEERID peerIdRequest;
    prepare_unreliable_packet(PEER_ID_INEXISTENT, &peerIdRequest.header);
    peerIdRequest.header.nextheader_type = TYPE_RELIABLE;
    peerIdRequest.reliable_header.seqnum = client->seqnum++;

    peerIdRequest.cmd = ClientOp_Request_Peer_Id;

    send(client->remote_fd, (void*) &peerIdRequest, sizeof(TOSERVER_REQUEST_PEERID), 0);

    TOCLIENT_ASSIGN_PEER_ID peer_id_pkt;
    recv(client->remote_fd, &peer_id_pkt, sizeof(TOCLIENT_ASSIGN_PEER_ID), 0);
    client->peer_id = peer_id_pkt.peer_id;
    printf("Peer id %i\n", client->peer_id);
    ack_serverpacket(client, peer_id_pkt.reliable_header.seqnum, peer_id_pkt.header.channel);
}

static void toserver_init_senden(MinetestClient* client) {
    const size_t size = sizeof(TOSERVER_INIT) + client->username_len;
    TOSERVER_INIT* initpkt = malloc(size);
    assert(initpkt != NULL);
    prepare_unreliable_packet(client->peer_id, &initpkt->header);
    initpkt->header.nextheader_type = TYPE_ORIGINAL;
    initpkt->command = ClientOp_ToServer_Init;
    initpkt->max_ver = MAX_PROTOCOL_VERSION;
    initpkt->min_ver = MIN_PROTOCOL_VERSION;
    initpkt->ser_ver = SER_VER;
    initpkt->name_len = client->username_len;

    // Copy name to outgoing buffer
    memcpy((void*) initpkt + sizeof(TOSERVER_INIT), client->username, client->username_len);

    send(client->remote_fd, initpkt, size, 0);

    free(initpkt);
}

void client_disconnect(MinetestClient* client) {
	TOSERVER_DISCO disconnect_pkt;
	prepare_unreliable_packet(client->peer_id, &disconnect_pkt.header);
	disconnect_pkt.header.nextheader_type = TYPE_CONTROL;
	disconnect_pkt.control.controltype = ClientOp_ToServer_Disco;
	send(client->remote_fd, &disconnect_pkt, sizeof(TOSERVER_DISCO), 0);
}

void client_verbindung_vorbereiten(MinetestClient* client, TOCLIENT_HELLO* toclient_hello_pkt) {
    memset(toclient_hello_pkt, 0x00, sizeof(TOCLIENT_HELLO));
    if (client->seqnum != SEQNUM_INITAL) {
        return;
    }
    peer_id_erlangen(client);
    toserver_init_senden(client);

    listen_until_pkt(client, toclient_hello_pkt, sizeof(TOCLIENT_HELLO), ServerOp_ToClient_Hello);
    ack_serverpacket(client, toclient_hello_pkt->reliable_header.seqnum, toclient_hello_pkt->header.channel);
}

void* client_srp_A(MinetestClient* client, TOSERVER_SRP_BYTES_A* srp_a, size_t* size) {
    client_ack_incomming(client);

    // screw the layout of this packet. multiple dynamic size buffers in one packet - several constant sizes are behind those...
    srp_a->header.sender_peer_id = client->peer_id;
    srp_a->reliable_header.seqnum = client->seqnum++;
    srp_a->header.channel = 0;

    send(client->remote_fd, srp_a, sizeof(TOSERVER_SRP_BYTES_A) + srp_a->len, 0);

    const size_t sb_buffsize = 0x1800; // again, 6kb should be enough! 512 Bytes theoretically are enough
    void* buff = malloc(sb_buffsize);
    if (buff == NULL) {
        fatal_error("Out of memory!");
        return;
    }
    // Receive raw data
    *size = listen_until_pkt(client, buff, sb_buffsize, ServerOp_ToClient_SRP_S_B);

    return buff;
}

// For keepalive-purposes
void client_ack_incomming(MinetestClient* client) {
    const size_t minsize = sizeof(generic_seqnum_peek_header);
    generic_seqnum_peek_header header;
    memset(&header, 0x00, minsize);

    while (recv(client->remote_fd, &header, minsize, MSG_DONTWAIT) > 0) {
        if (header.header.protocol_id != PROTOCOL_ID) {
            continue;
        }
        if (header.header.nextheader_type != TYPE_RELIABLE) {
            continue;
        }
        ack_serverpacket(client, header.reliable_header.seqnum, header.header.channel);
    }
}

union auth_accept_reject_union {
    TOCLIENT_ACCESS_DENIED denied;
    TOCLIENT_AUTH_ACCEPT accept;
    TOCLIENT_ACCEPT_SUDO sudo;
};

bool client_srp_M(MinetestClient* client, TOSERVER_SRP_BYTES_M* srp_m) {
    client_ack_incomming(client);

    srp_m->header.sender_peer_id = client->peer_id;
    srp_m->reliable_header.seqnum = client->seqnum++;
    srp_m->header.channel = 0;

    send(client->remote_fd, srp_m, sizeof(TOSERVER_SRP_BYTES_M) + srp_m->len, 0);


    union auth_accept_reject_union accept_reject;
    generic_seqnum_peek_header* header = (generic_seqnum_peek_header*) &accept_reject;
    memset(&accept_reject, 0x00, sizeof(accept_reject));
    size_t read;

    // listen_until_pkt isn't sufficient here
    do {
        read = recv(client->remote_fd, &accept_reject, sizeof(accept_reject), 0);
    } while(header->command != ServerOp_ToClient_Auth_Accept &&
        header->command != ServerOp_ToClient_Access_Denied &&
        header->command != ServerOpt_ToClietn_Sudo_Accept
    );

    if (header->command == ServerOp_ToClient_Access_Denied) {
        return false;
    }
    if (header->command == ServerOpt_ToClietn_Sudo_Accept) {
        puts("In Sudo-Mode!");
        return true;
    }

    client->sudo_methods = accept_reject.accept.sudo_methods;

    client_send_init2(client);
    client_send_ready(client);
    return true;
}

void client_send_init2(MinetestClient* client) {
    TOSERVER_INIT2 init2;
    memset(&init2, 0x00, sizeof(TOSERVER_INIT2));

    prepare_unreliable_packet(client->peer_id, &init2.header);
    init2.header.nextheader_type = TYPE_RELIABLE;
    prepare_reliable_header(&init2.reliable_header, &client->seqnum, TYPE_ORIGINAL);
    init2.command = ClientOp_ToServer_Init2;

    send(client->remote_fd, &init2, sizeof(init2), 0);
}

void client_send_ready(MinetestClient* client) {
    client_ack_incomming(client);
    TOSERVER_CLIENT_READY readypkt;
    memset(&readypkt, 0x00, sizeof(TOSERVER_CLIENT_READY));

    prepare_unreliable_packet(client->peer_id, &readypkt.header);
    readypkt.header.nextheader_type = TYPE_RELIABLE;
    prepare_reliable_header(&readypkt.reliable_header, &client->seqnum, TYPE_ORIGINAL);
    readypkt.command = ClientOp_ToServer_Ready;

    readypkt.major_version = MAJOR_VERSION;
    readypkt.minor_version = MINOR_VERSION;
    readypkt.patch_version = PATCH_VERSION;

    readypkt.version_string_len = 0;

    send(client->remote_fd, &readypkt, sizeof(readypkt), 0);
}

void client_change_password(MinetestClient* client, uint16_t* seqnum, char* new_password) {
    void* salt = NULL;
    size_t len_salt;
    void* verifier = NULL;
    size_t len_verifier;
    SRP_Result res = srp_create_salted_verification_key(SRP_SHA256, SRP_NG_2048,
        client->username, new_password, strlen(new_password),
        (unsigned char**) &salt, &len_salt, (unsigned char**) &verifier, &len_verifier,
        NULL, NULL
    );

    if (salt == NULL || verifier == NULL || res != SRP_OK) {
        fatal_error("Out of memory!");
    }

    // Packet
    const size_t pkt_total_size = sizeof(TOSERVER_SRP_FIRST) + len_salt + len_verifier;
    TOSERVER_SRP_FIRST* pkt = calloc(1, pkt_total_size);
    // Thanks to calloc: is_empty = false, so less pointer arithmatik
    if (pkt == NULL) {
        fatal_error("Out of memory!");
    }

    prepare_unreliable_packet(client->peer_id, &pkt->header);
    pkt->header.nextheader_type = TYPE_RELIABLE;
    prepare_reliable_header(&pkt->reliable_header, &client->seqnum, TYPE_ORIGINAL);
    pkt->command = ClientOp_ToServer_SRP_First;
    pkt->srp_salt_len = len_salt;

    memcpy((void*) pkt + SRP_FIRST_SALT_OFFSET, salt, len_salt);

    uint16_t* len_verifier_ptr = (uint16_t*) ((void*) pkt + SRP_FIRST_SALT_OFFSET + len_salt);
    *len_verifier_ptr = htons(len_verifier);

    memcpy((void*) len_verifier_ptr + sizeof(uint16_t), verifier, len_verifier);

    send(client->remote_fd, pkt, pkt_total_size, 0);

    free(pkt);
    free(salt);
    free(verifier);
}
// EOF
