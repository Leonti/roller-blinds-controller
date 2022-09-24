from core.nrf import create_listener
from core.updater import Updater
import time
from machine import Pin
#import micropython
#micropython.alloc_emergency_exception_buf(100)

BLIND_ID = 3
IS_LEFT = False

nrf_irc_pin = Pin(3, Pin.IN)

nrf = create_listener(BLIND_ID)
#nrf.print_details()

command_processor = None
# try:
from app.commands import CommandProcesor
command_processor = CommandProcesor(IS_LEFT)
# except:
#   print('Failed to create command processor')

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
          updater = Updater(nrf)
          updater.start_update()
        elif command_processor is not None:
          #try:
          next_ack = command_processor.on_command(received_string)
          if len(next_ack) >= 1:
            nrf.load_ack(bytes(next_ack, 'utf-8'), 1)
          #except:
          #  print('Failed to process command')

    if command_processor is not None:
      #try:  
        command_processor.tick()
        time.sleep(0.0001)
      #except:
      #  print('exception')
      #  continue

#print('Running...')
#run()