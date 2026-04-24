import socket

PI_IP = "192.168.50.2"
PI_PORT = 5000

def send_line(sock: socket.socket, text: str) -> None:
    sock.sendall((text + "\n").encode("utf-8"))

def print_help():
    print("\nThruster Test Commands")
    print("----------------------")
    print("arm                     -> ARM")
    print("disarm                  -> DISARM")
    print("stop                    -> TEST:STOP")
    print("manual                  -> MODE:MANUAL")
    print("test <thruster> <pwm>   -> TEST:Tn:value")
    print("                          Example: test 3 1600")
    print("help                    -> show help")
    print("quit                    -> exit\n")

def main():
    print(f"Connecting to {PI_IP}:{PI_PORT}...")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((PI_IP, PI_PORT))
    print("Connected.")

    print_help()

    try:
        while True:
            raw = input("thruster-test> ").strip()
            if not raw:
                continue

            parts = raw.split()
            cmd = parts[0].lower()

            if cmd == "quit" or cmd == "exit":
                break

            if cmd == "help":
                print_help()
                continue

            if cmd == "arm":
                send_line(sock, "ARM")
                print("Sent: ARM")
                continue

            if cmd == "disarm":
                send_line(sock, "DISARM")
                print("Sent: DISARM")
                continue

            if cmd == "stop":
                send_line(sock, "TEST:STOP")
                print("Sent: TEST:STOP")
                continue

            if cmd == "manual":
                send_line(sock, "MODE:MANUAL")
                print("Sent: MODE:MANUAL")
                continue

            if cmd == "test":
                if len(parts) != 3:
                    print("Usage: test <thruster 1-6> <pwm 1000-2000>")
                    continue

                try:
                    thruster = int(parts[1])
                    pwm = int(parts[2])
                except ValueError:
                    print("Thruster and PWM must be integers.")
                    continue

                if thruster < 1 or thruster > 6:
                    print("Thruster must be between 1 and 6.")
                    continue

                if pwm < 1000 or pwm > 2000:
                    print("PWM must be between 1000 and 2000.")
                    continue

                line = f"TEST:T{thruster}:{pwm}"
                send_line(sock, line)
                print(f"Sent: {line}")
                continue

            print("Unknown command. Type 'help' for options.")

    except KeyboardInterrupt:
        print("\nExiting.")
    finally:
        try:
            sock.close()
        except Exception:
            pass

if __name__ == "__main__":
    main()