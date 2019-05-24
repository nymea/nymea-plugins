#!/bin/bash

# Install neuron_tcp_server 1.0.1
wget https://github.com/UniPiTechnology/neuron-tcp-modbus-overlay/archive/v1.0.1.zip
unzip v1.0.1.zip
cd neuron-tcp-modbus-overlay-1.0.1
yes n | bash $PWD/install.sh
cd ..
