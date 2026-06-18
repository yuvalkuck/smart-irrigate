#/bin/bash
dexec mqtt mosquitto_pub -h 127.0.0.1 -t "/client/configuration" -f $1
