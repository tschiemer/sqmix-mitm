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

#include "Command.h"

#include <cstring>
#include <cstdio>
#include <cassert>

#include "log.h"

namespace SQMixMitm {


    void Command::Factory::usingVersion(Version &version) {

        static Version v1_5_10(1,5,10,0);

        if (version == v1_5_10){
            commandHeaderTable_ = (CommandHeaderTableRef)kCommandHeaderTable_v1_5_10;
        }

        if (commandHeaderTable_ == nullptr){
            log(LogLevelInfo, "Command::Factory: unknown version, will NOT send command", version.major(), version.minor(), version.patch());
        }
    }


    Command Command::Factory::midiFaderLevel(unsigned char channel, unsigned char level){

        if (!isValid()){
            return Command();
        }
        if (0 <= channel && channel <= 31){
            return Command();
        }

        CommandData data = {channel, 0, level, 0};

        Command cmd(MidiFaderLevel, headerForType(MidiFaderLevel), data);

//            printf("%02x%02x%02x%02x %02x%02x%02x%02x\n", cmd.bytes_[0], cmd.bytes_[1], cmd.bytes_[2], cmd.bytes_[3], cmd.bytes_[4], cmd.bytes_[5], cmd.bytes_[6], cmd.bytes_[7]);

        return cmd;
    }


    Command::Command(Type type, Factory::CommandHeader &header, Factory::CommandData &data){
        type_ = type;
        memcpy(bytes_, header, sizeof(Factory::CommandHeader));
        memcpy(bytes_ + sizeof(Factory::CommandHeader), data, sizeof(Factory::CommandData));
    }

    Command::Command(Command &cmd) {
        type_ = cmd.type_;
        memcpy(bytes_, cmd.bytes_, sizeof(bytes_));
    }

} // SQMixMitm