import serial, time

# 试两个串口
for port in ['/dev/cu.usbserial-0001', '/dev/cu.usbmodem0000635287691']:
    try:
        s = serial.Serial(port, 115200, timeout=2)
        print(f"\n--- {port} ---")
        # 等心跳
        start = time.time()
        while time.time() - start < 2:
            data = s.read(64)
            if data:
                print("RX:", repr(data))
                break
        else:
            print("无数据")
        s.close()
    except Exception as e:
        print(f"{port}: 错误 {e}")
