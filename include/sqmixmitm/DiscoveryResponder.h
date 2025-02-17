/**
* sqmix_mitm
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

#ifndef SQMIX_MITM_DISCOVERYRESPONDER_H
#define SQMIX_MITM_DISCOVERYRESPONDER_H

#include <string>
#include <thread>

namespace SQMixMitm {

    class DiscoveryResponder {

    public:

        enum State {Stopped, Starting, Running, Stopping};

    public:

        static constexpr unsigned int Port = 51320;
        static constexpr char DiscoveryMessage[] = "SQ Find";

    protected:

        std::string name_ = "";

        int sockfd_ = -1;

        std::atomic<State> state_ = Stopped;

        std::thread * thread_ = nullptr;

    public:

        std::string &name(){ return name_; }

        void name(char name[]){
            name_ = name;
        }

        void name(std::string &name){
            name_ = name;
        }

        State state(){ return state_; }

        int start();
        int stop();

    };

} // SQMixMitm

#endif //SQMIX_MITM_DISCOVERYRESPONDER_H
