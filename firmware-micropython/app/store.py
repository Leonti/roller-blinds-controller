import app.settings

def read_last_position(motor):
    try:
        with open(f"position_{motor}", "r") as f:
            return int(f.read())
    except OSError:
        return 0

def store_position(motor, position):
    with open(f"position_{motor}", "w") as f:
        f.write(f"{position}")

def read_settings(motor):
    try:
        with open(f"settings_{motor}", "rb") as f:
            return f.read()
    except OSError:
        return []

def store_settings(motor, settings):
    with open(f"settings_{motor}", "wb") as f:
        f.write(settings)

class Store:

  def __init__(self, motor):
    self._motor = motor
    self._last_position = read_last_position(motor)
    self._settings = app.settings.Settings(read_settings(motor))

  def store_last_position(self, last_position):
    if last_position != self._last_position: 
      print(f'storing last position {self._motor}: {last_position}')
      self._last_position = last_position
      store_position(self._motor, self._last_position)

  def get_last_position(self):
    return self._last_position

  def store_bottom_limit(self, bottom_limit):
    print(f'storing bottom position {bottom_limit} on motor {self._motor}')
    self._settings.bottom_limit = bottom_limit
    store_settings(self._motor, self._settings.get_settings_as_bytes())

  def get_slowdown_percent(self):
    return self._settings.slowdown_percent

  def get_bottom_limit(self):
    return self._settings.bottom_limit

  def get_manual_speed_up(self):
    return self._settings.manual_speed_up

  def get_manual_speed_down(self):
    return self._settings.manual_speed_down

  def store_default_max_speed(self, default_max_speed):
    print(f'storing default max speed {default_max_speed} on motor {self._motor}')
    self._settings.default_max_speed = default_max_speed
    store_settings(self._motor, self._settings.get_settings_as_bytes())

  def get_default_max_speed(self):
    return self._settings.default_max_speed

  def get_pid_kp(self):
    return self._settings.pid_kp

  def get_pid_kd(self):
    return self._settings.pid_kd

  def get_pid_ki(self):
    return self._settings.pid_ki

  def store_settings_from_string(self, as_string):
    self._settings.parse_settings_from_string(as_string)
    store_settings(self._motor, self._settings.get_settings_as_bytes())

  def get_settings_as_string(self):
    return self._settings.get_settings_as_string()  