class Elevator:
    def __init__(self, id, capacity=15, floor_count=16):
        self.id = id
        self.capacity = capacity
        self.current_floor = 1
        self.direction = 'stop'
        self.door_open = False
        self.passengers = []       # 乘客对象列表
        self.target_floors = []    # 停靠楼层列表（包括内呼和外呼）
        self.floor_count = floor_count
        self.time_to_next = 0.0    # 到达下一楼层还需时间（秒）

    def get_load(self):
        return len(self.passengers)

    def add_passenger(self, passenger, target_floor):
        if self.get_load() < self.capacity:
            self.passengers.append(passenger)
            self.target_floors.append(target_floor)
            self.target_floors.sort()
            passenger.board_time = None  # 稍后设置
            return True
        return False

    def remove_passenger(self, passenger):
        if passenger in self.passengers:
            self.passengers.remove(passenger)

    def update_motion(self, dt):
        """更新电梯运动状态，返回是否到达楼层"""
        if self.direction == 'stop':
            self.time_to_next = 0.0
            return False
        if self.time_to_next <= 0:
            # 需要移动一层
            if self.direction == 'up':
                self.current_floor += 1
            elif self.direction == 'down':
                self.current_floor -= 1
            # 移动一层需要1秒，再重置计时器
            self.time_to_next = 1.0
            # 检查是否到达目标楼层
            if self.current_floor in self.target_floors:
                self.door_open = True
                # 到达目标，处理乘客进出（由外部调用）
                return True
        else:
            self.time_to_next -= dt
        return False

    def decide_direction(self):
        """根据目标楼层决定下一步方向"""
        if not self.target_floors:
            self.direction = 'stop'
            return
        if self.direction == 'stop':
            # 选择最近目标的方向
            nearest = min(self.target_floors, key=lambda f: abs(f - self.current_floor))
            if nearest > self.current_floor:
                self.direction = 'up'
            elif nearest < self.current_floor:
                self.direction = 'down'
        # 如果当前方向没有前方目标，则反向
        if self.direction == 'up' and all(f < self.current_floor for f in self.target_floors):
            self.direction = 'down'
        elif self.direction == 'down' and all(f > self.current_floor for f in self.target_floors):
            self.direction = 'up'