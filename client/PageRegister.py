import tkinter as tk               
from tkinter import font as tkfont
import logging
import socket
import time

#page 3

class PageRegister(tk.Frame):

    def sendRegister(self, user, password):
        logging.info("Register for {} {}".format(user, password))
        self.controller.sock.send(b'r\n')
        time.sleep(0.05)
        data = self.controller.sock.recv(4096)

        self.controller.sock.send(bytes(user, 'utf-8'))
        self.controller.sock.send(b'\n')
        time.sleep(0.05)
        data = self.controller.sock.recv(4096)
        
        self.controller.sock.send(bytes(password, 'utf-8'))
        self.controller.sock.send(b'\n')
        time.sleep(0.05)
        data = self.controller.sock.recv(4096)

        should_data = 'ok'
        if data.decode('utf-8').split('\n')[0] != should_data:
            self.label_error.config(text="Error registering: {}".format(data.decode('utf-8').split('\n')[0]))
            return

        self.controller.show_frame("PageStart")

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        label = tk.Label(self, text="Please register", font=controller.title_font)
        label.pack(side="top", fill="x", pady=10)

        label_username = tk.Label(self, text="Username:")
        label_username.pack()
        entry_username = tk.Entry(self)
        entry_username.pack()

        label_password = tk.Label(self, text="Password:")
        label_password.pack()
        entry_password = tk.Entry(self, show="*")
        entry_password.pack()

        label_password = tk.Label(self, text="Repeat Password:")
        label_password.pack()
        entry_password2 = tk.Entry(self, show="*")
        entry_password2.pack()

        button = tk.Button(self, text="Register",
                           command=lambda:self.sendRegister(
                                entry_username.get(), 
                                entry_password.get()))
        button.pack()

        buttonBack = tk.Button(self, text="Back",
                           command=lambda:self.controller.show_frame("PageStart"))
        buttonBack.pack()

        self.label_error = tk.Label(self, text="")
        self.label_error.pack(side="top", fill="x", pady=10)