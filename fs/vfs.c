#include "vfs.h"
#include "fat16.h"
#include "libc/heap.h"
#include "libc/mem.h"
#include "libc/string.h"

/*
#undef serial_debug
#define serial_debug(...)
*/

/**
 * RAMDISK
 */
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
    kernel_printf("[x] created config.txt\n");
  } else {
    kernel_printf("[ ] failed to create config.txt\n");
  }
  int fd;
  if (ramdisk_vfs.open(&ramdisk_vfs, "config.txt", VFS_WRITE, &fd) ==
      VFS_SUCCESS) {
    char *config_data = "debug_mode=1\nmax_users=100\ntimeout=30\n";
    int64_t written =
        ramdisk_vfs.write(&ramdisk_vfs, fd, config_data, strlen(config_data));
    ramdisk_vfs.close(&ramdisk_vfs, fd);
    kernel_printf("[x] wrote %d bytes to config.txt\n", (int)written);
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
      kernel_printf("--- file contents ---\n");
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

/**
 * UNIFIED
 */

vfs g_unified_vfs = {0};
static unified_vfs_data_t g_unified_data = {0};

// parse path like "/disk/config.txt" into mount="disk", path="config.txt"
static int unified_parse_path(const char *path, char *mount_point,
                              char *relative_path) {
  if (!path || path[0] != '/') {
    serial_debug("unified: invalid path '%s'", path ? path : "NULL");
    return -1;
  }

  path++;

  if (*path == '\0') {
    strcpy(mount_point, "");
    strcpy(relative_path, "/");
    return 0;
  }

  const char *slash = strchr(path, '/');

  if (!slash) {
    strcpy(mount_point, path);
    strcpy(relative_path, "/");
    return 0;
  }

  int mount_len = slash - path;
  strncpy(mount_point, path, mount_len);
  mount_point[mount_len] = '\0';

  strcpy(relative_path, slash);

  serial_debug("unified: parsed '%s' -> mount='%s', path='%s'", path - 1,
               mount_point, relative_path);
  return 0;
}

static vfs *unified_find_target_vfs(const char *mount_point) {
  for (int i = 0; i < g_unified_data.mount_count; i++) {
    if (g_unified_data.mounts[i].active &&
        strcmp(g_unified_data.mounts[i].mount_point, mount_point) == 0) {
      return g_unified_data.mounts[i].target_vfs;
    }
  }
  return NULL;
}

static int unified_alloc_fd(vfs *target_vfs, int target_fd) {
  for (int i = 0; i < 32; i++) {
    if (!g_unified_data.fd_map[i].in_use) {
      g_unified_data.fd_map[i].unified_fd = i;
      g_unified_data.fd_map[i].target_vfs = target_vfs;
      g_unified_data.fd_map[i].target_fd = target_fd;
      g_unified_data.fd_map[i].in_use = 1;
      return i;
    }
  }
  return -1;
}

static int unified_chdir(vfs *fs, const char *path) {
  if (!fs || !path)
    return VFS_ERROR;

  serial_debug("unified: chdir to '%s'", path);

  if (strcmp(path, "/") == 0) {
    strcpy(fs->current_path, "/");
    strcpy(g_unified_data.current_path, "/");
    return VFS_SUCCESS;
  }

  char mount_point[64], relative_path[512];
  if (unified_parse_path(path, mount_point, relative_path) != 0) {
    return VFS_ENOTDIR;
  }

  if (strcmp(relative_path, "/") == 0) {
    vfs *target_vfs = unified_find_target_vfs(mount_point);
    if (!target_vfs) {
      serial_debug("unified: mount point '%s' not found", mount_point);
      return VFS_ENOTDIR;
    }

    strcpy(fs->current_path, path);
    strcpy(g_unified_data.current_path, path);
    return VFS_SUCCESS;
  }

  vfs *target_vfs = unified_find_target_vfs(mount_point);
  if (!target_vfs) {
    serial_debug("unified: mount point '%s' not found", mount_point);
    return VFS_ENOTDIR;
  }

  int result = target_vfs->chdir(target_vfs, relative_path);
  if (result == VFS_SUCCESS) {
    strcpy(fs->current_path, path);
    strcpy(g_unified_data.current_path, path);
  }

  return result;
}

static int unified_open(vfs *fs, const char *name, vfs_mode mode, int *fd) {
  if (!fs || !name || !fd)
    return VFS_ERROR;

  serial_debug("unified: open '%s'", name);

  char mount_point[64], relative_path[512];
  if (unified_parse_path(name, mount_point, relative_path) != 0) {
    return VFS_ENOENT;
  }

  if (mount_point[0] == '\0') {
    *fd = unified_alloc_fd(NULL, -1); // special case: no target VFS
    if (*fd < 0)
      return VFS_EMFILE;

    serial_debug("unified: opened root directory on fd %d", *fd);
    return VFS_SUCCESS;
  }

  vfs *target_vfs = unified_find_target_vfs(mount_point);
  if (!target_vfs) {
    serial_debug("unified: mount point '%s' not found", mount_point);
    return VFS_ENOENT;
  }

  int target_fd;
  int result = target_vfs->open(target_vfs, relative_path, mode, &target_fd);
  if (result != VFS_SUCCESS) {
    return result;
  }

  *fd = unified_alloc_fd(target_vfs, target_fd);
  if (*fd < 0) {
    target_vfs->close(target_vfs, target_fd);
    return VFS_EMFILE;
  }

  serial_debug("unified: opened '%s' -> %s:%s (unified_fd=%d, target_fd=%d)",
               name, mount_point, relative_path, *fd, target_fd);
  return VFS_SUCCESS;
}

static int unified_create(vfs *fs, const char *path, int perm) {
  if (!fs || !path)
    return VFS_ERROR;

  serial_debug("unified: create '%s'", path);

  char mount_point[64], relative_path[512];
  if (unified_parse_path(path, mount_point, relative_path) != 0) {
    return VFS_ERROR;
  }

  vfs *target_vfs = unified_find_target_vfs(mount_point);
  if (!target_vfs) {
    serial_debug("unified: mount point '%s' not found", mount_point);
    return VFS_ENOTDIR;
  }

  return target_vfs->create(target_vfs, relative_path, perm);
}

static int64_t unified_read(vfs *fs, int fd, void *buf, uint64_t count,
                            uint8_t *out, int *bytes_read) {
  if (!fs || fd < 0 || fd >= 32)
    return VFS_EBADF;

  if (!g_unified_data.fd_map[fd].in_use) {
    return VFS_EBADF;
  }

  vfs *target_vfs = g_unified_data.fd_map[fd].target_vfs;
  int target_fd = g_unified_data.fd_map[fd].target_fd;

  // special case: root directory read
  if (!target_vfs) {
    *bytes_read = 0;
    return VFS_SUCCESS;
  }

  return target_vfs->read(target_vfs, target_fd, buf, count, out, bytes_read);
}

static int64_t unified_write(vfs *fs, int fd, const void *buf, uint64_t count) {
  if (!fs || fd < 0 || fd >= 32)
    return VFS_EBADF;

  if (!g_unified_data.fd_map[fd].in_use) {
    return VFS_EBADF;
  }

  vfs *target_vfs = g_unified_data.fd_map[fd].target_vfs;
  int target_fd = g_unified_data.fd_map[fd].target_fd;

  if (!target_vfs)
    return VFS_EBADF; // can't write to root directory

  return target_vfs->write(target_vfs, target_fd, buf, count);
}

static int64_t unified_readdir(vfs *fs, int fd, vfs_stat *st, int nst) {
  if (!fs || fd < 0 || fd >= 32)
    return VFS_EBADF;

  if (!g_unified_data.fd_map[fd].in_use) {
    return VFS_EBADF;
  }

  vfs *target_vfs = g_unified_data.fd_map[fd].target_vfs;
  int target_fd = g_unified_data.fd_map[fd].target_fd;

  // special case: reading root directory "/" shows mount points
  if (!target_vfs) {
    int entries = 0;
    for (int i = 0; i < g_unified_data.mount_count && entries < nst; i++) {
      if (g_unified_data.mounts[i].active) {
        st[entries].size = 0; // directories have no size
        st[entries].type = FS_TYPE_DIRECTORY;
        st[entries].cluster = 0;
        strcpy(st[entries].name, g_unified_data.mounts[i].mount_point);
        entries++;
      }
    }
    serial_debug("unified: root readdir returned %d mount points", entries);
    return entries;
  }

  return target_vfs->readdir(target_vfs, target_fd, st, nst);
}

static int unified_close(vfs *fs, int fd) {
  if (!fs || fd < 0 || fd >= 32)
    return VFS_EBADF;

  if (!g_unified_data.fd_map[fd].in_use) {
    return VFS_EBADF;
  }

  vfs *target_vfs = g_unified_data.fd_map[fd].target_vfs;
  int target_fd = g_unified_data.fd_map[fd].target_fd;

  int result = VFS_SUCCESS;

  if (target_vfs) {
    result = target_vfs->close(target_vfs, target_fd);
  }

  g_unified_data.fd_map[fd].in_use = 0;
  g_unified_data.fd_map[fd].target_vfs = NULL;
  g_unified_data.fd_map[fd].target_fd = -1;

  serial_debug("unified: closed fd %d", fd);
  return result;
}

static int64_t unified_seek(vfs *fs, int fd, int64_t offset, int whence) {
  if (!g_unified_data.fd_map[fd].in_use)
    return VFS_EBADF;

  vfs *target_vfs = g_unified_data.fd_map[fd].target_vfs;
  int target_fd = g_unified_data.fd_map[fd].target_fd;

  if (!target_vfs)
    return VFS_EBADF;

  return target_vfs->seek(target_vfs, target_fd, offset, whence);
}

static int unified_stat(vfs *fs, const char *path, vfs_stat *st) {
  char mount_point[64], relative_path[512];
  if (unified_parse_path(path, mount_point, relative_path) != 0) {
    return VFS_ERROR;
  }

  vfs *target_vfs = unified_find_target_vfs(mount_point);
  if (!target_vfs)
    return VFS_ENOENT;

  return target_vfs->stat(target_vfs, relative_path, st);
}

static int unified_fstat(vfs *fs, int fd, vfs_stat *st) {
  if (!g_unified_data.fd_map[fd].in_use)
    return VFS_EBADF;

  vfs *target_vfs = g_unified_data.fd_map[fd].target_vfs;
  int target_fd = g_unified_data.fd_map[fd].target_fd;

  if (!target_vfs)
    return VFS_EBADF;

  return target_vfs->fstat(target_vfs, target_fd, st);
}

void unified_vfs_init(void) {
  memset(&g_unified_data, 0, sizeof(unified_vfs_data_t));
  strcpy(g_unified_data.current_path, "/");

  strcpy(g_unified_vfs.name, "unified");
  strcpy(g_unified_vfs.current_path, "/");
  g_unified_vfs.usercode = (void *)&g_unified_data;

  g_unified_vfs.chdir = unified_chdir;
  g_unified_vfs.open = unified_open;
  g_unified_vfs.create = unified_create;
  g_unified_vfs.read = unified_read;
  g_unified_vfs.readdir = unified_readdir;
  g_unified_vfs.write = unified_write;
  g_unified_vfs.seek = unified_seek;
  g_unified_vfs.stat = unified_stat;
  g_unified_vfs.fstat = unified_fstat;
  g_unified_vfs.close = unified_close;
  g_unified_vfs.remove = NULL; // TODO: implement
  g_unified_vfs.rename = NULL; // TODO: implement

  serial_debug("unified: VFS initialized");
}

int unified_mount(const char *mount_point, vfs *target_vfs) {
  if (!mount_point || !target_vfs)
    return VFS_ERROR;

  if (g_unified_data.mount_count >= UNIFIED_MAX_MOUNTS) {
    return VFS_ENOSPC;
  }

  strcpy(g_unified_data.mounts[g_unified_data.mount_count].mount_point,
         mount_point);
  g_unified_data.mounts[g_unified_data.mount_count].target_vfs = target_vfs;
  g_unified_data.mounts[g_unified_data.mount_count].active = 1;
  g_unified_data.mount_count++;

  serial_debug("unified: mounted '%s' -> %s", mount_point, target_vfs->name);
  return VFS_SUCCESS;
}

vfs *init_vfs() {
  unified_vfs_init();

  static fat_bpb_t bpb = {0};
  fat16_read_bpb(&bpb);

  fs_node_t *root = fs_build_root(&bpb);

  static vfs fat_16_vfs = {0};
  static fat16_vfs_data fat16_usercode = {0};
  fat16_usercode.root = root;
  fat16_usercode.current_dir = root;
  fat16_usercode.bpb = &bpb;

  vfs_init_fat16(&fat_16_vfs, &fat16_usercode);

  static vfs ramdisk_vfs = {0};
  static ramdisk_vfs_data ramdisk_data = {0};
  vfs_init_ramdisk(&ramdisk_vfs, &ramdisk_data);

  unified_mount("fd", &fat_16_vfs);
  unified_mount("ramdisk", &ramdisk_vfs);

  return &g_unified_vfs;
}

#define TREE_BRANCH "|-- "
#define TREE_LAST "`-- "
#define TREE_VERTICAL "|   "
#define TREE_SPACE "    "
void vfs_print_tree_recursive(vfs *fs, const char *path, char *prefix);
void vfs_print_tree_node(vfs *fs, const char *path, const char *name,
                         vfs_stat *stat, char *prefix, int is_last) {
  if (!fs || !name || !stat)
    return;

  kernel_printf("%s%s", prefix, is_last ? TREE_LAST : TREE_BRANCH);

  if (stat->type == FS_TYPE_DIRECTORY) {
    kernel_printf("%s/\n", name);
  } else {
    kernel_printf("%s (%d bytes)\n", name, stat->size);
  }

  if (stat->type == FS_TYPE_DIRECTORY) {
    char full_path[512];
    if (strcmp(path, "/") == 0) {
      sprintf(full_path, 512, "/%s", name);
    } else {
      sprintf(full_path, 512, "%s/%s", path, name);
    }

    char new_prefix[256];
    strcpy(new_prefix, prefix);
    strcat(new_prefix, is_last ? TREE_SPACE : TREE_VERTICAL);

    vfs_print_tree_recursive(fs, full_path, new_prefix);
  }
}

void vfs_print_tree_recursive(vfs *fs, const char *path, char *prefix) {
  if (!fs || !path)
    return;

  int fd;
  if (fs->open(fs, path, VFS_READ, &fd) != VFS_SUCCESS) {
    return;
  }

  vfs_stat entries[32]; // assume max 32 entries per directory
  int64_t count = fs->readdir(fs, fd, entries, 32);
  fs->close(fs, fd);

  if (count <= 0) {
    return;
  }

  for (int i = 0; i < count; i++) {
    int is_last = (i == count - 1);
    vfs_print_tree_node(fs, path, entries[i].name, &entries[i], prefix,
                        is_last);
  }
}

void vfs_print_tree(vfs *fs, const char *root_path) {
  if (!fs || !root_path)
    return;

  if (strcmp(root_path, "/") == 0) {
    kernel_printf("/\n");
    vfs_print_tree_recursive(fs, "/", "");
  } else {
    vfs_stat root_stat;
    if (fs->stat(fs, root_path, &root_stat) == VFS_SUCCESS) {
      kernel_printf("%s", root_path);
      if (root_stat.type == FS_TYPE_DIRECTORY) {
        kernel_printf("/\n");
        vfs_print_tree_recursive(fs, root_path, "");
      } else {
        kernel_printf(" (%d bytes)\n", root_stat.size);
      }
    } else {
      kernel_printf("Error: Cannot access %s\n", root_path);
    }
  }
}

void vfs_print_current_tree(vfs *fs) {
  if (!fs)
    return;

  char current_path[512];
  strcpy(current_path, fs->current_path);

  vfs_print_tree(fs, current_path);
}