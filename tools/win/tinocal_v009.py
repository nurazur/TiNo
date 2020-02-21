#!/usr/bin/env python

import os, sys

if os.name is not'nt':
    print("app is made for Windows; exit.")
    sys.exit(1)

import msvcrt, atexit, time

from select import select
import serial
import struct

SerialPort = 'COM10'
SerialBaud = 38400
PassWord = b"TheQuickBrownFox"

######### Python version ########
PY2 = (sys.version_info.major==2)
PY3 = (sys.version_info.major==3)

##########       EEPROM MAPPING      ##########
ADR_NUM_ACTIONS = 318
ADR_ACTIONS = ADR_NUM_ACTIONS + 1
MAX_NUM_ACTIONS = 40
EEPROM_SIZE = 1024

##########      Terminal Funktions - OS dependent       #########

# switch to normal terminal
def set_normal_term():
    pass

# switch to unbuffered terminal
def set_curses_term():
    pass

def putch(ch):
    msvcrt.putch(ch)

def getch():
    return msvcrt.getch()

def getche():
    return msvcrt.getche()

def kbhit():
    return msvcrt.kbhit()



def crc16(data):
    crc = 0xFFFF;

    if len(data) == 0:
        return 0

    for i in range(len(data)):
        dbyte = int(data[i])
        #dbyte = data[i]
        crc ^= (dbyte << 8)
        crc &= 0xFFFF
        for j in range(8):
            mix = crc & 0x8000
            crc = (crc << 1)
            crc &= 0xFFFF
            if (mix):
                crc = crc ^ 0x1021
            crc &= 0xFFFF
        #print (("%i, data: %i, crc: %x") % (i, data[i], crc))
    return crc;


def read_raw(port):
    raw_line=""
    if port.inWaiting() > 0:
        time.sleep(0.05)
        raw_line = port.read(port.inWaiting())
        if b'\r\n' not in raw_line[-2:]:
            done = False
            ts = time.time()
            while not done:
                if port.inWaiting() > 0:
                    raw_line += port.read(port.inWaiting())
                    if b'\r\n' in raw_line[-2:]: done = True
                else:
                    time.sleep(.01)
                if time.time() - ts > 1:
                    break;
    return raw_line


def help():
    print("'exit'          terminate program")
    print("'help'  or '?'  print this help text")
    print("'c'             measure ADC and store in EEPROM.")
    print("'copy' or 'cp'  copy file content to EEPROM. syntax: cp, <filename>")
    print("'cs'            verify checksum.")
    print("'fe'            receive 10 packets from external source, calculate mean and store in EEPROM")
    print("'g' or 'get'    store eeprom content to file. Syntax: 'g(et),<filename>'")
    print("'la'            List definitions of actions the node shall execute")
    print("'ls'            List EEPROM basic configuration.")
    print("'m'             Measure VCC with calibrated values")
    print("'quit'          terminate program")
    print("'read'  or 'r'  read from EEPROM. Syntax: 'r(ead),<addr>'")
    print("'ri'            read 16 bit integer from EEPROM. Syntax: 'ri(ead),<addr>'")
    print("'rf'            read float from EEPROM. Syntax: 'ri(ead),<addr>'")
    print("'s'             request checksum update and store in EEPROM.")
    print("'sy'            activate and/or re-sync a node with the rolling-code. Syntax: 'sy,<node-id>")
    print("'vddcal'        calibrate VCC measurement. Syntax: 'v(ddcal),<VCC at device in mV>'")
    print("'write' or 'w'  write value to to EEPROM.  Syntax: 'w(rite),<addr>,<value>'")
    print("'wf'            write float value to EEPROM. Syntax: 'wf,<addr>,<value>'")
    print("'wl'            write long int value to to EEPROM.  Syntax: 'wl,<addr>,<value>', value format can be hex")
    print("'wu'            write unsigned int value to EEPROM. Syntax: 'wu,<addr>,<value>'")
    print("'x'             exit calibration mode and continue with loop()")

