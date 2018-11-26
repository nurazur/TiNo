#!/usr/bin/env python

import sys, msvcrt, atexit, time
from select import select
import serial
import struct

SerialPort = 'COM10'
SerialBaud = 57600
#PassWord = "WiNW_AzurdelaMer"
PassWord = "TheQuickBrownFox"


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
        if '\r\n' not in raw_line[-2:]:
            done = False
            ts = time.time()
            while not done:
                if port.inWaiting() > 0:
                    raw_line += port.read(port.inWaiting())
                    if '\r\n' in raw_line[-2:]: done = True
                else:
                    time.sleep(.01)
                if time.time() - ts > 1:
                    break;
    return raw_line
    
    
def help():
    print "'exit'          terminate program"
    print "'help'  or '?'  print this help text"
    print "'c'             measure ADC and store in EEPROM."
    print "'copy' or 'cp'  copy file content to EEPROM. syntax: cp, <filename>"
    print "'cs'            verify checksum."
    print "'fe'            receive 10 packets from external source, calculate mean and store in EEPROM"
    print "'g' or 'get'    store eeprom content to file. Syntax: 'g(et),<filename>'"
    print "'ls'            List EEPROM content."
    print "'m'             Measure VCC with calibrated values"
    print "'quit'          terminate program"
    print "'read'  or 'r'  read from EEPROM. Syntax: 'r(ead),<addr>'"
    print "'ri'            read 16 bit integer from EEPROM. Syntax: 'ri(ead),<addr>'"
    print "'rf'            read float from EEPROM. Syntax: 'ri(ead),<addr>'"
    print "'s'             request checksum update and store in EEPROM."
    print "'vddcal'        calibrate VCC measurement. Syntax: 'v(ddcal),<VCC at device in mV>'"
    print "'write' or 'w'  write value to to EEPROM.  Syntax: 'w(rite),<addr>,<value>'"
    print "'wf'            write float value to EEPROM. Syntax: 'wf,<addr>,<value>'"
    print "'wl'            write long int value to to EEPROM.  Syntax: 'wl,<addr>,<value>', value format can be hex"
    print "'wu'            write unsigned int value to EEPROM. Syntax: 'wu,<addr>,<value>'"
    print "'x'             exit calibration mode and continue with loop()"

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
    try:
        items = str2parse.split(',')
        if len(items) > 2:
            if items[1] in mem.keys(): #verbose 
                addr  = mem[items[1]]
            else:
                addr = int(items[1]) # items[1] is type string in any case! here we must change to integer. 
            
            if addr in memtypes.keys():
                if memtypes[addr] is 'b':
                    if items[2][:2] == '0x':
                        val =  int(items[2],16)
                    else:
                        val =  int(items[2])

                    if addr >= 0 and addr < 512 and val >=0 and val < 256:
                        outstr = "w,%i,%i\n" % (addr, val)
                        print outstr
                        serial.write(outstr)
                    else:
                        print "Error: incorrect value(s)"

                elif memtypes[addr] is 'i':
                    if items[2][:2] == '0x':
                        val =  int(items[2],16)
                    else:
                        val =  int(items[2])
                        
                    if addr >= 0 and addr < 511:
                        outstr = "wi,%i,%i\n" % (addr, val)
                        print outstr
                        serial.write(outstr)
                    else:
                        print "Error: icorrect value(s)"

                elif memtypes[addr] is 'f':
                    val =  float(items[2])           
                    outstr = "wf,%i,%f\n" % (addr, val)
                    print outstr
                    serial.write(outstr)
                else:
                    print "wrong type"
            else:
                print "Error: invalid address parameter"
        else:
            print "Error: missing parameters"
    except:
        print "ERROR"        

        
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
                
            if addr >= 0 and addr < 511 and val >=0 and val < pow(2,16):
                outstr = "w,%i,%i\n" % (addr, val&0xff) # LSB
                serial.write(outstr)
                time.sleep(.25)
                outstr = "w,%i,%i\n" % (addr+1, val>>8) # MSB
                serial.write(outstr)
            else:
                print "Error: icorrect value(s)"
        else:
            print "Error: missing parameters"
    except:
        #print "ERROR"
        print sys.exc_info()
        
def write_eeprom_float(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 2:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])
            
            val =  float(items[2])
            
            outstr = "wf,%i,%f\n" % (addr, val)
            print outstr
            serial.write(outstr)
            
        else:
            print "Error: missing parameters"
    except:
        #print "ERROR"
        print sys.exc_info()


def write_eeprom_long(str2parse, serial):
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
                
            for i in range(4):
                outstr="w,%i,%i\n" % (addr+i, val & 0xff)
                val >>= 8
                serial.write(outstr)
                time.sleep(.25)
        else:
            print "Error: missing parameters"
    except:
        print "Error"
        
