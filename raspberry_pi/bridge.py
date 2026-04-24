import socket
import serial
import threading
import traceback

SERIAL_PORT = "/dev/serial0"
BAUD = 115200

CONTROL_HOST = "0.0.0.0"
CONTROL_PORT = 5000

TELEM_HOST = "0.0.0.0"
TELEM_PORT = 5001

ser = serial.Serial(SERIAL_PORT, BAUD, timeout=0.05)

control_clients = []
telemetry_clients = []

control_lock = threading.Lock()
telemetry_lock = threading.Lock()

def remove_client(client_list, lock, conn):
    with lock:
        if conn in client_list:
            client_list.remove(conn)
    try:
        conn.close()
    except Exception:
        pass

def broadcast(client_list, lock, data):
    dead = []
    with lock:
        for c in client_list:
            try:
                c.sendall(data)
            except Exception:
                dead.append(c)

        for c in dead:
            try:
                client_list.remove(c)
            except Exception:
                pass
            try:
                c.close()
            except Exception:
                pass

def serial_reader():
    while True:
        try:
            data = ser.readline()
            if not data:
                continue

            try:
                text = data.decode(errors="ignore").strip()
            except Exception:
                text = ""

            if text and not text.startswith("ACK:SET"):
                print("STM32 >", text)

            broadcast(telemetry_clients, telemetry_lock, data)

        except Exception as e:
            print("Serial reader error:", e)
            traceback.print_exc()

def handle_control_client(conn, addr):
    print("Control connected from:", addr)
    with control_lock:
        control_clients.append(conn)

    buffer = ""

    try:
        while True:
            data = conn.recv(1024)
            if not data:
                break

            try:
                text = data.decode(errors="ignore")
            except Exception:
                text = ""

            if text:
                buffer += text

                while "\n" in buffer:
                    line, buffer = buffer.split("\n", 1)
                    line = line.strip()
                    if not line:
                        continue

                    ser.write((line + "\n").encode("utf-8"))
                    ser.flush()

    except Exception as e:
        print("Control client error:", addr, e)
    finally:
        remove_client(control_clients, control_lock, conn)
        print("Control disconnected:", addr)

def handle_telemetry_client(conn, addr):
    print("Telemetry connected from:", addr)
    with telemetry_lock:
        telemetry_clients.append(conn)

    try:
        while True:
            data = conn.recv(256)
            if not data:
                break
    except Exception:
        pass
    finally:
        remove_client(telemetry_clients, telemetry_lock, conn)
        print("Telemetry disconnected:", addr)

def control_server_thread():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((CONTROL_HOST, CONTROL_PORT))
    server.listen(5)

    print(f"Control server listening on {CONTROL_HOST}:{CONTROL_PORT}")

    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_control_client, args=(conn, addr), daemon=True).start()

def telemetry_server_thread():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((TELEM_HOST, TELEM_PORT))
    server.listen(5)

    print(f"Telemetry server listening on {TELEM_HOST}:{TELEM_PORT}")

    while True:
        conn, addr = server.accept()
        threading.Thread(target=handle_telemetry_client, args=(conn, addr), daemon=True).start()

if __name__ == "__main__":
    try:
        threading.Thread(target=serial_reader, daemon=True).start()
        threading.Thread(target=control_server_thread, daemon=True).start()
        threading.Thread(target=telemetry_server_thread, daemon=True).start()

        while True:
            threading.Event().wait(3600)

    except KeyboardInterrupt:
        print("\nBridge stopped.")
    finally:
        try:
            ser.close()
        except Exception:
            pass