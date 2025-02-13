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

#ifndef SQMIX_MITM_COMMAND_H
#define SQMIX_MITM_COMMAND_H

#include <bit>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <cassert>

namespace SQMixMitm {

    class Command {

        friend class MixMitm;

    public:

        enum Types {
            MidiFaderLevel      = 0xf71f1f0d
        };

    protected:

        unsigned char bytes[8] = {0,0,0,0,0,0,0,0};


        Command(Types type){
#if defined(LITTLE_ENDIAN)
            bytes[3] = type & 0xff;
            bytes[2] = (type >> 8) & 0xff;
            bytes[1] = (type >> 16) & 0xff;
            bytes[0] = (type >> 24) & 0xff;
#elif defined(BIG_ENDIAN)
            bytes[0] = type & 0xff;
            bytes[1] = (type >> 8) & 0xff;
            bytes[2] = (type >> 16) & 0xff;
            bytes[3] = (type >> 24) & 0xff;
#endif
//            printf("%02x%02x%02x%02x\n", bytes[0], bytes[1], bytes[2], bytes[3]);
        }


    public:

        Command(Command &cmd){
//            memcpy(bytes, cmd.bytes, sizeof(bytes));
            *(uint64_t*)bytes = *(uint64_t*)cmd.bytes;
        }

        static Command midiFaderLevel(uint8_t channel, uint8_t level){
            assert(0 <= channel && channel <= 31);

            Command cmd(MidiFaderLevel);

            cmd.bytes[4] = channel;
            cmd.bytes[6] = level;

//            printf("%02x%02x%02x%02x %02x%02x%02x%02x\n", cmd.bytes[0], cmd.bytes[1], cmd.bytes[2], cmd.bytes[3], cmd.bytes[4], cmd.bytes[5], cmd.bytes[6], cmd.bytes[7]);

            return cmd;
        }

        uint32_t type(){
#if defined(LITTLE_ENDIAN)
            return (bytes[0] << 24) + (bytes[1] << 16) +(bytes[2] << 8) + bytes[3];
#elif defined(BIG_ENDIAN)
            return *((uint32_t*)bytes);
#endif
        }


    };

} // SQMixMitm

#endif //SQMIX_MITM_COMMAND_H
