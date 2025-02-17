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

#ifndef SQMIX_MITM_DISCOVERY_H
#define SQMIX_MITM_DISCOVERY_H

#include <thread>

namespace SQMixMitm {

    class Discovery {

    public:

        enum State {Stopped, Starting, Running, Stopping};

        class Mixer {
        public:
            std::string name;
            std::string ip;
            std::chrono::steady_clock::time_point lastSeen;

//        protected:
            Mixer(std::string &aName, std::string &anIp){
                name = aName;
                ip = anIp;
                lastSeen = std::chrono::steady_clock::now();
            }
            void updateLastSeen(){
                lastSeen = std::chrono::steady_clock::now();
            }
            bool isOlderThan(std::chrono::steady_clock::time_point timepoint){
                return (lastSeen < timepoint);
            }

            bool operator==(const std::string& otherIp) {
                return ip == otherIp;
            }
        };

        typedef std::function<void(Mixer&)> EventCallback;

    public:

        static constexpr unsigned int Port = 51320;
        static constexpr char DiscoveryMessage[] = "SQ Find";
        static constexpr std::chrono::seconds DefaultTimeout = std::chrono::seconds(5);

    protected:

        std::chrono::seconds timeout_ = DefaultTimeout;

        EventCallback foundCallback_ = nullptr;
        EventCallback timeoutCallback_ = nullptr;

        std::vector<Mixer*> mixerList_;

        int sockfd_ = -1;

        std::atomic<State> state_ = Stopped;

        std::thread * broadcastThread_ = nullptr;
        std::thread * listenThread_ = nullptr;

    public:

        void timeout(std::chrono::seconds timeout){
            timeout_ = timeout;
        }

        void onFound(EventCallback callback){
            foundCallback_ = callback;
        }

        void onTimeout(EventCallback callback){
            timeoutCallback_ = callback;
        }

        std::vector<Mixer*> * mixerList(){
            return &mixerList_;
        }

        State state(){ return state_; }


        int start();
        int stop();
    };

} // SQMixMitm

#endif //SQMIX_MITM_DISCOVERY_H
