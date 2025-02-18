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

#ifndef SQMIX_MITM_EVENT_H
#define SQMIX_MITM_EVENT_H


//#include "firmware.h"
#include "Version.h"

namespace SQMixMitm {

    class Event {

    public:

        enum Type {
            ChannelSelect       = 0,
            LayerSelect         = 1,

            MidiFaderLevel      = 2,
            MidiFaderSelect     = 3,
            MidiFaderMute       = 4,
            MidiFaderPAFL       = 5,

            MidiSoftKey         = 6,
            MidiSoftRotary      = 7,

            MidiMmc             = 8,

            _COUNT_
        };


        class Parser {
        protected:

            typedef const unsigned char EventHeader[4];
            typedef const EventHeader EventHeaderTable[_COUNT_];

            typedef EventHeaderTable * EventHeaderTableRef;

            static constexpr EventHeaderTable kEventHeaderTable_v1_5_10 = {
                    {0xf7, 0x33, 0x33, 0x09}, // channel select
                    {0xf7, 0x08, 0x09, 0x0b}, // layer select
                    {0xf7, 0x09, 0x1f, 0x0d}, // midifader level
                    {0xf7, 0x08, 0x1f, 0x11}, // midifader select
                    {0xf7, 0x08, 0x1f, 0x12}, // midifader mute
                    {0xf7, 0x08, 0x1f, 0x13}, // midifader pafl
                    {0xf7, 0x00, 0x1f, 0x14}, // midi soft key
                    {0xf7, 0x21, 0x1f, 0x14}, // midi soft rotary
                    {0xf7, 0x20, 0x1f, 0x0b}, // midi mmc
            };

            static constexpr EventHeaderTableRef kEventHeaderTableFallback = &kEventHeaderTable_v1_5_10;

        protected:
            EventHeaderTableRef eventHeaderTable_ = nullptr;

        public:
            void usingVersion(Version &version);
            int parse(unsigned char bytes[], int len, Event &event);
        };


    protected:

        Type type_;
        unsigned char data_[4] = {0,0,0,0};

    public:

        Event(){}
        Event(Event &event);

        inline Type type(){ return type_; }
        inline unsigned char * data(){ return data_; }
        int size(){ return sizeof(data_); }


        inline unsigned char data0(){ return data_[0];}
        inline unsigned char data1(){return data_[1];}
        inline unsigned char data2(){return data_[2];}
        inline unsigned char data3(){return data_[3];}


        inline unsigned char ChannelSelect_channel(){ return data1(); }

        inline unsigned char LayerSelect_layer(){ return data0(); }

        inline unsigned char MidiFaderLevel_channel(){ return data0(); }
        inline unsigned char MidiFaderLevel_value(){ return data2(); }

        inline unsigned char MidiFaderMute_channel(){ return data0(); }
        inline unsigned char MidiFaderSelect_channel(){ return data0(); }
        inline unsigned char MidiFaderPAFL_channel(){ return data0(); }

        inline unsigned char MidiSoftKey_channel(){ return data0() & 0x0f; }
        inline unsigned char MidiSoftKey_type(){ return data0() & 0xf0; }
        inline unsigned char MidiSoftKey_value1(){ return data1(); }
        inline unsigned char MidiSoftKey_value2(){ return data2(); }

        inline unsigned char MidiMmc_cmd(){ return data0(); }

        inline unsigned char MidiSoftRotary_channel(){ return data0() & 0x0f; }
        inline unsigned char MidiSoftRotary_type(){ return data0() & 0xf0; }
        inline unsigned char MidiSoftRotary_value1(){ return data1(); }
        inline unsigned char MidiSoftRotary_value2(){ return data2(); }

    };

} // SQMixMitm

#endif //SQMIX_MITM_EVENT_H
