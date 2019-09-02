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
PY2 = (sys.version_info.major==2)
PY3 = (sys.version_info.major==3)

print(os.name)
print ("Python Version: %s" % sys.version)

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
    print("'ls'            List EEPROM content.")
    print("'m'             Measure VCC with calibrated values")
    print("'quit'          terminate program")
    print("'read'  or 'r'  read from EEPROM. Syntax: 'r(ead),<addr>'")
    print("'ri'            read 16 bit integer from EEPROM. Syntax: 'ri(ead),<addr>'")
    print("'rf'            read float from EEPROM. Syntax: 'ri(ead),<addr>'")
    print("'s'             request checksum update and store in EEPROM.")
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
      CHECKSUMb"

      
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
#    print key, mem[key], memtypes[key]

   

def write_eeprom(str2parse, serial):
    #try:
        items = str2parse.split(b',')
        if len(items) > 2:
            thekey = items[1].decode()
            if thekey in list(mem.keys()): #verbose 
                addr  = mem[thekey]
            else:
                addr = int(items[1]) 
            
            if addr in list(memtypes.keys()):
                if memtypes[addr] is 'b':
                    if items[2][:2] == b'0x':
                        val =  int(items[2],16)
                    else:
                        val =  int(items[2])

                    if addr >= 0 and addr < 512 and val >=0 and val < 256:
                        outstr = b"w,%i,%i\n" % (addr, val)
                        print(outstr.decode())
                        serial.write(outstr)
                    else:
                        print("Error: incorrect value(s)")

                elif memtypes[addr] is 'i':
                    if items[2][:2] == '0x':
                        val =  int(items[2],16)
                    else:
                        val =  int(items[2])
                        
                    if addr >= 0 and addr < 511:
                        outstr = b"wi,%i,%i\n" % (addr, val)
                        print(outstr.decode())
                        serial.write(outstr)
                    else:
                        print("Error: icorrect value(s)")

                elif memtypes[addr] is 'f':
                    val =  float(items[2])           
                    outstr = b"wf,%i,%f\n" % (addr, val)
                    print(outstr.decode())
                    serial.write(outstr)
                else:
                    print("wrong type")
            else:
                print("Error: invalid address parameter")
        else:
            print("Error: missing parameters")
    #except:
        #print("ERROR")        

        
def write_eeprom_uint(str2parse, serial):
    try:
        items = str2parse.split(b',')
        if len(items) > 2:
            thekey = items[1].decode()
            if thekey in list(mem.keys()):
                addr  = mem[thekey]
            else:
                addr = int(items[1])
            
            if items[2][:2] == '0x':
                val =  int(items[2],16)
            else:
                val =  int(items[2])
                
            if addr >= 0 and addr < 511 and val >=0 and val < pow(2,16):
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
        #print "ERROR"
        print(sys.exc_info())
        
def write_eeprom_float(str2parse, serial):
    try:
        items = str2parse.split(b',')
        if len(items) > 2:
            thekey = items[1].decode()
            if thekey in list(mem.keys()):
                addr  = mem[thekey]
            else:
                addr = int(items[1])
            
            val =  float(items[2])
            
            outstr = b"wf,%i,%f\n" % (addr, val)
            print(outstr.decode())
            serial.write(outstr)
            
        else:
            print("Error: missing parameters")
    except:
        #print "ERROR"
        print(sys.exc_info())


def write_eeprom_long(str2parse, serial):
    try:
        items = str2parse.split(b',')
        if len(items) > 2:
            thekey = items[1].decode()
            if thekey in list(mem.keys()):
                addr  = mem[thekey]
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
        items = str2parse.split(b',')
        if len(items) > 1:
            thekey = items[1].decode()
            if thekey in list(mem.keys()):
                addr  = mem[thekey]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < 512:
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
        items = str2parse.split(b',')
        if len(items) > 1:
            thekey = items[1].decode()
            if thekey in list(mem.keys()):
                addr  = mem[thekey]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < 512:
                outstr = b"rf,%i\n" % (addr)
                serial.write(outstr)
            else:
                print("Error: incorrect value(s)")
        else:
            print("Error: missing parameters")
    except:
        #print "ERROR"
        print(sys.exc_info())
        
