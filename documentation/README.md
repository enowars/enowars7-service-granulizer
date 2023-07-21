# Granulizer Documentation

#### 1. What is Granulizer?
#### 2. Architecture
#####      2.1 Setup
#####      2.2 Models
#### 2. Underlaying Filter Algorithm
#####
#####
#### 3. Vulnerability (Path Traversal)
#####      3.1. Vulnerable Code
#####      3.2. Exploit
#####      3.3. Fix
#### 4. Vulnerability (Guessable Sharing Key)
#####      4.1 Vulnerable Code
#####      4.2 Exploit
#####      4.3 Fix

## 1. What is Granulizer?
Granulizer is a service designed for Enowars 7. This service offers functionality for upload music, applying an audio filter to it, and downloading it again. The used audio filter is a granular synthesis filter (https://en.wikipedia.org/wiki/Granular_synthesis). Granular synthesis is a technique for creating new sounds based on given input. It works by splitting up the existing sound sample into different parts, so called slices, and rearanging them randomly. The new slices also may have different lengths than the original to alter the sound even more. Granular synthesis is commonly used to generate sounds in electronic music.

This service offers different functionalities, such as:
* register
* login
* upload wav - uploads a .wav file into own profile, encoded as base64
* upload pcm - uploads a .pcm (pulse-code modulation) file into own profile, encoded as base64
* download wav - downloads a .wav file from own profile, encoded as base64
* download pcm - downloads a .pcm file from own profile, encoded as base64
* granulize - performs granulization algorithm with random parameters on .pcm or .wav file
* granulize info - more details about last granulization process
* set option granular_rate - sets the number of grains per second for a wave file. 10 is the default value
* set option grain timelength - sets the length of the new sample. 2 is the default value.
* set option volume - sets the volume of the output sample. Has to be between 1 - 100. 100 is the default value, which is the highest volume
* sharing allow - allows access for of own granulized files for other users. A key will be generated and prompted which can be used to access the personal account.
* sharing disallow - disallows the sharing (default = disallow).
* sharing use key - uses a key to access other users shared granulized files
* help - this prompt
* quit - quits the program

## 2. Architecture
### 2.1 Setup
Granulizer is written in C. The service has only one docker container, which can be started in the **service** folder with 
```docker compose up --build -d```

The underlaying data for the users are stored inside the *users/* folder, which will be automatically created when the service is started. All user data older than 30 minutes will be automatically deleted, this is performed in the *entrypoint.sh*. The docker container for the service is:
* granulizer_service_granulizer_1

The checker is based on the older enochecker2 framework, it can be started in the *checker/* folder by using ```docker compose up --build -d```. This starts two different docker containers:
* granulizer_checker-granulizer-checker-1 
* granulizer_checker-granulizer-mongo-1

### Feature: Granulizing
The *granulizing* function is the main functionality of this service. A normal workflow of using this service in the command line looks like this:
* register
* login
* upload wav / pcm
* granulize
    * choosing file
* download wav / pcm

#### Supported File Formats
##### PCM (Pulse-code Modulation) 
Raw data. The granulizing algorithm will be applied byte-per-byte. This means, one byte is regarded as one slice in granulizing.
##### WAV
Audio file. Currently supported are 8bit / 16bit audio data, the file has to be smaller than 1 megabyte. 

### Feature: Allowing music creation sharing
It is possible to allow the sharing of own uploaded samples. This allows other users, how have the access key, to use these samples and granulize them directly. Therefore, they won't get the original file, but instead always a granulized version of it. 
This can be activated when logged in by the command: **sharing allow**. This returns a key which can by used by other users for granulizing the samples.

Another user, how has the private key, can use access the other users files by **sharing use key**.

If it is wished to deactivate this, use *sharing disallow*. The sharing function is deactivated by default.

## 3. Vulnerability (Path Traversal)
The first vulnerability lies in the *granulize* command. It is possible to access other users files, by granulizing them:
```../username/flag.pcm```
With that, the granulized version of the flag can be retrieved. The new file is stored in the own user account and is called *granulized.pcm / granulized.wav*. Now the second challenge is to understand how the audio filter was applied and to reverse it, in order to get the flag.

### 3. Reversing the audio algorithm
For retrieving necessary information about the granulization process, use the *granulize info* command. This looks for example as:
```
granular_number_samples = 5
granular_order_samples = [4,0,2,1,3]
granular_order_timelens = [2,2,2,2,2]
granular_order_buffer_lens = [2,2,2,2,2]
```
The granulized output file looks like: 
```
oo��HHedlll^eelkll
```
Now its visible what the granulizing algorithm is doing:
* every original slice is here time stretched by the factor 2 (also shown in *granular_order_timelens*)
* the time stretched slices are randomly rearranged (the new order is given by *granular_order_samples*)
* Between each new slice, a fade in and fade out is generated. This has the same length as the slice from before. In the granulized output, the fade slice is visible by apparently random characters. In the granulized music, the overlay slices are well audible. If you listen closely, first, the previous slice will be faded out, and the next slice will be faded in at the same time. For best results hearing this effect, set the grains time length to 2.

Now an algorithm can be written to restore the original data, which was:  ```Hello```

### Known problems