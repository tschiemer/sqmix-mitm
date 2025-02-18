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

#ifndef SQMIX_MITM_MIXMITM_H
#define SQMIX_MITM_MIXMITM_H

#include <thread>
#include <map>

//#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/poll.h>

#include "Version.h"
#include "Event.h"
#include "Command.h"

namespace SQMixMitm {

    class MixMitm {

    public:

        enum State {Stopped, Starting, Running, Stopping};

        enum ConnectionState {Disconnected, Connected};

        typedef std::function<void(State)> StateChangedCallback;

        typedef std::function<void(ConnectionState, Version&)> ConnectionStateChangedCallback;

        typedef std::function<void(Event&)> EventCallback;


    protected:

        enum InternalState {
            ListeningForApp         = 0,
            AwaitClientUdpPort         ,
            ConnectToMixer          ,
            AwaitMixerUdpPort       ,
            ReadyAwaitingVersion,
            Ready,
            Disconnection,
        };

    public:

        static constexpr unsigned int UDPStreamingPort = 51324;
        static constexpr unsigned int TCPControlPort = 51326;

        static constexpr unsigned char MsgHeader = 0xf7;
        static constexpr unsigned char MsgVariableSizeType = 0x08;

        static constexpr unsigned char MsgKeepAlive[]        = {0x7f, 0x05, 0x00, 0x00, 0x00, 0x00};

        static constexpr unsigned char MsgUdpPortInfo[]      = {0x7f, 0x00, 0x02, 0x00, 0x00, 0x00, 0, 0};

        static constexpr unsigned char MsgVersionRequest[]   = {0x7f, 0x01, 0x00, 0x00, 0x00, 0x00};
        static constexpr unsigned char MsgVersionResponseHdr[]  = {0x7f, 0x02, 0x0c, 0x00, 0x00, 0x00};


//        static constexpr unsigned char MsgSoftControlMIDIHdr[]   = {0xf7, 0x21, 0x1f, 0x14};

    protected:

        std::atomic<State> state_ = Stopped;
        std::atomic<InternalState> internalState_;

        int udpServerSockfd_ = -1;
        int tcpServerSockfd_ = -1;

        struct {
            int tcpSockfd = -1;
            struct in_addr addr = {0};
            in_port_t udpPort = 0;
        } client_;

        struct {
            struct in_addr addr = {0};
            struct {
                int sockfd = -1;
                in_port_t localPort = 0;
                in_port_t remotePort = 0;
            } udp;
            struct {
                int sockfd = -1;
            } tcp;
        } mixer_;

        std::thread * thread_ = nullptr;

    protected:

        Version version_;
//        bool versionIsSupported_ = false; // in principle not needed, but saving computation

        StateChangedCallback stateChangedCallback_ = nullptr;
        ConnectionStateChangedCallback connectionStateChangedCallback_ = nullptr;

        Event::Parser eventParser_;
        std::map<Event::Type, EventCallback> eventCallbacks_;

        Command::Factory commandFactory_;

    public:


    protected:

        void setInternalState(InternalState state);

        int setSocketBlocking(int sockfd, int enable);
        int setSocketTimeout(int sockfd, std::chrono::milliseconds ms);

        int startTcpServer();
        int stopTcpServer();
        int processTcpServer();

        int processTcpClient();
        int disconnectTcpClient();

        int startUdpServer();
        int stopUdpServer();
        int processUdpServer();

        int connectToMixer();
        int disconnectFromMixer();
        int processMixerTcp();
        int processMixerUdp();

        int sendUdpPortTo(int sockfd, int port);

        void waitUntilEvent(int timeout_ms);
        int processingLoop();

        void publishEvent(Event &event);

    public:

        State state(){ return state_; }
        ConnectionState connectionState() { return (internalState_ == Ready ? Connected : Disconnected); }

        Version version(){ return version_; }

        void onStateChanged(StateChangedCallback callback){ stateChangedCallback_ = callback; }
        void onConnectionStateChanged(ConnectionStateChangedCallback callback){ connectionStateChangedCallback_ = callback; }

        void onEvent(Event::Type type, EventCallback callback);

        Command::Factory &commandFactory() { return commandFactory_; }
        void sendCommand(Command command);

        int start(std::string &mixerIp);

        int start(char mixerIp[]){
            std::string str = mixerIp;
            return start(str);
        }

        int stop();


    };


} // SQMixMitm

#endif //SQMIX_MITM_MIXMITM_H
