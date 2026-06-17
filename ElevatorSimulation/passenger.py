class Passenger:
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
