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


#include "Version.h"

namespace SQMixMitm {

    class Command {

    public:

        enum Type {
            MidiFaderLevel = 0,

            _COUNT_,
            _INVALID_ = _COUNT_
        };

        class Factory {

            friend class Command;

        protected:

            typedef const unsigned char CommandHeader[4];
            typedef const CommandHeader CommandHeaderTable[_COUNT_];
            typedef CommandHeaderTable * CommandHeaderTableRef;
            typedef unsigned char CommandData[4];

            static constexpr CommandHeaderTable kCommandHeaderTable_v1_5_10 = {
                    {0xf7, 0x1f, 0x1f, 0x0d} // MidiFaderLevel
            };

        protected:

            CommandHeaderTableRef commandHeaderTable_ = nullptr;

            inline CommandHeader & headerForType(Type type){
                return (*commandHeaderTable_)[type];
            }

        public:

            void usingVersion(Version &version);

            inline bool isValid(){ return commandHeaderTable_ != nullptr; }

            Command midiFaderLevel(unsigned char channel, unsigned char level);

        };

    protected:

        Type type_ = _INVALID_;
        unsigned char bytes_[8] = {0, 0, 0, 0, 0, 0, 0, 0};

    protected:

        Command(){};
        Command(Type type, Factory::CommandHeader &header, Factory::CommandData &data);
//        Command(Type type, Factory::CommandHeader &header, unsigned char data[]);

    public:

        Command(Command &cmd);

        Type type(){ return type_; }

        unsigned char * bytes(){ return bytes_; }
        int size(){ return sizeof(bytes_); }

        bool isValid(){ return type_ != _INVALID_; }
    };

} // SQMixMitm

#endif //SQMIX_MITM_COMMAND_H
