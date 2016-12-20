#!/bin/bash

killall loginserver
killall worldmanager
killall worldserver
killall gateserver

./loginserver/loginserver > /log/loginserver.log &
sleep 2

./worldmanager/worldmanager 1 > /log/worldmanager.log &
sleep 2

./worldserver/worldserver 11 > /log/worldserver11.log &
./worldserver/worldserver 12 > /log/worldserver12.log &

./gateserver/gateserver 91 > /log/gateserver91.log &
./gateserver/gateserver 92 > /log/gateserver92.log &

echo "runing ... check /log/xxx.log"
