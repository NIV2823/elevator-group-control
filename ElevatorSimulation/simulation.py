import random
from passenger import Passenger
from scheduler import Scheduler

class Simulation:
    def __init__(self, building, floor_count, elevator_count, sim_duration=100):
        self.building = building
        self.floor_count = floor_count
        self.elevator_count = elevator_count
        self.sim_duration = sim_duration
        self.current_time = 0.0
        self.passengers = []           # 所有乘客（包括未上电梯的）
        self.finished = False
        self.statistics = None         # 稍后绑定
        self.last_update_time = 0.0

    def generate_random_passenger(self, rate=0.5):
        # 每0.1秒约rate*0.1的概率生成一个乘客
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

    def process_arrival(self, elevator):
        """电梯到达某一楼层，处理乘客上下"""
        floor = elevator.current_floor
        # 外呼清除
        for direction in ['up', 'down']:
            self.building.clear_outside_call(floor, direction)
        # 乘客下电梯
        to_remove = []
        for p in elevator.passengers:
            if p.target_floor == floor:
                p.alight_time = self.current_time
                to_remove.append(p)
                # 统计行程时间
                if p.board_time:
                    travel = self.current_time - p.board_time
                    if self.statistics:
                        self.statistics.total_travel_time += travel
                        self.statistics.passenger_count += 1
        for p in to_remove:
            elevator.passengers.remove(p)
            if floor in elevator.target_floors:
                elevator.target_floors.remove(floor)
        # 乘客上电梯（从等待队列中找）
        waiting_passengers = [p for p in self.passengers if p.start_floor == floor and p.board_time is None]
        for p in waiting_passengers:
            if elevator.get_load() + p.count <= elevator.capacity:
                if elevator.add_passenger(p, p.target_floor):
                    p.board_time = self.current_time
                    # 等待时间统计
                    if self.statistics:
                        self.statistics.total_wait_time += (p.board_time - p.create_time)

    def step(self, dt=0.1):
        if self.current_time >= self.sim_duration:
            self.finished = True
            return
        self.current_time += dt
        # 生成新乘客
        self.generate_random_passenger()
        # 更新每部电梯的运动
        for e in self.building.elevators:
            # 先决定方向
            e.decide_direction()
            # 更新运动
            arrived = e.update_motion(dt)
            if arrived:
                self.process_arrival(e)
        # 更新拥挤度统计（如果绑定了statistics）
        if self.statistics:
            total_load = sum(e.get_load() for e in self.building.elevators)
            total_capacity = sum(e.capacity for e in self.building.elevators)
            self.statistics.update(total_load, total_capacity, dt)

    def get_statistics(self):
        return self.statistics