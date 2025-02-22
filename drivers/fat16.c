#include "fat16.h"
#include "drivers/low_level.h"
#include "drivers/serial.h"
#include "libc/mem.h"
#include "libc/string.h"
#include "libc/types.h"

#undef serial_debug
#define serial_debug(...)

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

fs_node_t *fs_build_tree(fat_bpb_t *bpb, uint16_t start_cluster,
                         fs_node_t *parent, int is_root) {
  fs_node_t *head = NULL;
  fs_node_t *prev = NULL;

  fat_entry_t entries[512];
  uint8_t buffer[512];

  uint16_t lba;
  uint16_t dir_size;

  if (is_root) {
    lba = bpb->reserved_sectors + (bpb->fat_count * bpb->sectors_per_fat);
    dir_size =
        (bpb->root_entries * sizeof(fat_entry_t)) / bpb->bytes_per_sector;
  } else {
    lba = fat16_cluster_to_lba(bpb, start_cluster);
    dir_size = 1;
  }

  serial_debug("building directory at LBA: %d (Cluster: %d, Root: %d)", lba,
               start_cluster, is_root);

  for (int i = 0; i < dir_size; i++) {
    ata_read_sector(lba + i, buffer);
    memcpy((char *)entries + (i * 512), (char *)buffer, 512);
  }

  for (int i = 0; i < bpb->root_entries; i++) {
    if (entries[i].name[0] == 0x00)
      break;
    if (entries[i].attributes == 0x0F)
      continue;

    fs_node_t *node = malloc(sizeof(fs_node_t));
    memset((uint8_t *)node, 0, sizeof(fs_node_t));

    char full_name[13] = {0};
    fat16_entry_extract_filename(&entries[i], full_name);
    if (!strcmp(full_name, ".") || !strcmp(full_name, "..")) {
      continue;
    }
    memcpy(node->name, full_name, 13);

    node->size = entries[i].file_size;
    node->start_cluster = entries[i].start_cluster;
    node->type =
        (entries[i].attributes & 0x10) ? FS_TYPE_DIRECTORY : FS_TYPE_FILE;
    node->parent = parent;

    if (entries[i].attributes & 0x10) {
      serial_debug(" [DIR] %s", full_name);
    } else {
      serial_debug(" file: %s, size: %d bytes, cluster: %d", full_name,
                   entries[i].file_size, entries[i].start_cluster);
    }

    if (node->type == FS_TYPE_DIRECTORY) {
      node->children = fs_build_tree(bpb, node->start_cluster, node, false);
    }

    if (!head) {
      head = node;
    } else {
      prev->next = node;
    }
    prev = node;
  }
  return head;
}

fs_node_t *fs_find_file(fs_node_t *root, const char *name) {
  fs_node_t *current = root;
  while (current) {
    if (!strcmp(current->name, name)) {
      return current;
    }
    if (current->type == FS_TYPE_DIRECTORY) {
      fs_node_t *found = fs_find_file(current->children, name);
      if (found)
        return found;
    }
    current = current->next;
  }
  return NULL;
}

void fs_read_file(fat_bpb_t *bpb, fs_node_t *file, uint8_t *out) {
  uint16_t lba = fat16_cluster_to_lba(bpb, file->start_cluster);
  int read = (file->size / 512) + 1;
  for (uint8_t i = 0; i < read; i++) {
    ata_read_sector(lba + i, (uint8_t *)(out + (i * 512)));
  }
}

void fs_print_tree(fs_node_t *node, int depth) {
  if (!node)
    return;

  for (int i = 0; i < depth; i++) {
    kernel_printf("|   ");
  }

  if (node->type == FS_TYPE_DIRECTORY) {
    kernel_printf("|-- %s\n", node->name);
  } else {
    kernel_printf("|-- %s (%d bytes)\n", node->name, node->size);
  }

  fs_print_tree(node->children, depth + 1);
  fs_print_tree(node->next, depth);
}
