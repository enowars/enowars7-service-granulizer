import tkinter as tk               
from tkinter import font as tkfont

#page 3

class PageLogin(tk.Frame):

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
                           command=lambda: controller.show_frame("StartPage"))
        button.pack()