#pragma once
#include "bits.h"
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
void fat16_list_directory(fat_bpb_t *bpb, uint16_t start_cluster, bool is_root);
void fat16_list_root(fat_bpb_t *bpb);