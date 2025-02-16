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
    bool midisend;
} opts = {
        .midisend = false,
};

static std::thread * midifader_send_thread = nullptr;

static void help(){
    printf(
            "Usage: %s [-es] <ip-of-mixer>\n"
            "Act as Man-in-the-Middle service for mixer with given IP\n"
            "Note: requires service to be discoverable (such as when using sq-discovery-responder)\n"
            "\nOptions:\n"
            "\t -e   Show events\n"
            "\t -s   Slowly fade MIDI faders 1+2 up from 0%% to 100%%\n"
            "\nExamples:\n"
            "%s -e channelselect -e midisoftcontrol 192.168.1.100\n"
            , argv0, argv0);
}

static const char * miditype_name(uint8_t type){
    switch (type & 0xf0){
        case SQMixMitm::Event::MidiType::NoteOff:
            return "Note Off";
        case SQMixMitm::Event::MidiType::NoteOn:
            return "Note On";
        case SQMixMitm::Event::MidiType::ControlChange:
            return "CC";
        case SQMixMitm::Event::MidiType::ProgramChange:
            return "PC";
        default:
            return "(midi type not in this LUT)";

    }
}

static const char * midimmc_name(uint8_t cmd){
    switch(cmd){
        case SQMixMitm::Event::MidiMmcType::Stop:
            return "Stop";
        case SQMixMitm::Event::MidiMmcType::Play:
            return "Play";
        case SQMixMitm::Event::MidiMmcType::FastForward:
            return "Fast Forward";
        case SQMixMitm::Event::MidiMmcType::Rewind:
            return "Rewind";
        case SQMixMitm::Event::MidiMmcType::Record:
            return "Record";
        case SQMixMitm::Event::MidiMmcType::Pause:
            return "Pause";
        default:
            return "(midi mmc command not in this LUT)";
    }
}

void onEventCallback(SQMixMitm::Event &event){
    if (event.type == SQMixMitm::Event::Types::ChannelSelect){
        printf("EVENT channel select %d\n", event.ChannelSelect_channel());
    }
    else if (event.type == SQMixMitm::Event::Types::MidiFaderLevel){
        printf("EVENT midifader level channel %d value %d\n", event.MidiFaderLevel_channel(), event.MidiFaderLevel_value());
    }
    else if (event.type == SQMixMitm::Event::Types::MidiFaderSelect){
        printf("EVENT midifader select channel %d\n", event.MidiFaderSelect_channel());
    }
    else if (event.type == SQMixMitm::Event::Types::MidiFaderMute){
        printf("EVENT midifader mute channel %d\n", event.MidiFaderMute_channel());
    }
    else if (event.type == SQMixMitm::Event::Types::MidiFaderPAFL){
        printf("EVENT midifader pafl channel %d\n", event.MidiFaderPAFL_channel());
    }
    else if (event.type == SQMixMitm::Event::Types::MidiSoftKey){
        printf("EVENT soft key type %s (%02x) channel %d value1 %d value2 %d\n",
               miditype_name(event.MidiSoftKey_type()),
               event.MidiSoftRotary_type(),
               event.MidiSoftKey_channel(),
               event.MidiSoftKey_value1(),
               event.MidiSoftKey_value2());
    }
    else if (event.type == SQMixMitm::Event::Types::MidiSoftRotary){
        printf("EVENT soft rotary type %s (%02x) channel %d value1 %d value2 %d\n",
               miditype_name(event.MidiSoftRotary_type()),
               event.MidiSoftRotary_type(),
               event.MidiSoftRotary_channel(),
               event.MidiSoftRotary_value1(),
               event.MidiSoftRotary_value2());
    }
    else if (event.type == SQMixMitm::Event::Types::MidiMmc){
        printf("EVENT Midi MMC command %s (%hhu)\n",
               midimmc_name(event.MidiMmc_cmd()),
               event.MidiMmc_cmd());
    }
    else {
        printf("EVENT but it' s kinda unknown hmmm\n");
    }
}

int main(int argc, char * argv[]) {

    argv0 = argv[0];

    int opt;

    while ((opt = getopt(argc, argv, "?hse")) != -1) {
        switch (opt) {
            case 's':
                opts.midisend = true;
                break;
            case 'e':
                if (optarg == 0){

                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::ChannelSelect);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFaderLevel);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFaderSelect);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFaderMute);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFaderPAFL);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiSoftKey);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiMmc);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiSoftRotary);
                }
                else if (strcmp(optarg, "channelselect") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::ChannelSelect);
                }
                else if (strcmp(optarg, "midifaderlevel") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFaderLevel);
                }
                else if (strcmp(optarg, "midifaderselect") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFaderSelect);
                }
                else if (strcmp(optarg, "midifadermute") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFaderMute);
                }
                else if (strcmp(optarg, "midifaderpafl") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiFaderPAFL);
                }
                else if (strcmp(optarg, "midisoftkey") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiSoftKey);
                }
                else if (strcmp(optarg, "midisoftrotary") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiSoftRotary);
                }
                else if (strcmp(optarg, "midimmc") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Types::MidiMmc);
                }
                else {
                    printf("ERROR: unknown event: %s\n", optarg);
                    help();
                    return EXIT_FAILURE;
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
        printf("ERROR missing argument\n");
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

    mixMitm.onConnectionStateChanged([&mixMitm, &opt](SQMixMitm::MixMitm::ConnectionState state, SQMixMitm::MixMitm::Version version){
        if (state == SQMixMitm::MixMitm::Connected) {
            printf(
                    "CONNECTION STATE connected (mixer firmware %d.%d.%d r%d)\n"
                    , version.major(), version.minor(), version.patch(), version.build()
            );

            if (opts.midisend){

                midifader_send_thread = new std::thread([&mixMitm](){

                    printf("Starting MIDI Fader 1+2 fade up\n" );

                    for(uint16_t i = 0; i <= 255; i += 10){
                        // sanity check
                        if (mixMitm.connectionState() == SQMixMitm::MixMitm::ConnectionState::Disconnected){
                            break;
                        }

                        mixMitm.sendCommand(
                                SQMixMitm::Command::midiFaderLevel(0,i)
                        );
                        mixMitm.sendCommand(
                                SQMixMitm::Command::midiFaderLevel(1,i)
                        );

                        std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    }

                    if (mixMitm.connectionState() == SQMixMitm::MixMitm::ConnectionState::Connected){
                        mixMitm.sendCommand(
                                SQMixMitm::Command::midiFaderLevel(0,0)
                        );
                        mixMitm.sendCommand(
                                SQMixMitm::Command::midiFaderLevel(1,0)
                        );
                    }

                    printf("Finished MIDI Fader 1+2 fade up\n" );
                });
            }
        }
        else if (state == SQMixMitm::MixMitm::Disconnected) {
            printf("CONNECTION STATE  disconnected\n");
        }
    });


    std::for_each(opts.listenForEvents.begin(), opts.listenForEvents.end(), [&mixMitm](SQMixMitm::Event::Type type){
       mixMitm.onEvent(type, onEventCallback);
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

    if (opts.midisend && midifader_send_thread != nullptr){
        midifader_send_thread->join();
    }


    return EXIT_SUCCESS;
}