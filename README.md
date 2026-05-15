## features
`+` done <br>
`~` functional, could be improved

| category              | component                 | status |                           |
| --------------------- | ------------------------- | ------ | ------------------------- |
| **memory management** | paging                    | +      | virtual memory support    |
|                       | dynamic memory allocation | ~      | heap management           |
| **file system**       | fat16                     | +      |                           |
| **input/output**      | keyboard                  | +      | keyboard input handling   |
|                       | serial                    | +      | serial communication      |
| **graphics**          | text mode (03h)           | +      | 80x25 text display        |
|                       | vga mode (12h)            | +      | 640x480 16-color graphics |
|                       | bdf font rendering        | ~      | bdf font support          |
| **networking**        | arp protocol              | +      |                           |
|                       | ip protocol               | +      |                           |
|                       | dhcp                      | +      | can get dynamic ip        |
|                       | icmp protocol             | +      | can be pinged             |
|                       | udp protocol              | +      |                           |
|                       | rtl8139 driver            | +      | network card              |
| **system**            | multitasking              | ~      |                           |

## building
### locally
```bash
apt install build-essential nasm qemu-system-x86 gdb clang-format tcpdump dnsmasq bridge-utils iproute2 net-tools procps mtools dosfstools
```
### docker
```bash
# build the docker container
docker build -t os-dev .

# run the container
sudo docker run -it --privileged --cap-add=NET_ADMIN \
    -v $(pwd):/workspace \
    -p 5901:5901 \
    os-dev bash

# setup the network inside the container
bash tools/dhcp.sh

# build and run emulation
make vnc
```

```bash
# attach from the outside
vncviewer localhost:5901
```

## vfs over 9p

The kernel exposes its VFS over 9P2000 on UDP port 9999. `tools/c9` contains a
C bridge and SWI-Prolog client library for talking to it from the host.

```bash
# build the os-dev image first (if not already done)
docker build -t os-dev .

# start the kernel, bridge, and a treewalk of the VFS
docker compose -f tools/c9/docker-compose.yml up
```

The treewalk walks the entire VFS and prints the directory tree:

```
connected — walking /

d  fd  (0 bytes)
  -  CORE.PL  (1387 bytes)
  d  FONTS  (0 bytes)
      -  VIII.BDF  (19389 bytes)
      -  tom-thumb.bdf  (19864 bytes)
d  ramdisk  (0 bytes)
```

See `tools/c9/prolog/p9.pl` for the Prolog API (`connect/4`, `ls/3`, `read_file/3`, `write_file/3`, `stat/3`).
