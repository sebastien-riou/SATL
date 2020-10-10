"""Library to transport ISO7816-4 APDUs over anything"""

__version__ = '1.2.1'
__title__ = 'pysatl'
__description__ = 'Library to transport ISO7816-4 APDUs over anything'
__long_description__ = """
A simple way to exchange ISO7816-4 APDUs over interfaces not covered in ISO7816-3.
ISO7816-3 standardize how to exchange APDUs over a "smart card UART" but does not cover other kind of communication interface such as I2C, SPI, JTAG, APB and so on.
Global Platform set out to standardize transport over I2C and SPI but as of today the draft is not publicly available and the timeline for an approved standard is anybody's guess.
Finally ISO7816-3, for historical reasons, is overly complicated and it is virtually impossible to get perfect interoperability without extensive field trials (some revisions of the standard seems to contradict each other).
"""
__uri__ = 'https://github.com/sebastien-riou/SATL'
__doc__ = __description__ + ' <' + __uri__ + '>'
__author__ = 'Sebastien Riou'
# For all support requests, please open a new issue on GitHub
__email__ = 'matic@nimp.co.uk'
__license__ = 'Apache 2.0'
__copyright__ = ''

class PySatl(object):
    """ SATL main class

    Generic SATL implementation. It interface to actual hardware via a
    "communication driver" which shall implement few functions.
    See :class:`SocketComDriver` and :class:`StreamComDriver` for example.

    Args:
        is_master (bool): Set to ``True`` to be master, ``False`` to be slave
        com_driver (object): A SATL communication driver
        skip_init (bool): If ``True`` the initialization phase is skipped

    """

    @property
    def DATA_SIZE_LIMIT(self):
        """int: max data field length"""
        return 1<<16

    @property
    def INITIAL_BUFFER_LENGTH(self):
        """int: initial length of buffer for the initialization phase"""
        return 4

    @property
    def LENLEN(self):
        """int: length in bytes of the length fields"""
        return 4

    @property
    def com(self):
        """object: Communication hardware driver"""
        return self._com

    @property
    def is_master(self):
        """bool: `True` if master, `False` if slave"""
        return self._is_master

    @property
    def other_bufferlen(self):
        """int: buffer length of the other side"""
        return self._other_bufferlen

    def __init__(self,is_master,com_driver,skip_init=False):
        self._com = com_driver
        self._is_master=is_master

        if self.com.ack & (False==skip_init):
            #this models the buffer length of the other side
            #our side buffer length is in self.com_driver.bufferlen
            self._other_bufferlen = self.INITIAL_BUFFER_LENGTH

            if is_master:
                self.com.tx(self.com.bufferlen.to_bytes(self.LENLEN,byteorder='little'))
                data = self.com.rx(self.LENLEN)
                self._other_bufferlen = int.from_bytes(data[0:self.LENLEN], byteorder='little', signed=False)
            else:
                data = self.com.rx(self.LENLEN)
                self._other_bufferlen = int.from_bytes(data[0:self.LENLEN], byteorder='little', signed=False)
                self.com.tx(self.com.bufferlen.to_bytes(self.LENLEN,byteorder='little'))

            assert(self.other_bufferlen >= self.com.granularity)
            assert(self.other_bufferlen >= self.com.sfr_granularity)
            assert(0==(self.other_bufferlen % self.com.granularity))
            assert(0==(self.other_bufferlen % self.com.sfr_granularity))
            if self._other_bufferlen<self.com.bufferlen:
                self.com.bufferlen = self.other_bufferlen
        else:
            self._other_bufferlen = self.com.bufferlen
            if not self.com.ack:
                assert(self._other_bufferlen>self.DATA_SIZE_LIMIT+2*self.LENLEN+4)

    def __pad(self,buf):
        """pad the buffer if necessary"""
        return Utils.pad(buf,self.com.granularity)

    def __padlen(self,l):
        """compute the length of the padded data of length l"""
        return Utils.padlen(l,self.com.granularity)

    def tx(self,apdu):
        """Transmit

        Args:
            apdu (object): if master, apdu shall be a :class:`CAPDU`. If slave, a :class:`RAPDU`.
        """
        if self.is_master:
            self.__master_tx(apdu)
        else:
            self.__slave_tx(apdu)

    def rx(self):
        """Receive

         Returns:
             If master, a :class:`RAPDU`. If slave, a :class:`CAPDU`.
         """
        if self.is_master:
            return self.__master_rx()
        else:
            return self.__slave_rx()

    def __master_tx(self,capdu):
        assert(self.is_master)
        assert(len(capdu.DATA)<=self.DATA_SIZE_LIMIT)
        assert(capdu.LE<=self.DATA_SIZE_LIMIT)
        buf = bytearray()
        fl=len(capdu.DATA) + 4 + 2*self.LENLEN
        buf+=fl.to_bytes(self.LENLEN,byteorder='little')
        buf+=capdu.LE.to_bytes(self.LENLEN,byteorder='little')
        buf.append(capdu.CLA)
        buf.append(capdu.INS)
        buf.append(capdu.P1)
        buf.append(capdu.P2)
        buf+=capdu.DATA
        self.__frame_tx(buf)

    def __master_rx(self):
        assert(self.is_master)
        data = self.__frame_rx()
        fl = int.from_bytes(data[0:self.LENLEN],byteorder='little',signed=False)
        #print(fl)
        sw = fl - 2
        le = max(sw - self.LENLEN,0)
        #print(len(data),le)
        rapdu = RAPDU(data[sw],data[sw+1],data[self.LENLEN:self.LENLEN+le])
        return rapdu

    def __slave_rx(self):
        assert(not self.is_master)
        headerlen = 4+2*self.LENLEN
        #print("slave_rx")
        data = self.__frame_rx()
        #print(data)
        le = int.from_bytes(data[self.LENLEN:2*self.LENLEN],byteorder='little',signed=False)
        h=2*self.LENLEN
        capdu = CAPDU(data[h],data[h+1],data[h+2],data[h+3],data[headerlen:],le)
        return capdu

    def __slave_tx(self,rapdu):
        assert(not self.is_master)
        le = len(rapdu.DATA)
        fl = le + self.LENLEN + 2
        data = bytearray(fl.to_bytes(self.LENLEN,byteorder='little'))
        data+=rapdu.DATA
        data.append(rapdu.SW1)
        data.append(rapdu.SW2)
        self.__frame_tx(data)

    def __frame_tx(self,data):
        """send a complete frame (either a C-TPDU or a R-TPDU), taking care of padding and splitting in chunk"""
        data=self.__pad(data)
        #print("__frame_tx: padded frame to send: ",data,flush=True)

        if len(data) < self.other_bufferlen:
            self.com.tx(data)
        else:
            chunks = (len(data)-1) // self.other_bufferlen
            #print("__frame_tx: %d full chunks + last"%chunks,flush=True)
            for i in range(0,chunks):
                self.com.tx(data[i*self.other_bufferlen:(i+1)*self.other_bufferlen])
                self.com.rx_ack()
            self.com.tx(data[chunks*self.other_bufferlen:])
        #print("__frame_tx done",flush=True)

    def __frame_rx(self):
        """receive a complete frame (either a C-TPDU or a R-TPDU), return it without pad"""
        self._rx_cnt=0
        flbytes = self.__rx(self.LENLEN)
        fl = int.from_bytes(flbytes[0:self.LENLEN],byteorder='little',signed=False)
        remaining = fl - len(flbytes)

        #print("flbytes:",Utils.hexstr(flbytes))
        #print("fl=%d, remaining=%d"%(fl,remaining),flush=True)

        data = flbytes+self.__rx(remaining)
        #remove padding
        data = data[0:fl]
        #print("received frame: ",data,flush=True)
        return data

    def __rx(self,length):
        """receive and send ack as needed"""
        remaining=length
        out=bytearray()
        if self._rx_cnt>0 and (0==(self._rx_cnt%self.com.bufferlen)):
            self.com.tx_ack()
        rxlen = min(remaining,self.com.bufferlen-(self._rx_cnt%self.com.bufferlen))
        #print("first_rxlen=",rxlen,flush=True)
        dat = self.com.rx(rxlen)
        self._rx_cnt+=rxlen
        remaining -= rxlen
        out += dat
        while(remaining>0):
            self.com.tx_ack()
            rxlen=min(remaining,self.com.bufferlen)
            dat = self.com.rx(rxlen)
            self._rx_cnt+=rxlen
            remaining -= rxlen
            out += dat
        return out

