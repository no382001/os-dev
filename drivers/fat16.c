#include "fat16.h"
#include "drivers/serial.h"
#include "libc/types.h"

void fat16_read_bpb(fat_bpb_t *bpb) {
  uint8_t buffer[512] = {0};
  ata_read_sector(0, buffer);
  memcpy((char *)bpb, (char *)buffer, sizeof(fat_bpb_t));

  serial_debug(" FAT16 info - %d sectors, %d bytes per sector, thats ~%dMB",
               bpb->total_sectors, bpb->bytes_per_sector,
               ((bpb->total_sectors * bpb->bytes_per_sector) / (1024 * 1024)));
}

static uint32_t fat16_cluster_to_lba(fat_bpb_t *bpb, uint16_t cluster) {
  uint32_t data_region_start = bpb->reserved_sectors +
                               (bpb->fat_count * bpb->sectors_per_fat) +
                               (bpb->root_entries * 32) / bpb->bytes_per_sector;
  return data_region_start + (cluster - 2) * bpb->sectors_per_cluster;
}

static void fat16_entry_extract_filename(fat_entry_t *e, char *out) {
  if (!e->name[0]) {
    strcpy("", out);
    return;
  }

  char name[9] = {0}, ext[4] = {0};
  memcpy(name, e->name, 8);
  memcpy(ext, e->ext, 3);

  for (int j = 7; j >= 0 && name[j] == ' '; j--)
    name[j] = '\0';
  for (int j = 2; j >= 0 && ext[j] == ' '; j--)
    ext[j] = '\0';

  if (ext[0]) {
    sprintf(out, strlen(name) + 1 + strlen(ext) + 1, "%s.%s", name,
            ext); // weird strlen offset, well...
  } else {
    strcpy(name, out);
  }
}

void fat16_list_directory(fat_bpb_t *bpb, uint16_t start_cluster, bool is_root) {
  fat_entry_t entries[512] = {0};
  uint8_t buffer[512];

  uint16_t lba;
  uint16_t dir_size;

  if (is_root) {
      // root directory is stored in a fixed region
      lba = bpb->reserved_sectors + (bpb->fat_count * bpb->sectors_per_fat);
      dir_size = (bpb->root_entries * sizeof(fat_entry_t)) / bpb->bytes_per_sector;
  } else {
      // subdirectories are stored in clusters
      if (start_cluster < 2) {
          serial_debug("ERROR: invalid directory cluster %d", start_cluster);
          return;
      }

      lba = fat16_cluster_to_lba(bpb,start_cluster);
      dir_size = 1; // each directory fits in 1 sector for now
  }

  serial_debug(" listing directory at LBA: %d (Cluster: %d, Root: %d)", lba, start_cluster, is_root);

  for (int i = 0; i < dir_size; i++) {
      ata_read_sector(lba + i, buffer);
      memcpy(((char *)entries) + (i * 512), (char*)buffer, 512);
  }

  for (int i = 0; i < bpb->root_entries; i++) {
      if (entries[i].name[0] == 0x00) break; // no more files
      if (entries[i].attributes == 0x0F) continue; // skip LFN garbage

      char full_name[13] = {0}; // 8 + 1 (.) + 3 + 1 (\0)
      fat16_entry_extract_filename(&entries[i], full_name);

      if (!strcmp(full_name,".") || !strcmp(full_name,"..")){
        continue;
      }

      if (entries[i].attributes & 0x10) {
          serial_debug(" [DIR] %s", full_name);
          fat16_list_directory(bpb, entries[i].start_cluster, false);
      } else {
          serial_debug(" file: %s, size: %d bytes, cluster: %d",
                       full_name, entries[i].file_size, entries[i].start_cluster);
      }
  }
}

void fat16_list_root(fat_bpb_t *bpb) {
  fat16_list_directory(bpb, 0, true);
}