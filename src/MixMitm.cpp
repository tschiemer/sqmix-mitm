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
#include <cassert>

#include "log.h"


#define BUFSIZE_MTU 1500

namespace SQMixMitm {

    MixMitm::Version::Version(Version &other){
        memcpy(bytes_, other.bytes_, sizeof(bytes_));
    }

    MixMitm::Version::Version(uint8_t major, uint8_t minor, uint8_t patch, uint16_t build){
        bytes_[7] = major;
        bytes_[8] = minor;
        bytes_[9] = patch;
        bytes_[10] = build & 0xff;
        bytes_[11] = (build >> 8) & 0xff;
    }

    void MixMitm::Version::fromBytes(char bytes[]){
        memcpy(bytes_, bytes, sizeof(bytes_));
    }

    void MixMitm::Version::clear(){
        memset(bytes_, 0, sizeof(bytes_));
    }

    bool MixMitm::Version::operator==(Version &other){
        return (major() == other.major() &&
                minor() == other.minor() &&
                (patch() == 0 || other.patch() == 0 || patch() == other.patch()));
    }
    bool MixMitm::Version::operator<(Version &other){
        return (major() < other.major() ||
                (major() == other.major() &&
                 (minor() < other.minor() ||
                  (minor() == other.minor() &&
                   (patch() < other.patch())))));
    }
    bool MixMitm::Version::operator>(Version &other){
        return (major() > other.major() ||
                (major() == other.major() &&
                 (minor() > other.minor() ||
                  (minor() == other.minor() &&
                   (patch() > other.patch())))));
    }
    bool MixMitm::Version::operator<=(Version &other){
        return operator==(other) || operator<(other);
    }
    bool MixMitm::Version::operator>=(Version &other){
        return operator==(other) || operator>(other);
    }

