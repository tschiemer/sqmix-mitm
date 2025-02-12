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

#include "MixMitm.h"

#include <unistd.h>
#include <sys/socket.h>
//#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


#define BUFSIZE_MTU 1500

namespace SQMixMitm {

    int MixMitm::setSocketBlocking(int sockfd, int enable){

        int flags = fcntl(sockfd, F_GETFL);
        if (flags < 0){
            perror("fcntl F_GETFL failed");
            return EXIT_FAILURE;
        }

        if (!enable){
            flags = flags | O_NONBLOCK;
        } else {
            flags = flags & ~O_NONBLOCK;
        }

        // Set the server socket to non-blocking mode
        if (fcntl(sockfd, F_SETFL, flags) < 0) {
            perror("fcntl F_SETFL failed");
            return EXIT_FAILURE;
        }

//        if (!enable) {
//        return setSocketTimeout(sockfd, std::chrono::milliseconds (0));
//        }

        return EXIT_SUCCESS;
    }

    int MixMitm::setSocketTimeout(int sockfd, std::chrono::milliseconds ms){
        // set recv timeout
        struct timeval timeout;
        timeout.tv_sec = ms.count() / 1000;
        timeout.tv_usec = (ms.count() % 1000) * 1000;

        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0){
            perror("setsockopt SO_RCVTIMEO failed" );
            return EXIT_FAILURE;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0){
            perror("setsockopt SO_SNDTIMEO failed" );
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }


//    void MixMitm::resetSession(){
//
//    }

    int MixMitm::startTcpServer(){

        // Creating socket file descriptor
        if ( (tcpServerSockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
            perror("tcp server socket creation failed");
            return EXIT_FAILURE;
        }

        //// set some socket options
        // reuse address

        int optval = 1;
        setsockopt(tcpServerSockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

        // set recv timeout
//        setSocketTimeout(tcpServerSockfd_, 0);


        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));

        // Filling server information
        servaddr.sin_family    = AF_INET; // IPv4
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(TCPControlPort);

        // Bind the socket with the server address
        if (bind(tcpServerSockfd_, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ){
            perror("tcp server bind failed");
            close(tcpServerSockfd_);
            return EXIT_FAILURE;
        }

        if (listen(tcpServerSockfd_,1) < 0){
            perror("tcp server listen failed");
            close(tcpServerSockfd_);
            return EXIT_FAILURE;
        }

        setSocketBlocking(tcpServerSockfd_, 0);

        return EXIT_SUCCESS;
    }


    int MixMitm::stopTcpServer(){

        if (tcpServerSockfd_ == -1){
            return EXIT_SUCCESS;
        }

        disconnectTcpClient();

        setSocketBlocking(tcpServerSockfd_, 1);
        close(tcpServerSockfd_);
        tcpServerSockfd_ = -1;

        return EXIT_SUCCESS;
    }

    int MixMitm::processTcpServer() {

        int sockfd = -1;

        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));

        socklen_t slen = sizeof(addr);

        if ((sockfd = accept(tcpServerSockfd_, (struct sockaddr *)&addr, &slen)) < 0){
            if (errno != EAGAIN && errno != EWOULDBLOCK){
                perror("tcp server accept error");
                return EXIT_FAILURE;
            }
        } else { // accepted new tcp connection request

            // if already connected to a client, just disconnect again
            if (connectionState_ != ListeningForApp) {
                close(sockfd);
            } else { // otherwise try to connect

                client_.tcpSockfd = sockfd;
                client_.addr.s_addr = addr.sin_addr.s_addr;

                char * ip = inet_ntoa(addr.sin_addr);
                printf("Connection from %s:%d\n", ip, ntohs(addr.sin_port));

                setSocketBlocking(sockfd, 0);

                setConnectionState(AwaitClientUdpPort);
            }

        } // accepted new tcp connection request

