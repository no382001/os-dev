#include "vfs.h"
#include "fat16.h"
#include "libc/mem.h"
#include "libc/string.h"

static ramdisk_file_t *ramdisk_find_file(ramdisk_vfs_data *data,
                                         const char *name) {
  for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (data->files[i].in_use && strcmp(data->files[i].name, name) == 0) {
      return &data->files[i];
    }
  }
  return NULL;
}

static ramdisk_file_t *ramdisk_alloc_file(ramdisk_vfs_data *data) {
  for (int i = 0; i < RAMDISK_MAX_FILES; i++) {
    if (!data->files[i].in_use) {
      memset(&data->files[i], 0, sizeof(ramdisk_file_t));
      data->files[i].in_use = 1;
      return &data->files[i];
    }
  }
  return NULL;
}

static int ramdisk_alloc_fd(ramdisk_vfs_data *data) {
  for (int i = 0; i < 32; i++) {
    if (!data->fd_table[i].in_use) {
      memset(&data->fd_table[i], 0, sizeof(ramdisk_fd_t));
      data->fd_table[i].in_use = 1;
      data->fd_table[i].fd = i;
      return i;
    }
  }
  return -1;
}

static int ramdisk_create(vfs *fs, const char *path, int perm) {
  if (!fs || !path)
    return VFS_ERROR;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  if (ramdisk_find_file(data, path)) {
    return VFS_EEXIST;
  }

  ramdisk_file_t *file = ramdisk_alloc_file(data);
  if (!file) {
    return VFS_EMFILE;
  }

  strcpy(file->name, path);
  file->type = FS_TYPE_FILE;
  file->size = 0;
  file->capacity = 0;
  file->data = NULL;

  serial_debug("ramdisk: created file '%s'", path);
  return VFS_SUCCESS;
}
static int ramdisk_open(vfs *fs, const char *name, vfs_mode mode, int *fd) {
  if (!fs || !name || !fd) {
    serial_debug("ramdisk: preliminary checks failed on `%s`", name);
    return VFS_ERROR;
  }

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  if (strcmp(name, "/") == 0) {
    *fd = ramdisk_alloc_fd(data);
    if (*fd < 0) {
      return VFS_EMFILE;
    }

    data->fd_table[*fd].file = NULL;
    data->fd_table[*fd].mode = mode;
    data->fd_table[*fd].position = 0;

    serial_debug("ramdisk: opened directory '/' on fd %d", *fd);
    return VFS_SUCCESS;
  }

  ramdisk_file_t *file = ramdisk_find_file(data, name);
  if (!file) {
    serial_debug("ramdisk: file not found `%s`", name);
    return VFS_ENOENT;
  }

  *fd = ramdisk_alloc_fd(data);
  if (*fd < 0) {
    return VFS_EMFILE;
  }

  data->fd_table[*fd].file = file;
  data->fd_table[*fd].mode = mode;
  data->fd_table[*fd].position = 0;

  serial_debug("ramdisk: opened '%s' on fd %d", name, *fd);
  return VFS_SUCCESS;
}

static int ramdisk_close(vfs *fs, int fd) {
  if (!fs || fd < 0 || fd >= 32)
    return VFS_EBADF;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  if (!data->fd_table[fd].in_use) {
    return VFS_EBADF;
  }

  data->fd_table[fd].in_use = 0;
  data->fd_table[fd].file = NULL;
  data->fd_table[fd].position = 0;

  serial_debug("ramdisk: closed fd %d", fd);
  return VFS_SUCCESS;
}

static int64_t ramdisk_read(vfs *fs, int fd, void *buf, uint64_t count,
                            uint8_t *unused, int *bytes_read) {
  if (!fs || fd < 0 || fd >= 32 || !buf || !bytes_read)
    return VFS_EBADF;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  if (!data->fd_table[fd].in_use) {
    return VFS_EBADF;
  }

  ramdisk_fd_t *file_desc = &data->fd_table[fd];
  ramdisk_file_t *file = file_desc->file;

  if (!file || !file->data) {
    *bytes_read = 0;
    return VFS_SUCCESS;
  }

  // calculate how much we can read
  uint64_t remaining = file->size - file_desc->position;
  if (count > remaining) {
    count = remaining;
  }

  if (count > 0) {
    memcpy(buf, file->data + file_desc->position, count);
    file_desc->position += count;
  }

  *bytes_read = count;
  serial_debug("ramdisk: read %d bytes from fd %d", (int)count, fd);
  return VFS_SUCCESS;
}

