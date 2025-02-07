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
*
* For code source also see: https://www.geeksforgeeks.org/udp-server-client-implementation-c/
*/

#include "DiscoveryResponder.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

namespace SQMixMitm {

    int DiscoveryResponder::start(){

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

        // set recv timeout
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));

        // Filling server information
        servaddr.sin_family    = AF_INET; // IPv4
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(Port);

        // Bind the socket with the server address
        if (bind(sockfd_, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ){
            perror("bind failed");
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        state_ = Running;

        thread_ = new std::thread([&](){

            char buffer[2*sizeof(DiscoveryMessage)]; // only need a buffer of this size


            struct sockaddr_in cliaddr;
            socklen_t len;
            int n;

            len = sizeof(cliaddr);  //len is value/result

            while (state_ == Running){

                memset(&cliaddr, 0, sizeof(cliaddr));

                if ((n = recvfrom(sockfd_, (char *)buffer, sizeof(buffer), 0, ( struct sockaddr *) &cliaddr, &len)) < 0){

                    // if timeout, just ignore
                    if (errno == EAGAIN){
                        // do nothing
                    } else {
                        perror("Unknown socket error, stopping\n");

                        // stop loop and end thread
                        state_ = Stopping;
                    }
                } else {

                    // basic sanity check if discovery request was sent
                    if (n == sizeof(DiscoveryMessage)-1 && memcmp(buffer, DiscoveryMessage, sizeof(DiscoveryMessage)-1) == 0){
                        if (sendto(sockfd_, (const char *)name_.data(), name_.length()+1, 0, (const struct sockaddr *) &cliaddr, len) < 0){
                            perror("sendto");
                        }
                    }
                }
            } // while running

            close(sockfd_);
        }); // worker thread

        return EXIT_SUCCESS;
    }

    int DiscoveryResponder::stop(){
        if (state_ == Stopped){
            return EXIT_SUCCESS;
        }
        state_ = Stopping;

        thread_->join();
        delete thread_;
        thread_ = nullptr;

        state_ = Stopped;

        return EXIT_SUCCESS;
    }

} // SQMixMitm