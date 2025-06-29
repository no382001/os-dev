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

void fs_read_file(fat_bpb_t *bpb, fs_node_t *file, uint8_t *out) {
  uint16_t lba = fat16_cluster_to_lba(bpb, file->start_cluster);
  int read = (file->size / 512) + 1;
  for (uint8_t i = 0; i < read; i++) {
    ata_read_sector(lba + i, (uint8_t *)(out + (i * 512)));
  }
}

#define TREE_BRANCH "|-- "
#define TREE_LAST "`-- "
#define TREE_VERTICAL "|   "
#define TREE_SPACE "    "

void fs_print_tree_list(fs_node_t *root) {
  if (!root) {
    kernel_printf("empty filesystem tree\n");
    return;
  }

  kernel_printf("filesystem tree:\n");

  fs_node_t *current = root;

  int root_count = 0;
  fs_node_t *temp = current;
  while (temp) {
    root_count++;
    temp = temp->next;
  }

  int current_node = 0;
  while (current) {
    current_node++;
    fs_print_tree(current, "", current_node == root_count);
    current = current->next;
  }
}

void fs_cleanup(fs_node_t *node) {
  if (!node) {
    return;
  }

  if (node->type == FS_TYPE_DIRECTORY && node->children) {
    fs_cleanup(node->children);
  }

  fs_node_t *next = node->next;

  kfree(node);

  fs_cleanup(next);
}

uint8_t lfn_calculate_checksum(const char *short_name) {
  uint8_t checksum = 0;
  for (int i = 0; i < 11; i++) {
    checksum = ((checksum & 1) << 7) + (checksum >> 1) + short_name[i];
  }
  return checksum;
}

// extract name part from LFN entry and convert UTF-16 to simple ASCII
int lfn_extract_name_part(lfn_entry_t *lfn, char *output, int part) {
  uint16_t temp_buffer[6];
  int count;

  switch (part) {
  case 1:
    memcpy(temp_buffer, lfn->name1, sizeof(lfn->name1));
    count = 5;
    break;
  case 2:
    memcpy(temp_buffer, lfn->name2, sizeof(lfn->name2));
    count = 6;
    break;
  case 3:
    memcpy(temp_buffer, lfn->name3, sizeof(lfn->name3));
    count = 2;
    break;
  default:
    return 0;
  }

  int chars_written = 0;
  for (int i = 0; i < count; i++) {
    if (temp_buffer[i] == 0x0000 || temp_buffer[i] == 0xFFFF) {
      break; // End of name or padding
    }

    // simple UTF-16 to ASCII conversion (only handle basic ASCII)
    if (temp_buffer[i] < 0x80) {
      output[chars_written++] = (char)temp_buffer[i];
    } else {
      output[chars_written++] = '?'; // replace non-ASCII with ?
    }
  }

  return chars_written;
}

