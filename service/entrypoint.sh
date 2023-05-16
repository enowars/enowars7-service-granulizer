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

ncat --keep-open --listen -p 4321 --no-shutdown \
    --wait 10s --sh-exec '/service/granulizer'
