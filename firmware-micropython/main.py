from core.nrf import create_listener
from core.updater import Updater
from core.led import Led
import time
from machine import Pin, reset
#import micropython
#micropython.alloc_emergency_exception_buf(100)

BLIND_ID = 2
IS_LEFT = True

led = Led(25)
nrf_irc_pin = Pin(3, Pin.IN)

nrf = create_listener(BLIND_ID)
#nrf.print_details()

command_processor = None
try:
  from app.commands import CommandProcesor
  command_processor = CommandProcesor(IS_LEFT)
except Exception as e:
  led.on_command_processor_create_error()
  print(f'Failed to create command processor {e}')

def run():
  while True:
    if nrf_irc_pin.value() == 0:
      if nrf.available():
        length, pipe_number = (nrf.any(), nrf.pipe)
        received = nrf.read()
        print(
            "Received {} bytes on pipe {}".format(length, pipe_number),
        )
        print("{}".format(received.decode("utf-8")))
        received_string = received.decode("utf-8")
        if received_string == 'START_UPDATE':
          led.on_update()
          updater = Updater(nrf)
          updater.start_update()
          print("Update finished, restarting")
          reset()
        elif command_processor is not None:
          try:
            next_ack = command_processor.on_command(received_string)
            if len(next_ack) >= 1:
              nrf.load_ack(bytes(next_ack, 'utf-8'), 1)
          except:
            print('Failed to process command')
            led.on_command_error()

    if command_processor is not None:
      try:  
        command_processor.tick()
        #time.sleep(0.0001)
      except:
        led.on_command_processor_tick_error()
        print('exception')
        continue
    led.tick()

print('Running...')
run()