# upload wav / pcm
# granulize
# download wav pcm
# settings


import tkinter as tk
from tkinter import font as tkfont
from tkinter import messagebox
from tkdial import *
import tkinterDnD
import socket
import logging
import time
import base64

# page 1, landing page

def base64_encode_file(file_path):
    with open(file_path, 'rb') as file:
        file_data = file.read()
        encoded_data = base64.b64encode(file_data)
        encoded_string = encoded_data.decode('utf-8')
    return encoded_string

def decode_base64_to_file(base64_string: str, filename: str):
    base64_bytes = base64_string.encode('utf-8')
    binary_data = base64.b64decode(base64_bytes)
    
    with open(filename, 'wb') as file:
        file.write(binary_data)

#makes sure that the buffer is not read too far
def read_line_byte_by_byte(sock: socket):
    data = b""
    while True:
        chunk = sock.recv(1)
        data += chunk
        if b"\n" in chunk:
            break
    return data.decode('utf-8')

#sometimes also reads data in after '\n', for precise reading use read_line_byte_by_byte
def read_line(sock: socket):
    data = b""
    while True:
        chunk = sock.recv(4096)
        data += chunk
        if b"\n" in chunk:
            break
    return data.decode('utf-8')

class PageMenu(tk.Frame):

    def uploadFile(self, filename):
        #assumes that we are logged in

        #TODO remove
        filename = "/home/luca/Dokumente/Uni/Informatik/CTFProject/code/enowars7-service-granulizer/service/src/users/a/bach.wav"

        #check if file has correct ending
        filetype = ""
        if filename.endswith(".wav"):
            filetype = "wav"
        elif filename.endswith(".pcm"):
            filetype = "pcm"
        else:
            messagebox.showerror('Error', 'File has to end with .pcm or .wav')
            return


        #read complete file and base64encode it
        try:
            base64_str = base64_encode_file(filename)
        except Exception:
            messagebox.showerror('Error', 'File couldn\'t be processed.')
            return
        #send it to server
        try:
            self.tmp_filename = b""
            if filetype == "wav":
                self.controller.sock.send(b'upload wav\n')
                self.tmp_filename = b"tmp.wav\n"
            else:
                self.controller.sock.send(b'upload pcm\n')
                self.tmp_filename = b"tmp.pcm\n"
            time.sleep(0.1)
            data = self.controller.sock.recv(4096)
            
            print(self.tmp_filename)
            #enter filename (for temp files)
            self.controller.sock.send(self.tmp_filename)
            time.sleep(0.1)
            data = self.controller.sock.recv(4096)
            print(data)

            #send data
            self.controller.sock.send(base64_str.encode())
            self.controller.sock.send(b'\n')        
            time.sleep(0.1)
            self.controller.sock.recv(4096000)
            print(data)
            print()
            time.sleep(0.1)
            res = self.controller.sock.recv(4096)
            print(res)
            res = res.decode('utf-8')
            if 'Success' not in res:
                self.label_error.config(text="Error: unexpected answer from server")
            else:
                self.label_error.config(text="Success uploading") 
        except Exception:
            self.label_error.config(text="Error connecting")
            return

    def uploadAction(self, event=None):
        #reset message
        self.label_error.config(text="") 
        
        self.chosen_filename = tk.filedialog.askopenfilename(master=self)
        self.uploadFile(self.chosen_filename)

    def granulizeAction(self, event=None):
        #send granulize command to granulize tmp.wav / tmp.pcm
        self.label_error.config(text="")

        if self.tmp_filename is None:
            self.label_error.config(text="Upload a file first")
            return
        
        self.controller.sock.send(b'granulize\n')
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        if b'Enter a file name: ' != res:
            self.label_error.config(text="Unexpected answer from server: {}".format(res))
        
        self.controller.sock.send(self.tmp_filename)
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if "written to file " not in res:
            self.label_error.config(text="Unexpected answer from server: {}".format(res))
            return
        self.label_error.config(text="Granulized successful")

    def download_file(self): #downloads the granulize.pcm / .wav and writes it to own folder 
        self.label_error.config(text="")

        if self.tmp_filename is None:
            self.label_error.config(text="Upload a file first")
            return

        file_out_name = ""
        if self.tmp_filename == b"tmp.wav\n":
            self.controller.sock.send(b'download wav\n')
            file_out_name = 'tmp_out.wav'
        else:
            self.controller.sock.send(b'download pcm\n')
            file_out_name = 'tmp_out.pcm'
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if "Filename: " != res:
            self.label_error.config(text="Unexpected answer from server: {}".format(res))
            return
        
        if self.tmp_filename == b"tmp.wav\n":
            self.controller.sock.send(b'granulized.wav\n')
        else:
            self.controller.sock.send(b'granulized.pcm\n')
        time.sleep(0.5)

        read_line_byte_by_byte(self.controller.sock)
        read_line_byte_by_byte(self.controller.sock)
        res = read_line(self.controller.sock)
        
        res = res.split('\n')
        res = res[0]
        res = res.replace('\n', '')
        
        #decode and write to file
        decode_base64_to_file(res, file_out_name) 
    

    def playAction(self, event=None):
        print("PLAY")
        self.download_file()
        

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        self.tmp_filename = None
        label = tk.Label(self, text="Granulizer", font=controller.title_font)
        label.pack(side="top", fill="x", pady=10)

        dialGrainsPerSecond = Dial(master=self, color_gradient=("black", "red"),
                unit_length=30, radius=100, text_color="white", 
                needle_color="red", text="Grains per s: ",
                integer=True, scroll_steps=1, start=2, end=200, )
        dialGrainsPerSecond.pack()

        buttonSelectUpload = tk.Button(self, text='Open File', command=self.uploadAction)
        buttonSelectUpload.pack(side="left")

        buttonGranulize = tk.Button(self, text='Granulize', command=self.granulizeAction)
        buttonGranulize.pack(side="left")

        buttonPlay = tk.Button(self, text='Play Granulized', command=self.playAction)
        buttonPlay.pack(side="left")

        self.label_error = tk.Label(self, text="")
        self.label_error.pack(side="top", fill="x", pady=10)