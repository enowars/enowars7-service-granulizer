import tkinter as tk
from tkdial import Dial

app = tk.Tk()

dial = Dial(app)
dial.grid(padx=10, pady=10)

app.mainloop()