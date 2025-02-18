# sqmix-mitm
Man-in-the-Middle basis for SQ Mix control

Please be aware that this library is intended to allow for integration of SQ mixing surfaces into other software solutions where the software provided is insufficient for actual integration.

The `sq-mitm` utility is an example of how channel select, midi faders and midi soft controls can be integrated. Noticably, for actual use official client software (such as SQ MixPad) is required. This is not a standalone solution. 


## Utils

### discovery
```shell
Usage: ./sq-discovery
Just shows which mixers were found and for which there is a timeout (of 5 secs)
```

### discovery-responder
```shell
Usage: ./sq-discovery-responder <name-of-pretend-mixer>
Listen to any SQ Mixer inquiries and respond such that *this* host is assumed to be a mixer of the SQ series.
```

### midi-control
```shell
Usage: ./sq-midi-control <ip-of-mixer>
Connects to mixer and receives MIDI data (well, actually the raw data stream) dumping it to stdout
```

### mitm
```shell
/sq-mitm [-e[<event>]]* [-s] <ip-of-mixer>
Act as Man-in-the-Middle service for mixer with given IP
Note: requires service to be discoverable (such as when using sq-discovery-responder)

Options:
	 -v            Increase verbosity (max 2x)
	 -e[<event>]   Show events, if no option specified shows all events (<event> in "channelselect", "midifader", "midisoftkey", "midisoftrotary", "midimmc")
	 -s            Slowly fade MIDI faders 1+2 up from 0% to 100%

Examples:
./sq-mitm -vv -e 192.168.1.100 # very verbose, shows all events
./sq-mitm -e channelselect 192.168.1.100 # show only channel select events
./sq-mitm -s 192.168.1.100 # send commands
```

## Protocol basics

### Ports

| Port  | Protocol | Purpose         | Details                                                                                                  |
|-------|----------|-----------------|----------------------------------------------------------------------------------------------------------|
| 51320 | UDP      | Discovery       | Request: Broadcast plaintext message "`SQ Find`"                                                         |
|       |          |                 | Response: plaintext null-terminated name of mixer as set in mixer config (ex. default for SQ6 "`SQ6\0`") |
| 51325 | TCP      | SQ MIDI Control | Raw MIDI data stream                                                                                     |
| 51324 | UDP      | SQ Mix          | Can be used for keep-alives (maybe only used for this?)                                                  |
| 51326 | TCP      | SQ Mix          | Control data port                                                                                        |

### Handshake

| Client                              | Action                                                                                                 | Mixer                        |
|-------------------------------------|--------------------------------------------------------------------------------------------------------|------------------------------|
|                                     | --> TCP connect to port 51326                                                                          |                              |
|                                     | --> TCP data: I use UDP port X (little endian):  7f0002000000XXXX                                      |                              |
|                                     | <-- TCP data: Cool, and I use UDP port Y : 7f0002000000YYYY                                            |                              |
| Start keep alive timer every second | --> UDP keep-alive packet from port X to port Y: 7f0500000000                                          |                              |
|                                     | <-- UDP                                                                                                | Starts streaming status data |
|                                     | --> TCP data: what' s your version?: 7f0100000000                                                      |                              |
|                                     | <-- TCP data: I use (version 1.5.10r4132 ..... build: release?): 7f020c000000 0201050a2410000000040000 |                              |
|                                     | < Many more initialization messages sent around, typically initialized by client. >                    |                              |


### Keep-Alive

The Client sends a keep-alive message every sec to the mixer through UDP with data `7f0500000000`.

The Mixer sends an identical message every sec.


### Some documented messages


```

likely msg layout

General:

    f7 <type> <size (4 byte le)> [<data>]

Likely type-dependent?:

    f7 <type> <subtype (2 byte)> <data (4 byte)> (total 8 data)




MUTE ON/OFF 1 - 4

Data: f7 08 0709 00 000000
Data: f7 08 0709 00 000100
Data: f7 08 0709 01 000100
Data: f7 08 0709 01 000000
Data: f7 08 0709 02 000100
Data: f7 08 0709 02 000000
Data: f7 08 0709 03 000100
Data: f7 08 0709 03 000000


FADER

Data: f7 09 070b 1820c03f -> 00

Data: f709070b0020008a

Data: f7 09 070b 00 200000 ch1 layerF
Data: f7 09 070b 00 209822 ch1 layerA
Data: f7 09 070b 00 200000 ch1
Data: f7 09 070b 00 20008a ch1 full
Data: f7 09 070b 01 20001d ch2
Data: f7 09 070b 02 20dd32 ch3
Data: f7 09 070b 03 20e046 ch4






```