# 9p

9P2000 client/server for the kernel.

`proto.[ch]` is adapted from [~ft/c9](https://git.sr.ht/~ft/c9) (public domain) —
a lightweight, zero-malloc, no-POSIX 9p library targeting low-resource MCUs.
Modified to use kernel types (`libc/types.h`, `libc/mem.h`, `libc/string.h`)
instead of the C standard library.

`server.[ch]` wires the server side of proto to the unified VFS,
listening on UDP port 9999.
