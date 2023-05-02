# enowars7-service-supersynth
Vulnerable synthesizer service 

## Service Description
- Command-line service with user authentication, which generates .wav file music for users
- service stores last generated wav file of a user ("cloud service")

## Features
- user_register
- user_login
- set_music_params
    - set_melody
    - set_num_voices
    - set_wave_form
    - (set_glider_between_num_voices)
- generate_music
    returns .wav data for 1s of mono channel music (approx. 70kB data)
- filter_music(saved_music, filter_params)
    applies a filter to the given saved_music which is saved in the cloud for the user and returns this .wav data

## Usage
- user_register
- user_login
- set_music_param
    - as wished
- generate_music
- filter_music
    optional
## Exploits
Easy:
Buffer Overflow when input is too long.

Advanced:
- arbitary read of other user data with filter_music, this function does not properly check the user identification and allows reading generated music data of all users
    FLAG: inside "admin" user, filter_music(music="some input", params="flag.txt")
Heres the catch:
    filter_music does not return the reading data as a string, the flag is encoded in a wave file inside the original music. The hacker has to understand and decode how the data is encoded inside the wav file.

