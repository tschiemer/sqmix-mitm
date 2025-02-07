/**
* sqmix-mitm
* Copyright (C) 2025  Philip Tschiemer
*
* This program is free software: you can redistribute it and/or modify
        * it under the terms of the GNU Affero General Public License as published by
        * the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
        * but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <cstdio>
#include <csignal>

#include "DiscoveryResponder.h"

int main(int argc, char * argv[]) {

    if (2 != argc){
        printf(
                "Usage: %s <name-of-pretend-mixer>\n"
                "Listen to any SQ Mixer inquiries and respond such that *this* host is assumed to be a mixer of the SQ series.\n"
                , argv[0]);
        return EXIT_SUCCESS;
    }

    SQMixMitm::DiscoveryResponder discoveryResponder;

    discoveryResponder.name(argv[1]);

    if (discoveryResponder.start()){
        return EXIT_FAILURE;
    }


    sigset_t wset;
    sigemptyset(&wset);
    sigaddset(&wset,SIGHUP);
    sigaddset(&wset,SIGINT);
    sigaddset(&wset,SIGTERM);
    int sig;

    // wait for any signal to stop
    printf("Wait for signal to stop..\n");
    sigwait(&wset,&sig);

    if (discoveryResponder.stop()){
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}
