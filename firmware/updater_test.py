from core.updater import Updater
from core.nrf import create_listener

updater = Updater(create_listener(0))

def run():
    updater.start_update()
    while True:
        updater.receive()
        
run()