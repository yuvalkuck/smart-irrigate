#/bin/bash
rm password.txt
docker exec -ti mqtt mosquitto_passwd -c -b /mosquitto/config/password.txt admin admin
docker exec -ti mqtt mosquitto_passwd -b /mosquitto/config/password.txt client client
docker exec -ti mqtt mosquitto_passwd -b /mosquitto/config/password.txt test test
docker restart mqtt
