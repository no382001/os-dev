ip tuntap add dev tap0 mode tap
ip link set dev tap0 up
ip addr add 192.168.100.1/24 dev tap0