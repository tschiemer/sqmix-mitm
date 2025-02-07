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

#include "Discovery.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>

//template<typename Clock, typename Duration>
//std::ostream &operator<<(
//        std::ostream &stream,
//        const std::chrono::time_point<Clock, Duration> &timepoint)
//{
//    Duration ago = timepoint.time_since_epoch();
//    return stream << ago;
//}

namespace SQMixMitm {


    bool operator==(Discovery::Mixer * lhs, const std::string& otherIp) {
        return lhs->ip == otherIp;
    }

    int Discovery::start(){

        if (state_ != Stopped){
            return EXIT_FAILURE;
        }
        state_ = Starting;

        // Creating socket file descriptor
        if ( (sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            perror("socket creation failed");
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        //// set some socket options

        // reuse port
        int optval = 1;
        setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

        // enable broadcast
        int broadcast=1;
        setsockopt(sockfd_, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof broadcast);

        // set recv timeout
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


        // Bind the socket with the server address
//        if (bind(sockfd_, (const struct sockaddr *)&bcaddr, sizeof(bcaddr)) < 0 ){
//            perror("bind failed");
//            state_ = Stopped;
//            return EXIT_FAILURE;
//        }

        state_ = Running;

        broadcastThread_ = new std::thread([&](){

            struct sockaddr_in bcaddr;

            memset(&bcaddr, 0, sizeof(bcaddr));

            // Filling server information
            bcaddr.sin_family    = AF_INET; // IPv4
            bcaddr.sin_addr.s_addr = INADDR_BROADCAST;
            bcaddr.sin_port = htons(Port);

            while (state_ == Running){

                // send inquiry message
                if (sendto(sockfd_, (const char *)DiscoveryMessage, sizeof(DiscoveryMessage)-1, 0, (const struct sockaddr *) &bcaddr, sizeof(bcaddr)) < 0){
                    perror("sendto");
                };

                // check for timeouts
                std::chrono::steady_clock::time_point tp = std::chrono::steady_clock::now() - timeout_;
                std::for_each(mixerList_.begin(), mixerList_.end(), [&](Mixer * mixer){
                    if (mixer->isOlderThan(tp)){

                        mixerList_.erase(std::remove(mixerList_.begin(), mixerList_.end(),mixer));

                        if (timeoutCallback_ != nullptr){
                            timeoutCallback_(*mixer);
                            delete mixer;
                        }
                    }
                });

                std::this_thread::sleep_for(std::chrono::seconds(1));
            }


        });

        listenThread_ = new std::thread([&](){

            struct sockaddr_in mixaddr;
            socklen_t len;
            int n;
            char buffer[512];

            len = sizeof(mixaddr);  //len is value/result


            while (state_ == Running){


                // reset source info
                memset(&mixaddr, 0, sizeof(mixaddr));

                if ((n = recvfrom(sockfd_, (char *)buffer, sizeof(buffer), 0, ( struct sockaddr *) &mixaddr, &len)) < 0){

                    // if timeout, just ignore
                    if (errno == EAGAIN){
                        // do nothing
                    } else {
                        perror("Unknown socket error, stopping\n");

                        // stop loop and end thread
                        state_ = Stopping;
                    }
                } else {
                    if (n > 0){
                        buffer[n] = '\0';

                        std::string name = buffer;
                        std::string ip = inet_ntoa(mixaddr.sin_addr);

                        std::vector<Mixer*>::const_iterator iter = std::find(mixerList_.begin(), mixerList_.end(),ip);

                        Mixer * mixer = nullptr;

                        if (iter == mixerList_.end()){
                            mixer = new Mixer(name, ip);

                            mixerList_.push_back(mixer);

                            if (foundCallback_ != nullptr){
                                foundCallback_(*mixer);
                            }
                        } else{

                            mixer = *iter;
                            mixer->updateLastSeen();

                        }

                    }
                }
            } // while running

            close(sockfd_);
        }); // worker thread

        return EXIT_SUCCESS;
    }

    int Discovery::stop(){
        if (state_ == Stopped){
            return EXIT_SUCCESS;
        }
        state_ = Stopping;

        listenThread_->join();
        delete listenThread_;
        listenThread_ = nullptr;

        broadcastThread_->join();
        delete broadcastThread_;
        broadcastThread_ = nullptr;

        std::for_each(mixerList_.begin(), mixerList_.end(), [](Mixer * m){
            delete m;
        });

        state_ = Stopped;

        return EXIT_SUCCESS;
    }

} // SQMixMitm