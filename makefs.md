
```bash
dd if=/dev/zero of=fat16.img bs=1M count=10
#10mb is the minimum for fat16
```
```bash
mkfs.fat -F 16 -S 512 fat16.img
```
```bash
sudo mkdir -p /mnt/fat16
sudo mount -o loop fat16.img /mnt/fat16
```