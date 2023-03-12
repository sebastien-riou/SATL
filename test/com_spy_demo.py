#!/usr/bin/env python3

import sys
import binascii
import io
import pprint
import time

import sys, os

import pysatl
import socket

#parameters that must be shared between master and slave
port = 5000
link_granularity = 4
ack = True
skip_init=False
#side specific parameters
buffer_length = 268
sfr_granularity = 4


if (len(sys.argv) > 3) | (len(sys.argv) < 2) | (sys.argv[1] not in ['slave', 'master']):
    print("ERROR: needs at least 1 arguement, accept at most 2 arguments")
    print("[slave | master] port")
    exit()

if len(sys.argv)>2:
    port = int(sys.argv[2])


# test case
LC = 0
LE = 600

print("%s buffer_length="%sys.argv[1],buffer_length)
print("%s link_granularity="%sys.argv[1],link_granularity)
print("%s sfr_granularity="%sys.argv[1],sfr_granularity)
print("%s ack="%sys.argv[1],ack)


refdat = bytearray()
for i in range(0,(1<<16)+1):
    refdat.append(i & 0xFF)

refdatle = bytearray()
for i in range(0,(1<<16)+1):
    refdatle.append((i & 0xFF) ^ 0xFF)

if sys.argv[1]=='slave':
    serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    serversocket.settimeout(None)
    serversocket.bind(('localhost', port))
    serversocket.listen(1) # become a server socket, maximum 1 connections

    connection, address = serversocket.accept()
    assert(serversocket.gettimeout() == None)

    link = connection
    slave_com = pysatl.SocketComDriver(link,buffer_length,link_granularity,sfr_granularity,ack)

    print("Slave connected")
    slave  = pysatl.PySatl(is_master=False,com_driver=slave_com,skip_init=skip_init)
    print("Slave init done")

    def slave_com_spy_frame_tx(data):
        print("SLAVE FRAME TX %3d bytes:"%len(data),pysatl.Utils.hexstr(data,skip_long_data=True))

    def slave_com_spy_frame_rx(data):
        print("SLAVE FRAME RX %3d bytes:"%len(data),pysatl.Utils.hexstr(data,skip_long_data=True))

    slave_com.spy_frame_tx = slave_com_spy_frame_tx
    slave_com.spy_frame_rx = slave_com_spy_frame_rx

    cmd = slave.rx()
    assert(len(cmd.DATA) == LC)
    assert(cmd.LE == LE)
    assert(cmd.DATA == refdat[0:LC])
    response = pysatl.RAPDU(0x90,0x00,refdatle[0:LE])
    slave.tx(response)
    
else:
    clientsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    clientsocket.settimeout(None)
    clientsocket.connect(('localhost', port))
    assert(clientsocket.gettimeout() == None)

    link = clientsocket
    master_com = pysatl.SocketComDriver(link,buffer_length,link_granularity,sfr_granularity,ack)
    master = pysatl.PySatl(is_master=True,com_driver=master_com,skip_init=skip_init)
    
    def com_spy_frame_tx(data):
        print("MASTER FRAME TX %3d bytes:"%len(data),pysatl.Utils.hexstr(data,skip_long_data=True))

    def com_spy_frame_rx(data):
        print("MASTER FRAME RX %3d bytes:"%len(data),pysatl.Utils.hexstr(data,skip_long_data=True))

    master_com.spy_frame_tx = com_spy_frame_tx
    master_com.spy_frame_rx = com_spy_frame_rx


    apdu = pysatl.CAPDU(CLA=1,INS=2,P1=3,P2=4,DATA=refdat[0:LC],LE=LE)
    print(apdu, flush=True)
    master.tx(apdu)
    response = master.rx()

    print(response, flush=True)

    assert(response.DATA == refdatle[0:LE])
    assert(response.SW1 == 0x90)
    assert(response.SW2 == 0x00)

    time.sleep(1)
    print("done", flush=True)
    #input("Press enter to quit ")
