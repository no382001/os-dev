// from https://github.com/szhou42/osdev/blob/master/src/include/rtl8139.h
#pragma once
#include "cpu/isr.h"
#include "libc/types.h"

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

#define RX_BUF_SIZE 8192

#define CAPR 0x38
#define RX_READ_POINTER_MASK (~3)
#define ROK (1 << 0)
#define RER (1 << 1)
#define TOK (1 << 2)
#define TER (1 << 3)
#define TX_TOK (1 << 15)

enum RTL8139_registers {
  MAG0 = 0x00,      // ethernet hardware address
  MAR0 = 0x08,      // multicast filter
  TxStatus0 = 0x10, // transmit status (Four 32bit registers)
  TxAddr0 = 0x20,   // tx descriptors (also four 32bit)
  RxBuf = 0x30,
  RxEarlyCnt = 0x34,
  RxEarlyStatus = 0x36,
  ChipCmd = 0x37,
  RxBufPtr = 0x38,
  RxBufAddr = 0x3A,
  IntrMask = 0x3C,
  IntrStatus = 0x3E,
  TxConfig = 0x40,
  RxConfig = 0x44,
  Timer = 0x48,    // A general-purpose counter
  RxMissed = 0x4C, // 24 bits valid, write clears
  Cfg9346 = 0x50,
  Config0 = 0x51,
  Config1 = 0x52,
  FlashReg = 0x54,
  GPPinData = 0x58,
  GPPinDir = 0x59,
  MII_SMI = 0x5A,
  HltClk = 0x5B,
  MultiIntr = 0x5C,
  TxSummary = 0x60,
  MII_BMCR = 0x62,
  MII_BMSR = 0x64,
  NWayAdvert = 0x66,
  NWayLPAR = 0x68,
  NWayExpansion = 0x6A,

  // undocumented registers, but required for proper operation
  FIFOTMS = 0x70, // FIFO Control and test
  CSCR = 0x74,    // chip Status and Configuration Register
  PARA78 = 0x78,
  PARA7c = 0x7c, // magic transceiver parameter register
};

typedef struct tx_desc {
  uint32_t phys_addr;
  uint32_t packet_size;
} tx_desc_t;

typedef struct rtl8139_dev {
  uint8_t bar_type;
  uint16_t io_base;
  uint32_t mem_base;
  int eeprom_exist;
  uint8_t mac_addr[6];
  char *rx_buffer;
  int tx_cur;
} rtl8139_dev_t;

void rtl8139_send_packet(void *data, uint32_t len);
void rtl8139_handler(registers_t *reg);
void rtl8139_init();
void read_mac_addr();
void receive_packet();
void get_mac_addr(uint8_t *src_mac_addr);
