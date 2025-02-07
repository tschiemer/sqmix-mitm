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

#ifndef SQMIX_MITM_MITMSERVICE_H
#define SQMIX_MITM_MITMSERVICE_H


namespace SQMixMitm {

    class MitmService {

    public:

        enum State {Stopped, Starting, Running, Stopping};

    public:

        static constexpr unsigned int UdpPort = 51324;
        static constexpr unsigned int TcpPort = 51326;

    protected:

        State state_ = Stopped;

    public:

        State state(){ return state_; }

    };

}

#endif //SQMIX_MITM_MITMSERVICE_H
