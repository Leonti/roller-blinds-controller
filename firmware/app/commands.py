raw_commands = {
    'UP_FOR_LIMIT': 1,
    'STOP_AND_SET_TOP_LIMIT': 2,
    'GO_DOWN_FOR_LIMIT': 3,
    'STOP_AND_SET_BOTTOM_LIMIT': 4,
    'OPEN_BLINDS': 5,
    'CLOSE_BLINDS': 6,
    'MOVE_BLINDS_TO': 7,
    'READ_RESPONSE': 8,
    'PREPARE_STATUS': 9,
    'PREPARE_SETTINGS': 10,
    'UPDATE_SETTINGS': 11,
}
commands = {k: str(v) for k, v in raw_commands.items()}

from app.motor import Motor
from app.store import Store

class CommandProcesor:

  def __init__(self, is_left):
    self._store_0 = Store(0)
    self._store_1 = Store(1)
    self._m0 = Motor(is_left, self._store_0, {
      'in1': 9,
      'in2': 8,
      'pwm': 10,
      'stby': 16,
      'enc2': 21,
      'enc1': 20,
    })
    self._m1 = Motor(is_left, self._store_1, {
      'in1': 14,
      'in2': 15,
      'pwm': 12,
      'stby': 16,
      'enc2': 19,
      'enc1': 18,
    })

  def on_command(self, received_string):

    print(f'On command {received_string}')
    split_command = received_string.split(':')
    command_id = split_command[0]
    command_set = {k for k in commands if commands[k] == command_id}
    if len(command_set) == 1:
      command = list(command_set)[0]

      if command == 'PREPARE_SETTINGS':
        side = split_command[1]
        settings = self._store_0.get_settings_as_string() if side == '0' else self._store_1.get_settings_as_string()
        return settings
      if command == 'PREPARE_STATUS':
        side = split_command[1]
        return self._m0.format_status() if side == '0' else self._m1.format_status()  
      elif command == 'READ_RESPONSE':
        print('Reading response, doing nothing')
      elif command == 'UPDATE_SETTINGS':
        side = split_command[1] 
        store = self._store_0 if side == '0' else self._store_1
        store.store_settings_from_string(':'.join(split_command[2:]))
      elif command == 'UP_FOR_LIMIT':
        side = split_command[1]
        m = self._m0 if side == '0' else self._m1
        m.go_setup_up()
      elif command == 'STOP_AND_SET_TOP_LIMIT':
        side = split_command[1]
        m = self._m0 if side == '0' else self._m1
        m.stop_and_set_top_limit()
      elif command == 'GO_DOWN_FOR_LIMIT':
        side = split_command[1]
        m = self._m0 if side == '0' else self._m1
        m.go_setup_down()
      elif command == 'STOP_AND_SET_BOTTOM_LIMIT':
        side = split_command[1]
        m = self._m0 if side == '0' else self._m1
        m.finish_bottom_setup()
      elif command == 'MOVE_BLINDS_TO':
        side = split_command[1]
        m = self._m0 if side == '0' else self._m1
        m.go_to_position_percent(int(split_command[2]))                             
      else:  
        print(command)

    return ""

  def tick(self):
    self._m0.update()
    self._m1.update()