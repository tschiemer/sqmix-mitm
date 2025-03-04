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

#include "Event.h"

#include <cstdint>
#include <cassert>
#include <exception>
#include <stdexcept>
#include <cstring>

#include "log.h"

namespace SQMixMitm {

    void Event::Parser::usingVersion(Version & version){

        static Version v1_5_10(1,5,10,0);
        static Version v1_6_0(1,6,0,0);

        if (version == v1_5_10){
            eventHeaderTable_ = (EventHeaderTableRef)kEventHeaderTable_v1_5_10;
        }

        if (version == v1_6_0){
            eventHeaderTable_ = (EventHeaderTableRef)kEventHeaderTable_v1_6_0;
        }

        if (eventHeaderTable_ == nullptr){
            eventHeaderTable_ = kEventHeaderTableFallback;

            log(LogLevelInfo, "EventParser: unknown version, falling back to %d.%d.%d, might work unreliably", version.major(), version.minor(), version.patch());
        }
    }

    int Event::Parser::parse(unsigned char bytes[], int len, Event &event){

        // basic sanity check
        if (len < 8){
            return 0;
        }

        for(int i = 0; i < Type::_COUNT_; i++){
            if (memcmp((*eventHeaderTable_)[i], bytes, sizeof(EventHeader)) == 0){

                event.type_ = (Type)i;
                memcpy(event.data_, bytes+sizeof(EventHeader), sizeof(event.data_));

                return 8;
            }
        }

        return 0;
    }

    Event::Event(Event &event){
        type_ = event.type_;
        memcpy(data_, event.data_, sizeof(data_));
    }

} // SQMixMitm