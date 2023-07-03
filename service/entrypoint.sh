#!/bin/bash

#if [ ! -e "/service/data/db.sqlite3" ]; then
#    /service/gendb /service/data/db.sqlite3
#fi

#while [ 1 ]; do
#	echo "[DB CLEANUP] @ $(date +%T)"
#	/service/cleandb /service/data/db.sqlite3
#	sleep 60
#done &

#chown -R service:service "/users/"

#!/bin/sh
cleaner () {
    CLEANUP_DIR=$1;
    echo "Starting cleaner function"
    sleep 1800 #30min
    echo "Cleaner function will now regularly clean"
    while true; do
        echo "$CLEANUP_DIR" $(whoami)
        find "$CLEANUP_DIR" -regex "$CLEANUP_DIR/.+" -mmin +30 -delete
        sleep 60
    done
}
DATA_DIR="/service/users"

ncat --keep-open --listen -p 4321 -m 1000 --no-shutdown \
    --wait 10s --sh-exec '/service/granulizer' &
cleaner "$DATA_DIR"
#tcpserver -c 1000 -v 0.0.0.0 4321 /service/granulizer &
