#/bin/bash -x
dexec="docker exec -ti mqtt"
\rm password.txt
touch password.txt
chmod 700 password.txt
# create users
${dexec} mosquitto_passwd -b /mosquitto/config/password.txt admin admin
${dexec} mosquitto_passwd -b /mosquitto/config/password.txt client client
${dexec} mosquitto_passwd -b /mosquitto/config/password.txt test test
# public topic
${dexec} mosquitto_pub -u admin -P admin -t "client/configuration" -m ""
${dexec} mosquitto_pub -u admin -P admin -t "client/upstream" -m ""
# sub
echo "${dexec} mosquitto_sub -u admin -P admin -t \"client/upstream\""
#restart
docker restart mqtt