# copy from C++ ENUM
mem_s = "NODEIDb, NETWORKIDb, GATEWAYIDb, \
      VCCatCALi, VCCADC_CALi, \
      SENDDELAYi, \
      FREQBANDb, \
      FREQ_CENTERf,\
      TXPOWERb, \
      REQUESTACKb, \
      LEDCOUNTb,\
      LEDPINb,\
      RXPINb,\
      TXPINb,\
      SDAPINb,\
      SCLPINb,\
      I2CPOWERPINb,\
      PCI0PINb,\
      PCI0TRIGGERb,\
      PCI1PINb,\
      PCI1TRIGGERb,\
      PCI2PINb,\
      PCI2TRIGGERb,\
      PCI3PINb,\
      PCI3TRIGGERb,\
      USE_CRYSTAL_RTCb,\
      ENCRYPTION_ENABLEb,\
      FEC_ENABLEb,\
      INTERLEAVER_ENABLEb,\
      EEPROM_VERSION_NUMBERb,\
      SOFTWAREVERSION_NUMBERi,\
      TXGAUSS_SHAPINGb,\
      SERIAL_ENABLEb,\
      IS_RFM69HWb,\
      PABOOSTb,\
      FDEV_STEPSi,\
      CHECKSUMi"


i=0
mem={} #dictionary
memtypes={}

mem_l = mem_s.split(',') # string converted to list

idx =0
for item in mem_l:
    k = item.strip()
    key = k[:-1]
    mem[key] = idx;
    mem[idx] = key;
    t = k[-1:]
    memtypes[key] = t
    memtypes[idx] = t
    if t is 'i':
        idx = idx +2
    elif t is 'f':
        idx = idx+4
    else:
        idx = idx + 1

#for index, item in enumerate(mem_l):
#    mem[item.strip()[:-1]] = index

#inv_mem = {v: k for k, v in mem.iteritems()}
#mem.update(inv_mem)

#for key in mem.keys():
#    print (key, mem[key], memtypes[key])


def write_eeprom_frequency_correction(serial):
    CEC = False
    SJK = False
    H   = False
    ALL = True
    try:
        offset = mem['CHECKSUM'] +2
        # one for all Crystals
        if ALL:
            a =   0.000104
            b =   -0.008386
            c =   0.023782
            d =   2.625568
        # SJK coefficients
        elif SJK:
            a =   0.00010445
            b = - 0.00956566
            c =   0.14759782
            d =   0.39464113

        # CEC coefficients normiert auf 23 degC
        elif CEC:
            a =   0.0000956320
            b = - 0.0079379912
            c = - 0.0013709436
            d =   3.06719812

        # -H coefficients, similar to SJK
        elif H:
            a =   0.00009504
            b = - 0.00768652
            c =   0.04871732
            d =   1.78931904
        else:
            a =   0
            b =   0
            c =   0
            d =   0

        print(a,b,c,d)
        for t in range (-30, 106):
            t2 = t * t
            t3 = t2 * t
            # coefficient formula is in ppm, but i want to write 16 bit integer values
            # so I multiply by 1000, so the coefficients are parts per billion.
            # to calculate f-steps, the driver calculates coeff * center_frequency / 61.03515625
            coeff = int(round((a*t3 + b*t2 + c*t + d) * 1000))
            if coeff > 32767:
                coeff = 32767
            #print("index: %i, memloc: %i, coeff(ppb): %i" % (t, t+30+offset, coeff))
            correction  = round(coeff * 865 / 1000 / 61.03515625)

            outstr = b"wi,%i,%i\n" % (offset, coeff)

            serial.write(outstr)
            temp = (offset - (mem['CHECKSUM'] +2))/2 -30
            print("%s, temp: %i, correction steps: %.2f" % (outstr[:-1], temp, correction))
            if SerialWaitForResponse(port):
                    read_raw(port)
            else:
                while not SerialWaitForResponse(port):
                    read_raw(port)
                    print(outstr[:-1])
                    serial.write(outstr)
                read_raw(port)

            offset += 2

    except:
        print("General ERROR when attempting to write frequency correction table")
        print(sys.exc_info())


