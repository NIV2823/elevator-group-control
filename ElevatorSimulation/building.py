from elevator import Elevator

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
