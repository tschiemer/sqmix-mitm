# sqmix-mitm
Man-in-the-Middle basis for SQ Mix control

## Utils

### discovery
```shell
Usage: ./discovery
Just shows which mixers were found and for which there is a timeout (of 5 secs)
```

### discovery-responder
```shell
Usage: ./discovery-responder <name-of-pretend-mixer>
Listen to any SQ Mixer inquiries and respond such that *this* host is assumed to be a mixer of the SQ series.
```

### midi-control
```shell
Usage: ./midi-control <ip-of-mixer>
Connects to mixer and receives MIDI data (well, actually the raw data stream) dumping it to stdout
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


### Some documented messages


```

likely msg layout

General:

    f7 <type> <size (4 byte le)> [<data>]

Likely type-dependent?:

    f7 <type> <subtype (2 byte)> <data (4 byte)> (total 8 bytes)


KEEP ALIVE (UDP)

7f0500000000

> Data: 7f 00 0200000038f8 -> send client udp port
< Data: 7f 00 020000007cc8 -> send server udp port

> UDP Data: 7f 05 00000000        (keep alive msg)

> Data: 7f 01 00000000
< Data: 7f 02 0c000000 0201050a2410000000040000 (version 1.5.10r4132 ..... build: release?)
 


SELECT DESELECT

1-8 (layer A)
Data: f7 08 0909 00 0001 00         Data: f7 33 3309 01 0001 00
Data: f7 08 0909 01 0001 01         Data: f7 33 3309 01 0101 00
Data: f7 08 0909 02 0001 02         Data: f7 33 3309 01 0201 00
Data: f7 08 0909 03 0001 03         Data: f7 33 3309 01 0301 00
Data: f7 08 0909 04 0001 04         Data: f7 33 3309 01 0401 00
Data: f7 33 3309 01 0501 00
Data: f7 08 0909 06 0001 06         Data: f7 33 3309 01 0601 00
Data: f7 08 0909 07 0001 07         Data: f7 33 3309 01 0701 00

Data: f708090907000000

25-26 (layer B)
Data: f7 08 0909 00 0001 18         Data: f7 33 3309 01 1801 00
Data: f7 08 0909 01 0001 19         Data: f7 33 3309 01 1901 00

deselect 26
Data: f708090901000000


1-3 (layer F)

Data: f7 33 3309 01000100 -> Data: f708090901000101
Data: f7 33 3309 01010100 -> Data: f708090902000102
Data: f7 33 3309 01020100

deselect 3
Data: f7 08 0909 02 000000

Data: f733330901 00 0100 ch1
Data: f733330901 58 0100 mix1
Data: f733330901 73 0100 matrix 1
Data: f733330901 68 0100 main




MUTE ON/OFF 1 - 4

Data: f7 08 0709 00 000000
Data: f7 08 0709 00 000100
Data: f7 08 0709 01 000100
Data: f7 08 0709 01 000000
Data: f7 08 0709 02 000100
Data: f7 08 0709 02 000000
Data: f7 08 0709 03 000100
Data: f7 08 0709 03 000000


SOFT CONTROLS : MIDI

Data: f7 21 1f14 b0 01 01 00 + relative ch0 cc1
Data: f7 21 1f14 b0 01 7f 00 - relative ch0 cc1

Data: f7 21 1f14 b0 01 02 00 ch0 cc2
Data: f7 21 1f14 b0 03 01 00 ch0 cc3
Data: f7 21 1f14 b0 04 01 00 ch0 cc4

Data: f7 21 1f14 b1 01 01 00 ch1 cc1
Data: f7 21 1f14 b0 01 02 00 absolute ch0 cc1
Data: f7 21 1f14 b0 01 63 00 absolute ch0 cc1 v99

Data: f7 21 1f14 90 00 7f 00 ch0 note on 0
Data: f7 21 1f14 80 00 00 00 ch0 note off 0

FADER

Data: f7 09 070b 1820c03f -> 00

Data: f7 09 070b 00 200000 ch1 layerF
Data: f7 09 070b 00 209822 ch1 layerA
Data: f7 09 070b 00 200000 ch1
Data: f7 09 070b 00 20008a ch1 full
Data: f7 09 070b 01 20001d ch2
Data: f7 09 070b 02 20dd32 ch3
Data: f7 09 070b 03 20e046 ch4

MIDI FADER

Data: f7 44 091e 00030300

Data: f7 09 1f0d 00 00 ff 00 fader1 full
Data: f7 09 1f0d 00 00 01 00 fader1 zeroish
Data: f7 09 1f0d 01 00 ff 00 fader2 full
Data: f7 09 1f0d 01 00 0f 00 fader2 zeroish

LAYER SELECT

Data: f7 08 090b 00 000000 layer A
Data: f7 08 090b 01000000 layer B
Data: f7 08 090b 02000000 layer C
Data: f7 08 090b 03000000 layer D
Data: f7 08 090b 04000000 layer E
Data: f7 08 090b 05000000 layer F





```