        return EXIT_SUCCESS;
    }
    int MixMitm::processTcpClient(){
//        printf("processTcpClient\n");

        int n;
        char buffer[BUFSIZE_MTU];

        if ((n = read(client_.tcpSockfd, buffer, sizeof(buffer))) <= 0){

            // if client hanged up
            if (n == 0){
                printf("client disconnected\n");
                setConnectionState(Disconnected);
                //TODO event?
                return EXIT_SUCCESS;
            }
            // if timeout, just ignore
            else if (errno == EAGAIN){
                // do nothing
                return EXIT_SUCCESS;
            }
            else {
                perror("TCP client: Unknown socket error\n");
                return EXIT_FAILURE;
            }
        }

        if (connectionState_ == Ready || connectionState_ == ReadyAwaitingVersion) {

            // pass along to app
            if (write(mixer_.tcp.sockfd, buffer, n) < 0){
                perror("failed forwarding to mixer.tcp");
                return EXIT_FAILURE;
            }
        }
        else if (connectionState_ == AwaitClientUdpPort){

            if (n == sizeof(MsgUdpPortInfo) && memcmp(buffer, MsgUdpPortInfo, sizeof(MsgUdpPortInfo)-2) == 0){

                // get clients udp port
                int p = (((unsigned char)buffer[7]) << 8) | buffer[6];
                client_.udpPort = htons(p);

                printf("Client UDP port = %d\n", htons(p));

                setConnectionState(ConnectToMixer);
            } else {
                perror("UDP client did not send used UDP port, hanging up");

                disconnectTcpClient();

                setConnectionState(ListeningForApp);
                return EXIT_FAILURE;
            }
        }
        else { // any other state
            printf("tcp.rx (%d) DISCARDING\n" , n);
            // TODO do nothing?
        }

        return EXIT_SUCCESS;
    }

    int MixMitm::disconnectTcpClient(){

        if (client_.tcpSockfd == -1){
            return EXIT_SUCCESS;
        }

        setSocketBlocking(client_.tcpSockfd, 1);
        close(client_.tcpSockfd);
        client_.tcpSockfd = -1;

        client_.addr.s_addr = 0;

        return EXIT_SUCCESS;
    }

    int MixMitm::startUdpServer(){

        // Creating socket file descriptor
        if ( (udpServerSockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            perror("udp server socket creation failed");
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        //// set some socket options
        // reuse port
        int optval = 1;
        setsockopt(udpServerSockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

        // set recv timeout
//        struct timeval timeout;
//        timeout.tv_sec = 0;
//        timeout.tv_usec = 500000;
//        setsockopt(udpServerSockfd_, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


        struct sockaddr_in servaddr;
        memset(&servaddr, 0, sizeof(servaddr));

        // Filling server information
        servaddr.sin_family    = AF_INET; // IPv4
        servaddr.sin_addr.s_addr = INADDR_ANY;
        servaddr.sin_port = htons(UDPStreamingPort);

        // Bind the socket with the server address
        if (bind(udpServerSockfd_, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ){
            perror("udp server bind failed");
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        // set non-blocking
        setSocketBlocking(udpServerSockfd_, 0);

        return EXIT_SUCCESS;
    }

    int MixMitm::stopUdpServer(){
        if (udpServerSockfd_ == -1){
            return EXIT_SUCCESS;
        }

        setSocketBlocking(udpServerSockfd_, 1);
        close(udpServerSockfd_);
        udpServerSockfd_ = -1;

        return EXIT_SUCCESS;
    }

    int MixMitm::processUdpServer(){

        struct sockaddr_in srcaddr;
        socklen_t len;
        int n;
        char buffer[BUFSIZE_MTU];

        len = sizeof(srcaddr);  //len is value/result

        memset(&srcaddr, 0, sizeof(srcaddr));

        bool keep_reading = true;

        // keep receiving packets until none available anymore
        while(keep_reading){

            if ((n = recvfrom(udpServerSockfd_, (char *)buffer, sizeof(buffer), 0, ( struct sockaddr *) &srcaddr, &len)) <= 0){

                keep_reading = false;

                // if timeout, just ignore
                if (n == 0 || errno == EAGAIN){
                    // no packets availa
//                    printf("no packets available\n" );
//                    std::this_thread::sleep_for(std::chrono::seconds(1));
                } else {
                    perror("udp server processing: Unknown socket error during read\n");
                }
            }
            // only process if connection is all ready && packet coming from actual client
            else if (connectionState_ == Ready){// && srcaddr.sin_addr.s_addr == client_.addr.s_addr && srcaddr.sin_port == client_.udpPort) {

                struct sockaddr_in dstaddr;
                memset(&dstaddr, 0, sizeof(dstaddr));

                len = sizeof(dstaddr);  //len is value/result

                // Filling server information
                dstaddr.sin_family = AF_INET;
                dstaddr.sin_port = mixer_.udp.remotePort;
                dstaddr.sin_addr.s_addr = mixer_.addr.s_addr;

                //
                if (sendto(mixer_.udp.sockfd, buffer, n, 0, (const struct sockaddr *) &dstaddr, len) < 0){
                    perror("UDP sendto mixer");
                }


                if (n == sizeof(MsgKeepAlive) && memcmp(buffer, MsgKeepAlive, sizeof(MsgKeepAlive)) == 0){
//                    printf("from Client: keep-alive\n" );
                } else {
                    //do nothing
                }
            }
        } // while reading

//        printf("< processUdpServer\n");
        return EXIT_SUCCESS;
    }

    int MixMitm::connectToMixer(){

        // Creating socket file descriptor
        if ( (mixer_.tcp.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0 ) {
            perror("tcpToMixerSockfd_ socket creation failed");
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        //// set some socket options

        struct sockaddr_in tcpservaddr;
        memset(&tcpservaddr, 0, sizeof(tcpservaddr));

        // Filling addr information
        tcpservaddr.sin_family    = AF_INET; // IPv4
        tcpservaddr.sin_port = htons(TCPControlPort); // TODO set correct port
        tcpservaddr.sin_addr.s_addr = mixer_.addr.s_addr;

        if (::connect(mixer_.tcp.sockfd, (const struct sockaddr *)&tcpservaddr, sizeof(tcpservaddr)) < 0){
            perror("tcp to mixer connect failed");
            return EXIT_FAILURE;
        }


        // Creating socket file descriptor
        if ( (mixer_.udp.sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            perror("socket creation failed");
            return EXIT_FAILURE;
        }

        //// set some socket options
        // reuse port
        int optval = 1;
        setsockopt(mixer_.udp.sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));


        struct sockaddr_in udpservaddr;
        memset(&udpservaddr, 0, sizeof(udpservaddr));

        // Filling server information
        udpservaddr.sin_family    = AF_INET; // IPv4
        udpservaddr.sin_addr.s_addr = INADDR_ANY;
        udpservaddr.sin_port = 0;

        // Bind the socket with the server address
        if (bind(mixer_.udp.sockfd, (const struct sockaddr *)&udpservaddr, sizeof(udpservaddr)) < 0 ){

            perror("udp to mixer bind failed");

            return EXIT_FAILURE;
        }

        socklen_t socklen = sizeof(udpservaddr);
        getsockname(mixer_.udp.sockfd, (struct sockaddr *)&udpservaddr, &socklen);
        mixer_.udp.localPort = udpservaddr.sin_port;

        printf("Mixer.UDP local port = %d\n", ntohs(udpservaddr.sin_port));

        setSocketBlocking(mixer_.tcp.sockfd, 0);
        setSocketBlocking(mixer_.udp.sockfd, 0);

        return EXIT_SUCCESS;
    }

    int MixMitm::disconnectFromMixer(){

        if (mixer_.udp.sockfd != -1){
            setSocketBlocking(mixer_.udp.sockfd, 1);
            close(mixer_.udp.sockfd);
            mixer_.udp.sockfd = -1;

            mixer_.udp.remotePort = 0;
            mixer_.udp.localPort = 0;
        }
        if (mixer_.tcp.sockfd != -1){
            setSocketBlocking(mixer_.tcp.sockfd, 1);
            close(mixer_.tcp.sockfd);
            mixer_.tcp.sockfd = -1;
        }

        return EXIT_SUCCESS;
    }

    int MixMitm::processMixerTcp(){

        int n;
        char buffer[BUFSIZE_MTU];

        if ((n = read(mixer_.tcp.sockfd, buffer, sizeof(buffer))) <= 0){

            // if mixer closed connection
            if (n == 0){
                //TODO cleanup
                // no data?
                printf("mixer disconnected\n");
                setConnectionState(Disconnected);
                return EXIT_SUCCESS;
            }
            // if timeout, just ignore
            else if (errno == EAGAIN){
                // do nothing
            }
            else {
                perror("TCP mixer: Unknown socket error\n");
                //TODO ?
                return EXIT_FAILURE;
            }
        }
        else if (connectionState_ == Ready || connectionState_ == ReadyAwaitingVersion) {

            // pass along to app

            if (n == 165){
                // local mac + ip
                constexpr unsigned char mac[] = {0x5c, 0x1b, 0xf4, 0x81, 0x22, 0x39};
                constexpr unsigned char ip[] = {10,0,10,253};


//                memcpy(&buffer[77], mac, sizeof(mac));
                // inverted copy
//                for(int i = 0; i < sizeof(mac); i++){
//                    buffer[77+i] = mac[5-i];
//                }
//                memcpy(&buffer[101], ip, sizeof(ip));
//
//                printf("ip inserted\n");
            }

            if (write(client_.tcpSockfd, buffer, n) < 0){
                perror("failed forwarding to mixer.tcp");
                return EXIT_FAILURE;
            }

            if (connectionState_ == ReadyAwaitingVersion){
                printf("ReadyAwaitingVersion rx (%d)\n", n);
                if (n == 18 && memcmp(buffer, MsgVersionResponseHdr, sizeof(MsgVersionResponseHdr)) == 0){
                    //TODO deal with version data (compatibility change?)
                }
                setConnectionState(Ready);
            }

        }
        else if (connectionState_ ==  AwaitMixerUdpPort){

//            printf("AwaitMixerUdpPort TCP.rx (%d)\n", n);

            if (n == sizeof(MsgUdpPortInfo) && memcmp(buffer, MsgUdpPortInfo, sizeof(MsgUdpPortInfo)-2) == 0){

                // get mixer udp port from msg
                int p = (((unsigned char)buffer[7]) << 8) | buffer[6];
                mixer_.udp.remotePort = htons(p);

                printf("Mixer UDP remote port = %d\n", p);

                // use this point in time to respond to client with own udp port msg
                if (sendUdpPortTo(client_.tcpSockfd, UDPStreamingPort)){
                    perror("failed to inform client of own UDP port");

                    return EXIT_FAILURE;
                }

                setConnectionState(ReadyAwaitingVersion);

            } else {
                perror("expected other first message from mixer");
                return EXIT_FAILURE;
            }
        }

        return EXIT_SUCCESS;
    }

    int MixMitm::processMixerUdp(){

        struct sockaddr_in srcaddr;
        socklen_t len;
        int n;
        char buffer[BUFSIZE_MTU];

        len = sizeof(srcaddr);  //len is value/result

        memset(&srcaddr, 0, sizeof(srcaddr));

        bool keep_reading = true;

        // keep receiving packets until none available anymore
        while(keep_reading){

            if ((n = recvfrom(mixer_.udp.sockfd, (char *)buffer, sizeof(buffer), 0, ( struct sockaddr *) &srcaddr, &len)) < 0){

                keep_reading = false;

                // if timeout, just ignore
                if (errno == EAGAIN){
                    // no packets available
                } else {
                    perror("udp server processing: Unknown socket error during read\n");
                }
            }
            // only process if ready && packet coming from mixer
            else if (connectionState_ == Ready && srcaddr.sin_addr.s_addr == mixer_.addr.s_addr && srcaddr.sin_port == mixer_.udp.remotePort) {

                struct sockaddr_in dstaddr;
                memset(&dstaddr, 0, sizeof(dstaddr));

                len = sizeof(dstaddr);  //len is value/result

                // Filling server information
                dstaddr.sin_family = AF_INET;
                dstaddr.sin_port = client_.udpPort;
                dstaddr.sin_addr.s_addr = client_.addr.s_addr;

                //
                if (sendto(udpServerSockfd_, buffer, n, 0, (const struct sockaddr *) &dstaddr, len) < 0){
                    perror("UDP sendto app");
                }


                if (n == sizeof(MsgKeepAlive) && memcmp(buffer, MsgKeepAlive, sizeof(MsgKeepAlive)) == 0){
//                    printf("from Mixer: keep-alive\n" );
                }
            }

        } // while reading

        return EXIT_SUCCESS;
    }

    int MixMitm::sendUdpPortTo(int sockfd, int port){

        int p = port;

        char msg[sizeof(MsgUdpPortInfo)];

        memcpy(msg, MsgUdpPortInfo, sizeof(MsgUdpPortInfo));

        msg[6] = p & 0xff;
        msg[7] = (p >> 8) & 0xff;

        if (write(sockfd,msg, sizeof(msg)) < 0){
            perror("socket write error");
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
    int MixMitm::sendVersionRequestTo(int sockfd){

        if (write(sockfd,MsgVersionRequest, sizeof(MsgVersionRequest)) < 0){
            perror("socket write error");
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }


    int MixMitm::processingLoop(){

        state_ = Running;

        while(state_ == Running){

            //TODO use select() or poll() to wait for an actual event to happen instead of busy waiting...

            // listens for connections, accepts only a single at any single time
            //TODO handle error returns
            processTcpServer();

            // handles incoming packets
            //TODO handle error returns
            processUdpServer();

            if (ListeningForApp < connectionState_ && connectionState_ < Disconnected){
                processTcpClient();
            }

            if (ConnectToMixer < connectionState_ && connectionState_ < Disconnected){
                //TODO handle error returns
                processMixerTcp();
                processMixerUdp();
            }
            else {
                if (connectionState_ == AwaitClientUdpPort){
                    // is handled in processTcpClient()
                    // if received: --> ConnectToMixer
                    // if other first message: --> TODO
                }
                if (connectionState_ == ConnectToMixer){
                    if (connectToMixer()){
                        perror("failed connection to mixer");

                        disconnectTcpClient();

//                        connectionState_ = ListeningForApp;
                        setConnectionState(ListeningForApp);
                        continue;
                    }

                    // connected to mixer

                    if (sendUdpPortTo(mixer_.tcp.sockfd, ntohs(mixer_.udp.localPort))){
                        //|| sendVersionRequestTo(mixer_.tcp.sockfd
                        perror("Failed sending udp port o mixer");

                        disconnectFromMixer();
                        disconnectTcpClient();
//                        connectionState_ = ListeningForApp;

                        setConnectionState(Disconnected);

                        continue;
                    }

                    setConnectionState(AwaitMixerUdpPort);
                }

                if (connectionState_ == Disconnected){

                    disconnectFromMixer();
                    disconnectTcpClient();

                    setConnectionState(ListeningForApp);
                }
            }

//            std::this_thread::sleep_for(std::chrono::microseconds (1000000));
        }

        disconnectTcpClient();
        disconnectFromMixer();

        return EXIT_SUCCESS;
    }

    int MixMitm::start(std::string &mixerIp){

        // parameter validation
        if (inet_aton(mixerIp.c_str(), &mixer_.addr) != 1){
            perror("Invalid ip??");
            return EXIT_FAILURE;
        }

        if (state_ != Stopped){
            return EXIT_FAILURE;
        }
        state_ = Starting;

        if (startTcpServer()){
            state_ = Stopped;
            return EXIT_FAILURE;
        }

        if (startUdpServer()){
            stopTcpServer();
            state_ = Stopped;
            return EXIT_FAILURE;
        }

//        connectionState_ = ListeningForApp;
        setConnectionState(ListeningForApp);

        thread_ = new std::thread(&MixMitm::processingLoop, this);

        // wait for thread to actually have started
        while (state_ == Starting){
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }

        return state_ == Running ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    int MixMitm::stop(){
        if (state_ == Stopped){
            return EXIT_SUCCESS;
        }
        state_ = Stopping;

        thread_->join();
        delete thread_;
        thread_ = nullptr;

        stopTcpServer();
        stopUdpServer();

        state_ = Stopped;

        return EXIT_SUCCESS;
    }

} // SQMixMitm