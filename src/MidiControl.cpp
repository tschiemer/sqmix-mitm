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

#include "MidiControl.h"

#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

namespace SQMixMitm {

    int MidiControl::connect(std::string &mixerIp, ReceivedDataCallback callback){

        if (state_ != Stopped){
            return EXIT_FAILURE;
        }
        state_ = Starting;

        // Creating socket file descriptor
        if ( (sockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
            perror("socket creation failed");
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        //// set some socket options

        // set recv timeout
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000;
        setsockopt(sockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));

        // Filling addr information
        servaddr.sin_family    = AF_INET; // IPv4
        servaddr.sin_port = htons(Port);

        if (inet_aton(mixerIp.c_str(), (struct in_addr*)&(servaddr.sin_addr.s_addr)) != 1){
            perror("Invalid ip??");
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        if (::connect(sockfd_, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0){
            perror("connect failed");
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        state_ = Running;

        thread_ = new std::thread([&, callback](){

            char buffer[512]; //

            int n;

            while (state_ == Running){

                if ((n = read(sockfd_, (char *)buffer, sizeof(buffer))) <= 0){

                    // if other side closed connection
                    if (n == 0){
                        state_ = Stopping;
                    }
                    // if timeout, just ignore
                    else if (errno == EAGAIN){
                        // do nothing
                    } else {
                        perror("Unknown socket error, stopping\n");

                        // stop loop and end thread
                        state_ = Stopping;
                    }
                } else {
                    callback(buffer, n);
                }
            } // while running

            close(sockfd_);
        }); // worker thread

        return EXIT_SUCCESS;
    }

    int MidiControl::disconnect(){
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

    int MidiControl::send(const void * data, size_t len){
        if (state_ != Running){
            return EXIT_FAILURE;
        }

        return write(sockfd_, data, len);
    }

} // SQMixMitm