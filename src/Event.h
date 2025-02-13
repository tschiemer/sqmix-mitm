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

#include <bit>
#include <cstdint>
//#include <cstdio>

namespace SQMixMitm {

    class Event {

    public:

        typedef uint32_t Type;
        typedef uint32_t Data;

    public:

        Type type;
        Data data;

        enum Types {
            LayerSelect         = 0xf708090b,
            MidiFaderLevel      = 0xf7091f0d,
            MidiFaderSelect     = 0xf7081f11,
            MidiFaderMute       = 0xf7081f12,
            MidiFaderPAFL       = 0xf7081f13,
            MidiSoftKey         = 0xf7001f14,
            MidiMmc             = 0xf7201f0b,
            MidiSoftRotary      = 0xf7211f14,
            ChannelSelect       = 0xf7333309

        };

        enum MidiType {
            NoteOff         = 0x80,
            NoteOn          = 0x90,
            ControlChange   = 0xb0,
            ProgramChange   = 0xc0
        };

        enum MidiMmcType {
            Stop            = 0x01,
            Play            = 0x02,
            FastForward     = 0x04,
            Rewind          = 0x05,
            Record          = 0x06,
            Pause           = 0x09
        };

        inline static bool isValidType(char bytes[]){
#if defined(LITTLE_ENDIAN)
            uint32_t type = (bytes[0] << 24) + (bytes[1] << 16) +(bytes[2] << 8) + bytes[3];
#elif defined(BIG_ENDIAN)
            uint32_t type = *((uint32_t*)bytes);
#endif
            return (
                type == LayerSelect ||
                type == MidiFaderLevel ||
                type == MidiFaderSelect ||
                type == MidiFaderMute ||
                type == MidiFaderPAFL ||
                type == MidiSoftKey ||
                type == MidiMmc ||
                type == MidiSoftRotary ||
                type == ChannelSelect
            );
        }

        Event(){}

        Event(char * bytes){
//            printf("e %02x%02x%02x%02x %02x%02x%02x%02x\n", (uint8_t)bytes[0], (uint8_t)bytes[1], (uint8_t)bytes[2], (uint8_t)bytes[3], (uint8_t)bytes[4], (uint8_t)bytes[5], (uint8_t)bytes[6], (uint8_t)bytes[7]);
#if defined(LITTLE_ENDIAN)
            type = (bytes[0] << 24) + (bytes[1] << 16) +(bytes[2] << 8) + bytes[3];
            data = (bytes[4] << 24) + (bytes[5] << 16) +(bytes[6] << 8) + bytes[7];
#elif defined(BIG_ENDIAN)
            type = *((uint32_t*)bytes);
            data = *((uint32_t*)(bytes+4));
#endif
//            printf("%08x %08x \n" , type, data);
        }


        inline uint8_t databyte0(){
            return (data >> 24) & 0xff;
        }
        inline uint8_t databyte1(){
            return (data >> 16) & 0xff;
        }
        inline uint8_t databyte2(){
            return (data >> 8) & 0xff;
        }
        inline uint8_t databyte3(){
            return data & 0xff;
        }


        inline uint8_t MidiFaderLevel_channel(){ return databyte0(); }
        inline uint8_t MidiFaderLevel_value(){ return databyte2(); }

        inline uint8_t MidiFaderMute_channel(){ return databyte0(); }
        inline uint8_t MidiFaderSelect_channel(){ return databyte0(); }
        inline uint8_t MidiFaderPAFL_channel(){ return databyte0(); }

        inline uint8_t MidiSoftKey_channel(){ return databyte0() & 0x0f; }
        inline uint8_t MidiSoftKey_type(){ return databyte0() & 0xf0; }
        inline uint8_t MidiSoftKey_value1(){ return databyte1(); }
        inline uint8_t MidiSoftKey_value2(){ return databyte2(); }

        inline uint8_t MidiMmc_cmd(){ return databyte0(); }

        inline uint8_t MidiSoftRotary_channel(){ return databyte0() & 0x0f; }
        inline uint8_t MidiSoftRotary_type(){ return databyte0() & 0xf0; }
        inline uint8_t MidiSoftRotary_value1(){ return databyte1(); }
        inline uint8_t MidiSoftRotary_value2(){ return databyte2(); }

        inline uint8_t LayerSelect_layer(){ return databyte0(); }

        inline uint8_t ChannelSelect_channel(){ return databyte1(); }
    };

} // SQMixMitm

#endif //SQMIX_MITM_EVENT_H
