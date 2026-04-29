#!/usr/bin/env python3
import serial

def crc16(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            crc = (crc >> 1) ^ 0xA001 if crc & 1 else crc >> 1
    return bytes([crc & 0xFF, (crc >> 8) & 0xFF])

def read_reg(port, addr, count=1):
    cmd = bytes([0x01, 0x04, (addr>>8)&0xFF, addr&0xFF, (count>>8)&0xFF, count&0xFF])
    s.write(cmd + crc16(cmd))
    resp = s.read(64)
    return resp.hex() if resp else 'NO DATA'

s = serial.Serial('/dev/cu.usbserial-0001', 115200, timeout=1)

tests = [
    (0x3E00, 1, '通道数'),
    (0x3E01, 1, '版本号'),
    (0x3340, 1, '电压Ch0'),
    (0x3300, 1, '温度Ch0'),
    (0x3100, 1, 'ZREAL Ch0'),
    (0x3140, 1, 'ZIMAG Ch0'),
    (0x4000, 1, 'ZM频率'),
]

for addr, cnt, name in tests:
    r = read_reg(s, addr, cnt)
    print(f'{name} (0x{addr:04X}): {r}')

s.close()
