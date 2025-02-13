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
#include <getopt.h>
#include <vector>

#include "MixMitm.h"



static char * argv0 = nullptr;

static struct {
    std::vector<SQMixMitm::Event::Type> listenForEvents;
} opts;

static void help(){
    printf(
            "Usage: %s [-e<event>]<ip-of-mixer>\n"
            "Act as Man-in-the-Middle service for mixer with given IP\n"
            "Note: requires service to be discoverable (such as when using sq-discovery-responder)\n"
            "\nOptions:\n"
            "\t -e<event>   Show events of this type (<event> := midifader, channelselect, midisoftcontrol)\n"
            "\nExamples:\n"
            "%s -e channelselect -e midisoftcontrol 192.168.1.100\n"
            , argv0, argv0);
}

int main(int argc, char * argv[]) {

    argv0 = argv[0];

    int opt;

    while ((opt = getopt(argc, argv, "?he:")) != -1) {
        switch (opt) {
            case 'n':
                break;
            case 'e':
                if (strcmp(optarg, "midifader") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFader);
                }
                else if (strcmp(optarg, "channelselect") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::ChannelSelect);
                }
                else if (strcmp(optarg, "midisoftcontrol") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiSoftControl);
                }
                break;

            case '?':
            case 'h':
            default: /* '?' */
                help();
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc){
        printf("ERROR missing argument");
        help();
        return EXIT_FAILURE;
    }

    char *ip = argv[optind];



    SQMixMitm::MixMitm mixMitm;

    mixMitm.onStateChanged([](SQMixMitm::MixMitm::State state){
        if (state == SQMixMitm::MixMitm::Running) {
            printf("SERVICE STATE running!\n");
        }
        else if (state == SQMixMitm::MixMitm::Stopped) {
            printf("SERVICE STATE stopped!\n");
        }
    });

    mixMitm.onConnectionStateChanged([&mixMitm](SQMixMitm::MixMitm::ConnectionState state){
        if (state == SQMixMitm::MixMitm::Connected) {
            SQMixMitm::MixMitm::Version version = mixMitm.version();
            printf(
                    "CONNECTION STATE connected (mixer firmware %d.%d.%d r%d)\n"
                    , version.major, version.minor, version.patch, version.build
            );
        }
        else if (state == SQMixMitm::MixMitm::Disconnected) {
            printf("CONNECTION STATE  disconnected\n");
        }
    });

    std::for_each(opts.listenForEvents.begin(), opts.listenForEvents.end(), [&mixMitm](SQMixMitm::Event::Type type){
       mixMitm.onEvent(type, [](SQMixMitm::Event &event){
          if (event.type == SQMixMitm::Event::Types::MidiFader){
              printf("EVENT midi fader %d value %d\n", event.MidiFader_fader(), event.MidiFader_value());
          }
          else if (event.type == SQMixMitm::Event::Types::ChannelSelect){
              printf("EVENT channel select %d\n", event.ChannelSelect_channel());
          }
          else if (event.type == SQMixMitm::Event::Types::MidiSoftControl){
              printf("EVENT soft control type %d channel %d value1 %d value2 %d\n", event.MidiSoftControl_type(), event.MidiSoftControl_channel(), event.MidiSoftControl_value1(), event.MidiSoftControl_value2());
          }
       });
    });


    printf("Starting services... \n");

    if (mixMitm.start(ip)){
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


    printf("Shutting down services..\n");
    if (mixMitm.stop()){
        return EXIT_FAILURE;
    }


    return EXIT_SUCCESS;
}