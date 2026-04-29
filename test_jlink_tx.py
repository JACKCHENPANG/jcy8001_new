import serial, time, subprocess

# 先让JLink手动往USART2_DR写数据
subprocess.run(['JLinkExe', '-device', 'STM32F103RC', '-if', 'swd', '-speed', '4000',
    '-CommanderScript', '/Users/jcy/Projects/JCY8001_new/test_tx.jlink'],
    capture_output=True, timeout=10)

# 然后读串口
s = serial.Serial('/dev/cu.usbserial-0001', 115200, timeout=2)
start = time.time()
while time.time() - start < 3:
    d = s.read(64)
    if d:
        print('收到:', repr(d))
        break
else:
    print('串口无数据')
s.close()