    void MixMitm::setInternalState(InternalState state){
        internalState_ = state;

        if (state == Disconnection){
            version_.clear();
        }

        if (connectionStateChangedCallback_ != nullptr){
            if (state == Ready){
                connectionStateChangedCallback_(Connected,version_);
            } else if (state == Disconnection){
                connectionStateChangedCallback_(Disconnected,version_);
            }
        }
    }

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
            if (internalState_ != ListeningForApp) {
                close(sockfd);
            } else { // otherwise try to connect

                client_.tcpSockfd = sockfd;
                client_.addr.s_addr = addr.sin_addr.s_addr;

                char * ip = inet_ntoa(addr.sin_addr);

                log(LogLevelInfo, "Connection from %s:%d", ip, ntohs(addr.sin_port));

                setSocketBlocking(sockfd, 0);

                setInternalState(AwaitClientUdpPort);
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
                setInternalState(Disconnection);

                log(LogLevelInfo, "Client disconnected");

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

        if (internalState_ == Ready || internalState_ == ReadyAwaitingVersion) {

            // pass along to app
            if (write(mixer_.tcp.sockfd, buffer, n) < 0){
                perror("failed forwarding to mixer.tcp");
                return EXIT_FAILURE;
            }

            if (internalState_ == Ready && n == 8){
                Event event(buffer);

                EventCallback callback = eventCallbacks_[event.type];
                if (callback){
                    callback(event);
                }
            }
        }
        else if (internalState_ == AwaitClientUdpPort){

            if (n == sizeof(MsgUdpPortInfo) && memcmp(buffer, MsgUdpPortInfo, sizeof(MsgUdpPortInfo)-2) == 0){

                // get clients udp port
                int p = (((unsigned char)buffer[7]) << 8) | buffer[6];
                client_.udpPort = htons(p);

                log(LogLevelDebug, "Client UDP port = %d", htons(p));

                setInternalState(ConnectToMixer);
            } else {
                perror("UDP client did not send used UDP port, hanging up");

                disconnectTcpClient();

                setInternalState(ListeningForApp);
                return EXIT_FAILURE;
            }
        }
        else { // any other state
            log(LogLevelDebug, "discarding tcp data (len=%d)", n);
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
            else if (internalState_ == Ready){// && srcaddr.sin_addr.s_addr == client_.addr.s_addr && srcaddr.sin_port == client_.udpPort) {

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

        log(LogLevelDebug, "Local UDP port for mixer con: %d", ntohs(udpservaddr.sin_port));

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
                setInternalState(Disconnection);

                log(LogLevelDebug, "Mixer disconnected");

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
        else if (internalState_ == Ready || internalState_ == ReadyAwaitingVersion) {

            // pass along to app

            if (write(client_.tcpSockfd, buffer, n) < 0){
                perror("failed forwarding to mixer.tcp");
                return EXIT_FAILURE;
            }

            if (internalState_ == ReadyAwaitingVersion){

                if (n == 18 && memcmp(buffer, MsgVersionResponseHdr, sizeof(MsgVersionResponseHdr)) == 0){

                    version_.fromBytes(buffer + sizeof(MsgVersionResponseHdr));

                }
                setInternalState(Ready);
            }

            // well, this here is not proper handling of the incoming data stream, but it's working well enough
            // the semantics of the protocol are a bit unclear, sometimes bytes 2-5 denote the length of the data
            // and sometimes it seems they denote a sort of subtype.
            for(unsigned int i = 0; i+7 < n;){

                // sanity check, if this fails then this is not the start of a message we should be looking at
                if (MsgHeader != (unsigned char)buffer[i]){
                    break;
                }

                // first check if it is a type known to us
                if (Event::isValidType(buffer)){
                    // if so, process it further
                    Event event(buffer + i);
                    publishEvent(event);

                    // for all we know, all these events are exactly 8 bytes long
                    i += 8;
                    continue;
                }

                // messages with variable size typically have a 0x08 as second byte
                // and data size is little-endian within the next four bytes
                if (MsgVariableSizeType == (unsigned char)buffer[i+1]){
                    i += 2;
                    unsigned int j = buffer[i++];
                    j += ((unsigned int)buffer[i++]) << 8;
                    j += ((unsigned int)buffer[i++]) << 16;
                    j += ((unsigned int)buffer[i++]) << 24;
//                    printf("++ %d\n" , j);
                    i += j;
                    continue;
                }

                // this is just a guess as many messages are just 8 bytes long
                i += 8;
            }

        }
        else if (internalState_ == AwaitMixerUdpPort){

//            printf("AwaitMixerUdpPort TCP.rx (%d)\n", n);

            if (n >= sizeof(MsgUdpPortInfo) && memcmp(buffer, MsgUdpPortInfo, sizeof(MsgUdpPortInfo)-2) == 0){

                // get mixer udp port from msg
                int p = (((unsigned char)buffer[7]) << 8) | buffer[6];
                mixer_.udp.remotePort = htons(p);

                log(LogLevelDebug, "UDP port of mixer = %d", p);

                // use this point in time to respond to client with own udp port msg
                if (sendUdpPortTo(client_.tcpSockfd, UDPStreamingPort)){
                    perror("failed to inform client of own UDP port");

                    return EXIT_FAILURE;
                }

                setInternalState(ReadyAwaitingVersion);

                // the client sends both messages right away, so the version request might get caught here already
                // in that case, let's just trigger sending it directly instead of passing it on later elsewhere
                // during the normal processing
                if (n == sizeof(MsgUdpPortInfo) + sizeof(MsgVersionRequest) && memcmp(buffer + sizeof(MsgUdpPortInfo), MsgVersionRequest, sizeof(MsgVersionRequest))){
                    write(mixer_.tcp.sockfd, MsgVersionRequest, sizeof(MsgVersionRequest));
                }

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
            else if (internalState_ == Ready && srcaddr.sin_addr.s_addr == mixer_.addr.s_addr && srcaddr.sin_port == mixer_.udp.remotePort) {

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

    void MixMitm::waitUntilEvent(int timeout_ms){

        nfds_t n = 0;
        struct pollfd fds[5];

        if (tcpServerSockfd_ != -1){
            fds[n++].fd = tcpServerSockfd_;
        }
        if (udpServerSockfd_ != -1){
            fds[n++].fd = udpServerSockfd_;
        }
        if (client_.tcpSockfd != -1){
            fds[n++].fd = client_.tcpSockfd;
        }
        if (mixer_.tcp.sockfd != -1){
            fds[n++].fd = mixer_.tcp.sockfd;
        }
        if (mixer_.udp.sockfd != -1){
            fds[n++].fd = mixer_.udp.sockfd;
        }

        for(int i = 0; i < n; i++){
            fds[i].events = POLLIN | POLLOUT;
            fds[i].revents = POLLHUP | POLLERR;
        }

        if (poll(fds, n, timeout_ms) < 0){
            perror("polling error?!");
        }
    }

    int MixMitm::processingLoop(){

        state_ = Running;

        while(state_ == Running){

            waitUntilEvent(1000);

            // listens for connections, accepts only a single at any single time
            //TODO handle error returns
            processTcpServer();

            // handles incoming packets
            //TODO handle error returns
            processUdpServer();

            if (ListeningForApp < internalState_ && internalState_ < Disconnection){
                processTcpClient();
            }

            if (ConnectToMixer < internalState_ && internalState_ < Disconnection){
                //TODO handle error returns
                processMixerTcp();
                processMixerUdp();
            }
            else {
                if (internalState_ == AwaitClientUdpPort){
                    // is handled in processTcpClient()
                    // if received: --> ConnectToMixer
                    // if other first message: --> TODO
                }
                if (internalState_ == ConnectToMixer){
                    if (connectToMixer()){
                        perror("failed connection to mixer");

                        disconnectTcpClient();

//                        connectionState_ = ListeningForApp;
                        setInternalState(ListeningForApp);
                        continue;
                    }

                    // connected to mixer

                    if (sendUdpPortTo(mixer_.tcp.sockfd, ntohs(mixer_.udp.localPort))){
                        //|| sendVersionRequestTo(mixer_.tcp.sockfd
                        perror("Failed sending udp port o mixer");

                        disconnectFromMixer();
                        disconnectTcpClient();
//                        connectionState_ = ListeningForApp;

                        setInternalState(Disconnection);

                        continue;
                    }

                    setInternalState(AwaitMixerUdpPort);
                }

                if (internalState_ == Disconnection){

                    disconnectFromMixer();
                    disconnectTcpClient();

                    setInternalState(ListeningForApp);
                }
            }
        } // while (Running)

        disconnectTcpClient();
        disconnectFromMixer();

        return EXIT_SUCCESS;
    }

    void MixMitm::publishEvent(Event &event){
//        printf("Publishing? %08x %08x\n", event.type, event.data);

        EventCallback callback = eventCallbacks_[event.type];

        // sanity check
        if (callback != nullptr){
            callback(event);
        }
    }

    void MixMitm::onEvent(Event::Type type, EventCallback callback){
        if (callback == nullptr){
            if (eventCallbacks_.contains(type)){
                eventCallbacks_.erase(type);
            }
        } else {
            eventCallbacks_[type] = callback;
        }
    }


    void MixMitm::sendCommand(Command command){
        if (internalState_ != Ready || state_ != Running){
            return;
        }

//        printf("Sending: ");
//        for(int i = 0; i < sizeof(command.bytes); i++){
//            printf("%02x", command.bytes[i]);
//        }
//        printf("\n");

        if (write(mixer_.tcp.sockfd, command.bytes, sizeof(command.bytes)) < 0){
            perror("Failed to send command");
        }
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
        setInternalState(ListeningForApp);

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