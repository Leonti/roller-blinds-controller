import os

class Updater:
    
    def __init__(self, nrf):
        self._nrf = nrf
        self._current_file = None
        self._duplicate_marker = 0
        self._is_updating = False     

    def _start_file(self, name):
        print(f"Starting a new file '{name}'")
        self._current_file = open(f"update/{name}", "wb")
        self._duplicate_marker = 0

    def _append_to_file(self, duplicate_marker, buffer):
        if duplicate_marker != self._duplicate_marker:
            self._duplicate_marker = duplicate_marker
            self._current_file.write(buffer)

    def _finish_file(self):
        print(f"Closing file")
        self._current_file.close()
    
    def _rm_dir(self, name):
        if name in os.listdir():
            for f in os.listdir(name):
                os.remove(f'{name}/{f}')
            os.rmdir(name)          

    def start_update(self):
        self._rm_dir('update')   
        os.mkdir('update')
        self._is_updating = True

        while self._is_updating is True:
            self._receive()

    def _finish_update(self):
        self._rm_dir('app')
        os.rename('update', 'app')
        self._is_updating

    def _receive(self):
        if self._nrf.available():
            received = self._nrf.read()

            operation = received[0]
            if operation == b'0'[0]:
                self._start_file(received[1:].decode('utf-8'))
            elif operation == b'1'[0]:
                self._append_to_file(received[1], received[2:])
            elif operation == b'2'[0]:
                self._finish_file()
            elif operation == b'3'[0]:
                self._finish_update()                
            return True
        return False