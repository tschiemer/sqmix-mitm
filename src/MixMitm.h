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

//#include <sys/socket.h>
#include <netinet/in.h>

namespace SQMixMitm {

    class MixMitm {

    public:

        enum State {Stopped, Starting, Running, Stopping};

        typedef std::function<void(State)> StateChangedCallback;

        typedef std::function<void(unsigned int)> ChannelSelectCallback;
        typedef std::function<void(unsigned char, unsigned char, unsigned char)> MidiCallback;

    protected:

        enum ConnectionState {
            ListeningForApp         = 0,
            AwaitClientUdpPort         ,
            ConnectToMixer          ,
            AwaitMixerUdpPort       ,
            ReadyAwaitingVersion,
            Ready,
            Disconnected,
            ClientDisconnected,
            MixerDisconnected
        };

    public:

        static constexpr unsigned int UDPStreamingPort = 51324;
        static constexpr unsigned int TCPControlPort = 51326;

        static constexpr unsigned char MsgKeepAlive[]        = {0x7f, 0x05, 0x00, 0x00, 0x00, 0x00};

        static constexpr unsigned char MsgUdpPortInfo[]      = {0x7f, 0x00, 0x02, 0x00, 0x00, 0x00, 0, 0};

        static constexpr unsigned char MsgVersionRequest[]   = {0x7f, 0x01, 0x00, 0x00, 0x00, 0x00};
        static constexpr unsigned char MsgVersionResponseHdr[]  = {0x7f, 0x02, 0x0c, 0x00, 0x00, 0x00};

        static constexpr unsigned char MsgSoftControlMIDIHdr[]   = {0xf7, 0x21, 0x1f, 0x14};

    protected:

        std::atomic<State> state_ = Stopped;
        std::atomic<ConnectionState> connectionState_;

        StateChangedCallback stateChangedCallback_ = nullptr;

        ChannelSelectCallback channelSelectCallback_ = nullptr;
        MidiCallback midiCallback_ = nullptr;

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

        void setConnectionState(ConnectionState state){
            connectionState_ = state;
//            printf("ConnectionState = %d\n", state);
        }

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
        int sendVersionRequestTo(int sockfd);

        int processingLoop();



    public:

        void onStateChanged(StateChangedCallback callback){ stateChangedCallback_ = callback; }

        void onChannelSelect(ChannelSelectCallback callback){ channelSelectCallback_ = callback; }
        void onMidi(MidiCallback callback){ midiCallback_ = callback; }

        State state(){ return state_; }

        int start(std::string &mixerIp);

        int start(char mixerIp[]){
            std::string str = mixerIp;
            return start(str);
        }

        int stop();


    };


} // SQMixMitm

#endif //SQMIX_MITM_MIXMITM_H
