import socket
import time
import pygame
import traceback

PI_IP = "192.168.50.2"
PI_PORT = 5000

SEND_INTERVAL = 0.10
BUTTON_COOLDOWN = 0.30
ARM_SETTLE_TIME = 1.0

PWM_MIN = 1000
PWM_NEUTRAL = 1500
PWM_MAX = 2000

SURGE_RANGE = 300
YAW_RANGE = 180
HEAVE_RANGE = 260

DEADZONE = 0.08
RAMP_STEP = 10

AXIS_SURGE = 1
AXIS_YAW = 0
AXIS_HEAVE = 3

BUTTON_ARM = 0
BUTTON_DISARM = 1
BUTTON_STOP = 2

thrusters = {
    "T1": PWM_NEUTRAL,
    "T2": PWM_NEUTRAL,
    "T3": PWM_NEUTRAL,
    "T4": PWM_NEUTRAL,
    "T5": PWM_NEUTRAL,
    "T6": PWM_NEUTRAL,
}

def clamp(value, low, high):
    return max(low, min(high, value))

def apply_deadzone(value, deadzone):
    if abs(value) < deadzone:
        return 0.0
    return value

def ramp(current, target, step):
    if current < target:
        return min(current + step, target)
    if current > target:
        return max(current - step, target)
    return current

def send_line(sock, text):
    sock.sendall((text + "\n").encode("utf-8"))

def send_set(sock, values):
    line = f"SET:{values['T1']},{values['T2']},{values['T3']},{values['T4']},{values['T5']},{values['T6']}"
    send_line(sock, line)

def main():
    print("RUNNING DOLPHIN SET-COMMAND CONTROLLER")
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.connect((PI_IP, PI_PORT))
    print(f"Connected to {PI_IP}:{PI_PORT}")

    pygame.init()
    pygame.joystick.init()

    if pygame.joystick.get_count() == 0:
        print("No controller detected.")
        input("Press Enter to close...")
        return

    js = pygame.joystick.Joystick(0)
    js.init()
    print("Controller detected:", js.get_name())

    armed = False
    arm_time = 0.0
    last_button_time = {"arm": 0.0, "disarm": 0.0, "stop": 0.0}

    while True:
        pygame.event.pump()
        now = time.time()

        surge = apply_deadzone(-js.get_axis(AXIS_SURGE), DEADZONE)
        yaw = apply_deadzone(js.get_axis(AXIS_YAW), DEADZONE)
        heave = apply_deadzone(-js.get_axis(AXIS_HEAVE), DEADZONE)

        if js.get_button(BUTTON_ARM) and (now - last_button_time["arm"] > BUTTON_COOLDOWN):
            for k in thrusters:
                thrusters[k] = PWM_NEUTRAL
            send_line(sock, "ARM")
            armed = True
            arm_time = now
            last_button_time["arm"] = now
            print("Sent: ARM")

        if js.get_button(BUTTON_DISARM) and (now - last_button_time["disarm"] > BUTTON_COOLDOWN):
            for k in thrusters:
                thrusters[k] = PWM_NEUTRAL
            send_line(sock, "DISARM")
            armed = False
            last_button_time["disarm"] = now
            print("Sent: DISARM")

        if js.get_button(BUTTON_STOP) and (now - last_button_time["stop"] > BUTTON_COOLDOWN):
            for k in thrusters:
                thrusters[k] = PWM_NEUTRAL
            send_line(sock, "STOP")
            last_button_time["stop"] = now
            print("Sent: STOP")

        horiz_left_target = clamp(
            PWM_NEUTRAL + int(surge * SURGE_RANGE) + int(yaw * YAW_RANGE),
            PWM_MIN, PWM_MAX
        )

        horiz_right_target = clamp(
            PWM_NEUTRAL + int(surge * SURGE_RANGE) - int(yaw * YAW_RANGE),
            PWM_MIN, PWM_MAX
        )

        vert_target = clamp(
            PWM_NEUTRAL + int(heave * HEAVE_RANGE),
            PWM_MIN, PWM_MAX
        )

        target = {
            "T1": vert_target,
            "T2": vert_target,
            "T3": horiz_left_target,
            "T4": horiz_right_target,
            "T5": vert_target,
            "T6": vert_target,
        }

        for k in thrusters:
            thrusters[k] = ramp(thrusters[k], target[k], RAMP_STEP)

        if armed:
            if (now - arm_time) < ARM_SETTLE_TIME:
                neutral_frame = {k: PWM_NEUTRAL for k in thrusters}
                send_set(sock, neutral_frame)
            else:
                send_set(sock, thrusters)

        time.sleep(SEND_INTERVAL)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        print("\nSCRIPT CRASHED:")
        print(type(e).__name__, e)
        traceback.print_exc()
        input("\nPress Enter to close...")
