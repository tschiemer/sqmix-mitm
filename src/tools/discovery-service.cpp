#include <cstdio>
#include <csignal>

#include "DiscoveryService.h"

int main(int argc, char * argv[]) {

    if (2 != argc){
        printf(
                "Usage: %s <name-of-pretend-mixer>\n"
                "Listen to any SQ Mixer inquiries and respond such that *this* host is assumed to be a mixer of the SQ series.\n"
                , argv[0]);
        return EXIT_SUCCESS;
    }

    SQMixMitm::DiscoveryService discoveryService;

    discoveryService.name(argv[1]);

    discoveryService.start();


    sigset_t wset;
    sigemptyset(&wset);
    sigaddset(&wset,SIGHUP);
    sigaddset(&wset,SIGINT);
    sigaddset(&wset,SIGTERM);
    int sig;

    // wait for any signal to stop
    printf("Wait for signal to stop..\n");
    sigwait(&wset,&sig);

    discoveryService.stop();


    return EXIT_SUCCESS;
}
