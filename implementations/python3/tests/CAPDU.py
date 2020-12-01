import os

import pysatl
from pysatl import CAPDU


if __name__ == "__main__":

    def check(hexstr, expected):
        capdu = CAPDU.from_hexstr(hexstr)
        if capdu != expected:
            raise Exception("Mismatch for input '"+hexstr+"'\nActual:   "+str(capdu)+"\nExpected: "+str(expected))


    def gencase(* ,LC ,LE):
        assert(LC <  0x10000)
        assert(LE <= 0x10000)
        data = os.getrandom(LC)
        hexstr = "00112233"
        if LC>0:
            if LC>0xFF:
                hexstr += "00%04X"%LC
            else:
                hexstr += "%02X" % LC
            hexstr += pysatl.Utils.hexstr(data, separator="")
        if LE>0:
            if LE == 0x10000:
                hexstr += "000000"
            elif LE>0x100:
                hexstr += "00%04X"%LE
            elif LE == 0x100:
                hexstr += "00"
            else:
                hexstr += "%02X" % LE
        expected = hexstr
        capdu = CAPDU(CLA=0x00, INS=0x11, P1=0x22, P2=0x33, DATA=data, LE=LE)
        hexstr = capdu.to_hexstr()
        if hexstr != expected:
            raise Exception("Mismatch for LC=%d, LE=%d"%(LC,LE)+"\nActual:   "+hexstr+"\nExpected: "+expected)

        b = capdu.to_bytes()
        assert(type(b) is bytes)
        return (hexstr, capdu)

    #check __repr__
    expected = "pysatl.CAPDU.from_hexstr('00112233015502')"
    capdu=None
    exec("capdu="+expected)
    assert(expected==repr(capdu))

    #check well formed inputs
    check("00112233", CAPDU(CLA=0x00, INS=0x11, P1=0x22, P2=0x33))
    check("00 11 22 33", CAPDU(CLA=0x00, INS=0x11, P1=0x22, P2=0x33))
    check("0x00,0x11,0x22,0x33", CAPDU(CLA=0x00, INS=0x11, P1=0x22, P2=0x33))

    #check we tolerate less well formed inputs
    check("00-11,22_33", CAPDU(CLA=0x00, INS=0x11, P1=0x22, P2=0x33))
    check("""0x00 0x11    0x22
          0x33""", CAPDU(CLA=0x00, INS=0x11, P1=0x22, P2=0x33))
    check("1 2 304", CAPDU(CLA=0x01, INS=0x02, P1=0x03, P2=0x04))

    LC_cases = [0,1,2,254,255,256,257,65534,65535]
    LE_cases = LC_cases + [65536]
    for LC in LC_cases:
        for LE in LE_cases:
            print(LC,LE)
            check(*gencase(LC=LC, LE=LE))