def write_eeprom(str2parse, serial):
    try:
        items = str2parse.split(b',')
        if len(items) > 2:
            thekey = items[1].decode()
            if thekey in list(mem.keys()): #verbose
                addr  = mem[thekey]
            else:
                addr = int(items[1]) # items[1] is type string in any case! here we must change to integer.

            if addr in list(memtypes.keys()):
                if memtypes[addr] is 'b':
                    if items[2][:2] == b'0x':
                        val =  int(items[2],16)
                    else:
                        val =  int(items[2])

                    if addr >= 0 and addr < EEPROM_SIZE and val >=0 and val < 256:
                        outstr = b"w,%i,%i\n" % (addr, val)
                        print(outstr.decode())
                        serial.write(outstr)
                    else:
                        print("Error: incorrect value(s)")

                elif memtypes[addr] is 'i':
                    if items[2][:2] == b'0x':
                        val =  int(items[2],16)
                    else:
                        val =  int(items[2])

                    if addr >= 0 and addr < EEPROM_SIZE-1:
                        outstr = b"wi,%i,%i\n" % (addr, val)
                        print(outstr.decode())
                        serial.write(outstr)
                    else:
                        print("Error: incorrect address value(s)")

                elif memtypes[addr] is 'f':
                    val =  float(items[2])
                    outstr = b"wf,%i,%f\n" % (addr, val)
                    print(outstr.decode())
                    serial.write(outstr)
                else:
                    print("wrong type")
            else:
                #print("Error: invalid address parameter")
                outstr = b"w,%s,%s\n" % (items[1], items[2])
                serial.write(outstr)
        else:
            print("Error: missing parameters")
    except:
        #print("ERROR")
        print(sys.exc_info())


def write_eeprom_uint(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 2:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])

            if items[2][:2] == '0x':
                val =  int(items[2],16)
            else:
                val =  int(items[2])

            if addr >= 0 and addr < EEPROM_SIZE-1 and val >=0 and val < pow(2,16):
                outstr = b"w,%i,%i\n" % (addr, val&0xff) # LSB
                serial.write(outstr)
                time.sleep(.25)
                outstr = b"w,%i,%i\n" % (addr+1, val>>8) # MSB
                serial.write(outstr)
            else:
                print("Error: icorrect value(s)")
        else:
            print("Error: missing parameters")
    except:
        #print("ERROR")
        print(sys.exc_info())



def write_eeprom_int16(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 2:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])

            if items[2][:2] == '0x':
                val =  int(items[2],16)
            else:
                val =  int(items[2])

            if addr >= 0 and addr < EEPROM_SIZE-1 and val > -pow(2,15) and val < pow(2,15):
                outstr = b"wi,%i,%i\n" % (addr, val&0xFFFF)
                serial.write(outstr)
                time.sleep(.25)
            else:
                print("Error: icorrect value(s)")
        else:
            print("Error: missing parameters")
    except:
        #print("ERROR")
        print(sys.exc_info())


def write_eeprom_float(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 2:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])

            val =  float(items[2])

            outstr = b"wf,%i,%f\n" % (addr, val)
            print(outstr.decode())
            serial.write(outstr)

        else:
            print("Error: missing parameters")
    except:
        #print("ERROR")
        print(sys.exc_info())


