#!/bin/sh


echo 0 | sudo tee /sys/kernel/debug/bluetooth/hci0/conn_latency
echo 6 | sudo tee /sys/kernel/debug/bluetooth/hci0/conn_min_interval
echo 7 | sudo tee /sys/kernel/debug/bluetooth/hci0/conn_max_interval



hcitool con
s=` hcitool con|grep handle|sed -e 's/.*handle //' | cut -d ' ' -f1 `
echo 'Handles:' $s

for HANDLE in $s; do
  echo hcitool lecup --handle $HANDLE --latency 0 --min 6 --max 9
  sudo hcitool lecup --handle $HANDLE --latency 0 --min 6 --max 9
  sudo hcitool lecup --handle $HANDLE --latency 0 --min 6 --max 8
done



