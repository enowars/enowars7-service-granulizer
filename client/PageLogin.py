import tkinter as tk               
from tkinter import font as tkfont
import logging
import time

#page 3

class PageLogin(tk.Frame):

    def sendLogin(self, user, password):
        logging.info("Login for {} {}".format(user, password))
        self.controller.sock.send(b'l\n')
        time.sleep(0.1)
        data = self.controller.sock.recv(4096)

        self.controller.sock.send(bytes(user, 'utf-8'))
        self.controller.sock.send(b'\n')
        time.sleep(0.1)
        data = self.controller.sock.recv(4096)
        
        self.controller.sock.send(bytes(password, 'utf-8'))
        self.controller.sock.send(b'\n')
        time.sleep(0.1)
        data = self.controller.sock.recv(4096)
        print(data)
        should_data = ' > '
        if data.decode('utf-8').split('\n')[2] != should_data:
            self.label_error.config(text="Error login: {}".format(data.decode('utf-8').split('\n')[0]))
            return

        self.controller.show_frame("PageMenu")

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        label = tk.Label(self, text="Please Login", font=controller.title_font)
        label.pack(side="top", fill="x", pady=10)

        label_username = tk.Label(self, text="Username:")
        label_username.pack()
        entry_username = tk.Entry(self)
        entry_username.pack()

        label_password = tk.Label(self, text="Password:")
        label_password.pack()
        entry_password = tk.Entry(self, show="*")
        entry_password.pack()

        button = tk.Button(self, text="Login",
                           command=lambda: self.sendLogin(
                            entry_username.get(), 
                            entry_password.get()))
        button.pack()

        buttonBack = tk.Button(self, text="Back",
                           command=lambda:self.controller.show_frame("PageStart"))
        buttonBack.pack()

        self.label_error = tk.Label(self, text="")
        self.label_error.pack(side="top", fill="x", pady=10)