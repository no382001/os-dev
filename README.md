```bash
docker build -t kernel-dev .

sudo docker run -it --privileged --cap-add=NET_ADMIN \
    -v $(pwd):/workspace \
    -p 5901:5901 \
    kernel-dev bash

vncviewer localhost:5901$
```