static int64_t ramdisk_write(vfs *fs, int fd, const void *buf, uint64_t count) {
  if (!fs || fd < 0 || fd >= 32 || !buf)
    return VFS_EBADF;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  if (!data->fd_table[fd].in_use) {
    return VFS_EBADF;
  }

  ramdisk_fd_t *file_desc = &data->fd_table[fd];
  ramdisk_file_t *file = file_desc->file;

  if (!file) {
    return VFS_EBADF;
  }

  if (!(file_desc->mode & VFS_WRITE)) {
    return VFS_EACCESS;
  }

  uint32_t new_size = file_desc->position + count;

  if (new_size > RAMDISK_MAX_FILESIZE) {
    return VFS_ENOSPC;
  }

  if (new_size > file->capacity) {
    uint32_t new_capacity =
        ((new_size + 1023) / 1024) * 1024; // round up to 1KB
    if (new_capacity > RAMDISK_MAX_FILESIZE) {
      new_capacity = RAMDISK_MAX_FILESIZE;
    }

    uint8_t *new_data = (uint8_t *)kmalloc(new_capacity);
    if (!new_data) {
      return VFS_ENOMEM;
    }

    // copy existing data
    if (file->data && file->size > 0) {
      memcpy(new_data, file->data, file->size);
      kfree(file->data);
    }

    file->data = new_data;
    file->capacity = new_capacity;
  }

  memcpy(file->data + file_desc->position, buf, count);
  file_desc->position += count;

  if (file_desc->position > file->size) {
    file->size = file_desc->position;
  }

  serial_debug("ramdisk: wrote %d bytes to fd %d", (int)count, fd);
  return count;
}

static int64_t ramdisk_seek(vfs *fs, int fd, int64_t offset, int whence) {
  if (!fs || fd < 0 || fd >= 32)
    return VFS_EBADF;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  if (!data->fd_table[fd].in_use) {
    return VFS_EBADF;
  }

  ramdisk_fd_t *file_desc = &data->fd_table[fd];
  ramdisk_file_t *file = file_desc->file;

  if (!file) {
    return VFS_EBADF;
  }

  int64_t new_pos;
  switch (whence) {
  case SEEK_SET:
    new_pos = offset;
    break;
  case SEEK_CUR:
    new_pos = file_desc->position + offset;
    break;
  case SEEK_END:
    new_pos = file->size + offset;
    break;
  default:
    return VFS_ERROR;
  }

  if (new_pos < 0)
    new_pos = 0;
  if (new_pos > file->size)
    new_pos = file->size;

  file_desc->position = new_pos;
  serial_debug("ramdisk: seek fd %d to position %d", fd, (int)new_pos);
  return new_pos;
}

static int ramdisk_stat(vfs *fs, const char *path, vfs_stat *st) {
  if (!fs || !path || !st)
    return VFS_ERROR;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  ramdisk_file_t *file = ramdisk_find_file(data, path);
  if (!file) {
    return VFS_ENOENT;
  }

  st->size = file->size;
  st->type = file->type;
  st->cluster = 0; // not applicable for ramdisk
  strcpy(st->name, file->name);

  return VFS_SUCCESS;
}

static int ramdisk_remove(vfs *fs, const char *name) {
  if (!fs || !name)
    return VFS_ERROR;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  ramdisk_file_t *file = ramdisk_find_file(data, name);
  if (!file) {
    return VFS_ENOENT;
  }

  if (file->data) {
    kfree(file->data);
  }

  file->in_use = 0;

  serial_debug("ramdisk: removed file '%s'", name);
  return VFS_SUCCESS;
}

static int ramdisk_chdir(vfs *fs, const char *path) {
  serial_debug("ramdisk: chdir unimplemented, this fs is flat!");
  return VFS_SUCCESS; // its flat
}

static int64_t ramdisk_readdir(vfs *fs, int fd, vfs_stat *st, int nst) {
  if (!fs || fd < 0 || fd >= 32 || !st || nst <= 0)
    return VFS_EBADF;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  if (!data->fd_table[fd].in_use) {
    return VFS_EBADF;
  }

  int entries_returned = 0;

  for (int i = 0; i < RAMDISK_MAX_FILES && entries_returned < nst; i++) {
    if (data->files[i].in_use) {
      st[entries_returned].size = data->files[i].size;
      st[entries_returned].type = data->files[i].type;
      st[entries_returned].cluster = 0;
      strcpy(st[entries_returned].name, data->files[i].name);
      entries_returned++;
    }
  }

  serial_debug("ramdisk: readdir returned %d entries", entries_returned);
  return entries_returned;
}

static int ramdisk_fstat(vfs *fs, int fd, vfs_stat *st) {
  if (!fs || fd < 0 || fd >= 32 || !st)
    return VFS_EBADF;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  if (!data->fd_table[fd].in_use) {
    return VFS_EBADF;
  }

  ramdisk_file_t *file = data->fd_table[fd].file;
  if (!file) {
    return VFS_EBADF;
  }

  st->size = file->size;
  st->type = file->type;
  st->cluster = 0;
  strcpy(st->name, file->name);

  serial_debug("ramdisk: fstat for fd %d: '%s', size=%d", fd, file->name,
               file->size);
  return VFS_SUCCESS;
}

