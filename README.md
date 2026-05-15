## what has

**memory** - paging, heap allocator, placement allocator for early boot

**filesystem** - unified VFS, ramdisk, FAT16

**networking** - RTL8139 driver, ARP, IP, ICMP, UDP, DHCP

**9P server** - kernel VFS exposed over 9P2000/UDP on port 9999; host can mount and read/write it

**multitasking** - preemptive round-robin scheduler, semaphores

**prolog repl** - embedded [trilog](https://github.com/no382001/trilog) interpreter with FFI; with some predicates to interact with the system

**display** - 80×25 text mode, VGA 640×480 16-color, BDF font rendering

## building
### locally
```bash
apt install build-essential nasm qemu-system-x86 gdb clang-format tcpdump dnsmasq bridge-utils iproute2 net-tools procps mtools dosfstools
git clone --recurse-submodules <repo>
make
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
connected - walking /

d  fd  (0 bytes)
  -  CORE.PL  (1387 bytes)
  d  FONTS  (0 bytes)
      -  VIII.BDF  (19389 bytes)
      -  tom-thumb.bdf  (19864 bytes)
d  ramdisk  (0 bytes)
```

See `tools/c9/prolog/p9.pl` for the Prolog API (`connect/4`, `ls/3`, `read_file/3`, `write_file/3`, `stat/3`).