def read_eeprom_int16_t(str2parse, serial):
    try:
        items = str2parse.split(b',')
        if len(items) > 1:
            thekey = items[1].decode()
            if thekey in list(mem.keys()):
                addr  = mem[thekey]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < 512:
                outstr = b"ri,%i\n" % (addr)
                serial.write(outstr)
            else:
                print("Error: incorrect value(s)")
        else:
            print("Error: missing parameters")
    except:
        #print "ERROR"
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
    outp=''
    if i in list(mem.keys()):
        #print (type(eepromlist[i]))
        t = memtypes[i]
        if t is 'b':
            outp = "%s=%i" %(mem[i], int(eepromlist[i]))
        elif t is 'i':
            if PY3:
                p= bytearray()
                p.append(int(eepromlist[i]))
                p.append(int(eepromlist[i+1]))
            else:
                p =chr(int(eepromlist[i])) + chr(int(eepromlist[i+1]))
            outp = "%s=%i" % (mem[i], struct.unpack('h', p)[0])
        elif t is 'f':
            if PY3:
                p= bytearray()
                p.append(int(eepromlist[i]))
                p.append(int(eepromlist[i+1]))
                p.append(int(eepromlist[i+2]))
                p.append(int(eepromlist[i+3]))
            else:
                p =chr(int(eepromlist[i])) + chr(int(eepromlist[i+1])) + chr(int(eepromlist[i+2])) + chr(int(eepromlist[i+3]))
            outp = "%s=%f" % (mem[i], struct.unpack('f', p)[0])
    else:
        #print "unknown parameter"
        pass
    return outp

    
def read_eeeprom_all(str2parse):
    eepromlist= str2parse[2:].split(b',')
    # print raw data (useful for debug only)
    #print eepromlist
    
    # print length of EEPROM content (useful for debug only)
    #print len(eepromlist)
    for i in range(len(eepromlist)):
        outp = parse_eeprom_list(eepromlist, i)
        if len(outp)>0:
            print(outp)


    
def eeprom2file(str2parse, filename):
    eepromlist= str2parse[2:].split(b',')
    file = open(filename, 'w')
    for i in range(len(eepromlist)):
        outp = parse_eeprom_list(eepromlist, i)
        if len(outp) > 0: 
            print(outp)
            file.write(outp+'\n')
    if calculate_checksum(eepromlist) ^ int(eepromlist[mem['CHECKSUM']]) == 0:
        print("Checksum ok.")
    else:
        print("\nPython calculated Checksum: 0x%x" % calculate_checksum(eepromlist))
    file.close()
        
def file2eeprom(str2parse, serial):  
    file = open(str2parse[3:],'r')
    content = file.readlines()
    file.close()
    #cs =0xaa # calculate checksum from file content, compare with checksum obtained from device after loading.
    for line in content:
        items = line.split('=')
        if len(items) > 1:
            items[0] = items[0].rstrip().lstrip()
            if items[0] in list(mem.keys()):
                addr = mem[items[0]]
                if memtypes[addr] is 'b':
                    if items[1][:2] == '0x':
                        val =  int(items[1],16)
                    else:
                        val =  int(items[1])

                    if addr >= 0 and addr < 512 and val >=0 and val < 256:
                        #if addr < mem['CHECKSUM']:
                            #cs ^= val
                        outstr = b"w,%i,%i\n" % (addr, val)
                        print(outstr[:-1])
                        serial.write(outstr)
                    else:
                        print("Error: incorrect value(s)")
 
                elif memtypes[addr] is 'i':
                    if items[1][:2] == '0x':
                        val =  int(items[1],16)
                    else:
                        val =  int(items[1])
                        
                    if addr >= 0 and addr < 511:
                        p = struct.pack('h', val)
                        #for c in p:
                            #cs ^= ord(c)
                        outstr = b"wi,%i,%i\n" % (addr, val)
                        print(outstr[:-1])
                        serial.write(outstr)
                    else:
                        print("Error: incorrect value(s)")

                elif memtypes[addr] is 'f':
                    val =  float(items[1])   
                    p= struct.pack('f',val)
                    #for c in p:
                        #cs ^= ord(c)
                    outstr = b"wf,%i,%f\n" % (addr, val)
                    print(outstr[:-1])
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

    print("now update checksum on device:")
    serial.write(b"s\n")
    if SerialWaitForResponse(port):
        recstr = read_raw(port)
        print("checksum: %s" % recstr.decode())

    
