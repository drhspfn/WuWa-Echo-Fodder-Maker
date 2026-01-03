import ctypes
import threading
import time
import tkinter as tk
from tkinter import messagebox
import winsound

user32 = ctypes.windll.user32

# ================= Configuration =================

BASE_W = 1920
BASE_H = 1080

# ================= WinAPI =================

INPUT_MOUSE = 0
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_ABSOLUTE = 0x8000
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004

VK_F8 = 0x77

class MOUSEINPUT(ctypes.Structure):
    _fields_ = [
        ("dx", ctypes.c_long),
        ("dy", ctypes.c_long),
        ("mouseData", ctypes.c_ulong),
        ("dwFlags", ctypes.c_ulong),
        ("time", ctypes.c_ulong),
        ("dwExtraInfo", ctypes.POINTER(ctypes.c_ulong)),
    ]

class INPUT(ctypes.Structure):
    _fields_ = [
        ("type", ctypes.c_ulong),
        ("mi", MOUSEINPUT),
    ]

def click_scaled(base_x: int, base_y: int):
    screen_w: int = user32.GetSystemMetrics(0)
    screen_h: int = user32.GetSystemMetrics(1)

    scale_x: float = screen_w / BASE_W
    scale_y: float = screen_h / BASE_H

    target_x = base_x * scale_x
    target_y = base_y * scale_y

    abs_x = int(target_x * 65535 / screen_w)
    abs_y = int(target_y * 65535 / screen_h)

    move = INPUT(
        type=INPUT_MOUSE,
        mi=MOUSEINPUT(abs_x, abs_y, 0,
                      MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE, 0, None)
    )

    down = INPUT(
        type=INPUT_MOUSE,
        mi=MOUSEINPUT(0, 0, 0, MOUSEEVENTF_LEFTDOWN, 0, None)
    )

    up = INPUT(
        type=INPUT_MOUSE,
        mi=MOUSEINPUT(0, 0, 0, MOUSEEVENTF_LEFTUP, 0, None)
    )

    user32.SendInput(1, ctypes.byref(move), ctypes.sizeof(move))
    time.sleep(0.02)
    user32.SendInput(1, ctypes.byref(down), ctypes.sizeof(down))
    time.sleep(0.01)
    user32.SendInput(1, ctypes.byref(up), ctypes.sizeof(up))

# ================= Pattern (Based on 1920x1080) =================

PATTERN = [
    ((288, 201), 1.0),   # Select Echo slot
    ((1710, 985), 2.0),  # Auto Select button
    ((522, 760), 0.8),   # Confirm Selection
    ((439, 991), 2.0),   # Upgrade Button
    ((439, 991), 0.5),   # Click again (skip animation/confirm)
    ((1814, 61), 1.0),   # Close/Back button
]

running = False

# ================= Automation =================

def automation_loop():
    global running

    winsound.Beep(1200, 150) # Start beep

    # Countdown
    for i in range(5, 0, -1):
        if not running: return
        winsound.Beep(800, 100)
        time.sleep(1)

    winsound.Beep(1500, 200) # Go!
    winsound.Beep(1500, 200)

    while running:
        for (x, y), pause in PATTERN:
            if not running: return
            click_scaled(x, y)
            time.sleep(pause)

# ================= Hotkey Listener =================

def hotkey_listener():
    global running
    while True:
        if user32.GetAsyncKeyState(VK_F8) & 1:
            running = False
            winsound.Beep(400, 300)
        time.sleep(0.05)

# ================= GUI =================

def start():
    global running
    if running: return
    running = True
    threading.Thread(target=automation_loop, daemon=True).start()

def stop():
    global running
    running = False
    winsound.Beep(400, 300)

def show_disclaimer():
    _ = messagebox.showinfo(
        "Instructions",
        "1. Open Item Upgrade screen in game.\n"
        "2. Settings: Level Cap, No Tuning Sync.\n"
        "3. Game must be in FULLSCREEN.\n\n"
        "Resolutions supported: 1080p, 1440p, 4K (16:9 aspect ratio).\n\n"
        "F8 — STOP"
    )

def main():
    threading.Thread(target=hotkey_listener, daemon=True).start()

    root = tk.Tk()
    root.title("EchoFlux - Auto Fodder")
    root.geometry("420x260")
    root.resizable(False, False)

    tk.Label(root, text="EchoFlux Automation", font=("Segoe UI", 14, "bold")).pack(pady=10)
    tk.Label(root, text="Supports: 1080p, 2K, 4K (Fullscreen)\nAspect Ratio 16:9 recommended", justify="center").pack(pady=5)

    tk.Button(root, text="Instructions", command=show_disclaimer, width=28).pack(pady=5)
    tk.Button(root, text="START (5s delay)", command=start, width=28, bg="#dddddd").pack(pady=5)
    tk.Button(root, text="STOP (F8)", command=stop, width=28, fg="red").pack(pady=5)

    root.mainloop()

if __name__ == "__main__":
    main()