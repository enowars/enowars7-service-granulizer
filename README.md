# enowars7-service-granulizer
Vulnerable granular synthesizer service 

## Service Description
- Command-line service with user authentication, which generates .wav file music for users
- service stores last generated wav file of a user ("cloud service")

## Features
- user_register
- user_login
- upload
    uploads a music file
- granulize
    performs granular synthesis on uploaded music and
- download
    downloads a music file
- granulize info
    returns more details about performed granulization

## Exploits
    Easy:
        Buffer Overflow when input is too long.

    Advanced:
        Path traversal on granulize command, returns granulized data from other user.
        Together with "granulize info" the original can be recalculated and the flag can be retrieved.
