from machine import Pin
import time

class Led:
  def __init__(self, pin):
    self._pin = Pin(pin, Pin.OUT)
    self._last_switch_time = time.ticks_ms()
    self._pattern = [1] + ([0] * 30)
    self._frequency = 50
    self._pattern_step = 0

  def on_command_error(self):
    self._pattern = [1, 0, 0, 0, 1] + ([0] * 10)
    self._frequency = 200
    self._pattern_step = 0

  def on_command_processor_create_error(self):
    self._pattern = [1, 0, 0, 0, 1, 0, 0, 0, 1] + ([0] * 10)
    self._frequency = 200
    self._pattern_step = 0

  def on_command_processor_tick_error(self):
    self._pattern = [1, 0, 1, 0, 1, 0, 1, 0]
    self._frequency = 200
    self._pattern_step = 0

  def on_update(self):
    self._pin.value(1)

  def tick(self):
    time_diff_ms = time.ticks_diff(time.ticks_ms(), self._last_switch_time)
    if time_diff_ms > self._frequency:
      self._pin.value(self._pattern[self._pattern_step])
      self._last_switch_time = time.ticks_ms()
      if self._pattern_step < len(self._pattern) - 1:
        self._pattern_step += 1
      else:
        self._pattern_step = 0