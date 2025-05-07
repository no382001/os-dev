# install u9fs, and c9

sudo socat TCP-LISTEN:564,fork EXEC:"u9fs -a none -D -l u9fs.log -u nobody ./disk/root"

# dial tcp!127.0.0.1!564 in c9 with fs9pc:191 nobody, or none
# fstat "/"