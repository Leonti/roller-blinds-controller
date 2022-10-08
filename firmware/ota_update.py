from core.update_source import UpdateSource
from core.nrf import Sender

def run():
    updater = UpdateSource(Sender(), 3)
    updater.update()