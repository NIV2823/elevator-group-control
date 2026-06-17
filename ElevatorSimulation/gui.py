import tkinter as tk
from tkinter import ttk
from building import Building
from simulation import Simulation
from statistics import Statistics

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
        self.stats_text = tk.Text(root, width=35, height=25)
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
        # 绑定统计对象
        self.stat = Statistics()
        self.sim.statistics = self.stat
        self.update_simulation()

    def update_simulation(self):
        if not self.running:
            return
        if self.sim.finished:
            self.running = False
            self.show_statistics()
            return
        self.sim.step(dt=0.1)
        self.draw_building()
        self.update_stats_display()
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

    def update_stats_display(self):
        # 实时显示统计信息摘要
        self.stats_text.delete(1.0, tk.END)
        if hasattr(self, 'stat') and self.stat:
            self.stats_text.insert(tk.END, f"当前时间: {self.sim.current_time:.1f} 秒\n")
            self.stats_text.insert(tk.END, f"总乘客数: {self.stat.passenger_count}\n")
            avg_wait = self.stat.total_wait_time / max(1, self.stat.passenger_count)
            self.stats_text.insert(tk.END, f"平均等待时间: {avg_wait:.2f} 秒\n")
            self.stats_text.insert(tk.END, f"拥挤度<50%: {self.stat.congestion_below50:.1f} 秒\n")
            self.stats_text.insert(tk.END, f"拥挤度50-80%: {self.stat.congestion_50_80:.1f} 秒\n")
            self.stats_text.insert(tk.END, f"拥挤度>80%: {self.stat.congestion_above80:.1f} 秒\n")

    def show_statistics(self):
        self.stats_text.delete(1.0, tk.END)
        self.stats_text.insert(tk.END, "========== 仿真结束 ==========\n")
        if hasattr(self, 'stat') and self.stat:
            avg_wait = self.stat.total_wait_time / max(1, self.stat.passenger_count)
            avg_travel = self.stat.total_travel_time / max(1, self.stat.passenger_count)
            self.stats_text.insert(tk.END, f"总乘客数: {self.stat.passenger_count}\n")
            self.stats_text.insert(tk.END, f"平均等待时间: {avg_wait:.2f} 秒\n")
            self.stats_text.insert(tk.END, f"平均行程时间: {avg_travel:.2f} 秒\n")
            self.stats_text.insert(tk.END, f"拥挤度<50%: {self.stat.congestion_below50:.1f} 秒\n")
            self.stats_text.insert(tk.END, f"拥挤度50-80%: {self.stat.congestion_50_80:.1f} 秒\n")
            self.stats_text.insert(tk.END, f"拥挤度>80%: {self.stat.congestion_above80:.1f} 秒\n")
        else:
            self.stats_text.insert(tk.END, "无统计数据\n")