### calibrate VCC ###
### principle: apply a known VCC to the DUT. Let it measure VCC / raw value. 
### store VCC and measured raw result in EEPROM, as VCC_CAL and ADC_CAL (2 bytes each)
### Measured VCC will be: VCC_CAL*ADC_CAL/measured result
def cal_vcc(str2parse, serial):
    try:
        items = str2parse.split(b',')
        if len(items) > 1:
            vcc = int(items[1])
            if vcc > 1800: # must be > 1.8V to operate normally.
                outstr = b"wi,%i,%i\n" % ( mem['VCCatCAL'] ,vcc)
                print(outstr[:-2].decode())
                serial.write(outstr)
                time.sleep(.25)
                serial.write(b"c\n") # request DUT to measure ADC value and store into eeprom.
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
            
def SerialWaitForResponse(port):
    done = False
    tstart = time.time()
    while not done:
        if port.inWaiting() > 0:
            done = True
        elif (time.time() - tstart) > 0.5:
            print("SerialWaitForResponse: timeout")
            return False
        time.sleep(0.01)
    return done
            
if __name__ == '__main__':
    argc=len(sys.argv)
    argcindex =0
    if argc > 1:
        SerialPort = sys.argv[1]
        argcindex=2
        if argc > 2:
            SerialBaud = sys.argv[2]
            argcindex +=1
    atexit.register(set_normal_term)
    set_curses_term()
    inputstr=b""
    cmd_sent = ""
    storefile = ''
    recstr =b""
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
            ch = getch() #ch is a byte string
            #print ord(ch)
            if ord(ch) is 224:
                keycode = ord(getch())
                #print ("extended keycode: ", end="")
                #print (keycode)
                print ("extended keycode: %i" % keycode)
                if keycode == 72:
                    print("\narrow up")
                    pass
                continue
            elif ord(ch) is 8: #Backspace
                prompt = b'\r<-- '
                putch(b'\r')
                for i in range(len(inputstr)+4):
                    putch(b' ')
                inputstr = inputstr[:-1]
                
                for c in prompt:
                    if PY3:
                        c = bytes([c])
                    putch(c)
                for c in inputstr:
                    if PY3:
                        c = bytes([c])
                    putch(c)

                continue
            else:
                if inputstr ==b"":
                    inprompt = b"<-- "
                    for c in inprompt: #c is an int in PY3
                        #putch(bytes([c])) # putch wants a byte string in PY3
                        if PY2:
                            putch(c) # this is good in PY2
                        elif PY3:
                            putch(bytes([c])) # putch wants a byte string in PY3
                putch(ch)
            if ord(ch) == 10 or ord(ch) == 13:
                print('')
                #sent_commands.append(inputstr)
                # do something with input string, i.e send over serial port
                if inputstr == b"quit" or inputstr == b"exit" or inputstr == b"q":
                    port.close()
                    break
                elif inputstr == b"help" or inputstr == b"?":
                    print("help:")
                    help()
                
                #write an item into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:6] == b"write," or inputstr[:2] == b"w,":
                    write_eeprom(inputstr, port)
                    #tstart = time.time()
                    #isset_timeout = True

                #write an long int into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:3] == b'wl,':
                    write_eeprom_long(inputstr, port)
                
                #write an unsigned int into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:3] == b'wu,':
                    write_eeprom_uint(inputstr, port)

                #write float value into EEPROM. Specify address by number or by its Name, like "write,NODEID,20'
                elif inputstr[:3] == b'wf,':
                    write_eeprom_float(inputstr, port)
                    
                #read a float value from EEPROM
                elif inputstr[:3] == b"rf,":
                    read_eeprom_float(inputstr, port)
                    
                #read a float value from EEPROM
                elif inputstr[:3] == b"ri,":
                    read_eeprom_int16_t(inputstr, port)
                    
                #read an item from EEPROM. Specify either address by number or by its name , like "read,NODEID"
                elif inputstr[:5] == b"read," or inputstr[:2] == b"r,":
                    read_eeprom(inputstr, port)
                    
                #calibrate VCC measurement. 
                elif inputstr[:7] == b"vddcal,":
                    cal_vcc(inputstr, port)
                    
                #request DUT to measure ADC and write result into EEPROM.
                elif inputstr == b'c':
                    port.write(b"c\n")
                
                #copy data from file to eeprom
                elif inputstr[:3] == b'cp,' or inputstr[:5] == b'copy,':
                    file2eeprom(inputstr, port)
                    
                # request a list with all EEPROM items
                elif inputstr == b'a' or inputstr == b'ls':
                    port.write(b"a\n")
                    cmd_sent = inputstr # need to store this command as inputstr is reset after this loop
                
                # measure FEI from a external reference signal and store in EEPROM
                elif inputstr[:2]==b'fe':
                    port.write(b"fe\n")
                    
                # get eeprom content and save in file
                elif inputstr[:2] == b'g,' or inputstr[:4] == b'get,':
                    cmd_sent = inputstr
                    storefile = inputstr.split(b',')[1]
                    port.write(b"a\n")
                
                # request update of checksum
                elif inputstr[:1] == b's':
                    port.write(b"s\n")
                    tstart = time.time()
                    isset_timeout = True
                    
                # Measure VCC with calibrated values
                elif inputstr == b'm':
                    port.write(b"m\n")
                    
                # exit calibration mode
                elif inputstr[:1] == b'x':
                    xstr = b"x\n"
                    port.write(xstr)
                    #print "%i bytes written." % len(xstr)

                # Verify checksum
                elif inputstr == b'cs':
                    port.write(b"cs\n")                    
                
                elif inputstr ==  b'pc' or inputstr == b'close':
                    port.close()
                    print("serial port closed.")
                    
                elif inputstr == b'po' or inputstr == b'open':
                    print("open serial port now")
                    OpenSerialPort(port)
                    
                elif inputstr == b'pwd':
                    port.write(PassWord + b'\n')
                    
                elif inputstr[:3] == b'pw,':
                    port.write(inputstr[3:] + '\n')

                else:
                    print("Invalid command: %s" % inputstr) 
                    # in order to test the echo function of the device, uncomment the following 3 lines.
                    #inputstr += '\r'
                    #num = port.write(inputstr)
                    #print "Number of bytes written to serial port: %i" %(num)
                
                inputstr =b""
                #inprompt = "<-- "
                #for c in inprompt:
                    #putch(c)
            else:
                inputstr += ch
        if port.isOpen() and port.inWaiting() > 0:
            isset_timeout = False
            #ts = time.time()
            recstrraw = read_raw(port)           # Achtung, string returned kann \r\n enthalten == 2 Strings!
            recstrlist = recstrraw.split(b'\r\n')
            prompt = "--> "
            del recstrlist[-1]
            for recstr in recstrlist:
                if len(recstr) >  0: 
                    if recstr ==b"CAL?":
                        port.write(b'y')
                        #print "received calibration request and sent 'y'"
                        #print time.time() -ts
                    elif recstr[:2] == b"a," and (cmd_sent == b'a' or cmd_sent == b'ls'):                        
                        read_eeeprom_all(recstr)
                    elif recstr[:2] == b"a," and (cmd_sent[:2] == b'g,' or cmd_sent[:4] == b'get,'):
                        eeprom2file(recstr, storefile)
                        storefile=''
                    elif recstr[:3] == b'cs,':
                        print("Checksum %s" % recstr[3:])
                    elif recstr[:8] == b'vcc_adc,':
                        flo = float(recstr.split(b',')[1])
                        print("VCC = %.0f mV" % flo)
                    elif recstr == b"calibration mode.":
                        print(recstr.decode())
                        # das Passwort senden wenn in der Kommandozeile als Option
                        if "-pwd" in sys.argv:                        
                            port.write(PassWord + b'\n')
                    elif recstr == b"Pass OK":
                        pwd_ok = True
                        print(recstr.decode())
                        # die Optionen abarbeiten
                        #if "-ls" in sys.argv:
                        #   cmd_sent = 'ls'
                        #   port.write("a\n")
                    else:
                        #print "%s (%i bytes)" % (recstr, len(recstr))
                        type(recstr)
                        print(prompt + recstr.decode())
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
                port.write(b"a\n")
            elif option == '-cs': # query EEPROM
                print('<-- cs')
                port.write(b"cs\n")
            elif option == '-s': # update checksum
                print('<-- s')
                port.write(b"s\n")
            elif option == '-x': # store EEPROM and leave calibration mode
                port.write(b'x\n')
            elif option[:4] == '-cp,': # copy a file to EEPROM
                print(option[4:])
                file2eeprom(option[1:], port) # bloed, aber ist halt so
            elif option == '-pc': # close serial port
                port.close()
            elif option == '-q': #quit eepromer tool
                port.close()
            elif option[:7] == "-vddcal":
                cal_vcc(option[1:].encode(), port)
            #break
                
        #sys.stdout.write(b'.')
        time.sleep(.05)
        #print '-'
      
      except serial.serialutil.SerialException:
        print("Serial Port Error")
        inputstr =b""
        port.close()
        #OpenSerialPort(port)
      except IOError:
        print('IO ERROR')
    
    print('done')
