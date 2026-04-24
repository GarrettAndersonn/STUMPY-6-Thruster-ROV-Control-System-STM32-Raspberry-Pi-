import socket
import traceback
import time
import csv
from pathlib import Path
from datetime import datetime, timedelta

PI_IP = "192.168.50.2"
PI_PORT = 5001

DRAW_INTERVAL = 0.20
LOG_RETENTION_DAYS = 7

CSV_FIELDS = [
    "local_time",
    "SEQ", "ARM", "MODE", "TEST", "TPWM",
    "T1", "T2", "T3", "T4", "T5", "T6",
    "O1", "O2", "O3", "O4", "O5", "O6",
    "IMU", "WHO",
    "AX", "AY", "AZ",
    "GX", "GY", "GZ",
    "TP", "R", "P",
    "BX", "BY", "BZ"
]

def is_real_telemetry_line(line: str) -> bool:
    return ("SEQ:" in line) and ("ARM:" in line) and ("MODE:" in line)

def parse_tokens(line: str):
    data = {}
    for token in line.strip().split():
        if ":" in token:
            key, value = token.split(":", 1)
            data[key] = value
    return data

def get(data, key, default="?"):
    return data.get(key, default)

def cleanup_old_logs(logs_root: Path):
    now = datetime.now()

    if not logs_root.exists():
        return

    for folder in logs_root.iterdir():
        if not folder.is_dir():
            continue

        try:
            folder_date = datetime.strptime(folder.name, "%Y-%m-%d")
        except ValueError:
            continue

        if now - folder_date > timedelta(days=LOG_RETENTION_DAYS):
            print(f"Deleting old log folder: {folder}")
            for file in folder.glob("*"):
                file.unlink(missing_ok=True)
            folder.rmdir()

def open_log_file():
    script_dir = Path(__file__).resolve().parent
    logs_root = script_dir / "logs"

    cleanup_old_logs(logs_root)

    date_folder = datetime.now().strftime("%Y-%m-%d")
    logs_dir = logs_root / date_folder
    logs_dir.mkdir(parents=True, exist_ok=True)

    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = logs_dir / f"telemetry_log_{timestamp}.csv"

    f = open(filename, "w", newline="", encoding="utf-8")
    writer = csv.DictWriter(f, fieldnames=CSV_FIELDS)
    writer.writeheader()

    print(f"Logging telemetry to: {filename}")
    return f, writer

def log_row(writer, data):
    row = {"local_time": datetime.now().isoformat(timespec="seconds")}
    for key in CSV_FIELDS:
        if key == "local_time":
            continue
        row[key] = data.get(key, "")
    writer.writerow(row)

def main():
    print("Connecting to Pi telemetry bridge...")

    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((PI_IP, PI_PORT))
    print(f"Connected to {PI_IP}:{PI_PORT}")

    log_file, csv_writer = open_log_file()

    buffer = ""
    last_draw = 0.0
    latest_data = {}

    try:
        while True:
            chunk = sock.recv(4096).decode("utf-8", errors="ignore")
            if not chunk:
                print("Connection closed.")
                break

            buffer += chunk

            while "\n" in buffer:
                line, buffer = buffer.split("\n", 1)
                line = line.strip()
                if not line:
                    continue

                # Ignore ACK / boot / calibration messages for display/logging
                if not is_real_telemetry_line(line):
                    continue

                latest_data = parse_tokens(line)
                log_row(csv_writer, latest_data)
                log_file.flush()

            now = time.time()
            if latest_data and (now - last_draw) >= DRAW_INTERVAL:
                last_draw = now
                data = latest_data

                print("\033[2J\033[H", end="")
                print("STUMPY TELEMETRY + MANUAL/TEST/ASSIST")
                print("=" * 108)
                print(
                    f"SEQ : {get(data, 'SEQ')}    "
                    f"ARM : {get(data, 'ARM')}    "
                    f"MODE : {get(data, 'MODE')}    "
                    f"TEST : {get(data, 'TEST')} @ {get(data, 'TPWM')}"
                )
                print(
                    f"IMU OK : {get(data, 'IMU')}    "
                    f"WHO : {get(data, 'WHO')}"
                )
                print("-" * 108)
                print("THRUSTER COMMAND / OUTPUT")
                print(f"T1 CMD:{get(data,'T1')} OUT:{get(data,'O1')}")
                print(f"T2 CMD:{get(data,'T2')} OUT:{get(data,'O2')}")
                print(f"T3 CMD:{get(data,'T3')} OUT:{get(data,'O3')}")
                print(f"T4 CMD:{get(data,'T4')} OUT:{get(data,'O4')}")
                print(f"T5 CMD:{get(data,'T5')} OUT:{get(data,'O5')}")
                print(f"T6 CMD:{get(data,'T6')} OUT:{get(data,'O6')}")
                print("-" * 108)
                print("CALIBRATED + FUSED IMU")
                print(f"ACCEL   AX:{get(data,'AX')} AY:{get(data,'AY')} AZ:{get(data,'AZ')}")
                print(f"GYRO    GX:{get(data,'GX')} GY:{get(data,'GY')} GZ:{get(data,'GZ')}")
                print(f"TEMP    {get(data,'TP')} C")
                print(f"ANGLE   ROLL:{get(data,'R')}   PITCH:{get(data,'P')}")
                print(f"BIAS    BX:{get(data,'BX')} BY:{get(data,'BY')} BZ:{get(data,'BZ')}")
                print("=" * 108)

    except KeyboardInterrupt:
        print("\nTelemetry stopped by user.")

    finally:
        try:
            sock.close()
        except Exception:
            pass
        try:
            log_file.close()
        except Exception:
            pass

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("Telemetry viewer crashed:")
        print(type(e).__name__, e)
        traceback.print_exc()
        input("Press Enter to close...")