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

#include "sqmixmitm/MixMitm.h"
#include "sqmixmitm/log.h"

#include "midi.h"


static char * argv0 = nullptr;

static struct {
    std::vector<SQMixMitm::Event::Type> listenForEvents;
    bool midisend;
    SQMixMitm::LogLevel logLevel;
} opts = {
        .midisend = false,
        .logLevel = SQMixMitm::LogLevelNone
};

static std::thread * midifader_send_thread = nullptr;

static void help(){
    printf(
            "Usage: %s [-v] [-e[<event>]]* [-s] <ip-of-mixer>\n"
            "Act as Man-in-the-Middle service for mixer with given IP\n"
            "Note: requires service to be discoverable (such as when using sq-discovery-responder)\n"
            "\nOptions:\n"
            "\t -v            Increase verbosity (max 2x)\n"
            "\t -e[<event>]   Show events, if no option specified shows all events (<event> in \"channelselect\", \"midifader\", \"midisoftkey\", \"midisoftrotary\", \"midimmc\")\n"
            "\t -s            Slowly fade MIDI faders 1+2 up from 0%% to 100%%\n"
            "\nExamples:\n"
            "%s -vv -e 192.168.1.100 # very verbose, shows all events\n"
            "%s -e channelselect 192.168.1.100 # show only channel select events\n"
            "%s -s 192.168.1.100 # send commands\n"
            , argv0, argv0, argv0, argv0);
}

static const char * miditype_name(uint8_t type){
    switch (type & 0xf0){
        case NoteOff:
            return "Note Off";
        case NoteOn:
            return "Note On";
        case ControlChange:
            return "CC";
        case ProgramChange:
            return "PC";
        default:
            return "(midi type not in this LUT)";

    }
}

static const char * midimmc_name(uint8_t cmd){
    switch(cmd){
        case Stop:
            return "Stop";
        case Play:
            return "Play";
        case FastForward:
            return "Fast Forward";
        case Rewind:
            return "Rewind";
        case Record:
            return "Record";
        case Pause:
            return "Pause";
        default:
            return "(midi mmc command not in this LUT)";
    }
}

void onEventCallback(SQMixMitm::Event &event){
    if (event.type() == SQMixMitm::Event::Type::ChannelSelect){
        printf("EVENT channel select %d\n", event.ChannelSelect_channel());
    }
    else if (event.type() == SQMixMitm::Event::Type::MidiFaderLevel){
        printf("EVENT midifader level channel %d value %d\n", event.MidiFaderLevel_channel(), event.MidiFaderLevel_value());
    }
    else if (event.type() == SQMixMitm::Event::Type::MidiFaderSelect){
        printf("EVENT midifader select channel %d\n", event.MidiFaderSelect_channel());
    }
    else if (event.type() == SQMixMitm::Event::Type::MidiFaderMute){
        printf("EVENT midifader mute channel %d\n", event.MidiFaderMute_channel());
    }
    else if (event.type() == SQMixMitm::Event::Type::MidiFaderPAFL){
        printf("EVENT midifader pafl channel %d\n", event.MidiFaderPAFL_channel());
    }
    else if (event.type() == SQMixMitm::Event::Type::MidiSoftKey){
        printf("EVENT soft key type %s (%02x) channel %d value1 %d value2 %d\n",
               miditype_name(event.MidiSoftKey_type()),
               event.MidiSoftRotary_type(),
               event.MidiSoftKey_channel(),
               event.MidiSoftKey_value1(),
               event.MidiSoftKey_value2());
    }
    else if (event.type() == SQMixMitm::Event::Type::MidiSoftRotary){
        printf("EVENT soft rotary type %s (%02x) channel %d value1 %d value2 %d\n",
               miditype_name(event.MidiSoftRotary_type()),
               event.MidiSoftRotary_type(),
               event.MidiSoftRotary_channel(),
               event.MidiSoftRotary_value1(),
               event.MidiSoftRotary_value2());
    }
    else if (event.type() == SQMixMitm::Event::Type::MidiMmc){
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

    while ((opt = getopt(argc, argv, "?hsve")) != -1) {
        switch (opt) {
            case 's':
                opts.midisend = true;
                break;
            case 'e':
                if (optarg == 0){

                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::ChannelSelect);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiFaderLevel);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiFaderSelect);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiFaderMute);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiFaderPAFL);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiSoftKey);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiMmc);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiSoftRotary);
                }
                else if (strcmp(optarg, "channelselect") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::ChannelSelect);
                }
                else if (strcmp(optarg, "midifader") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiFaderLevel);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiFaderSelect);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiFaderMute);
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiFaderPAFL);
                }
                else if (strcmp(optarg, "midisoftkey") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiSoftKey);
                }
                else if (strcmp(optarg, "midisoftrotary") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiSoftRotary);
                }
                else if (strcmp(optarg, "midimmc") == 0){
                    opts.listenForEvents.push_back(SQMixMitm::Event::Type::MidiMmc);
                }
                else {
                    printf("ERROR: unknown event: %s\n", optarg);
                    help();
                    return EXIT_FAILURE;
                }
                break;

            case 'v':
                if (optarg){
                    SQMixMitm::setLogLevel((SQMixMitm::LogLevel)atoi(optarg));
                } else {

                }

                break;

            case '?':
            case 'h':
                help();
                return EXIT_SUCCESS;

            default: /* '?' */
                printf("Unknown option: %c\n", opt);
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

    mixMitm.onConnectionStateChanged([&mixMitm, &opt](SQMixMitm::MixMitm::ConnectionState state, SQMixMitm::Version version){
        if (state == SQMixMitm::MixMitm::Connected) {
            printf(
                    "CONNECTION STATE connected (mixer firmware %d.%d.%d r%d)\n"
                    , version.major(), version.minor(), version.patch(), version.build()
            );

            if (opts.midisend){

                if (!mixMitm.commandFactory().isValid()){
                    printf("NOT sending midi, assuming command might be invalid and might crash mixer\n");
                } else {

                    midifader_send_thread = new std::thread([&mixMitm](){



                        printf("Starting MIDI Fader 1+2 fade up\n" );

                        for(uint16_t i = 0; i <= 255; i += 10){
                            // sanity check
                            if (mixMitm.connectionState() == SQMixMitm::MixMitm::ConnectionState::Disconnected){
                                break;
                            }

                            mixMitm.sendCommand(
                                    mixMitm.commandFactory().midiFaderLevel(0,i)
                            );
                            mixMitm.sendCommand(
                                    mixMitm.commandFactory().midiFaderLevel(1,i)
                            );

                            std::this_thread::sleep_for(std::chrono::milliseconds(500));
                        }

                        if (mixMitm.connectionState() == SQMixMitm::MixMitm::ConnectionState::Connected){
                            mixMitm.sendCommand(
                                    mixMitm.commandFactory().midiFaderLevel(0,0)
                            );
                            mixMitm.sendCommand(
                                    mixMitm.commandFactory().midiFaderLevel(1,0)
                            );
                        }

                        printf("Finished MIDI Fader 1+2 fade up\n" );
                    });
                }
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