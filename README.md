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

```bash
# build the docker container
docker build -t os-dev .

# run the container
sudo docker run -it --privileged --cap-add=NET_ADMIN \
    -v $(pwd):/workspace \
    -p 5901:5901 \
    os-dev bash

# setup the network inside the container
chmod +x ./dhcp.sh && ./dhcp.sh

# build and run emulation
make vnc
```

```bash
# attach from the outside
vncviewer localhost:5901
```
