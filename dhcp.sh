ip tuntap add dev tap0 mode tap
ip addr add 192.168.100.1/24 dev tap0
ip link set tap0 up

dnsmasq --interface=tap0 --dhcp-range=192.168.100.10,192.168.100.50,12h --log-dhcp --no-daemon &