from machine import Pin, PWM
from utime import sleep
import time
import _thread
import micropython
micropython.alloc_emergency_exception_buf(100)

class Motor:
    
    def __init__(self, is_left, store, pins):
        self._is_left = is_left
        self._store = store
        
        self._in1 = Pin(pins['in1'],Pin.OUT)
        self._in2 = Pin(pins['in2'], Pin.OUT)
        self._pwm = PWM(Pin(pins['pwm']))
        self._pwm.freq(1000)
        self._stby = Pin(pins['stby'], Pin.OUT)
        self._stby.value(1)
        
        self._pos = self._store.get_last_position()

        self._enc2 = Pin(pins['enc2'], Pin.IN)
        self._enc1 = Pin(pins['enc1'], Pin.IN)
        self._enc1.irq(trigger = Pin.IRQ_RISING, handler=self.handle_enc1, hard = True)

        self._going_down = False

        self._state = 0
        self._last_update_time_ms = None
        self._last_position_store_update_time_ms = None
        self._last_non_goal_update = None
        
        self._MAX_PWM = 100
        
        self._is_setup = False
        self._is_measuring_default_speed = False
        
        # when powered on we assume that it's the desired goal position
        self._goal_reached_at = None
        self._goal_reached = False
        self._setpoint = self._pos

        self._last_position_update = None
        self._speed_previous_error = 0.0
        self._PID_i = 0.0
        self._pwm_speed = 0
        self._max_speed = None

        self._reset_pid()

        self._direction_changes = 0
        self._false_triggers = 0

    def _reset_pid(self):
        self._speed_previous_error = 0.0
        self._PID_i = 0.0
        self._last_update_time_ms = time.ticks_ms()
        self._last_non_goal_update = time.ticks_ms()
        self._last_position_update = self._pos

    def _get_slowdown_steps(self):
        limit = self._store.get_bottom_limit()
        return round((limit * self._store.get_slowdown_percent())/100)

    def handle_enc1(self, pin):
        if pin.value() != 1:
            self._false_triggers += 1

        if self._enc2.value() == 1:
            if not self._going_down:
                self._direction_changes += 1
            self._going_down = True    
            self._pos -= 1 if self._is_left else -1
        else:
            if self._going_down:
                self._direction_changes += 1
            self._going_down = False
            self._pos += 1 if self._is_left else -1

    def go_to_position_percent(self, position_percent):
        limit = self._store.get_bottom_limit()
        position = round((limit * position_percent)/100)
        #+print(f'going to position {position}, slowdown steps: {self._get_slowdown_steps()}')
        self._setpoint = position

        steps_left = abs(position - self._pos)
        if steps_left <= self._get_slowdown_steps():
            #+print("Position difference is shorter than slowdown period, using default values for pwm speed and max speed")
            approximate_pwm_speed = self._calculate_speed(steps_left, self._get_slowdown_steps(), self._MAX_PWM)
            self._pwm_speed = approximate_pwm_speed if self._pos < self._setpoint else -approximate_pwm_speed
            self._max_speed = self._store.get_default_max_speed() if self._pos < self._setpoint else -self._store.get_default_max_speed()              
        else:
            self._max_speed = None
        
        self._goal_reached_at = None
        self._goal_reached = False
        
    def go_setup_up(self):
        self._is_setup = True
        speed = self._MAX_PWM * self._store.get_manual_speed_up() / 100
        #+print(f'Go setup up with pwm: {speed}')
        self._direction_changes = 0
        self.up(speed)

    def stop_and_set_top_limit(self):     
        self._set_speed(0)
        self._pos = 0
        self._store.store_last_position(0)
        print(f'stop and set top limit, direction changes: {self._direction_changes}')
        self._direction_changes = 0

    def go_setup_down(self):
        self._is_setup = True
        speed = self._MAX_PWM * self._store.get_manual_speed_down() / 100
        #+print(f'Go setup down with pwm: {speed}')
        self.down(speed)

    def finish_bottom_setup(self):
        self._store.store_bottom_limit(self._pos)
        self._is_setup = False
        self._is_measuring_default_speed = True
        self._direction_changes = 0
        self.go_to_position_percent(50)    

    def _calculate_speed(self, steps_left, slowdown_steps, max_speed): 
        if steps_left > slowdown_steps:
            return max_speed
        
        left_ratio = steps_left/slowdown_steps
        return max_speed * left_ratio

    def _store_last_position_update(self):
        if self._last_position_store_update_time_ms is None:
            self._last_position_store_update_time_ms = time.ticks_ms()
            return
        time_diff_ms = time.ticks_diff(time.ticks_ms(), self._last_position_store_update_time_ms)  
        if time_diff_ms > 1000:
            #+print(f'Pos: {self._pos}, direction changes: {self._direction_changes}')
            self._last_position_store_update_time_ms = time.ticks_ms()

            # don't wirte to FS while motor is moving to avoid interrupt delays
            if self._goal_reached and not self._is_setup:
                self._store.store_last_position(self._pos)

    def format_status(self):
        return f"{self._pos}:{self._setpoint}:{1 if self._goal_reached else 0}"

    def _check_goal_reach(self):
        if self._goal_reached == True:
            return True

        # 1% tolerance for reaching the goal
        tolerance = self._store.get_bottom_limit() * 0.005

        if abs(self._pos - self._setpoint) <= tolerance:
            if self._goal_reached_at is not None and time.ticks_diff(time.ticks_ms(), self._goal_reached_at) > 1000:
                #+print('GOAL STABILISED')
                self.stop()
                self._goal_reached = True
                print(f'GOAL STABILISED, direction changes: {self._direction_changes}')
            else:
                self.short_break()
                if self._goal_reached_at is None:
                  self._goal_reached_at = time.ticks_ms()
            
            return True

        return False

    def update(self):        
        self._store_last_position_update()

        if self._is_setup:
            return

        if self._check_goal_reach():
            return

        if self._last_update_time_ms is None:
            self._last_update_time_ms = time.ticks_ms()                
        time_diff_ms = time.ticks_diff(time.ticks_ms(), self._last_update_time_ms)  
        
        if time_diff_ms < 40:
            return

        position_diff = self._pos - self._last_position_update
        
        self._last_update_time_ms = time.ticks_ms()
        self._last_position_update = self._pos

        # once we moved 10% up
        if self._is_measuring_default_speed == True and self._pos/self._store.get_bottom_limit() < 0.9:
            self._is_measuring_default_speed = False
            actual_speed = abs(position_diff / time_diff_ms)
            self._store.store_default_max_speed(actual_speed)

        # going down
        if self._pos < self._setpoint:
            steps_left = self._setpoint - self._pos
            if steps_left <= self._get_slowdown_steps():
                actual_speed = position_diff / time_diff_ms
                if self._max_speed is None:
                    self._max_speed = actual_speed
                desired_speed = self._calculate_speed(steps_left, self._get_slowdown_steps(), self._max_speed)
                #+print(f'steps left: {steps_left}, desired speed: {desired_speed}, actual_speed: {actual_speed}, max speed: {self._max_speed}')
                self._update_pid_speed(time_diff_ms, actual_speed, desired_speed)
            else:
                self._set_speed(self._MAX_PWM)
                
        # going up
        elif self._pos > self._setpoint:
            steps_left = self._pos - self._setpoint
            if steps_left <= self._get_slowdown_steps():
                actual_speed = position_diff / time_diff_ms
                if self._max_speed is None:
                    self._max_speed = actual_speed     
                desired_speed = self._calculate_speed(steps_left, self._get_slowdown_steps(), self._max_speed)
                #+print(f'steps left: {steps_left}, desired speed: {desired_speed}, actual_speed: {actual_speed}, max speed: {self._max_speed}')
                self._update_pid_speed(time_diff_ms, actual_speed, desired_speed)
            else:
                self._set_speed(-self._MAX_PWM)

    def _update_pid_speed(self, time_diff_ms, actual_speed, desired_speed):        
        speed_error = desired_speed - actual_speed
        PID_p = self._store.get_pid_kp() * speed_error
        
        #dist_diference = speed_error - self._speed_previous_error
        PID_d = self._store.get_pid_kd() * ((speed_error - self._speed_previous_error)/time_diff_ms) if self._speed_previous_error != 0.0 else 0
        self._PID_i = self._PID_i + (self._store.get_pid_ki() * speed_error)
        
        #print(f'position travelled {position_diff}, actual_speed: {actual_speed}, pid_p: {PID_p}, pid_i: {self._PID_i} pwm_speed: {self._pwm_speed}')
        
        self._speed_previous_error = speed_error

        PID_total = (PID_p + self._PID_i + PID_d) * (self._MAX_PWM/100)
        #+print(f'PID_total: {PID_total}, {PID_p} + {self._PID_i} + {PID_d}')
        
        if PID_total > 0:
            total_adjustment = min(5, PID_total)
            pwm_speed = min(self._MAX_PWM, self._pwm_speed + total_adjustment)
        else:
            total_adjustment = max(-5, PID_total)
            pwm_speed = max(-self._MAX_PWM, self._pwm_speed + total_adjustment)
            
        new_speed = self._pwm_speed + total_adjustment    

        if new_speed > 0:
            pwm_speed = min(self._MAX_PWM, new_speed)
        else:
            pwm_speed = max(-self._MAX_PWM, new_speed)

        self._set_speed(pwm_speed)        


    def _set_speed(self, speed):
        #+print(f'Setting speed to {speed}')
        self._pwm_speed = speed
        if speed < 0:
            self.up(abs(speed))
        elif speed > 0:
            self.down(abs(speed))
        elif speed == 0:
            self.short_break()

    def up(self, speed):
        if not self._is_left:
            self._ccw(speed)
        else:
            self._cw(speed)
            
    def down(self, speed):
        if not self._is_left:
            self._cw(speed)
        else:
            self._ccw(speed)        

    def _ccw(self, duty):
        self._stby.value(1)
        self._in1.value(1)
        self._in2.value(0)
        duty_16 = int((duty*65536)/100)
        self._pwm.duty_u16(duty_16)

    def _cw(self, duty):
        self._stby.value(1)
        self._in1.value(0)
        self._in2.value(1)
        duty_16 = int((duty*65536)/100)
        self._pwm.duty_u16(duty_16)
        
    def stop(self):
        self._in1.value(0)
        self._in2.value(0)
        self._pwm.duty_u16(0)
        #self._stby.value(0)

    def short_break(self):
        self._stby.value(1)
        self._in1.value(1)
        self._in2.value(1)
        duty_16 = int(65536)
        self._pwm.duty_u16(duty_16)