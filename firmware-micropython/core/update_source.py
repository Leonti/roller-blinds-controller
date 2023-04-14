import time
import os

class UpdateSource:
    
    def __init__(self, sender, blind_id):
        self._sender = sender
        self._blind_id = blind_id
    
    def update(self):
        print("Starting update")
        result = self._send(bytes('START_UPDATE', 'utf-8'))
        if result is not True:
            print("Failed to start update")    
            return False

        for name in os.listdir('update'):
            if name.endswith('.py'):
                res = self._send_file(name)
                if res is not True:
                    print(f"Failed to send file '{name}'")    
                    return False
        print(f"Finishing update")             
        return self._send(b'3')    
    
    def _send_file(self, file):
        print(f"Sending file '{file}'")
        res = self._send(b'0' + bytes(str(file), 'utf-8'))
        if res is not True:
            print(f"Failed to start update for file '{file}'") 
            return False
        time.sleep(0.2)
        with open(f"update/{file}", "rb") as f:
            id = 0
            while 1:
                file_part = f.read(30)
                if len(file_part) == 0:
                    break
                buffer = b'1' + bytes(str(id), 'utf-8') + file_part
                res = self._send(buffer)
                if res is not True:  
                    return False
                id = 0 if id == 1 else 1
                time.sleep(0.05)
        print(f"Finishing file '{file}'")        
        return self._send(b'2')       
    
    def _send(self, buffer):
        retries = 0      
        while retries < 60:
            result = self._sender.send(self._blind_id, buffer)
            if result:
                return True
            else:
                print("send() failed or timed out")
                time.sleep(0.01)
                retries += 1