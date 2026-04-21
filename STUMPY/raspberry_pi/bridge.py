import socket
import serial
import threading

SERIAL_PORT = "/dev/serial0"
BAUD = 115200

CONTROL_HOST = "0.0.0.0"
CONTROL_PORT = 5000

TELEM_HOST = "0.0.0.0"
TELEM_PORT = 5001

ser = serial.Serial(SERIAL_PORT, BAUD, timeout=0.05)

telemetry_clients = []
telemetry_lock = threading.Lock()
control_client = None
control_lock = threading.Lock()

control_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
control_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
control_server.bind((CONTROL_HOST, CONTROL_PORT))
control_server.listen(1)

telem_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
telem_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
telem_server.bind((TELEM_HOST, TELEM_PORT))
telem_server.listen(5)

print(f"Control server listening on {CONTROL_HOST}:{CONTROL_PORT}")
print(f"Telemetry server listening on {TELEM_HOST}:{TELEM_PORT}")

def broadcast_telemetry(data):
    dead = []
    with telemetry_lock:
        for c in telemetry_clients:
            try:
                c.sendall(data)
            except Exception:
                dead.append(c)

        for c in dead:
            try:
                telemetry_clients.remove(c)
                c.close()
            except Exception:
                pass

def serial_reader():
    while True:
        try:
            data = ser.readline()
            if data:
                try:
                    print("STM32 >", data.decode(errors="ignore").strip())
                except Exception:
                    pass
                broadcast_telemetry(data)
        except Exception as e:
            print("Serial reader error:", e)

def handle_control(conn, addr):
    global control_client
    print("Control connected from:", addr)

    with control_lock:
        if control_client is not None:
            try:
                control_client.close()
            except Exception:
                pass
        control_client = conn

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            try:
                print("CTRL >", data.decode(errors="ignore").replace("\n", "\\n"))
            except Exception:
                pass

            ser.write(data)
            ser.flush()
    except Exception as e:
        print("Control client error:", addr, e)
    finally:
        with control_lock:
            if control_client is conn:
                control_client = None
        try:
            conn.close()
        except Exception:
            pass
        print("Control disconnected:", addr)

def handle_telemetry(conn, addr):
    print("Telemetry connected from:", addr)

    with telemetry_lock:
        telemetry_clients.append(conn)

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break
            # telemetry clients should not send anything
    except Exception as e:
        print("Telemetry client error:", addr, e)
    finally:
        with telemetry_lock:
            if conn in telemetry_clients:
                telemetry_clients.remove(conn)
        try:
            conn.close()
        except Exception:
            pass
        print("Telemetry disconnected:", addr)

def control_accept_loop():
    while True:
        conn, addr = control_server.accept()
        threading.Thread(target=handle_control, args=(conn, addr), daemon=True).start()

def telemetry_accept_loop():
    while True:
        conn, addr = telem_server.accept()
        threading.Thread(target=handle_telemetry, args=(conn, addr), daemon=True).start()

threading.Thread(target=serial_reader, daemon=True).start()
threading.Thread(target=control_accept_loop, daemon=True).start()
threading.Thread(target=telemetry_accept_loop, daemon=True).start()

while True:
    threading.Event().wait(3600)