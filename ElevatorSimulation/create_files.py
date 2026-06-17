# -*- coding: utf-8 -*-
import os

# 确保当前目录是脚本所在目录
os.chdir(os.path.dirname(os.path.abspath(__file__)))

files_content = {
    "elevator.py": '''class Elevator:
    def __init__(self, id, capacity=15, floor_count=16):
        self.id = id
        self.capacity = capacity
        self.current_floor = 1
        self.direction = 'stop'   # 'up', 'down', 'stop'
        self.door_open = False
        self.passengers = []
        self.target_floors = []
    def get_load(self):
        return len(self.passengers)
''',

    "building.py": '''from elevator import Elevator

class Building:
    def __init__(self, floor_count, elevator_count, elevator_capacity=15):
        self.floor_count = floor_count
        self.elevators = [Elevator(i, elevator_capacity, floor_count) for i in range(elevator_count)]
        # 外呼请求简单存储
        self.outside_calls = {}
        for floor in range(1, floor_count+1):
            if floor < floor_count:
                self.outside_calls[(floor, 'up')] = False
            if floor > 1:
                self.outside_calls[(floor, 'down')] = False
    def add_outside_call(self, floor, direction):
        self.outside_calls[(floor, direction)] = True
    def clear_outside_call(self, floor, direction):
        self.outside_calls[(floor, direction)] = False
''',

    "scheduler.py": '''import math

class Scheduler:
    @staticmethod
    def estimate_arrival_time(elevator, call_floor, call_dir):
        if elevator.direction == 'stop':
            return abs(elevator.current_floor - call_floor) * 2.0
        # 简化预测：同向且前方直接计算距离，否则增加折返惩罚
        if elevator.direction == call_dir:
            if call_dir == 'up' and call_floor >= elevator.current_floor:
                return (call_floor - elevator.current_floor) * 2.0
            elif call_dir == 'down' and call_floor <= elevator.current_floor:
                return (elevator.current_floor - call_floor) * 2.0
        # 反向或需要折返
        if elevator.direction == 'up':
            farthest = max(elevator.target_floors) if elevator.target_floors else elevator.current_floor
            return (farthest - elevator.current_floor) * 2 + (farthest - call_floor) * 2
        else:
            farthest = min(elevator.target_floors) if elevator.target_floors else elevator.current_floor
            return (elevator.current_floor - farthest) * 2 + (call_floor - farthest) * 2

    @staticmethod
    def allocate(building, floor, direction):
        best_elevator = None
        best_time = float('inf')
        for e in building.elevators:
            if e.get_load() >= e.capacity:
                continue
            t = Scheduler.estimate_arrival_time(e, floor, direction)
            if t < best_time:
                best_time = t
                best_elevator = e
        if best_elevator:
            best_elevator.target_floors.append(floor)
            best_elevator.target_floors.sort()
            building.clear_outside_call(floor, direction)
            return best_elevator
        return None
''',

    "passenger.py": '''class Passenger:
    def __init__(self, create_time, start_floor, target_floor, count=1):
        self.create_time = create_time
        self.start_floor = start_floor
        self.target_floor = target_floor
        self.count = count
        self.board_time = None
        self.alight_time = None
    def waiting_time(self):
        if self.board_time:
            return self.board_time - self.create_time
        return None
''',

    "statistics.py": '''class Statistics:
    def __init__(self):
        self.total_wait_time = 0
        self.total_travel_time = 0
        self.passenger_count = 0
        self.congestion_below50 = 0
        self.congestion_50_80 = 0
        self.congestion_above80 = 0
    def update(self, elevator_load, capacity, dt):
        ratio = elevator_load / capacity if capacity > 0 else 0
        if ratio < 0.5:
            self.congestion_below50 += dt
        elif ratio < 0.8:
            self.congestion_50_80 += dt
        else:
            self.congestion_above80 += dt
''',

    "simulation.py": '''import random
from passenger import Passenger
from scheduler import Scheduler

class Simulation:
    def __init__(self, building, floor_count, elevator_count, sim_duration=100):
        self.building = building
        self.floor_count = floor_count
        self.elevator_count = elevator_count
        self.sim_duration = sim_duration
        self.current_time = 0.0
        self.passengers = []
        self.finished = False
    def generate_random_passenger(self, rate=0.5):
        if random.random() < rate * 0.1:
            start = random.randint(1, self.floor_count)
            target = random.randint(1, self.floor_count)
            while target == start:
                target = random.randint(1, self.floor_count)
            num = random.randint(1, 4)
            p = Passenger(self.current_time, start, target, num)
            self.passengers.append(p)
            direction = 'up' if target > start else 'down'
            self.building.add_outside_call(start, direction)
            Scheduler.allocate(self.building, start, direction)
            return p
        return None
    def step(self, dt=0.1):
        if self.current_time >= self.sim_duration:
            self.finished = True
            return
        self.current_time += dt
        self.generate_random_passenger()
        # 简化：电梯移动由外部控制，这里预留
    def get_statistics(self):
        return self.statistics if hasattr(self, 'statistics') else None
''',

    "gui.py": '''import tkinter as tk
from tkinter import ttk
from building import Building
from simulation import Simulation

class ElevatorGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("电梯群控调度仿真")
        # 控制面板
        control_frame = ttk.LabelFrame(root, text="控制面板")
        control_frame.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        ttk.Label(control_frame, text="楼层数:").grid(row=0, column=0)
        self.floor_var = tk.IntVar(value=16)
        ttk.Spinbox(control_frame, from_=5, to=25, textvariable=self.floor_var, width=5).grid(row=0, column=1)
        ttk.Label(control_frame, text="电梯数:").grid(row=0, column=2)
        self.elevator_var = tk.IntVar(value=4)
        ttk.Spinbox(control_frame, from_=1, to=8, textvariable=self.elevator_var, width=5).grid(row=0, column=3)
        ttk.Label(control_frame, text="模拟时间(秒):").grid(row=0, column=4)
        self.time_var = tk.IntVar(value=100)
        ttk.Spinbox(control_frame, from_=10, to=300, textvariable=self.time_var, width=5).grid(row=0, column=5)
        self.start_btn = ttk.Button(control_frame, text="开始仿真", command=self.start_sim)
        self.start_btn.grid(row=0, column=6, padx=10)
        # 画布
        self.canvas = tk.Canvas(root, bg='white', width=600, height=500)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        # 统计区
        self.stats_text = tk.Text(root, width=30, height=20)
        self.stats_text.pack(side=tk.RIGHT, fill=tk.Y)
        self.running = False
    def start_sim(self):
        if self.running:
            return
        self.running = True
        floor_n = self.floor_var.get()
        ele_n = self.elevator_var.get()
        sim_time = self.time_var.get()
        self.building = Building(floor_n, ele_n)
        self.sim = Simulation(self.building, floor_n, ele_n, sim_time)
        self.update_simulation()
    def update_simulation(self):
        if not self.running:
            return
        if self.sim.finished:
            self.running = False
            self.stats_text.insert(tk.END, "仿真结束\\n")
            return
        self.sim.step(dt=0.1)
        self.draw_building()
        self.root.after(100, self.update_simulation)
    def draw_building(self):
        self.canvas.delete("all")
        floor_h = 30
        start_y = 50
        floor_n = self.building.floor_count
        for floor in range(floor_n, 0, -1):
            y = start_y + (floor_n - floor) * floor_h
            self.canvas.create_rectangle(50, y, 200, y+floor_h, outline='black')
            self.canvas.create_text(30, y+floor_h/2, text=str(floor))
        for i, e in enumerate(self.building.elevators):
            x = 250 + i * 60
            y = start_y + (floor_n - e.current_floor) * floor_h
            self.canvas.create_rectangle(x, y, x+50, y+floor_h, fill='lightblue')
            self.canvas.create_text(x+25, y+floor_h/2, text=f"{e.get_load()}/{e.capacity}")
            dir_text = {'up':'↑', 'down':'↓', 'stop':'○'}[e.direction]
            self.canvas.create_text(x+25, y-10, text=dir_text)
''',

    "main.py": '''import tkinter as tk
from gui import ElevatorGUI

if __name__ == "__main__":
    root = tk.Tk()
    app = ElevatorGUI(root)
    root.mainloop()
'''
}

# 写入文件
for filename, content in files_content.items():
    with open(filename, 'w', encoding='utf-8') as f:
        f.write(content)
    print(f"已创建: {filename}")

print("所有文件创建完成！现在可以运行 python main.py")