import tkinter as tk               
from tkinter import font as tkfont
import socket
import logging
import time

#page 1, landing page

class PageConnect(tk.Frame):

    def connect(self, ip: str, port: int, controller):
        logging.info("Connect to {}:{}".format(ip, port))
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(2)
            sock.connect((ip, port))
            time.sleep(0.1)
            should_data = b"  _____ _____            _   _ _    _ _      _____ ____________ _____  \n / ____|  __ \\     /\\   | \\ | | |  | | |    |_   _|___  /  ____|  __ \\ \n| |  __| |__) |   /  \\  |  \\| | |  | | |      | |    / /| |__  | |__) | \n| | |_ |  _  /   / /\\ \\ | . ` | |  | | |      | |   / / |  __| |  _  / \n| |__| | | \\ \\  / ____ \\| |\\  | |__| | |____ _| |_ / /__| |____| | \\ \\ \n \\_____|_|  \\_\\/_/    \\_\\_| \\_|\\____/|______|_____/_____|______|_|  \\_\\ \n\nHello! Do you want to login (l) or register (r)?\n >"
            data = sock.recv(4096)
            if (data != should_data):
                logging.warn("Received unexpected data from server")
                return
            self.controller.sock = sock
            logging.info("Connected")
            self.controller.show_frame("PageStart")
        except:
            #write error message
            self.label_error.config(text="Error connecting")

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        label = tk.Label(self, text="Connect to Audio Processing Server", font=controller.title_font)
        label.pack(side="top", fill="x", pady=10)

        label_ip = tk.Label(self, text="IP:")
        label_ip.pack()
        v = tk.StringVar(self, value='localhost')
        entry_ip = tk.Entry(self, textvariable=v)
        entry_ip.pack()

        label_port = tk.Label(self)
        label_port.pack()
        p = tk.StringVar(self, value='2345')
        entry_port = tk.Entry(self, textvariable=p)
        entry_port.pack()

        button = tk.Button(self, text="Connect",
                            command=lambda:self.connect(
                                entry_ip.get(), 
                                int(entry_port.get()),
                                controller
                                ))
        button.pack()

        self.label_error = tk.Label(self, text="")
        self.label_error.pack(side="top", fill="x", pady=10)