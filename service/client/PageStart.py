import tkinter as tk               
from tkinter import font as tkfont

#page 2, after client connected

class PageStart(tk.Frame):

    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        self.controller = controller
        label = tk.Label(self, text="Granulizer", font=controller.title_font)
        label.pack(side="top", fill="x", pady=10)

        buttonRegister = tk.Button(self, text="Register",
                            command=lambda: controller.show_frame("PageRegister"))
        buttonLogin = tk.Button(self, text="Login",
                            command=lambda: controller.show_frame("PageLogin"))
        buttonRegister.pack()
        buttonLogin.pack()