import usb.core
import usb.util
import signal
import sys
import traceback
import time
import os

dev = None
sleep_timer = 0.01
sram_transfer_val = 2
rom_transfer_val = 1
rom_bank_size = 0x40
rom_banks = [2,4,8,16,32,64,128,256,512,0,0,0,0,0,0,0,0,0,72,80,96]
sram_banks_sizes = [0,8,0x20,0x20,0x20,0x20,2]
sram_banks = [0,1,1,4,16,8,1]
section_size = 0x100
normal_nybble = 0x10
check_nybble = 0x40

def rom_transfer(data, transfer_size):
    print("Starting ROM dump...")
    size = rom_bank_size
    banks = rom_banks[transfer_size]
    
    return transfer_bank(data, size, banks)
    
def sram_transfer(data, transfer_size):
    print("Starting SRAM dump...")
    size = sram_banks_sizes[transfer_size]
    banks = sram_banks[transfer_size]
        
    return transfer_bank(data, size, banks)

def transfer_bank(data, size, banks):
    if(banks == 0):
        print("Nothing to dump!")
        return None
        
    res = []
    for i in range(banks):
        print("Bank " + str(i+ 1) + " out of " + str(banks) + "!")
        for j in range(size):
            print("Section " + str(j+ 1) + " out of " + str(size) + "!")
            res += read_section(data)
    return res

def read_section(data):
    buf = []
    checked = False
    half = False
    
    while(not checked):
        for i in range(2):
            accepted = False
            while not accepted:
                sleep_func()
                sendByte(data[i])
                recv = receiveByte()
                high_recv = recv & 0xF0
                if(high_recv == check_nybble or high_recv == normal_nybble):
                    #print("send: 0x%02x" % data[i])
                    #print("recv: 0x%02x" % recv)
                    if(high_recv == check_nybble):
                        if(half):
                            checked = True
                        half = True
                    accepted = True
                    data[i] = recv & 0xF;
                #else:
                    #print("BAD RECV: 0x%02x" % recv)
        val = (data[1] | (data[0] << 4))
        if(checked):
            if(val == 0):
                checked = False
                buf = []
        else:
            buf += [val]
            #print("Data: 0x%02x" % val)
            if(half):
                # Handle a "nybble" desync
                sleep_func()
                sendByte(data[0])
                recv = receiveByte()
        half = False
        
    return buf[0:len(buf)-1] # There is an extra transfer in order to ensure the last byte is correct. Discard it

def transfer_func():
    print("Waiting for the transfer to start...")
    
    data = [0,0]
    buf = read_section(data) # Read the starting information
    
    if(len(buf) > 2):
        print("The transfer was previously interrupted. Please reset the GameBoy!")
        return
        
    transfer_type = buf[0]
    transfer_size = buf[1]
    if(transfer_type == rom_transfer_val):
        res = rom_transfer(data, transfer_size)
    elif(transfer_type == sram_transfer_val):
        res = sram_transfer(data, transfer_size)
    
    if res is not None:
        if(len(sys.argv) > 1):
            target = sys.argv[1]
        else:
            target = input("Please enter where to save the dump: ")
        
        newFile = open(target, "wb")
        newFile.write(bytearray(res))
        newFile.close()
    return
    
# Function needed in order to make sure there is enough time for the slave to prepare the next byte.
def sleep_func():
    time.sleep(sleep_timer)

# Code dependant on this connection method
def sendByte(byte_to_send):
    epOut.write(byte_to_send.to_bytes(1, byteorder='big'))
    return

def receiveByte():
    return int.from_bytes(epIn.read(epIn.wMaxPacketSize, 100), byteorder='big')

# Things for the USB connection part
def exit_gracefully():
    if dev is not None:
        usb.util.dispose_resources(dev)
        if(os.name != "nt"):
            if reattach:
                dev.attach_kernel_driver(0)
    print('Done.')

def signal_handler(sig, frame):
    print('You pressed Ctrl+C!')
    exit_gracefully()

signal.signal(signal.SIGINT, signal_handler)

# The execution path
try:
    devices = list(usb.core.find(find_all=True,idVendor=0xcafe, idProduct=0x4011))
    for d in devices:
        #print('Device: %s' % d.product)
        dev = d

    if dev is None:
        raise ValueError('Device not found')

    reattach = False
    if(os.name != "nt"):
        if dev.is_kernel_driver_active(0):
            try:
                reattach = True
                dev.detach_kernel_driver(0)
                print("kernel driver detached")
            except usb.core.USBError as e:
                sys.exit("Could not detach kernel driver: %s" % str(e))
        else:
            print("no kernel driver attached")

    dev.reset()

    dev.set_configuration()

    cfg = dev.get_active_configuration()

    #print('Configuration: %s' % cfg)

    intf = cfg[(2,0)]   # Or find interface with class 0xff

    #print('Interface: %s' % intf)

    epIn = usb.util.find_descriptor(
        intf,
        custom_match = \
        lambda e: \
            usb.util.endpoint_direction(e.bEndpointAddress) == \
            usb.util.ENDPOINT_IN)

    assert epIn is not None

    #print('EP In: %s' % epIn)

    epOut = usb.util.find_descriptor(
        intf,
        # match the first OUT endpoint
        custom_match = \
        lambda e: \
            usb.util.endpoint_direction(e.bEndpointAddress) == \
            usb.util.ENDPOINT_OUT)

    assert epOut is not None

    #print('EP Out: %s' % epOut)

    # Control transfer to enable webserial on device
    #print("control transfer out...")
    dev.ctrl_transfer(bmRequestType = 1, bRequest = 0x22, wIndex = 2, wValue = 0x01)

    transfer_func()
    
    exit_gracefully()
except:
    #traceback.print_exc()
    print("Unexpected exception: ", sys.exc_info()[0])
    exit_gracefully()