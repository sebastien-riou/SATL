"""Library to transport ISO7816-4 APDUs over anything"""
import io
import logging

__version__ = '1.2.5'
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

    @property
    def spy_frame_tx(self):
        """functions called to spy on each tx frame, without padding"""
        return self._spy_frame_tx

    @spy_frame_tx.setter
    def spy_frame_tx(self, value):
        self._spy_frame_tx = value

    @property
    def spy_frame_rx(self):
        """functions called to spy on each rx frame, without padding"""
        return self._spy_frame_rx

    @spy_frame_rx.setter
    def spy_frame_rx(self, value):
        self._spy_frame_rx = value

    def __init__(self,is_master,com_driver,skip_init=False):
        self._com = com_driver
        self._is_master = is_master
        self._spy_frame_tx = None
        self._spy_frame_rx = None

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

        if self._spy_frame_tx is not None:
            self._spy_frame_tx(data)

        data=self.__pad(data)

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

        if self._spy_frame_rx is not None:
            self._spy_frame_rx(data)

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


    def to_str(self,*,skip_long_data=False):
        """String representation"""
        out = "C-APDU %02X %02X %02X %02X"%(self.CLA,self.INS,self.P1,self.P2)
        if len(self.DATA) > 0:
            out += " - LC=%5d DATA: "%(len(self.DATA))
            out += Utils.hexstr(self.DATA,skip_long_data=skip_long_data)
        if self.LE>0:
            out += " - LE=%5d"%(self.LE)
        return out

    def __str__(self):
        return self.to_str()

    def __repr__(self):
        return "pysatl.CAPDU.from_hexstr('"+self.to_hexstr()+"')"

    def __eq__(self, other):
        """Overrides the default implementation"""
        if isinstance(other, CAPDU):
            ours = self.__dict__.items()
            theirs = other.__dict__.items()
            return ours == theirs
        return NotImplemented

    @staticmethod
    def from_bytes(apdu_bytes):
        """Create a CAPDU from its bytes representation"""
        # CAPDU FORMATS:
        # CASE1:  CLA INS P1 P2
        # CASE2S: CLA INS P1 P2 LE                 (                       LE  from 01   to   FF)
        # CASE2E: CLA INS P1 P2 00 LE2             (                       LE2 from 0001 to 0000)
        # CASE3S: CLA INS P1 P2 LC DATA            (LC  from   01 to   FF                       )
        # CASE3E: CLA INS P1 P2 00 LC2 DATA        (LC2 from 0001 to FFFF                       )
        # CASE4S: CLA INS P1 P2 LC DATA LE         (LC  from   01 to   FF  LE2 from 0001 to 0000)
        # CASE4E: CLA INS P1 P2 LC DATA 00 LE2     (LC  from   01 to   FF, LE2 from 0001 to 0000) #not standard but tolerated
        # CASE4E: CLA INS P1 P2 00 LC2 DATA LE     (LC2 from 0001 to FFFF, LE  from 01   to   FF) #not standard but tolerated
        # CASE4E: CLA INS P1 P2 00 LC2 DATA 00 LE2 (LC2 from 0001 to FFFF, LE2 from 0001 to 0000)
        apdu_len=len(apdu_bytes)
        abytes = io.BytesIO(apdu_bytes)
        header = abytes.read(4)
        lc = 0
        le = 0
        has_le = 0
        has_long_le = 0
        if apdu_len>4:
            p3 = int.from_bytes(abytes.read(1),byteorder='big')
            if apdu_len==5:
                has_le = 1
                le = p3
                if 0==le:
                    le=0x100
            elif 0==p3:
                if apdu_len==7:
                    has_le=1
                    has_long_le = 1
                    le = int.from_bytes(abytes.read(2), byteorder='big')
                    if 0==le:
                        le = 0x10000
                else:
                    lc = int.from_bytes(abytes.read(2),byteorder='big')
                    if apdu_len>(4+3+lc):
                        has_le = 1
                        has_long_le = apdu_len>(4+3+lc+1)
            else:
                lc = p3
                if apdu_len>(4+1+lc):
                    has_le = 1
                    has_long_le = apdu_len>(4+1+lc+1)
        has_lc = lc>0
        has_long_lc = lc>255
        DATA = None
        if has_lc:
            len_no_le = 5 + 2 * has_long_lc + lc
            remaining = apdu_len - len_no_le
            if remaining:
                has_le = 1
                if remaining>1:
                    has_long_le = 1
            DATA = abytes.read(lc)
            if has_le:
                if has_long_le:
                    le = int.from_bytes(abytes.read(3),byteorder='big')
                    assert(le <= 0xFFFF )
                    if 0 == le:
                        le = 0x10000
                else:
                    le = int.from_bytes(abytes.read(1), byteorder='big')
                    if 0 == le:
                        le = 0x100
        return CAPDU(CLA=header[0],INS=header[1],P1=header[2],P2=header[3],DATA=DATA,LE=le)

    def to_ba(self):
        """Convert to a bytearray"""
        out = bytearray()
        out.append(self.CLA)
        out.append(self.INS)
        out.append(self.P1)
        out.append(self.P2)
        LC = len(self.DATA)
        LE = self.LE
        case4e = (LC>0) and (LE>0) and ((LC > 0xFF) or (LE > 0x100))
        if LC > 0:
            if (LC > 0xFF) or case4e :
                out.append(0x00)
                out += LC.to_bytes(2, byteorder='big')
            else:
                out += LC.to_bytes(1, byteorder='big')
            out += self.DATA
        if LE > 0:
            if LE == 0x10000:
                if not case4e:
                    out.append(0x00)
                out.append(0x00)
                out.append(0x00)
            elif LE > 0x100 or case4e:
                if not case4e:
                    out.append(0x00)
                out += LE.to_bytes(2, byteorder='big')
            else:
                if LE == 0x100:
                    out += bytearray([0])
                else:
                    out += LE.to_bytes(1, byteorder='big')
        return out

    def to_bytes(self):
        """Convert to bytes"""
        return bytes(self.to_ba())

    @staticmethod
    def from_hexstr(hexstr):
        """Convert a string of hex digits to CAPDU"""
        b = Utils.ba(hexstr)
        return CAPDU.from_bytes(b)

    def to_hexstr(self,*,skip_long_data=False, separator=""):
        """Convert to a string of hex digits"""
        b = self.to_ba()
        header_len = 4
        lc=len(self.DATA)
        if lc:
            header_len = 5
            if lc>255:
                header_len = 7
        dat = Utils.hexstr(b[:header_len], separator=separator)
        if lc:
            dat += separator + Utils.hexstr(b[header_len:header_len+lc], separator=separator,skip_long_data=skip_long_data)
        if self.LE:
            dat += separator + Utils.hexstr(b[header_len+lc:], separator=separator)
        return dat

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

    def to_str(self,*,skip_long_data=False):
        out = "R-APDU %02X %02X"%(self.SW1,self.SW2)
        if len(self.DATA) > 0:
            out += " - LE=%5d DATA: "%(len(self.DATA))
            out += Utils.hexstr(self.DATA,skip_long_data=skip_long_data)
        return out

    def __str__(self):
        return self.to_str()

    def __repr__(self):
        return "pysatl.RAPDU.from_hexstr('"+self.to_hexstr()+"')"

    @staticmethod
    def from_bytes(apdu_bytes):
        """Create a RAPDU from its bytes representation"""
        l = len(apdu_bytes)-2
        assert(l >= 0)
        data = apdu_bytes[0:l]
        sw1 = apdu_bytes[-2]
        sw2 = apdu_bytes[-1]
        return RAPDU(SW1=sw1,SW2=sw2,DATA=data)

    def to_ba(self):
        """Convert to bytearray"""
        out = bytearray()
        out += self.DATA
        out.append(self.SW1)
        out.append(self.SW2)
        return out

    def to_bytes(self):
        """Convert to bytes"""
        return bytes(self.to_ba())

    def swBytes(self):
        """SW1 and SW2 as bytearray"""
        out = bytearray()
        out.append(self.SW1)
        out.append(self.SW2)
        return out

    def matchSW(self, swPat):
        """Check if Status Word match a given hexadecimal pattern, 'X' match anything"""
        swInt = (self.SW1 << 8) | self.SW2
        swStr = format(swInt, 'x')
        swPat = swPat.lower()
        for i in range(0,len(swPat)):
            expected = swPat[i]
            if expected == 'x':
                continue
            d = swStr[i]
            if d != expected:
                return False
        return True

    def matchDATA(self, dataPat):
        """Check if DATA match a given hexadecimal pattern, 'X' match anything"""
        dataPat = dataPat.lower()
        for i in range(0,len(dataPat)):
            expected = dataPat[i]
            if expected == 'x':
                continue
            b = self.DATA[i>>1]
            if 0 == (i % 2):
                d = (b >> 4) & 0xF
            else:
                d = b & 0xF
            if d != int(expected,16):
                logging.debug("Mismatch at char %d: expected %x, got %x"%(i,expected,d))
                return False
        return True

    @staticmethod
    def from_hexstr(hexstr):
        """Convert a string of hex digits to RAPDU"""
        b = Utils.ba(hexstr)
        return RAPDU.from_bytes(b)

    def to_hexstr(self,*,skip_long_data=False):
        """Convert to a string of hex digits"""
        b = self.to_ba()
        if len(b)>16:
            dat = Utils.hexstr(b[:-2], separator="",skip_long_data=skip_long_data) + Utils.hexstr(b[-2:], separator="")
        else:
            dat = Utils.hexstr(b, separator="")
        return dat

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
    def hexstr(bytes, head="", separator=" ", tail="", *,skip_long_data=False):
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
            l=len(bytes)
            if skip_long_data and l>16:
                first = pformat % ((bytes[ 0] + 256) % 256)
                last  = pformat % ((bytes[-1] + 256) % 256)
                return (separator.join([first,"...%d bytes..."%(l-2),last])).rstrip()
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
                if bstr[0:2] == "0x":
                    bstr = bstr[2:]
                if bstr != "":
                    l = len(bstr)
                    if(l % 2):
                        bstr = "0"+bstr
                        l+=1
                    out += bytearray.fromhex(bstr)

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