class CAPDU(object):
    """ISO7816-4 C-APDU

    All parameters are read/write attributes.
    There is no restriction on `CLA` and `INS` values.
    There is no check on DATA length and LE value.
    """

    def __init__(self,CLA,INS,P1,P2,DATA=bytearray(),LE=0):
        self.CLA = CLA
        self.INS = INS
        self.P1 = P1
        self.P2 = P2
        self.DATA = DATA
        if DATA is None:
            self.DATA = bytearray()
        self.LE = LE

    def __str__(self):
        out = "C-APDU %02X %02X %02X %02X"%(self.CLA,self.INS,self.P1,self.P2)
        if len(self.DATA) > 0:
            out += " - LC=%5d DATA: "%(len(self.DATA))
            out += Utils.hexstr(self.DATA)
        if self.LE>0:
            out += " - LE=%5d"%(self.LE)
        return out

class RAPDU(object):
    """ISO7816-4 R-APDU

    All parameters are read/write attributes.
    There is no restriction on `SW1` and `SW2` valuesself.
    There is no check on DATA length.
    """

    def __init__(self,SW1,SW2,DATA=bytearray()):
        self.DATA = DATA
        if DATA is None:
            self.DATA = bytearray()
        self.SW1 = SW1
        self.SW2 = SW2

    def __str__(self):
        out = "R-APDU %02X %02X"%(self.SW1,self.SW2)
        if len(self.DATA) > 0:
            out += " - LE=%5d DATA: "%(len(self.DATA))
            out += Utils.hexstr(self.DATA)
        return out

