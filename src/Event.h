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

#include <cstdint>
#include <cstdio>

namespace SQMixMitm {

    class Event {

    public:

        typedef uint32_t Type;
        typedef uint32_t Data;

    public:

        Type type;
        Data data;

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

        enum Types {
            MidiFader       = 0xf7091f0d,
            MidiSoftControl = 0xf7211f14,
            LayerSelect     = 0xf708090b,
            ChannelSelect   = 0xf7333309,
            ChannelFader    = 0xf709070b,
            
        };

        inline uint32_t databyte0(){
            return (data >> 24) & 0xff;
        }
        inline uint32_t databyte1(){
            return (data >> 16) & 0xff;
        }
        inline uint32_t databyte2(){
            return (data >> 8) & 0xff;
        }
        inline uint32_t databyte3(){
            return data & 0xff;
        }


        inline uint32_t MidiFader_fader(){ return databyte0(); }
        inline uint32_t MidiFader_value(){ return databyte2(); }

        inline uint32_t MidiSoftControl_channel(){ return databyte0() & 0x0f; }
        inline uint32_t MidiSoftControl_type(){ return databyte0() & 0xf0; }
        inline uint32_t MidiSoftControl_value1(){ return databyte1(); }
        inline uint32_t MidiSoftControl_value2(){ return databyte2(); }

        inline uint32_t LayerSelect_layer(){ return databyte0(); }

        inline uint32_t ChannelSelect_channel(){ return databyte1(); }
        
        inline uint32_t ChannelFader_fader(){ return databyte0(); }
        inline uint32_t ChannelFader_value(){ return 0; } // todo
    };

} // SQMixMitm

#endif //SQMIX_MITM_EVENT_H
