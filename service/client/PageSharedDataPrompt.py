import tkinter as tk               
from tkinter import font as tkfont
import time

class PageSharedDataPrompt(tk.Frame):

    def granulizeShared(self, username, key, filename):
        self.controller.sock.send(b"sharing use key\n")
        time.sleep(0.1)
        data = self.controller.sock.recv(4096)
        
        self.controller.sock.send(("{}\n".format(username)).encode("utf-8"))
        time.sleep(0.1)
        data = self.controller.sock.recv(4096)
        
        self.controller.sock.send(("{}\n".format(key)).encode("utf-8"))
        time.sleep(0.1)
        data = self.controller.sock.recv(4096)
        
        self.controller.sock.send(("{}\n".format(filename)).encode("utf-8"))
        time.sleep(0.1)
        data = self.controller.sock.recv(4096)
        line = data.decode('utf-8')
        
        if "written to file" not in line:
            self.label_error.config(text="Error granulizing file")
        else:
            self.controller.show_frame("PageMenu")
        
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        label = tk.Label(self, text="Enter username + shared key", font=controller.title_font)
        label.pack(side="top", fill="x", pady=10)

        label_username = tk.Label(self, text="Username:")
        label_username.pack()
        entry_username = tk.Entry(self)
        entry_username.pack()

        label_password = tk.Label(self, text="Shared Key:")
        label_password.pack()
        entry_password = tk.Entry(self)
        entry_password.pack()

        label_filename = tk.Label(self, text="File name:")
        label_filename.pack()
        entry_filename = tk.Entry(self)
        entry_filename.pack()

        button = tk.Button(self, text="Granulize File",
                           command=lambda: self.granulizeShared(
                            entry_username.get(), 
                            entry_password.get(),
                            entry_filename.get()))
        button.pack()

        buttonBack = tk.Button(self, text="Back",
                           command=lambda:self.controller.show_frame("PageStart"))
        buttonBack.pack()

        self.label_error = tk.Label(self, text="")
        self.label_error.pack(side="top", fill="x", pady=10)