def write_eeprom_long(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 2:
            if items[1] in mem.keys():
                addr = mem[items[1]]
            else:
                addr = int(items[1])

            if items[2][:2] == '0x':
                val =  int(items[2],16)
            else:
                val =  int(items[2])

            for i in range(4):
                outstr = b"w,%i,%i\n" % (addr+i, val & 0xff)
                val >>= 8
                serial.write(outstr)
                time.sleep(.25)
        else:
            print("Error: missing parameters")
    except:
        #print("Error")
        print(sys.exc_info())

def read_eeprom(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 1:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < EEPROM_SIZE:
                outstr = b"r,%i\n" % (addr)
                serial.write(outstr)
            else:
                print("Error: icorrect value(s)")
        else:
            print("Error: missing parameters")
    except:
        #print("ERROR")
        print(sys.exc_info())

def read_eeprom_float(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 1:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < EEPROM_SIZE-4:
                outstr = b"rf,%i\n" % (addr)
                serial.write(outstr)
            else:
                print("Error: incorrect value(s)")
        else:
            print("Error: missing parameters")
    except:
        #print("ERROR")
        print(sys.exc_info())

def read_eeprom_int16_t(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 1:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < EEPROM_SIZE-2:
                outstr = b"ri,%i\n" % (addr)
                serial.write(outstr)
            else:
                print("Error: incorrect value(s)")
        else:
            print("Error: missing parameters")
    except:
        #print("ERROR")
        print(sys.exc_info())

def calculate_checksum(eeprom_list):
    cs =0xaa
    for i in range(mem['CHECKSUM']):
        cs ^= int(eeprom_list[i])
    return cs & 0xff

# eepromlist is the list of raw eeprom data
# i is a particular index and must be in the dictionary
# accordingly the value is read out and a human readble Variable name as well
def parse_eeprom_list(eepromlist, i):
    outp='' #type string
    if i in mem.keys():
        t = memtypes[i]
        if t is 'b':
            outp = "%s=%s" %(mem[i], eepromlist[i])
        elif t is 'i':
            p =chr(int(eepromlist[i])) + chr(int(eepromlist[i+1]))
            outp = "%s=%i" % (mem[i], struct.unpack('h', p)[0])
        elif t is 'f':
            p =chr(int(eepromlist[i])) + chr(int(eepromlist[i+1])) + chr(int(eepromlist[i+2])) + chr(int(eepromlist[i+3]))
            outp = "%s=%f" % (mem[i], struct.unpack('f', p)[0])
    else:
        #print("unknown parameter")
        pass
    return outp


def read_eeeprom_all(str2parse):
    eepromlist= str2parse[2:].split(',')
    # print raw data (useful for debug only)
    #print eepromlist

    # print length of EEPROM content (useful for debug only)
    #print len(eepromlist)
    for i in range(len(eepromlist)):
        outp = parse_eeprom_list(eepromlist, i)
        if len(outp)>0:
            print(outp)



def read_actions():
    outstr = b"r,%i" % ADR_NUM_ACTIONS
    node =''
    port.write(outstr + b'\n')
    if SerialWaitForResponse(port):
        recstr = read_raw(port) # recstr is a byte string
    print ((b"Number of actions: %s" % recstr[:-2]).decode())
    numactions = int(recstr[:-2])

    port.write(b"r,0\n")
    if SerialWaitForResponse(port):
        thisnode = read_raw(port)
        #print("this node is: %s" % thisnode)


    for action in range(numactions):
        print('')
        addr = ADR_ACTIONS + action *4
        print (b"Action %i:" % action)

        outstr = b"r,%i\n" % addr
        port.write(outstr)
        if SerialWaitForResponse(port):
            node = read_raw(port)
            print((b"Node:  %s" % node[:-2]).decode())

        outstr = b"r,%i\n" % (addr+1)
        port.write(outstr)
        if SerialWaitForResponse(port):
            recstr = read_raw(port)
            print((b"Port:  %s" % recstr[:-2]).decode())

        outstr = b"r,%i\n" % (addr+2)
        port.write(outstr)
        if SerialWaitForResponse(port):
            recstr = read_raw(port)
            mask = int(recstr)
            triggersource = ''
            if node != thisnode:
                if mask & 0x1:
                    triggersource += " Heartbeat"
                if mask & 0x2:
                    triggersource += " PCI0"
                if mask & 0x4:
                    triggersource += " PCI1"
                if mask & 0x8:
                    triggersource += " PCI2"
                if mask & 0x10:
                    triggersource += " PCI3"
            else:
                if mask & 0x1:
                    triggersource += " PCI0"
                if mask & 0x2:
                    triggersource += " PCI1"
                if mask & 0x4:
                    triggersource += " PCI2"
                if mask & 0x8:
                    triggersource += " PCI3"

            print("Mask:  %s (%s )" % (recstr[:-2], triggersource))


        outstr = b"r,%i\n" % (addr+3)
        port.write(outstr)
        if SerialWaitForResponse(port):
            recstr = read_raw(port)
            pulsdur =''
            onoff = int(recstr) & 0x3
            if onoff == 0:
                mode ="OFF"
            elif onoff == 1:
                mode = "ON"
            elif onoff == 2:
                mode ="TOGGLE"
            else: #onoff == 3:
                mode = "PULSE"
                p = (int(recstr) & 0x7C)>>2
                p = pow(2,p-1)
                pulsdur = ", %.1f seconds" % p

            print("OnOff: %s (%s%s)" % (recstr[:-2], mode, pulsdur))



def eeprom2file(str2parse, filename):
    eepromlist= str2parse[2:].split(b',') # list of strings
    file = open(filename, 'w')
    for i in range(len(eepromlist)):
        outp = parse_eeprom_list(eepromlist, i)
        if 'USE_CRYSTAL_RTC' in outp or 'EEPROM_VERSION_NUMBER' in outp or 'SOFTWAREVERSION_NUMBER' in outp:
            continue
        if len(outp) > 0:
            print(outp)
            file.write(outp+'\n')
    #crc16_checksum = crc16(eepromlist[:-2])
    #if crc16_checksum ^ int(eepromlist[mem['CHECKSUM']]) == 0:
    #    print("Checksum ok.")
    #else:
    #    print("\nPython calculated Checksum: %i" % crc16_checksum)
    file.close()

def file2eeprom(str2parse, serial):
    file = open(str2parse[3:],'r')
    content = file.readlines()
    file.close()

    for line in content:
        line = line.split('#',1)[0]    # remove comments from line
        items = line.split('=')
        if len(items) > 1:
            items[0] = items[0].strip()
            if items[0] in mem.keys():
                addr = mem[items[0]]
                # write Byte
                if memtypes[addr] is 'b':
                    if items[1][:2] == '0x':
                        val =  int(items[1],16)
                    else:
                        val =  int(items[1])

                    if addr >= 0 and addr < EEPROM_SIZE and val >=0 and val < 256:
                        outstr = b"w,%i,%i\n" % (addr, val)
                        print(outstr[:-1].decode())
                        serial.write(outstr)
                    else:
                        print("Error: incorrect value(s)")

                #write 16 bit Integer
                elif memtypes[addr] is 'i':
                    if items[1][:2] == '0x':
                        val =  int(items[1],16)
                    else:
                        val =  int(items[1])

                    if addr >= 0 and addr < EEPROM_SIZE-1:
                        outstr = b"wi,%i,%i\n" % (addr, val)
                        print(outstr[:-1].decode())
                        serial.write(outstr)
                    else:
                        print("Error: incorrect value(s)")

                #write 32 bit float
                elif memtypes[addr] is 'f':
                    val =  float(items[1])
                    outstr = b"wf,%i,%f\n" % (addr, val)
                    print(outstr[:-1].decode())
                    serial.write(outstr)
                else:
                    print("wrong type")

                if SerialWaitForResponse(port):
                    read_raw(port)
                else:
                    while not SerialWaitForResponse(port):
                        read_raw(port)
                        print(outstr[:-1])
                        serial.write(outstr)
                    read_raw(port)

    serial.write("s\n")
    if SerialWaitForResponse(port):
        recstr = read_raw(port)
        print("checksum: %s" % recstr)

    data_arr = bytearray(MAX_NUM_ACTIONS *4) # initialized with zero's
    act_params = {'NODE':0,'PORT':1,'MASK':2,'ONOFF':3}
    number_of_actions = 0
    for line in content:
        line = line.split('#',1)[0]              # remove comments from line
        if 'ACTION' in line[:6]:                 # for example: ACTION19.NODE = 25
            params = line.split('=')             # ['ACTION19.NODE ', ' 25']
            if len(params) >1:
                act = params[0].split('.')       # ['ACTION19', 'NODE ']
                act_num = int(act[0][6:])        # 19
                if len(act) > 1:
                    act_offs = act[1].rstrip()   # 'NODE'
                    if act_offs in list(act_params.keys()):
                        addr = act_num * 4 + act_params[act_offs]
                        if act_offs == 'NODE':
                            number_of_actions += 1
                    else:
                        continue # line does not contain a valid a key
                else:
                    continue # line does not contain a key
                act_val = int(params[1])         # example: 25
                data_arr[addr] = act_val         # put value at right place in array, because sequence is important for CRC16

                outstr = b"w,%i,%i\n" % (addr + ADR_ACTIONS, act_val)
                print(outstr[:-1])
                serial.write(outstr)
                if SerialWaitForResponse(port):
                    read_raw(port)
                else:
                    while not SerialWaitForResponse(port):
                        read_raw(port)
                        print(outstr[:-1])
                        serial.write(outstr)
                    read_raw(port)
            else:
                continue #line does not contain a '='

    outstr = "w,%i,%i\n" % (ADR_NUM_ACTIONS, number_of_actions)
    print(outstr[:-1])
    serial.write(outstr)  # write number of actions into EEPROM
    if SerialWaitForResponse(port):
        read_raw(port)
    data_arr[4*number_of_actions:4*MAX_NUM_ACTIONS] =[] # strip unused bytes from array
    cs = crc16(data_arr)
    cs_adr = ADR_ACTIONS + MAX_NUM_ACTIONS *4
    outstr = b"wi,%i,%i\n" %(cs_adr, cs)
    serial.write(outstr)    # write CRC16 checksum into EEPROM
    if SerialWaitForResponse(port):
        read_raw(port)
    #print("cs - addresse: %i" % cs_adr)
    print("checksum calculated by Python: 0x%x" %(cs))



### calibrate VCC ###
### principle: apply a known VCC to the DUT. Let it measure VCC / raw value.
### store VCC and measured raw result in EEPROM, as VCC_CAL and ADC_CAL (2 bytes each)
### Measured VCC will be: VCC_CAL*ADC_CAL/measured result
def cal_vcc(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 1:
            vcc = int(items[1])
            if vcc > 1800: # must be > 1.8V to operate normally.
                outstr = "wi,%i,%i\n" % ( mem['VCCatCAL'] ,vcc)
                print(outstr[:-2])
                serial.write(outstr)
                #time.sleep(.25)
                #outstr = "w,%i,%i\n" % (mem['VCCatCAL_LSB'], (vcc & 0xFF) )
                #print(outstr[:-2])
                #serial.write(outstr)
                time.sleep(.25)
                serial.write("c\n") # request DUT to measure ADC value and store into eeprom.
            else:
                print("too small VCC! abort calibration.")
                return
        else:
            print("Error: missing parameters")
    except:
        print(" VCC calibration failed.")

def is_timeout(tstart, delayms, isset):
    if isset and ((time.time() -  tstart) > delayms/1000.0):
        return True
    else:
        return False

def OpenSerialPort(port):
    while not port.isOpen():
        try:
            port.open()
            print("ready.")
        except:
            time.sleep(0.1)


def rolling_code_sync(instr):
    try:
        if instr =='all': #reset allnodes into "not tracked" mode
            for i in range(513,1024,2):
                outstr = "w,%i,255" % i
                print(outstr)
                port.write(outstr +'\n')
                if SerialWaitForResponse(port):
                    read_raw(port)

        else:
            node = int(instr) #activate and reset specific node (re-sync)
            node = 513 + 2 * node
            outstr = "w,%i,254" % node
            print(outstr)
            port.write(outstr +'\n')
    except:
        print("Sync for Node failed")




def SerialWaitForResponse(port):
    done = False
    tstart = time.time()
    while not done:
        if port.inWaiting() > 0:
            done = True
        #elif (time.time() - tstart) > 0.5:
        elif (time.time() - tstart) > 2:
            print("SerialWaitForResponse: timeout")
            return False
        time.sleep(0.01)
    return done

if __name__ == '__main__':
    argc=len(sys.argv)
    argcindex =1
    if argc > argcindex:
        if sys.argv[argcindex][0] is not '-':
            SerialPort = sys.argv[argcindex]
            argcindex +=1
        if argc > argcindex:
            if sys.argv[argcindex][0] is not '-':
                SerialBaud = sys.argv[argcindex]
                argcindex +=1
    atexit.register(set_normal_term)
    set_curses_term()
    inputstr=""
    cmd_sent = ""
    storefile = ''
    recstr =""
    tstart = time.time()
    isset_timeout = False
    timeout_delay = 500 # timeout in ms
    port  = serial.Serial()
    port.port = SerialPort
    port.baudrate = SerialBaud

    #Oeffne Seriellen Port
    OpenSerialPort(port)
    '''
    while not port.isOpen():
        try:
            port.open()
            print "ready."
        except:
            time.sleep(0.1)
    '''
    '''
    # timeout?
    if not port.isOpen():
        print "cannot connect to serial port."
        sys.exit(1)
    '''
    #sent_commands = list()
    pwd_ok = False
    while 1:
      try:
        if kbhit():
            ch = getch()
            #print ord(ch)
            if ord(ch) is 224:
                keycode = ord(getch())
                if keycode == 72:
                    print("\narrow up")
                    pass
                continue
            elif ord(ch) is 8: #Backspace
                prompt = '\r<-- '
                putch('\r')
                for i in range(len(inputstr)+4):
                    putch(' ')
                inputstr = inputstr[:-1]
                for c in prompt:
                    putch(c)
                for c in inputstr:
                    putch(c)
                continue
            else:
                if inputstr =="":
                    inprompt = "<-- "
                    for c in inprompt:
                        putch(c)
                putch(ch)
            if ch == chr(10) or ch == chr(13):
                print('')
                #sent_commands.append(inputstr)
                # do something with input string, i.e send over serial port
                if inputstr == "quit" or inputstr == "exit" or inputstr == "q":
                    port.close()
                    break
                elif inputstr == "help" or inputstr == "?":
                    print("help:")
                    help()

                #write an item into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:6] == "write," or inputstr[:2] == "w,":
                    write_eeprom(inputstr, port)
                    #tstart = time.time()
                    #isset_timeout = True

                #write an long int into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:3] == 'wl,':
                    write_eeprom_long(inputstr, port)

                #write an unsigned int into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:3] == 'wu,':
                    write_eeprom_uint(inputstr, port)

                #write an unsigned int into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:3] == 'wi,':
                    write_eeprom_int16(inputstr, port)

                #write float value into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:3] == 'wf,':
                    write_eeprom_float(inputstr, port)

                #read a float value from EEPROM
                elif inputstr[:3] == "rf,":
                    read_eeprom_float(inputstr, port)

                #read a float value from EEPROM
                elif inputstr[:3] == "ri,":
                    read_eeprom_int16_t(inputstr, port)

                #read an item from EEPROM. Specify either address by number or by its name , like "read,NODEID"
                elif inputstr[:5] == "read," or inputstr[:2] == "r,":
                    read_eeprom(inputstr, port)

                #calibrate VCC measurement.
                elif inputstr[:7] == "vddcal,":
                    cal_vcc(inputstr, port)

                #request DUT to measure ADC and write result into EEPROM.
                elif inputstr == 'c':
                    port.write("c\n")

                #copy data from file to eeprom
                elif inputstr[:3] == 'cp,' or inputstr[:5] == 'copy,':
                    file2eeprom(inputstr, port)

                # request a list with all EEPROM items
                elif inputstr == 'a' or inputstr == 'ls':
                    port.write("a\n")
                    cmd_sent = inputstr # need to store this command as inputstr is reset after this loop

                # list actions definition
                elif inputstr=='la':
                    read_actions()

                # measure FEI from a external reference signal and store in EEPROM
                elif inputstr[:2]=='fe':
                    port.write("fe\n")

                # load frequency correction table into EEPROM. Values are calculated by Python from cubic parameters
                elif inputstr == 'ft' or inputstr == 'frequency_table':
                    write_eeprom_frequency_correction(port)

                # get eeprom content and save in file
                elif inputstr[:2] == 'g,' or inputstr[:4] == 'get,':
                    cmd_sent = inputstr
                    storefile = inputstr.split(',')[1]
                    port.write("a\n")

                #reset rolling code for node x
                elif inputstr[:3] == 'sy,':
                    rolling_code_sync(inputstr[3:])

                # request update of checksum
                elif inputstr[:1] == 's':
                    port.write("s\n")
                    tstart = time.time()
                    isset_timeout = True

                # Measure VCC with calibrated values
                elif inputstr == 'm':
                    port.write("m\n")

                # exit calibration mode
                elif inputstr[:1] == 'x':
                    xstr = "x\n"
                    port.write(xstr)
                    #print("%i bytes written." % len(xstr))

                # Verify checksum
                elif inputstr == 'cs':
                    port.write("cs\n")

                elif inputstr ==  'pc' or inputstr == 'close':
                    port.close()
                    print("serial port closed.")

                elif inputstr == 'po' or inputstr == 'open':
                    print("open serial port now")
                    OpenSerialPort(port)

                elif inputstr == 'pwd':
                    port.write(PassWord + '\n')

                elif inputstr[:3] == 'pw,':
                    port.write(inputstr[3:] + '\n')

                else:
                    print("Invalid command: %s" % inputstr)
                    # in order to test the echo function of the device, uncomment the following 3 lines.
                    #inputstr += '\r'
                    #num = port.write(inputstr)
                    #print("Number of bytes written to serial port: %i" %(num))

                inputstr =""
                #inprompt = "<-- "
                #for c in inprompt:
                    #putch(c)
            else:
                inputstr += ch
        if port.isOpen() and port.inWaiting() > 0:
            isset_timeout = False
            #ts = time.time()
            recstrraw = read_raw(port)           # Achtung, string returned kann \r\n enthalten == 2 Strings!
            recstrlist = recstrraw.split('\r\n')
            prompt = "--> "
            del recstrlist[-1]
            for recstr in recstrlist:
                if len(recstr) >  0:
                    if recstr =="CAL?":
                        port.write("y")
                        #print("received calibration request and sent 'y'")
                        #print (time.time() -ts)
                    elif recstr[:2] == "a," and (cmd_sent == 'a' or cmd_sent == 'ls'):
                        #print(recstr)
                        read_eeeprom_all(recstr)
                    elif recstr[:2] == "a," and (cmd_sent[:2] == 'g,' or cmd_sent[:4] == 'get,'):
                        eeprom2file(recstr, storefile)
                        storefile=''
                    elif recstr[:3] == 'cs,':
                        print("Checksum %s" % recstr[3:])
                    elif recstr[:8] == 'vcc_adc,':
                        print("VCC = %.0f mV" % float(recstr.split(',')[1]))
                    elif recstr == "calibration mode.":
                        print(recstr)
                        # das Passwort senden wenn in der Kommandozeile als Option
                        if "-pwd" in sys.argv:
                            port.write(PassWord + '\n')
                    elif recstr == "Pass OK":
                        pwd_ok = True
                        print(recstr)
                        # die Optionen abarbeiten
                        #if "-ls" in sys.argv:
                        #   cmd_sent = 'ls'
                        #   port.write("a\n")
                    else:
                        #print "%s (%i bytes)" % (recstr, len(recstr))
                        print(prompt + recstr)
                else:
                    print("garbage!")
            #inprompt = "<-- "
            #for c in inprompt:
                #putch(c)
        else:
            if is_timeout(tstart, timeout_delay, isset_timeout):
                isset_timeout = False
                print("timeout!!")
        if pwd_ok and argc > argcindex:
            option = sys.argv[argcindex]
            argcindex += 1
            if option =='-ls': # list EEPROM content
                cmd_sent = 'ls'
                port.write("a\n")
            elif option == '-cs': # query EEPROM
                print('<-- cs')
                port.write("cs\n")
            elif option == '-s': # update checksum
                print('<-- s')
                port.write("s\n")
            elif option == '-x': # store EEPROM and leave calibration mode
                port.write('x\n')
            elif option[:4] == '-cp,': # copy a file to EEPROM
                print(option[4:])
                file2eeprom(option[1:], port) # bloed, aber ist halt so
            elif option == '-pc': # close serial port
                port.close()
            elif option == '-q': #quit eepromer tool
                port.close()
                break

        #sys.stdout.write('.')
        time.sleep(.05)
        #print ('-')

      except serial.serialutil.SerialException:
        print("Serial Port Error")
        inputstr =""
        port.close()
        #OpenSerialPort(port)
      except IOError:
        print('IO ERROR')
      except:
        print(sys.exc_info())

    print('done')
