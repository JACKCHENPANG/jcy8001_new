import serial, time

s = serial.Serial('/dev/cu.usbserial-0001', 115200, timeout=3)

# 先看看能不能收到心跳 "tick"
print("等待心跳...")
start = time.time()
while time.time() - start < 3:
    data = s.read(64)
    if data:
        print("RX:", repr(data))

# 发Modbus
cmd = bytearray([0x01, 0x04, 0x3E, 0x01, 0x00, 0x01])
crc = 0xFFFF
for b in cmd:
    crc ^= b
    for _ in range(8):
        if crc & 1: crc = (crc >> 1) ^ 0xA001
        else: crc >>= 1
cmd += bytearray([crc & 0xFF, (crc >> 8) & 0xFF])

print("TX:", cmd.hex())
s.write(cmd)
time.sleep(1)
resp = s.read(64)
print("Modbus RX:", resp.hex() if resp else "NO DATA")

s.close()
