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

#ifndef SQMIX_MITM_MIDICONTROL_H
#define SQMIX_MITM_MIDICONTROL_H

#include <thread>

namespace SQMixMitm {

    class MidiControl {

    public:

        enum State {Stopped, Starting, Running, Stopping};

        typedef std::function<void(char [], unsigned int)> ReceivedDataCallback;

    public:

        static constexpr unsigned int Port = 51325;

    protected:

        int sockfd_ = -1;

        std::atomic<State> state_ = Stopped;

        std::thread * thread_ = nullptr;

    public:

        State state(){ return state_; }

        int connect(std::string &mixerIp, ReceivedDataCallback callback);

        int connect(char mixerIp[], ReceivedDataCallback callback){
            return connect(mixerIp, callback);
        }

        int disconnect();

        int send(const void * data, size_t len);
    };

} // SQMixMitm

#endif //SQMIX_MITM_MIDICONTROL_H