int fat16_read_long_filename(fat_entry_t *entries, int start_index,
                             char *long_name) {
  char temp_name[256] = {0};
  int name_pos = 0;
  int current_index = start_index;

  // read LFN entries in reverse order
  while (current_index >= 0 && entries[current_index].attributes == LFN_ATTR) {
    lfn_entry_t *lfn = (lfn_entry_t *)&entries[current_index];

    char part1[16] = {0}, part2[16] = {0}, part3[16] = {0};
    int len1 = lfn_extract_name_part(lfn, part1, 1);
    int len2 = lfn_extract_name_part(lfn, part2, 2);
    int len3 = lfn_extract_name_part(lfn, part3, 3);

    char name_part[32] = {0};
    memcpy(name_part, part1, len1);
    memcpy(name_part + len1, part2, len2);
    memcpy(name_part + len1 + len2, part3, len3);

    // prepend to temp_name (since entries are in reverse order)
    int part_len = len1 + len2 + len3;
    memmove(temp_name + part_len, temp_name, name_pos);
    memcpy(temp_name, name_part, part_len);
    name_pos += part_len;

    // check if this is the last (first) LFN entry
    if (lfn->order & LFN_LAST_ENTRY) {
      break;
    }

    current_index--;
  }

  memcpy(long_name, temp_name, name_pos);
  long_name[name_pos] = '\0';

  return name_pos > 0 ? current_index + 1 : -1; // return index of 8.3 entry
}
void fat16_entry_extract_filename_lfn(fat_entry_t *entries, int index,
                                      char *short_name, char *long_name) {
  fat_entry_t *e = &entries[index];

  if (!e->name[0]) {
    strcpy(short_name, "");
    strcpy(long_name, "");
    return;
  }

  char name[9] = {0}, ext[4] = {0};
  memcpy(name, e->name, 8);
  memcpy(ext, e->ext, 3);

  // remove trailing spaces
  for (int j = 7; j >= 0 && name[j] == ' '; j--)
    name[j] = '\0';
  for (int j = 2; j >= 0 && ext[j] == ' '; j--)
    ext[j] = '\0';

  // build short name
  if (ext[0]) {
    sprintf(short_name, 64, "%s.%s", name, ext);
  } else {
    strcpy(short_name, name);
  }

  // try to read long filename
  long_name[0] = '\0';
  if (index > 0) {
    // check if previous entries are LFN entries
    int lfn_start = index - 1;
    while (lfn_start >= 0 && entries[lfn_start].attributes == LFN_ATTR) {
      lfn_start--;
    }
    lfn_start++; // move to first LFN entry

    if (lfn_start < index && entries[lfn_start].attributes == LFN_ATTR) {
      char short_name_padded[11];
      memcpy(short_name_padded, e->name, 8);
      memcpy(short_name_padded + 8, e->ext, 3);

      lfn_entry_t *first_lfn = (lfn_entry_t *)&entries[lfn_start];
      uint8_t expected_checksum = lfn_calculate_checksum(short_name_padded);

      if (first_lfn->checksum == expected_checksum) {
        fat16_read_long_filename(entries, index - 1, long_name);
      }
    }
  }

  // if no long filename found, use short name
  if (long_name[0] == '\0') {
    strcpy(long_name, short_name);
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

  for (int i = 0; i < dir_size; i++) {
    ata_read_sector(lba + i, buffer);
    memcpy((char *)entries + (i * 512), (char *)buffer, 512);
  }

  for (int i = 0; i < bpb->root_entries; i++) {
    // skip LFN entries - they'll be processed with their 8.3 entry
    if (entries[i].attributes == LFN_ATTR) {
      continue;
    }

    if (entries[i].name[0] == 0x00)
      break;
    if ((uint8_t)entries[i].name[0] == 0xE5)
      continue; // deleted entry

    fs_node_t *node = (fs_node_t *)kmalloc(sizeof(fs_node_t));
    memset((uint8_t *)node, 0, sizeof(fs_node_t));

    char short_name[64] = {0};
    char long_name[256] = {0};

    fat16_entry_extract_filename_lfn(entries, i, short_name, long_name);

    // skip . and .. entries
    if (!strcmp(long_name, ".") || !strcmp(long_name, "..")) {
      kfree(node);
      continue;
    }

    memcpy(node->name, short_name, 13);
    strcpy(node->long_name, long_name);

    node->size = entries[i].file_size;
    node->start_cluster = entries[i].start_cluster;
    node->type =
        (entries[i].attributes & 0x10) ? FS_TYPE_DIRECTORY : FS_TYPE_FILE;
    node->parent = parent;

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

void fs_print_tree(fs_node_t *node, char *prefix, int is_last) {
  if (!node)
    return;

  kernel_printf("%s%s", prefix, is_last ? TREE_LAST : TREE_BRANCH);

  char *display_name =
      (node->long_name[0] != '\0') ? node->long_name : node->name;

  if (node->type == FS_TYPE_DIRECTORY) {
    kernel_printf("%s\n", display_name);
  } else {
    kernel_printf("%s (%d bytes)\n", display_name, node->size);
  }

  if (node->type == FS_TYPE_DIRECTORY && node->children) {
    char new_prefix[256];
    strcpy(new_prefix, prefix);
    strcat(new_prefix, is_last ? TREE_SPACE : TREE_VERTICAL);

    fs_node_t *child = node->children;
    int child_count = 0;
    fs_node_t *temp = child;
    while (temp) {
      child_count++;
      temp = temp->next;
    }

    int current_child = 0;
    while (child) {
      current_child++;
      fs_print_tree(child, new_prefix, current_child == child_count);
      child = child->next;
    }
  }
}

fs_node_t *fs_find_file(fs_node_t *root, const char *name) {
  fs_node_t *current = root;
  while (current) {
    if (!strcmp(current->name, name) || !strcmp(current->long_name, name)) {
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