import math

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