def read_eeprom(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 1:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < 512:
                outstr = "r,%i\n" % (addr)
                serial.write(outstr)
            else:
                print "Error: icorrect value(s)"
        else:
            print "Error: missing parameters"
    except:
        print "ERROR"
        
def read_eeprom_float(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 1:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < 512:
                outstr = "rf,%i\n" % (addr)
                serial.write(outstr)
            else:
                print "Error: incorrect value(s)"
        else:
            print "Error: missing parameters"
    except:
        #print "ERROR"
        print sys.exc_info()
        
def read_eeprom_int16_t(str2parse, serial):
    try:
        items = str2parse.split(',')
        if len(items) > 1:
            if items[1] in mem.keys():
                addr  = mem[items[1]]
            else:
                addr = int(items[1])
            if addr >= 0 and addr < 512:
                outstr = "ri,%i\n" % (addr)
                serial.write(outstr)
            else:
                print "Error: incorrect value(s)"
        else:
            print "Error: missing parameters"
    except:
        #print "ERROR"
        print sys.exc_info()
'''       
def verify_checksum(eeprom_list):
    cs =0xaa
    for item in eeprom_list:
        cs ^= item
    return cs & 0xff
'''        
def calculate_checksum(eeprom_list):
    cs =0xaa
    for i in range(mem['CHECKSUM']):
        cs ^= int(eeprom_list[i])
    return cs & 0xff


def parse_eeprom_list(eepromlist, i):
    outp=''
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
        #print "unknown parameter"
        pass
    return outp

    
def read_eeeprom_all(str2parse):
    eepromlist= str2parse[2:].split(',')
    print eepromlist
    print len(eepromlist)
    for i in range(len(eepromlist)):
        outp = parse_eeprom_list(eepromlist, i)
        if len(outp)>0:
            print outp
    '''
    if calculate_checksum(eepromlist) ^ int(eepromlist[mem['CHECKSUM']]) == 0:
        print "Checksum ok."
    else:
        print "\nPython calculated Checksum: 0x%x" % calculate_checksum(eepromlist)
    '''

    
def eeprom2file(str2parse, filename):
    eepromlist= str2parse[2:].split(',')
    #file = open("tmp.conf", 'w')
    file = open(filename, 'w')
    for i in range(len(eepromlist)):
        outp = parse_eeprom_list(eepromlist, i)
        if len(outp) > 0: 
            print outp
            file.write(outp+'\n')
    if calculate_checksum(eepromlist) ^ int(eepromlist[mem['CHECKSUM']]) == 0:
        print "Checksum ok."
    else:
        print "\nPython calculated Checksum: 0x%x" % calculate_checksum(eepromlist)
    file.close()
        
def file2eeprom(str2parse, serial):  
    file = open(str2parse[3:],'r')
    content = file.readlines()
    file.close()
    cs =0xaa # calculate checksum from file content, compare with checksum obtained from device after loading.
    for line in content:
        items = line.split('=')
        if len(items) > 1:
            items[0] = items[0].rstrip().lstrip()
            if items[0] in mem.keys():
                addr = mem[items[0]]
                if memtypes[addr] is 'b':
                    if items[1][:2] == '0x':
                        val =  int(items[1],16)
                    else:
                        val =  int(items[1])

                    if addr >= 0 and addr < 512 and val >=0 and val < 256:
                        if addr < mem['CHECKSUM']:
                            cs ^= val
                        outstr = "w,%i,%i\n" % (addr, val)
                        print outstr[:-1]
                        serial.write(outstr)
                    else:
                        print "Error: incorrect value(s)"
 
                elif memtypes[addr] is 'i':
                    if items[1][:2] == '0x':
                        val =  int(items[1],16)
                    else:
                        val =  int(items[1])
                        
                    if addr >= 0 and addr < 511:
                        p = struct.pack('h', val)
                        for c in p:
                            cs ^= ord(c)
                        outstr = "wi,%i,%i\n" % (addr, val)
                        print outstr[:-1]
                        serial.write(outstr)
                    else:
                        print "Error: incorrect value(s)"

                elif memtypes[addr] is 'f':
                    val =  float(items[1])   
                    p= struct.pack('f',val)
                    for c in p:
                        cs ^= ord(c)
                    outstr = "wf,%i,%f\n" % (addr, val)
                    print outstr[:-1]
                    serial.write(outstr)
                else:
                    print "wrong type"                      
                        
                if SerialWaitForResponse(port):
                    read_raw(port)
                else:
                    while not SerialWaitForResponse(port):
                        read_raw(port)
                        print outstr[:-1]
                        serial.write(outstr)
                    read_raw(port)  

    print "calculated checksum: %x" % (cs&0xff)
    print"now update checksum on device:"
    serial.write("s\n")
    if SerialWaitForResponse(port):
        recstr = read_raw(port)
        print "checksum: %s" % recstr
    read_eeprom("r,CHECKSUM", port)
    if SerialWaitForResponse(port):
        gotcs = int(read_raw(port))
        if gotcs == cs:
            print "SUCCESS!! checksum = %X" % (cs)
        else:
            print "FAIL!! checksum on device: %x, should be: %x" %(gotcs, cs)
    
    
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
                print outstr[:-2]
                serial.write(outstr)
                #time.sleep(.25)
                #outstr = "w,%i,%i\n" % (mem['VCCatCAL_LSB'], (vcc & 0xFF) )
                #print outstr[:-2]
                #serial.write(outstr)
                time.sleep(.25)
                serial.write("c\n") # request DUT to measure ADC value and store into eeprom.
            else:
                print "too small VCC! abort calibration."
                return
        else:
            print "Error: missing parameters"
    except:
        print" VCC calibration failed."

def is_timeout(tstart, delayms, isset):
    if isset and ((time.time() -  tstart) > delayms/1000.0):
        return True
    else:
        return False

def OpenSerialPort(port):
    while not port.isOpen():
        try:
            port.open()
            print "ready."
        except:  
            time.sleep(0.1)
            
def SerialWaitForResponse(port):
    done = False
    tstart = time.time()
    while not done:
        if port.inWaiting() > 0:
            done = True
        elif (time.time() - tstart) > 0.5:
            print"SerialWaitForResponse: timeout"
            return False
        time.sleep(0.01)
    return done
            
if __name__ == '__main__':
    argc=len(sys.argv)
    if argc > 1:
        SerialPort = sys.argv[1]
        if argc > 2:
            SerialBaud = sys.argv[2]
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
   
    while 1:
      try:
        if kbhit():
            ch = getch()
            #print ord(ch)
            if ord(ch) is 224:
                keycode = ord(getch())
                if keycode == 72:
                    print "\narrow up"
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
                print
                #sent_commands.append(inputstr)
                # do something with input string, i.e send over serial port
                if inputstr == "quit" or inputstr == "exit" or inputstr == "q":
                    port.close()
                    break
                elif inputstr == "help" or inputstr == "?":
                    print "help:"
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
                
                # measure FEI from a external reference signal and store in EEPROM
                elif inputstr[:2]=='fe':
                    port.write("fe\n")
                    
                # get eeprom content and save in file
                elif inputstr[:2] == 'g,' or inputstr[:4] == 'get,':
                    cmd_sent = inputstr
                    storefile = inputstr.split(',')[1]
                    port.write("a\n")
                
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
                    #print "%i bytes written." % len(xstr)

                # Verify checksum
                elif inputstr == 'cs':
                    port.write("cs\n")                    
                
                elif inputstr ==  'pc' or inputstr == 'close':
                    port.close()
                    print "serial port closed."
                    
                elif inputstr == 'po' or inputstr == 'open':
                    print "open serial port now"
                    OpenSerialPort(port)
                    
                elif inputstr == 'pwd':
                    port.write(PassWord + '\n')

                else:
                    print "Invalid command: %s" % inputstr 
                    # in order to test the echo function of the device, uncomment the following 3 lines.
                    #inputstr += '\r'
                    #num = port.write(inputstr)
                    #print "Number of bytes written to serial port: %i" %(num)
                
                inputstr =""
                #inprompt = "<-- "
                #for c in inprompt:
                    #putch(c)
            else:
                inputstr += ch
        if port.isOpen() and port.inWaiting() > 0:
            isset_timeout = False
            #ts = time.time()
            recstrraw = read_raw(port)           # Achtung, string returned kann \r\n enthalte == 2 Strings!
            recstrlist = recstrraw.split('\r\n')
            prompt = "--> "
            del recstrlist[-1]
            for recstr in recstrlist:
                if len(recstr) >  0: 
                    if recstr =="CAL?":
                        port.write("y")
                        #print "received calibration request and sent 'y'"
                        #print time.time() -ts
                    elif recstr[:2] == "a," and (cmd_sent == 'a' or cmd_sent == 'ls'):
                        #print recstr
                        read_eeeprom_all(recstr)
                    elif recstr[:2] == "a," and (cmd_sent[:2] == 'g,' or cmd_sent[:4] == 'get,'):
                        eeprom2file(recstr, storefile)
                        storefile=''
                    elif recstr[:3] == 'cs,':
                        print "Checksum %s" % recstr[3:]
                    elif recstr[:8] == 'vcc_adc,':
                        print "VCC = %.0f mV" % float(recstr.split(',')[1])
                    else:
                        #print "%s (%i bytes)" % (recstr, len(recstr))
                        print prompt + recstr
                else:
                    print "garbage!"
            #inprompt = "<-- "
            #for c in inprompt:
                #putch(c)
        else:
            if is_timeout(tstart, timeout_delay, isset_timeout):
                isset_timeout = False
                print "timeout!!"
        
        #sys.stdout.write('.')
        time.sleep(.05)
        #print '-'
      
      except serial.serialutil.SerialException:
        print "Serial Port Error"
        inputstr =""
        port.close()
        #OpenSerialPort(port)
    
    print 'done'
