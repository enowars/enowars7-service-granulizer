# Usage of the client

1. Register / Login
2. Upload a file (or use the default files 'bach' or 'space')
3. Play around with the settings
4. Granulize it 
5. Play it

Note: The server will automatically disconnect you after 2min.

## Installation

Install dependecies from the requirements.txt:
`pip install requirements.txt`

Start the GUI:
`python Main.py`

## Known problems when starting
Depending on your installed python version, there could be a problem using the tkinter python library, which relies on the Tk library. This is a known bug under MacOS. If an error similar to that occurs, try to upgrade python (or don't use this client anyway, and connect via terminal instead: nc 'ip' 2345).

Older python versions could have a problem loading the used style for tkinter. If that is the case, comment the lines 44 and 45 (with app.call) in Main.py.