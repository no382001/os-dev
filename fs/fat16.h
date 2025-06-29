#pragma once
#include "libc/types.h"

typedef struct {
  uint8_t jump[3];
  char oem_name[8];
  uint16_t bytes_per_sector;
  uint8_t sectors_per_cluster;
  uint16_t reserved_sectors;
  uint8_t fat_count;
  uint16_t root_entries;
  uint16_t total_sectors;
  uint8_t media_descriptor;
  uint16_t sectors_per_fat;
  uint16_t sectors_per_track;
  uint16_t heads;
  uint32_t hidden_sectors;
  uint32_t large_sector_count;
} __attribute__((packed)) fat_bpb_t;
// bios parameter block

typedef struct {
  char name[8];
  char ext[3];
  uint8_t attributes;
  uint8_t reserved;
  uint8_t create_time;
  uint16_t create_time_fine;
  uint16_t create_date;
  uint16_t last_access;
  uint16_t high_cluster;
  uint16_t mod_time;
  uint16_t mod_date;
  uint16_t start_cluster;
  uint32_t file_size;
} __attribute__((packed)) fat_entry_t;

void fat16_read_bpb(fat_bpb_t *bpb);

typedef enum { FS_TYPE_FILE, FS_TYPE_DIRECTORY } fs_node_type_t;

typedef struct {
  uint8_t order;          // order of this entry in sequence
  uint16_t name1[5];      // first 5 UTF-16 characters
  uint8_t attributes;     // always 0x0F for LFN
  uint8_t type;           // always 0x00 for LFN
  uint8_t checksum;       // checksum of short name
  uint16_t name2[6];      // next 6 UTF-16 characters
  uint16_t start_cluster; // always 0x0000 for LFN
  uint16_t name3[2];      // last 2 UTF-16 characters
} __attribute__((packed)) lfn_entry_t;

typedef struct fs_node_t fs_node_t;
struct fs_node_t {
  char name[13];
  char long_name[256]; // utf8
  uint32_t size;
  uint16_t start_cluster;
  fs_node_type_t type;
  fs_node_t *parent;
  fs_node_t *children;
  fs_node_t *next;
};

fs_node_t *fs_build_tree(fat_bpb_t *bpb, uint16_t start_cluster,
                         fs_node_t *parent, int is_root);
fs_node_t *fs_find_file(fs_node_t *root, const char *name);
void fs_read_file(fat_bpb_t *bpb, fs_node_t *file, uint8_t *buffer);
void fs_print_tree(fs_node_t *node, char *prefix, int is_last);
void fs_print_tree_list(fs_node_t *root);
void fs_cleanup(fs_node_t *node);

uint8_t lfn_calculate_checksum(const char *short_name);
int lfn_extract_name_part(lfn_entry_t *lfn, char *output, int part);
int fat16_read_long_filename(fat_entry_t *entries, int start_index,
                             char *long_name);
void fat16_entry_extract_filename_lfn(fat_entry_t *entries, int index,
                                      char *short_name, char *long_name);

#define LFN_LAST_ENTRY 0x40
#define LFN_DELETED 0xE5
#define LFN_ATTR 0x0F
#define MAX_LFN_LENGTH 255