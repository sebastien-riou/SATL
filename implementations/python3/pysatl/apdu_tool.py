import re
import argparse
import os
import sys
import logging
import traceback
import pysatl

class EtsiTs101955(object):
    COMMENT_MARKER = "REM"
    COMMAND_MARKER = "CMD"
    RESET_MARKER = "RST"
    INIT_MARKER = "INI"
    OFF_MARKER = "OFF"

    def __init__(self, cmdHandler):
        self._cmdHandler = cmdHandler

    def runStream(self, scriptStream, *, line_cnt = 0):
        lineBuf = ""
        for line in scriptStream:
            line_cnt += 1
            if line in ["\n", "\r"]:
                line = ""
            elif len(line):
                while line[-1] in ["\n", "\r"]:  # remove end of line characters
                    line = line[:-1]
                    if 0 == len(line):
                        continue
            if 0 == len(line):
                continue
            lineBreak = line[-1] == "\\"
            if lineBreak:
                lineBuf += line[:-1]
                continue

            line = lineBuf + line
            lineBuf = ""
            logging.debug("line %4d: '%s'" % (line_cnt, line))
            if 0 == len(line):
                continue
            if line.startswith(EtsiTs101955.COMMENT_MARKER):
                self._cmdHandler.comment(line[len(EtsiTs101955.COMMENT_MARKER):])
                continue
            tokens = line.split()
            if tokens[0] == EtsiTs101955.RESET_MARKER:
                self._cmdHandler.reset()
            elif tokens[0] == EtsiTs101955.OFF_MARKER:
                self._cmdHandler.off()
            elif tokens[0] == EtsiTs101955.INIT_MARKER:
                datstr = line[len(tokens[0]):]
                dat = pysatl.Utils.ba(datstr)
                self._cmdHandler.init(dat)
            elif tokens[0] == EtsiTs101955.COMMAND_MARKER:
                params = line[len(tokens[0]):]
                cmd_params_pattern = re.compile(r"(.*)\[(.*)\]\s*\((.*)\)")
                matchRes = cmd_params_pattern.match(params)
                if matchRes is not None:
                    capdustr = matchRes.group(1)
                    leDatStr = matchRes.group(2).replace(" ","").replace("\t","").lower()
                    swStr = matchRes.group(3).replace(" ","").replace("\t","").lower()
                else:
                    cmd_params_pattern = re.compile(r"(.*)\s*\((.*)\)")
                    matchRes = cmd_params_pattern.match(params)
                    capdustr = matchRes.group(1)
                    leDatStr = ""
                    swStr = matchRes.group(2)
                swStr = swStr.replace(" ","").replace("\t","").lower()
                capdu = pysatl.CAPDU.from_hexstr(capdustr)
                rapdu = self._cmdHandler.apdu(capdu,leDatStr,swStr)
                swlist = swStr.split(",")
                swMatch = False
                for sw in swlist:
                    swMatch |= rapdu.matchSW(sw)
                if not swMatch:
                    raise Exception("RAPDU does not match any of the expected status word")
                if not rapdu.matchDATA(leDatStr):
                    raise Exception("RAPDU does not match expected outgoing data")
            else:
                raise Exception("line %d, syntax not supported: '%s'"%(line_cnt,line))

    def runFile(scriptFile, apduHandler):
        tool = EtsiTs101955(apduHandler)
        with open(scriptFile) as script:
            tool.runStream(script)

class CmdHandler(object):
    """Base class for command handlers"""

    def __init__(self):
        pass

    def apdu(self, capdu, leDatStr="", swStr=""):
        dat = pysatl.Utils.ba(leDatStr.replace('x','0'))
        sw=0
        swStr = swStr.split(",")[0]
        for i in range(0,len(swStr)):
            d = swStr[i]
            sw = (sw << 4) | int(d,16)
        sw1=sw >> 8
        sw2=sw & 0xFF
        rapdu = pysatl.RAPDU(SW1=sw1,SW2=sw2, DATA=dat)
        line = "CMD "
        header_len = 4
        lc=len(capdu.DATA)
        if lc:
            header_len = 5
            if lc>255:
                header_len = 7
        else:
            header_len = 5
            if capdu.LE>256:
                header_len = 7
        dat = capdu.to_ba()
        line += pysatl.Utils.hexstr(dat[:header_len])
        if len(capdu.DATA) > 0:
            line += " \\\n "
            dat = capdu.DATA
            while len(dat) > 16:
                line += pysatl.Utils.hexstr(dat[0:16]) + " \\\n "
                dat = dat[16:]
            line += pysatl.Utils.hexstr(dat)
        if len(rapdu.DATA) > 0:
            line += " \\\n ["
            dat = rapdu.DATA
            while len(dat) > 16:
                line += pysatl.Utils.hexstr(dat[0:16]) + " \\\n "
                dat = dat[16:]
            line += pysatl.Utils.hexstr(dat)
            line += " ] \\\n"
        elif capdu.LE > 0:
            line += " []"
        line += " ("+ pysatl.Utils.hexstr(rapdu.swBytes()) +")"
        logging.info(line)
        return rapdu

    def reset(self):
        logging.info("RST")

    def init(self, dat):
        logging.info("INIT "+pysatl.Utils.hexstr(dat))

    def off(self):
        logging.info("OFF")

    def comment(self, msg):
        logging.info("REM %s" % (msg))

class ApduTool(object):
    """ETSI TS 101 955 script player"""

    def __init__(self, argv):
        scriptname = os.path.basename(__file__)
        parser = argparse.ArgumentParser(scriptname)
        #TODO: pass argv to parser.
        levels = ('DEBUG', 'INFO', 'WARNING', 'ERROR', 'CRITICAL')
        parser.add_argument('--log-level', default='INFO', choices=levels)
        parser.add_argument('--script', default="stdin", help='path to script', type=str)
        options = parser.parse_args()
        root = logging.getLogger()
        root.setLevel(options.log_level)
        if options.script == "stdin":
            player = EtsiTs101955(CmdHandler())
            player.runStream(sys.stdin)
        else:
            EtsiTs101955.runFile(options.script,CmdHandler())

if __name__ == "__main__":
    ApduTool(sys.argv)
