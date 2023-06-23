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
        tmp_filename = b""
        if filetype == "wav":
            self.controller.sock.send(b'upload wav\n')
            tmp_filename = b"tmp.wav\n"
        else:
            self.controller.sock.send(b'upload pcm\n')
            tmp_filename = b"tmp.pcm\n"
        time.sleep(0.1)
        data = self.controller.sock.recv(4096)
        
        print(tmp_filename)
        #enter filename (for temp files)
        self.controller.sock.send(tmp_filename)
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
        #TODO weitermachen, result checken if 'Success'

    def uploadAction(self, event=None):
        self.chosen_filename = tk.filedialog.askopenfilename(master=self)
        self.uploadFile(self.chosen_filename)

    def granulizeAction(self, event=None):
        print("LETS GO")

    def playAction(self, event=None):
        print("PLAY")

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
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