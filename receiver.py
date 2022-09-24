from core.nrf import create_listener
from radio_command import RadioCommand
import time
from motor import Motor

commands = [
    "POSITION",
    "UP",
    "DOWN",
    "STOP",
    "STOP_AND_SAVE_END_LIMIT",
    "STOP_AND_SAVE_ZERO_POSITION"
]

def read_last_position(motor):
    try:
        with open(f"position_{motor}", "r") as f:
            return int(f.read())
    except OSError:
        return 0

def read_limit(motor):
    try:
        with open(f"limit_{motor}", "r") as f:
            return int(f.read())
    except OSError:
        return -1

m0 = Motor(read_last_position(0), read_limit(0))
m1 = Motor(read_last_position(1), read_limit(1))

def store_position(motor, position):
    with open(f"position_{motor}", "w") as f:
        f.write(f"{position}")

def store_limit(motor, position):
    with open(f"limit_{motor}", "w") as f:
        f.write(f"{position}")

def on_command(command):
    
    command_parts = command.split(",")
    
    cmd = command_parts[0]
    
    print(f'received command is {command}')

    if cmd == 'POSITION':
        motor = command_parts[1]
        position_percent = command_parts[2]
        print(f'going to position {position_percent} on motor {motor}')
        if motor == 0:
            m0.go_to_position(position_percent)
        elif motor == 1:
            m1.go_to_position(position_percent)
    elif cmd == 'UP':
        motor = command_parts[1]
        print(f'motor {motor} up')
        if motor == 0:
            m0.up(40)
        elif motor == 1:
            m1.up(40)
    elif cmd == 'DOWN':
        motor = command_parts[1]
        print(f'motor {motor} down')
        if motor == 0:
            m0.down(40)
        elif motor == 1:
            m1.down(40)
    elif cmd == 'STOP':
        print('stopping manually')
        m0.stop()
        m1.stop()        
    elif cmd == 'STOP_AND_SAVE_END_LIMIT':
        motor = command_parts[1]
        print(f'stopping motor {motor} and saving lower limit')
        if motor == 0:
            m0.stop()
            limit = m0.get_position()
            store_limit(0, limit)
            m0.set_limit(limit)
            m0.go_to_position(50)
        elif motor == 1:
            m1.stop()
            store_limit(1, m1.get_position())
            m1.set_limit(limit)
            m1.go_to_position(50)
    elif cmd == 'STOP_AND_SAVE_ZERO_POSITION':
        motor = command_parts[1]
        print(f'stopping motor {motor} and saving zero position')
        if motor == 0:
            m0.stop()
            store_position(0, 0)
            m0.set_position(0)
        elif motor == 1:
            m1.stop()
            store_position(1, 0)
            m1.set_position(0)       
    else:
        print('UNKNOWN_COMMAND')

rd = RadioCommand(create_listener(0), on_command)

def run():
    last_time = time.time()
    last_m0_position = 0
    last_m1_position = 0
    while True:
        m0_position = m0.get_position()
        m1_position = m1.get_position()
        if time.time() - last_time > 1:
            last_time = time.time()
            if m0_position != last_m0_position:
                last_m0_position = m0_position
                print(f'updating m0 position to {m0_position}')
                store_position(0, m0_position)
                rd.update_status(1, m0_position)
            if m1_position != last_m1_position:
                last_m1_position = m1_position
                print(f'updating m1 position to {m1_position}')
                store_position(1, last_m1_position)
                rd.update_status(1, last_m1_position)     
        rd.receive()
        m0.tick()
        m1.tick()