class SocketComDriver(object):
    """Parameterized model for a communication peripheral and low level rx/tx functions

    Args:
        sock (socket): `socket` object used for communication
        bufferlen (int): Number of bytes that can be received in a row at max rate
        granularity (int): Smallest number of bytes that can be transported over the link
        ack (bool): if ``False``, :func:`tx_ack` and :func:`rx_ack` do nothing

    """
    def __init__(self,sock,bufferlen=4,granularity=1,sfr_granularity=1,ack=True):
        self._sock = sock
        adapter=self.SocketAsStream(sock)
        self._impl = StreamComDriver(adapter,bufferlen,granularity,sfr_granularity,ack)

    class SocketAsStream(object):
        def __init__(self,sock):
            self._sock=sock

        def write(self,data):
            self._sock.send(data)

        def read(self,length):
            return self._sock.recv(length)

    @property
    def sock(self):
        """socket: `socket` object used for communication"""
        return self._sock

    @property
    def bufferlen(self):
        """int: Number of bytes that can be received in a row at max rate"""
        return self._impl.bufferlen

    @bufferlen.setter
    def bufferlen(self, value):
        #No doc string here, all doc must be in the getter docstring
        self._impl.bufferlen = value

    @property
    def granularity(self):
        """int: Smallest number of bytes that can be transported over the link"""
        return self._impl.granularity

    @property
    def sfr_granularity(self):
        """int: Smallest number of bytes that can be accessed via the hardware on this side"""
        return self._impl.sfr_granularity

    @property
    def ack(self):
        """bool: if ``False``, :func:`tx_ack` and :func:`rx_ack` do nothing"""
        return self._impl.ack

    def tx_ack(self):
        self._impl.tx_ack()

    def rx_ack(self):
        self._impl.rx_ack()

    def tx(self,data):
        """Transmit data

        Args:
            data (bytes): bytes to transmit, shall be compatible with :func:`sfr_granularity` and :func:`granularity`
        """
        self._impl.tx(data)

    def rx(self,length):
        """Receive data

        Args:
            length (int): length to receive, shall be compatible with :func:`granularity` and smaller or equal to :func:`bufferlen`

        Returns:
            bytes: received data, padded with zeroes if necessary to be compatible with :func:`sfr_granularity`
        """
        return self._impl.rx(length)


