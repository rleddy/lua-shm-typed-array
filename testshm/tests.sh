#!/bin/sh

./lshmtester   &
sleep 2; ./lshmtester2  &

wait
echo all processes complete
