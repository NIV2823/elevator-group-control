class Statistics:
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
