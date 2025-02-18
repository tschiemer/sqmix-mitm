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

#ifndef SQMIX_MITM_VERSION_H
#define SQMIX_MITM_VERSION_H

//#include "firmware.h"


namespace SQMixMitm {

    class Version {

    protected:

        unsigned char bytes_[12] = {0,0,0,0,0,0,0,0,0,0,0,0};

    public:

        Version(){}

        Version(Version &other);

        Version(unsigned char major, unsigned char minor, unsigned char patch, unsigned short build);

        unsigned char * bytes(){ return bytes_; }

        void fromBytes(unsigned char bytes[]);
        void clear();

        // this is mere speculation
        inline unsigned int model(){ return bytes_[0]; }

        inline unsigned int major(){ return bytes_[1]; }
        inline unsigned int minor(){ return bytes_[2]; }
        inline unsigned int patch(){ return bytes_[3]; }
        inline unsigned int build(){ return bytes_[4] + ((bytes_[5]) << 8); }

        bool operator==(Version &other);
        bool operator<(Version &other);
        bool operator>(Version &other);
        bool operator<=(Version &other);
        bool operator>=(Version &other);
    };

} // SQMixMitm

#endif //SQMIX_MITM_VERSION_H
