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

#include "Version.h"

#include <cstring>
#include <cstdio>

namespace SQMixMitm {

//    bool Version::isSupported(unsigned char major, unsigned char minor, unsigned char patch, unsigned short build){
//        return true;
//    }

    Version::Version(Version &other){
        memcpy(bytes_, other.bytes_, sizeof(bytes_));
    }

    Version::Version(unsigned char major, unsigned char minor, unsigned char patch, unsigned short build){
        bytes_[1] = major;
        bytes_[2] = minor;
        bytes_[3] = patch;
        bytes_[4] = build & 0xff;
        bytes_[5] = (build >> 8) & 0xff;
    }

    void Version::fromBytes(unsigned char bytes[]){
        memcpy(bytes_, bytes, sizeof(bytes_));
    }

    void Version::clear(){
        memset(bytes_, 0, sizeof(bytes_));
    }

//    bool Version::isSupported(){
//        return isSupported(major(), minor(), patch(), build());
//    }

//    Version::operator Firmware () {
//        return versioningToFirmware(major(), minor(), patch(), build());
//    }

    bool Version::operator==(Version &other){
        return (major() == other.major() &&
                minor() == other.minor() &&
                (patch() == 0 || other.patch() == 0 || patch() == other.patch()));
    }
    bool Version::operator<(Version &other){
        return (major() < other.major() ||
                (major() == other.major() &&
                 (minor() < other.minor() ||
                  (minor() == other.minor() &&
                   (patch() < other.patch())))));
    }
    bool Version::operator>(Version &other){
        return (major() > other.major() ||
                (major() == other.major() &&
                 (minor() > other.minor() ||
                  (minor() == other.minor() &&
                   (patch() > other.patch())))));
    }
    bool Version::operator<=(Version &other){
        return operator==(other) || operator<(other);
    }
    bool Version::operator>=(Version &other){
        return operator==(other) || operator>(other);
    }
    
} // SQMixMitm