static int ramdisk_rename(vfs *fs, const char *oldpath, const char *newpath) {
  if (!fs || !oldpath || !newpath)
    return VFS_ERROR;

  ramdisk_vfs_data *data = (ramdisk_vfs_data *)fs->usercode;

  ramdisk_file_t *file = ramdisk_find_file(data, oldpath);
  if (!file) {
    serial_debug("ramdisk: rename failed - '%s' not found", oldpath);
    return VFS_ENOENT;
  }

  if (ramdisk_find_file(data, newpath)) {
    serial_debug("ramdisk: rename failed - '%s' already exists", newpath);
    return VFS_EEXIST;
  }

  if (strlen(newpath) >= RAMDISK_MAX_FILENAME) {
    serial_debug("ramdisk: rename failed - new name too long");
    return VFS_ERROR;
  }

  strcpy(file->name, newpath);

  serial_debug("ramdisk: renamed '%s' to '%s'", oldpath, newpath);
  return VFS_SUCCESS;
}

void vfs_init_ramdisk(vfs *fs, ramdisk_vfs_data *data) {
  if (!fs || !data)
    return;

  memset(data, 0, sizeof(ramdisk_vfs_data));
  strcpy(data->current_path, "/");

  strcpy(fs->name, "ramdisk");
  strcpy(fs->current_path, "/");
  fs->usercode = (void *)data;

  fs->chdir = ramdisk_chdir; // its flat
  fs->open = ramdisk_open;
  fs->create = ramdisk_create;
  fs->read = ramdisk_read;
  fs->readdir = ramdisk_readdir;
  fs->write = ramdisk_write;
  fs->seek = ramdisk_seek;
  fs->stat = ramdisk_stat;
  fs->fstat = ramdisk_fstat;
  fs->close = ramdisk_close;
  fs->remove = ramdisk_remove;
  fs->rename = ramdisk_rename;

  serial_debug("ramdisk: initialized");
}

void ramdisk_demo() {

  vfs ramdisk_vfs = {0};
  ramdisk_vfs_data ramdisk_data = {0};
  vfs_init_ramdisk(&ramdisk_vfs, &ramdisk_data);

  if (ramdisk_vfs.create(&ramdisk_vfs, "config.txt", 0) == VFS_SUCCESS) {
    kernel_printf("[x] Created config.txt\n");
  } else {
    kernel_printf("[ ] Failed to create config.txt\n");
  }
  int fd;
  if (ramdisk_vfs.open(&ramdisk_vfs, "config.txt", VFS_WRITE, &fd) ==
      VFS_SUCCESS) {
    char *config_data = "debug_mode=1\nmax_users=100\ntimeout=30\n";
    int64_t written =
        ramdisk_vfs.write(&ramdisk_vfs, fd, config_data, strlen(config_data));
    ramdisk_vfs.close(&ramdisk_vfs, fd);
    kernel_printf("[x] Wrote %d bytes to config.txt\n", (int)written);
  } else {
    kernel_printf("[ ] failed to write to config.txt");
  }

  if (ramdisk_vfs.open(&ramdisk_vfs, "config.txt", VFS_WRITE, &fd) ==
      VFS_SUCCESS) {
    char *config_data = "debug_mode=1\nmax_users=100\ntimeout=30\n";
    int64_t written =
        ramdisk_vfs.write(&ramdisk_vfs, fd, config_data, strlen(config_data));
    ramdisk_vfs.close(&ramdisk_vfs, fd);
    kernel_printf("[x] Wrote %d bytes to config.txt again\n", (int)written);
  } else {
    kernel_printf("[ ] failed to write to config.txt");
  }

  if (ramdisk_vfs.open(&ramdisk_vfs, "/", VFS_READ, &fd) == VFS_SUCCESS) {
    vfs_stat entries[10];
    int64_t count = ramdisk_vfs.readdir(&ramdisk_vfs, fd, entries, 10);

    kernel_printf("found %d files:\n", (int)count);
    for (int i = 0; i < count; i++) {
      kernel_printf("  %s - %d bytes\n", entries[i].name, entries[i].size);
    }
    ramdisk_vfs.close(&ramdisk_vfs, fd);
  } else {
    kernel_printf("[ ] failed to open /");
  }

  if (ramdisk_vfs.open(&ramdisk_vfs, "config.txt", VFS_READ, &fd) ==
      VFS_SUCCESS) {
    char buffer[256] = {0};
    int bytes_read;

    int64_t result = ramdisk_vfs.read(&ramdisk_vfs, fd, buffer,
                                      sizeof(buffer) - 1, NULL, &bytes_read);
    ramdisk_vfs.close(&ramdisk_vfs, fd);

    if (result == VFS_SUCCESS) {
      kernel_printf("[x] read %d bytes from config.txt:\n", bytes_read);
      kernel_printf("--- file Contents ---\n");
      kernel_printf("%s", buffer);
      kernel_printf("--- end of File ---\n");
    } else {
      kernel_printf("[ ] failed to read from config.txt (error %d)\n",
                    (int)result);
    }
  } else {
    kernel_printf("[ ] failed to open config.txt for reading\n");
  }
}