class StreamComDriver(object):
    """Parameterized model for a communication peripheral and low level rx/tx functions"""
    def __init__(self,stream,bufferlen=3,granularity=1,sfr_granularity=1,ack=False):
        self._stream = stream
        if granularity > sfr_granularity:
            assert(0==(granularity % sfr_granularity))
        if granularity < sfr_granularity:
            assert(0==(sfr_granularity % granularity))
        #shall be power of 2
        assert(1==bin(granularity).count("1"))
        assert(1==bin(sfr_granularity).count("1"))

        self._granularity=granularity
        self._sfr_granularity=sfr_granularity
        self._ack=ack
        if ack:
            self._bufferlen=bufferlen
        else:
            #if no ack then we have hardware flow control, this is equivalent to infinite buffer size
            self._bufferlen = 1<<32 - 1

    @property
    def sream(self):
        """stream: `stream` object used for communication"""
        return self._stream

    @property
    def bufferlen(self):
        """int: Number of bytes that can be received in a row at max rate"""
        return self._bufferlen

    @bufferlen.setter
    def bufferlen(self, value):
        #No doc string here, all doc must be in the getter docstring
        self._bufferlen = value

    @property
    def granularity(self):
        """int: Smallest number of bytes that can be transported over the link"""
        return self._granularity

    @property
    def sfr_granularity(self):
        """int: Smallest number of bytes that can be accessed via the hardware on this side"""
        return self._sfr_granularity

    @property
    def ack(self):
        """bool: if ``False``, :func:`tx_ack` and :func:`rx_ack` do nothing"""
        return self._ack

    def tx_ack(self):
        if self.ack:
            #print("send ack",flush=True)
            data=bytearray([ord(b'A')] * self.granularity)
            self._stream.write(data)

    def rx_ack(self):
        if self.ack:
            #print("wait ack",flush=True)
            dat = self._stream.read(self._granularity)
            #print("ack recieved: ",dat,flush=True)

    def tx(self,data):
        """Transmit data

        Args:
            data (bytes): bytes to transmit, shall be compatible with :func:`sfr_granularity` and :func:`granularity`
        """
        #print("tx ",data,flush=True)
        assert(0==(len(data) % self._sfr_granularity))
        assert(0==(len(data) % self._granularity))
        self._stream.write(data)

    def rx(self,length):
        """Receive data

        Args:
            length (int): length to receive, shall be compatible with :func:`granularity` and smaller or equal to :func:`bufferlen`

        Returns:
            bytes: received data, padded with zeroes if necessary to be compatible with :func:`sfr_granularity`
        """
        #print("rx %d "%length,end="",flush=True)
        assert(length<=self._bufferlen)
        data = bytearray()
        remaining = length+Utils.padlen(length,self._granularity)
        #print("length=",length,flush=True)
        #print("remaining=",remaining,flush=True)
        while(remaining):
            #print("remaining=",remaining)
            #print("receive: ",end="")
            dat = self._stream.read(remaining)
            #print("received: ",Utils.hexstr(dat))
            if 0==len(dat):
                print(self._stream)
                #print(self._stream.timeout)
                raise Exception("Connection broken")
            data += dat
            remaining -= len(dat)
        if self._ack & (len(data)>self._bufferlen):
            raise ValueError("RX overflow, data length = %d"%len(data))
        assert(0==(len(data) % self._granularity))

        #padding due to SFRs granularity
        data = Utils.pad(data,self._sfr_granularity)
        #print("received data length after padding = ",len(data),flush=True)
        return data

class Utils(object):
    """Helper class"""

    @staticmethod
    def pad(buf,granularity):
        """pad the buffer if necessary (with zeroes)"""
        l = len(buf)
        if 0 != (l % granularity):
            v=0
            buf += v.to_bytes(Utils.padlen(l,granularity),'little')
        return buf

    @staticmethod
    def padlen(l,granularity):
        """compute the length of the pad for data of length l to get the requested granularity"""
        nunits = (l+granularity-1) // granularity
        return granularity * nunits - l

    @staticmethod
    def hexstr(bytes, head="", separator=" ", tail=""):
        """Returns an hex string representing bytes

        Args:
            bytes: a list of bytes to stringify, e.g. [59, 22] or a bytearray
            head: the string you want in front of each bytes. Empty by default.
            separator: the string you want between each bytes. One space by default.
            tail: the string you want after each bytes. Empty by default.
        """
        if bytes is not bytearray:
            bytes = bytearray(bytes)
        if (bytes is None) or bytes == []:
            return ""
        else:
            pformat = head+"%-0.2X"+tail
            return (separator.join(map(lambda a: pformat % ((a + 256) % 256), bytes))).rstrip()

    @staticmethod
    def int_to_bytes(x, width=-1, byteorder='little'):
        if width<0:
            width = (x.bit_length() + 7) // 8
        b = x.to_bytes(width, byteorder)
        return b

    @staticmethod
    def int_to_ba(x, width=-1, byteorder='little'):
        if width<0:
            width = (x.bit_length() + 7) // 8
        b = x.to_bytes(width, byteorder)
        return bytearray(b)

    @staticmethod
    def to_int(ba, byteorder='little'):
        b = bytes(ba)
        return int.from_bytes(b, byteorder)

    @staticmethod
    def ba(hexstr_or_int):
        """Extract hex numbers from a string and returns them as a bytearray
        It also handles int and list of int as argument
        If it cannot convert, it raises ValueError
        """
        try:
            t1 = hexstr_or_int.lower()
            t2 = "".join([c if c.isalnum() else " " for c in t1])
            t3 = t2.split(" ")
            out = bytearray()
            for bstr in t3:
                if bstr != "":
                    l = len(bstr)
                    if(l % 2):
                        bstr = "0"+bstr
                        l+=1
                    for p in range(0,l,2):
                        s=bstr[p:p+2]
                        out.extend((bytearray.fromhex(s)))
        except:
            #seems arg is not a string, assume it is a int
            try:
                out = Utils.int_to_ba(hexstr_or_int)
            except:
                # seems arg is not an int, assume it is a list
                try:
                    out = bytearray(hexstr_or_int)
                except:
                    raise ValueError()
        return out
