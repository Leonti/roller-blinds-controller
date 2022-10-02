import time
import os

class UpdateSource:
    
    def __init__(self, nrf):
        self._nrf = nrf
    
    def update(self):
        for name in os.listdir('update'):
            self._send_file(name)
        self._send(b'3')    
    
    def _send_file(self, file):
        self._send(b'0' + bytes(str(file), 'utf-8'))
        time.sleep(0.2)
        with open(f"update/{file}", "rb") as f:
            id = 0
            while 1:
                file_part = f.read(30)
                if len(file_part) == 0:
                    break
                buffer = b'1' + bytes(str(id), 'utf-8') + file_part
                self._send(buffer)
                id = 0 if id == 1 else 1
                time.sleep(0.05)
        self._send(b'2')       
    
    def _send(self, buffer):
        retries = 0      
        while retries < 60:
            result = self._nrf.send(buffer)  # save the response (ACK payload)
            if result:
                print("Transmission successful!")
                if isinstance(result, bool):
                    print("Received an empty ACK packet")
                else:
                    print("Received: {}".format(result.decode("utf-8")))
                return True    
            else:
                print("send() failed or timed out")
                time.sleep(0.01)
                retries += 1