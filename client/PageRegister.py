import tkinter as tk               
from tkinter import font as tkfont
import logging

#page 3

class PageRegister(tk.Frame):

    def sendRegister(self, user, password):
        logging.info("Register for {} {}".format(user, password))
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