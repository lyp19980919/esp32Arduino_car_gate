import serial
import keyboard
import time

# ====== 修改为你的网关 ESP32 串口号 ======
SERIAL_PORT = 'COM8'   # Windows 示例，Linux/Mac 用 '/dev/ttyUSB0' 或 '/dev/ttyACM0'
BAUD_RATE = 115200

# 当前按下的键
current_key = None
last_sent = 0

# 按键映射：键盘键 → 小车指令
KEY_MAP = {
    'w': 'F',   # 前进
    's': 'B',   # 后退
    'a': 'L',   # 左转
    'd': 'R',   # 右转
}

def main():
    global current_key, last_sent

    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
        time.sleep(2)  # 等 ESP32 复位稳定
        print(f"已连接 {SERIAL_PORT}，使用 W/A/S/D 控制小车，ESC 退出")
    except Exception as e:
        print(f"串口打开失败: {e}")
        return

    while True:
        if keyboard.is_pressed('esc'):
            print("退出")
            break

        # 检测当前按下了哪个方向键
        pressed = None
        for key, cmd in KEY_MAP.items():
            if keyboard.is_pressed(key):
                pressed = cmd
                break

        # 每 100ms 发送一次当前状态（与网关的 SEND_INTERVAL_MS 对齐）
        now = time.time()
        if now - last_sent >= 0.1:
            last_sent = now

            if pressed:
                # 按下方向键：发送对应指令，速度 200
                msg = f"{pressed}20\n"
            else:
                # 没有按下任何方向键：发送停止
                msg = "S\n"

            ser.write(msg.encode())
            # 可选：打印调试
            # print(f"发送: {msg.strip()}")

    ser.close()

if __name__ == '__main__':
    main()