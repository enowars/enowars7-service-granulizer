# upload wav / pcm
# granulize
# download wav pcm
# settings


import tkinter as tk
from tkinter import font as tkfont
from tkinter.filedialog import askopenfilename
from tkinter import messagebox
from tkdial import *
import socket
from playsound import playsound
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

    def uploadBachAction(self):
        self.uploadFile("bach.wav")

    def uploadSpaceAction(self):
        self.uploadFile("space.wav")

    def uploadFile(self, filename):
        #assumes that we are logged in

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
            print("After filename:")
            print(data)

            #send data
            self.controller.sock.send(base64_str.encode())
            self.controller.sock.send(b'\n')        
            time.sleep(0.1)
            res = self.controller.sock.recv(4096)
            print(res)
            res = res.decode('utf-8')
            if 'Success' not in res:
                self.label_error.config(text="Error: unexpected answer from server")
            else:
                self.label_error.config(text="Success uploading") 
        except Exception as e:
            print(e)
            self.label_error.config(text="Error connecting")
            return

    def uploadAction(self, event=None):
        self.label_error.config(text="") #reset message
        self.chosen_filename = askopenfilename(master=self)
        self.uploadFile(self.chosen_filename)

    def sendCurrentOptions(self, 
                            option_granular_rate: int, 
                            option_timelength: int,
                            option_volume: int):
        #option granular rate
        self.controller.sock.send(b'set option granular_rate\n')
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if 'Number of grains per second:' not in res:
            self.label_error.config(text="Unexpected answer from server")
            return False
        self.controller.sock.send(str(option_granular_rate).encode('utf-8'))
        self.controller.sock.send(b'\n')
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if 'ok\n' not in res:
            print("Wrong answer:", res)
            self.label_error.config(text="Error setting option 'granular rate'")
            return False
        
        #option grain timelength
        self.controller.sock.send(b'set option grain timelength\n')
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if 'New timelength of sample:' not in res:
            self.label_error.config(text="Unexpected answer from server")
            return False
        self.controller.sock.send(str(option_timelength).encode('utf-8'))
        self.controller.sock.send(b'\n')
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if 'ok\n' not in res:
            print("Wrong answer:", res)
            self.label_error.config(text="Error setting option 'granular rate'")
            return False

        #option volume
        self.controller.sock.send(b'set option volume\n')
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if 'New volume of sample:' not in res:
            self.label_error.config(text="Unexpected answer from server")
            return False
        self.controller.sock.send(str(option_volume).encode('utf-8'))
        self.controller.sock.send(b'\n')
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if 'ok\n' not in res:
            print("Wrong answer:", res)
            self.label_error.config(text="Error setting option 'granular rate'")
            return False

        return True

    def granulizeSharedAction(self, event=None):
        self.label_error.config(text="")

        self.controller.show_frame("PageSharedDataPrompt")
    
    def activateKeySharing(self, event=None) -> str:
        self.label_error.config(text="")
        self.text_key.configure(state="normal")
        self.text_key.delete(1.0, "end")
        self.text_key.insert(1.0, "")
        self.text_key.configure(state="disabled")

        self.controller.sock.send(b"sharing allow\n")
        time.sleep(0.1)
        res = self.controller.sock.recv(4096)
        res = res.decode('utf-8')
        if "already" in res:
            self.label_error.config(text="Error")
            self.text_key.configure(state="normal")
            self.text_key.delete(1.0, "end")    
            self.text_key.configure(state="disabled")
            return
        res = res.split(" ")[2]
        key = res[:64]
        
        s = "Key: {}".format(key)

        self.label_error.config(text="Success")
        self.text_key.configure(state="normal")
        self.text_key.delete(1.0, "end")
        self.text_key.insert(1.0, s)
        self.text_key.configure(state="disabled")
        
    def deactiveKeySharing(self, event=None):
        self.label_error.config(text="")
        self.controller.sock.send(b"sharing disallow\n")
        time.sleep(0.1)
        self.controller.sock.recv(4096)
        self.label_error.config(text="Success")
        self.text_key.configure(state="normal")
        self.text_key.delete(1.0, "end")
        self.text_key.insert(1.0, "")
        self.text_key.configure(state="disabled")
        
    def granulizeAction(self, event=None):
        #send granulize command to granulize tmp.wav / tmp.pcm
        self.label_error.config(text="")

        if self.tmp_filename is None:
            self.label_error.config(text="Upload a file first")
            return
        
        worked = self.sendCurrentOptions(
            self.dialGrainsPerSecond.get(),
            self.dialSampleTimelength.get(),
            self.dialVolume.get())
        
        if not worked:
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
        print("File downloaded successfully")

    def playAction(self, event=None):
        print("PLAY")
        self.download_file()
        
        #play sound
        if self.tmp_filename == b"tmp.wav\n":
            playsound('tmp_out.wav')
            print("File played")
        else:
            self.label_error.config(text="Raw data file was downloaded into tmp.pcm, but can't be played")

    def logout(self):
        print("Logging out")
        self.controller.sock.send(b'quit\n')
        time.sleep(0.1)
        self.controller.sock.close()
        self.controller.show_frame("PageConnect")
        print("Logged out")
    
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        self.tmp_filename = None
        label = tk.Label(self, text="Granulizer", font=controller.title_font)
        label.pack(side="top", fill="x", pady=10)

        self.text_key =  tk.Text(self, height=1, borderwidth=0)
        self.text_key.insert(1.0, "")
        self.text_key.configure(state="disabled")
        self.text_key.pack(side="bottom", fill="y", pady=10)

        self.label_error = tk.Label(self, text="")
        self.label_error.pack(side="bottom", fill="x", pady=10)
        
        frameDials = tk.Frame(self)
        frameDials.pack(side="right")

        self.dialGrainsPerSecond = Dial(master=self, color_gradient=("black", "green"),
                unit_length=30, radius=100, text_color="white", 
                needle_color="green", text="Grains per s: ",
                integer=True, scroll_steps=1, start=2, end=200, )
        self.dialGrainsPerSecond.set(10)
        self.dialGrainsPerSecond.pack(in_=frameDials, side="right")

        self.dialSampleTimelength = Dial(master=self, color_gradient=("white", "blue"),
                unit_length=30, radius=100, text_color="white", 
                needle_color="blue", text="Timelength: ",
                integer=True, scroll_steps=1, start=1, end=10, )
        self.dialSampleTimelength.set(2)
        self.dialSampleTimelength.pack(in_=frameDials, side="right")

        self.dialVolume = Dial(master=self, color_gradient=("black", "red"),
                unit_length=30, radius=100, text_color="white", 
                needle_color="red", text="Volume: ",
                integer=True, scroll_steps=1, start=1, end=100, )
        self.dialVolume.set(100)
        self.dialVolume.pack(in_=frameDials, side="right")

        frameUpload = tk.Frame(self, width=100)
        frameUpload.pack(side="left")
        elementWidth = 20

        buttonSelectBach = tk.Button(self, text='Open Bach', command=self.uploadBachAction, width=elementWidth)
        buttonSelectBach.pack(in_=frameUpload, side="top")

        buttonSelectSpace = tk.Button(self, text='Open Space', command=self.uploadSpaceAction, width=elementWidth)
        buttonSelectSpace.pack(in_=frameUpload, side="top")

        buttonSelectUpload = tk.Button(self, text='Open File', command=self.uploadAction, width=elementWidth)
        buttonSelectUpload.pack(in_=frameUpload, side="top")

        buttonGranulize = tk.Button(self, text='Granulize', command=self.granulizeAction, width=elementWidth)
        buttonGranulize.pack(in_=frameUpload, side="top")

        buttonGranulizeShared = tk.Button(self, text='Granulize shared file', command=self.granulizeSharedAction, width=elementWidth)
        buttonGranulizeShared.pack(in_=frameUpload, side="top")

        buttonPlay = tk.Button(self, text='Play Granulized', command=self.playAction, width=elementWidth)
        buttonPlay.pack(in_=frameUpload, side="top")

        buttonActivateKeySharing = tk.Button(self, text='Activate key sharing', command=self.activateKeySharing, width=elementWidth)
        buttonActivateKeySharing.pack(in_=frameUpload, side="top")

        buttonDeactivateKeySharing = tk.Button(self, text='Deactivate key sharing', command=self.deactiveKeySharing, width=elementWidth)
        buttonDeactivateKeySharing.pack(in_=frameUpload, side="top")


        buttonLogout = tk.Button(self, text="Logout",
                           command=self.logout, width=elementWidth)
        buttonLogout.pack(in_=frameUpload, side="top")