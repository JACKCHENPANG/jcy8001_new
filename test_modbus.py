import serial, time

s = serial.Serial('/dev/cu.usbserial-0001', 115200, timeout=1)

# Modbus 读版本号: 01 04 3E 01 00 01
cmd = bytearray([0x01, 0x04, 0x3E, 0x01, 0x00, 0x01])
crc = 0xFFFF
for b in cmd:
    crc ^= b
    for _ in range(8):
        if crc & 1:
            crc = (crc >> 1) ^ 0xA001
        else:
            crc >>= 1
cmd += bytearray([crc & 0xFF, (crc >> 8) & 0xFF])

print('TX:', cmd.hex())
s.write(cmd)
time.sleep(0.5)
resp = s.read(64)
if resp:
    print('RX:', resp.hex())
else:
    print('RX: NO DATA')

# 也试一下直接发字符看有没有echo
s.write(b'hello\r\n')
time.sleep(0.3)
resp2 = s.read(64)
if resp2:
    print('ECHO:', repr(resp2))
else:
    print('ECHO: NO DATA')

s.close()
