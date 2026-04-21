import socket
import traceback

PI_IP = "192.168.50.2"
PI_PORT = 5001

PWM_MIN = 1000
PWM_NEUTRAL = 1500
PWM_MAX = 2000

latest = {
    "ARM": "?",
    "T1": "1500", "O1": "1500",
    "T2": "1500", "O2": "1500",
    "T3": "1500", "O3": "1500",
    "T4": "1500", "O4": "1500",
    "T5": "1500", "O5": "1500",
    "T6": "1500", "O6": "1500",
    "IMU": "?",
    "AXmg": "?",
    "AYmg": "?",
    "AZmg": "?",
    "GXdps": "?",
    "GYdps": "?",
    "GZdps": "?",
}

def parse_line(line):
    parts = line.strip().split()
    for p in parts:
        if ":" in p:
            k, v = p.split(":", 1)
            if k in latest:
                latest[k] = v

def to_int_or_none(v):
    try:
        return int(float(v))
    except Exception:
        return None

def fmt_pwm(v):
    x = to_int_or_none(v)
    if x is None:
        return "?"
    return str(x)

def fmt_pct(v):
    x = to_int_or_none(v)
    if x is None:
        return "?"
    span = PWM_MAX - PWM_MIN
    pct = ((x - PWM_MIN) / span) * 100.0
    pct = max(0.0, min(100.0, pct))
    return f"{pct:5.1f}%"

def fmt_int(v):
    x = to_int_or_none(v)
    if x is None:
        return "?"
    return str(x)

def print_dashboard():
    print("\033[2J\033[H", end="")
    print("DOLPHIN 6-THRUSTER TELEMETRY + MINIMAL IMU")
    print("=" * 92)
    print(f"ARM STATE : {latest['ARM']}    IMU OK : {latest['IMU']}")
    print("-" * 92)
    print("THRUSTER COMMAND / OUTPUT")
    print(f"T1 VERT_FRONT_LEFT   CMD:{fmt_pwm(latest['T1']):>5}  OUT:{fmt_pwm(latest['O1']):>5}  {fmt_pct(latest['O1'])}")
    print(f"T2 VERT_FRONT_RIGHT  CMD:{fmt_pwm(latest['T2']):>5}  OUT:{fmt_pwm(latest['O2']):>5}  {fmt_pct(latest['O2'])}")
    print(f"T3 HORIZ_LEFT        CMD:{fmt_pwm(latest['T3']):>5}  OUT:{fmt_pwm(latest['O3']):>5}  {fmt_pct(latest['O3'])}")
    print(f"T4 HORIZ_RIGHT       CMD:{fmt_pwm(latest['T4']):>5}  OUT:{fmt_pwm(latest['O4']):>5}  {fmt_pct(latest['O4'])}")
    print(f"T5 VERT_REAR_LEFT    CMD:{fmt_pwm(latest['T5']):>5}  OUT:{fmt_pwm(latest['O5']):>5}  {fmt_pct(latest['O5'])}")
    print(f"T6 VERT_REAR_RIGHT   CMD:{fmt_pwm(latest['T6']):>5}  OUT:{fmt_pwm(latest['O6']):>5}  {fmt_pct(latest['O6'])}")
    print("-" * 92)
    print("MINIMAL IMU")
    print(f"ACCEL  AX:{fmt_int(latest['AXmg']):>6} mg   AY:{fmt_int(latest['AYmg']):>6} mg   AZ:{fmt_int(latest['AZmg']):>6} mg")
    print(f"GYRO   GX:{fmt_int(latest['GXdps']):>6} dps  GY:{fmt_int(latest['GYdps']):>6} dps  GZ:{fmt_int(latest['GZdps']):>6} dps")
    print("=" * 92)

def main():
    print("Connecting to Pi bridge...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((PI_IP, PI_PORT))
    print(f"Connected to {PI_IP}:{PI_PORT}")

    buffer = ""

    while True:
        data = sock.recv(1024)
        if not data:
            print("Connection closed.")
            break

        buffer += data.decode(errors="ignore")

        while "\n" in buffer:
            line, buffer = buffer.split("\n", 1)
            parse_line(line)
            print_dashboard()

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("Telemetry viewer crashed:")
        print(type(e).__name__, e)
        traceback.print_exc()
        input("Press Enter to close...")
