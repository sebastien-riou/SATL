# SATL - Simple APDU Transport Layer

A simple way to exchange ISO7816-4 APDUs over interfaces not covered
in ISO7816-3.

This repository provides a basic implementation in C and in Python3.
If you have other needs, you can create your own with a fraction of the effort needed for ISO7816-3.


## Motivations
ISO7816-3 standardize how to exchange APDUs over a "smart card UART" but does not cover
other kind of communication interface such as I2C, SPI, JTAG, APB and so on.

Global Platform set out to standardize transport over I2C and SPI but as of today the draft is not publicly available and the timeline for an approved standard is anybody's guess.

Finally ISO7816-3, for historical reasons, is overly complicated and it is virtually impossible to get perfect interoperability without extensive field trials (some revisions of the standard seems to contradict each other).

## Goals   
* Simplicity / code size:
  * Simple specification allowing to exchange APDUs over any communication interface
  * No timeouts defined at transport level
* Versatility and minimal hardware requirement
  * use software flow control when hardware does not handle it
  * ability to deal with minimum transfer granularity
  * ability to deal with minimum read/write granularity of SFRs (APB granularity is 32 bit for example)
* Efficiency
  * Limit number of handshakes to the minimum
  * APDUs with large data shall be transferred as efficiently as in ISO7816-3 T=1
* Length,Value encoding to allow easy encapsulation

## Non Goals
* Negociation of physical transport parameters other than buffer size
* Transfer speed of short APDUs
* Efficiency in error cases
* Transmission errors detection  

## Minimum hardware requirements
* Half duplex communication link

## Padding
Some interfaces have a transfer granularity larger than a single byte. When the length of the transfer is not a multiple of the transfer unit, extra bytes have to be sent, we refer to those bytes as "PAD". The position of the pad shall be always on the last transferred unit and on the most significant bytes of the transfer unit. The value of the pad is meaning less and shall be ignored.

## Initialization phase
When the link implements flow control, initialization phase is not needed and therefore skipped entirely. In the absence of flow control, the following
sequence takes place:

* The master send 4 bytes to indicate the Master Buffer Size (MBLEN): the maximum number of bytes it can receive in a row at maximum transfer speed.
* The slave answer with 4 bytes to indicate the Slave Buffer Size (SBLEN).

NOTE: The initialization phase may be skipped in cases where interoperability is not an issue. For example in an integrated UICC both the master and the slave are on the same die and use a dedicated communication interface, both MBLEN and SBLEN can be simply hardcoded.

## ACK signal
When the link implements flow control, ACK signal is not needed and therefore skipped entirely.

In the absence of flow control, when a TPDU is larger than the receiver buffer size, the transmitter shall split it in chunks matching exactly the receiver buffer length. Only the last chunk may be smaller than the receiver buffer size. After sending each chunk, except the last one, the transmitter shall wait for the ACK signal. The receiver shall send the ACK signal when it is ready to receive the next chunk.

On interfaces which support 0 length transfers, the ACK signal shall be such transfer. On other interfaces it shall be the shortest possible transfer, the value is meaning less and shall be ignored.

## Command TPDU format
All ISO7816-4 C-APDU cases are mapped to a single C-TPDU format

|FL |LE |CLA|INS|P1 |P2 |DATA |PAD|
|-  |-  |-  |-  |-  |-  |-    |-  |

* FL,LE
  * 32 bits values in little endian format
  * FL is the Frame length: the total length of the C-TPDU excluding PAD, so ISO7816-4's LC = FL - 12
  * LE value is the same as ISO7816-4's LE, just encoded differently
* CLA,INS,P1,P2
  * Single byte values
  * Exactly the same values as in the ISO7816-4 C-APDU
* DATA
  * exactly FL - 12 bytes
* PAD
  * 0 or more bytes

NOTE: ISO7816-4 LC and LE are limited to 0x10000. C-TPDU FL is limited to 0x1000C, higher values are
reserved for future use. For example they could be used to encode a compact frame format for optimizing
short APDUs if this turns out to be critical in some use cases.

## Response TPDU format
A single R-TPDU format cover all cases

|FL |DATA  |SW1   |SW2   | PAD |
|-  |-     |-     |-     |-    |

* FL: Total length of the R-TPDU excluding PAD
  * 32 bits values in little endian format
  * FL - 6 is the length of the DATA field, it shall be inferior or equal to LE field in the command TPDU
* DATA
  * exactly FL - 6 bytes
* SW1, SW2
  * Single byte values
  * Exactly the same values as in the ISO7816-4 R-APDU
* PAD
  * 0 or more bytes

## Mapping of APDU cases to SATL full frames
See [this page](apdu_cases_full_frames.md)

## Command - Response exchanges

The master initiate the communication by sending a command TPDU. splitting it in chunks as needed. The slave answer with a response TPDU, splitting it in chunks as needed. If the slave does not support the command, it shall nevertheless receive the full command TPDU and send the error as a properly formated response TPDU. If the master receives a response TPDU indicating an unexpected LE value, it shall receive the full response TPDU or reinitialize the slave (by means of reset, power off/power on ...).

## Rational
### MBLEN, SBLEN and ACK signal
ISO7816-3 T=0 supports only 255 bytes of data per TPDU, this results in many interface turn around on half duplex link and this lead to poor performances. In addition this force low end devices to reserve 255 bytes of memory as "APDU buffer" as such devices are often not fast enough to process the data on the fly (and even when they are, this may be impossible due to the nature of the command).

ISO7816-3 T=1 mostly solve those problems but requires the transfer of several bytes merely to ask for the next chunk and each chunk comes with the overhead of a prolog and an epilog. Another drawback is the complexity
involved by the maintenance of frame number and redundancy check.

SATL allows both low cost implementation and high communication efficiency by supporting the buffer size parameters (MBLEN, SBLEN) and a TPDU splitting/chaining mechanism (ACK signal). It is simpler than ISO7816-3 T=0 and more efficient on long APDUs. It is simpler than ISO7816-3 T=1 as it does not have frame numbers,
redundancy check, parameters negotiation and error handling.

### FL in R-TPDU
According to ISO7816, R-APDUs are allowed to contain less data than specified in
the C-APDU LE field. As a result the transport layer cannot impose a simple
rule like "all requested byte shall be sent". The master needs therefore a mean
to know the length of the response (since we do not want to rely on timeouts).  

### Little endian encoding
Most low end architectures are little endian.
