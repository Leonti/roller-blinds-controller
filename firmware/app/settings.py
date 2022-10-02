import app.base64 as base64
import struct

def float_to_bytes(value):
  integer_part, fractional_part = divmod(value, 1.0)
  fractional_scaled = int(fractional_part * 100)
  return struct.pack('>B', int(integer_part)) + struct.pack('>B', fractional_scaled)

def bytes_to_float(bytes):
  integer_part = struct.unpack('>B', bytes[:1])[0]
  fractional_scaled = struct.unpack('>B', bytes[1:])[0]
  return integer_part + fractional_scaled/100

class Settings:
  def __init__(self, bytes):
    self._parse_settings_from_bytes(bytes)

  def get_settings_as_bytes(self):
    return (struct.pack('>L', self.bottom_limit) 
      + struct.pack('>B', self.slowdown_percent)
      + struct.pack('>B', self.manual_speed_up)
      + struct.pack('>B', self.manual_speed_down)
      + float_to_bytes(self.default_max_speed)
      + float_to_bytes(self.pid_kp)
      + float_to_bytes(self.pid_kd)
      + float_to_bytes(self.pid_ki)
    )

  def _parse_settings_from_bytes(self, bytes):
    length = len(bytes)

    self.bottom_limit = struct.unpack('>L', bytes[0:4])[0] if length >= 4 else 0
    self.slowdown_percent = struct.unpack('>B', bytes[4:5])[0] if length >= 5 else 20
    self.manual_speed_up = struct.unpack('>B', bytes[5:6])[0] if length >= 6 else 70
    self.manual_speed_down = struct.unpack('>B', bytes[6:7])[0] if length >= 7 else 50
    self.default_max_speed = bytes_to_float(bytes[7:9]) if length >= 9 else 0.0
    self.pid_kp = bytes_to_float(bytes[9:11]) if length >= 11 else 175.0
    self.pid_kd = bytes_to_float(bytes[11:13]) if length >= 13 else 5.0
    self.pid_ki = bytes_to_float(bytes[13:15]) if length >= 15 else 4.0

    print(f"Current settings: '{self.as_string()}'")

  def as_string(self):
    return (f'{self.bottom_limit}:{self.slowdown_percent}:{self.manual_speed_up}:{self.manual_speed_down}:{self.default_max_speed}:'
      + f'{self.pid_kp}:{self.pid_kd}:{self.pid_ki}'
    )

  def get_settings_as_string(self):
    return base64.b64encode(self.get_settings_as_bytes())

  def parse_settings_from_string(self, as_string):
    self._parse_settings_from_bytes(base64.b64decode(as_string))    