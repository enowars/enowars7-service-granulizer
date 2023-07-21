import tkinter as tk               
from tkinter import font as tkfont

from PageConnect import *
from PageLogin import *
from PageRegister import *
from PageStart import *
from PageMenu import *
from PageSharedDataPrompt import *

class SampleApp(tk.Tk):

    def __init__(self, *args, **kwargs):
        tk.Tk.__init__(self, *args, **kwargs)

        self.title_font = tkfont.Font(family='Helvetica', size=18, weight="bold", slant="italic")

        container = tk.Frame(self)
        container.pack(side="top", fill="both", expand=True)
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        self.frames = {}
        for F in (PageConnect, PageStart, PageLogin, PageRegister, PageMenu, PageSharedDataPrompt):
            page_name = F.__name__
            frame = F(parent=container, controller=self)
            self.frames[page_name] = frame
            frame.grid(row=0, column=0, sticky="nsew")

        self.show_frame("PageConnect")
        #self.show_frame("PageMenu")

    def show_frame(self, page_name):
        frame = self.frames[page_name]
        frame.tkraise()

if __name__ == "__main__":
    logger = logging.getLogger()
    logger.setLevel(logging.INFO)
    logging.info("Start client")
    
    app = SampleApp()
    app.title("Granulizer Client")
    app.call("source", "azure/azure.tcl")
    app.call("set_theme", "dark")
    app.mainloop()