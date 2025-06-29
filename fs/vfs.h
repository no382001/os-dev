#pragma once
#include "fat16.h"

typedef struct vfs vfs;

typedef enum {
  VFS_READ = 1,
  VFS_WRITE = 2,
  VFS_RDWR = 3,
  VFS_CREATE = 4,
  VFS_APPEND = 8
} vfs_mode;

typedef struct {
  uint32_t size;
  fs_node_type_t type;
  uint16_t cluster;
  char name[256];
} vfs_stat;

typedef struct {
  int fd;
  fs_node_t *node;
  uint32_t position;
  vfs_mode mode;
  int in_use;
} fat16_fd_t;

typedef struct {
  fat_bpb_t *bpb;
  fs_node_t *root;
  fs_node_t *current_dir;
  fat16_fd_t fd_table[32];
} fat16_vfs_data;

typedef struct vfs {
  char name[32];
  char current_path[512];
  void *usercode;

  int (*chdir)(vfs *fs, const char *path);
  int (*open)(vfs *fs, const char *name, vfs_mode mode, int *fd);
  int (*create)(vfs *fs, const char *path, int perm);
  int64_t (*read)(vfs *fs, int fd, void *buf, uint64_t count, uint8_t *out,
                  int *bytes_read);
  int64_t (*readdir)(vfs *fs, int fd, vfs_stat *st, int nst);
  int64_t (*write)(vfs *fs, int fd, const void *buf, uint64_t count);
  int64_t (*seek)(vfs *fs, int fd, int64_t offset, int whence);
  int (*stat)(vfs *fs, const char *path, vfs_stat *st);
  int (*fstat)(vfs *fs, int fd, vfs_stat *st);
  int (*close)(vfs *fs, int fd);
  int (*remove)(vfs *fs, const char *name);
  int (*rename)(vfs *fs, const char *oldpath, const char *newpath);
} vfs;

typedef enum {
  VFS_SUCCESS = 0,
  VFS_ERROR,
  VFS_ENOENT,
  VFS_EBADF,
  VFS_EEXIST,
  VFS_EACCESS,
  VFS_EMFILE,
  VFS_EISDIR,
  VFS_ENOTDIR,
  VFS_ENOMEM,
  VFS_ENOSPC,
} vfs_err_t;

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

void vfs_init_fat16(vfs *fs, fat16_vfs_data *code);

#define RAMDISK_MAX_FILES 128
#define RAMDISK_MAX_FILENAME 64
#define RAMDISK_MAX_FILESIZE (64 * 1024) // 64KB per file

typedef struct {
  char name[RAMDISK_MAX_FILENAME];
  uint8_t *data;
  uint32_t size;
  uint32_t capacity;
  int in_use;
  fs_node_type_t type;
} ramdisk_file_t;

typedef struct {
  int fd;
  ramdisk_file_t *file;
  uint32_t position;
  vfs_mode mode;
  int in_use;
} ramdisk_fd_t;

typedef struct {
  ramdisk_file_t files[RAMDISK_MAX_FILES];
  ramdisk_fd_t fd_table[32];
  char current_path[512];
} ramdisk_vfs_data;

void vfs_init_ramdisk(vfs *fs, ramdisk_vfs_data *data);