import time
from machine import Pin, SPI
from core.rf24_micropython import RF24

senders = b"1Node"
addresses = [b"2Node", b"3Node", b"4Node", b"Node5"]

cfg = {"spi": 0, "miso": 4, "mosi": 7, "sck": 6, "csn": 5, "ce": 17}
csn = Pin(cfg["csn"], mode=Pin.OUT, value=1)
ce = Pin(cfg["ce"], mode=Pin.OUT, value=0)
spi = SPI(cfg["spi"], sck=Pin(cfg["sck"]), mosi=Pin(cfg["mosi"]), miso=Pin(cfg["miso"]))

def create_listener(receiver_node):
    print(f'Creating listener for node {receiver_node}')
    p0 = Pin(0, Pin.OUT)
    p0.value(1)
    time.sleep(0.5)
    nrf = RF24(spi, csn, ce)
    nrf.ack = True
    nrf.pa_level = -12
    nrf.open_rx_pipe(1, addresses[receiver_node])
    nrf.interrupt_config(data_sent=False, data_fail=False)
    nrf.listen = True
    return nrf

class Sender:
    def __init__(self):
        p0 = Pin(0, Pin.OUT)
        p0.value(1)
        time.sleep(0.5)

        nrf = RF24(spi, csn, ce)
        nrf.ack = True
        nrf.pa_level = -12
        nrf.listen = False
        self._nrf = nrf

    def send(self, client, buffer):
        self._nrf.open_tx_pipe(addresses[client])
        return self._nrf.send(buffer)
    
    def send_with_retries(self, client, buffer):
        retries = 0
        while retries < 5:
            result = self.send(client, buffer)
            if result:
                return result   
            else:
                print("send() failed or timed out")
                time.sleep(0.01)
                retries += 1
        return False