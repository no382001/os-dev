#include "mouse.h"
#include "cpu/isr.h"
#include "low_level.h"
#include "screen.h"

#define MOUSE_PORT_DATA 0x60
#define MOUSE_PORT_STATUS 0x64
#define MOUSE_PORT_CMD 0x64
#define MOUSE_CMD_WRITE 0xD4
#define MOUSE_ENABLE_AUX 0xA8
#define MOUSE_GET_COMPAQ 0x20
#define MOUSE_SET_COMPAQ 0x60

static mouse_ctx_t mouse_ctx = {0};

static void mouse_wait_write(void) {
  int timeout = 100000;
  while (timeout--) {
    if ((port_byte_in(MOUSE_PORT_STATUS) & 0x02) == 0)
      return;
  }
}

static void mouse_wait_read(void) {
  int timeout = 100000;
  while (timeout--) {
    if ((port_byte_in(MOUSE_PORT_STATUS) & 0x01) == 1)
      return;
  }
}

static void mouse_write(uint8_t data) {
  mouse_wait_write();
  port_byte_out(MOUSE_PORT_CMD, MOUSE_CMD_WRITE);
  mouse_wait_write();
  port_byte_out(MOUSE_PORT_DATA, data);
}

static uint8_t mouse_read(void) {
  mouse_wait_read();
  return port_byte_in(MOUSE_PORT_DATA);
}

static void default_mouse_handler(mouse_state_t *state) {
  (void)state;
  // default: do nothing, apps can register their own
}

static void mouse_callback(registers_t *regs) {
  (void)regs;

  // check if this is actually mouse data (bit 5 of status)
  uint8_t status = port_byte_in(MOUSE_PORT_STATUS);
  if (!(status & 0x20)) {
    return; // not mouse data
  }

  uint8_t data = port_byte_in(MOUSE_PORT_DATA);

  switch (mouse_ctx.cycle) {
  case 0:
    if (data & 0x08) { // bit 3 always set in valid packets
      mouse_ctx.bytes[0] = data;
      mouse_ctx.cycle++;
    }
    break;

  case 1:
    mouse_ctx.bytes[1] = data;
    mouse_ctx.cycle++;
    break;

  case 2:
    mouse_ctx.bytes[2] = data;
    mouse_ctx.cycle = 0;

    // save old button state
    uint8_t old_buttons = mouse_ctx.state.buttons;

    // parse packet
    mouse_ctx.state.buttons = mouse_ctx.bytes[0] & 0x07;
    mouse_ctx.state.buttons_changed = old_buttons ^ mouse_ctx.state.buttons;

    // x delta (signed)
    int16_t dx = mouse_ctx.bytes[1];
    if (mouse_ctx.bytes[0] & 0x10)
      dx |= 0xFF00;

    // y delta (signed, inverted)
    int16_t dy = mouse_ctx.bytes[2];
    if (mouse_ctx.bytes[0] & 0x20)
      dy |= 0xFF00;

    mouse_ctx.state.dx = dx;
    mouse_ctx.state.dy = -dy;

    // update position
    mouse_ctx.state.x += dx;
    mouse_ctx.state.y -= dy;

    // clamp
    if (mouse_ctx.state.x < mouse_ctx.min_x)
      mouse_ctx.state.x = mouse_ctx.min_x;
    if (mouse_ctx.state.x > mouse_ctx.max_x)
      mouse_ctx.state.x = mouse_ctx.max_x;
    if (mouse_ctx.state.y < mouse_ctx.min_y)
      mouse_ctx.state.y = mouse_ctx.min_y;
    if (mouse_ctx.state.y > mouse_ctx.max_y)
      mouse_ctx.state.y = mouse_ctx.max_y;

    // call handler
    if (mouse_ctx.current_handler) {
      mouse_ctx.current_handler(&mouse_ctx.state);
    }
    break;
  }
}

void mouse_set_bounds(int32_t min_x, int32_t max_x, int32_t min_y,
                      int32_t max_y) {
  mouse_ctx.min_x = min_x;
  mouse_ctx.max_x = max_x;
  mouse_ctx.min_y = min_y;
  mouse_ctx.max_y = max_y;

  // Clamp current position
  if (mouse_ctx.state.x < min_x)
    mouse_ctx.state.x = min_x;
  if (mouse_ctx.state.x > max_x)
    mouse_ctx.state.x = max_x;
  if (mouse_ctx.state.y < min_y)
    mouse_ctx.state.y = min_y;
  if (mouse_ctx.state.y > max_y)
    mouse_ctx.state.y = max_y;
}

void init_mouse(void) {
  // enable auxiliary mouse device
  mouse_wait_write();
  port_byte_out(MOUSE_PORT_CMD, MOUSE_ENABLE_AUX);

  // get compaq status byte
  mouse_wait_write();
  port_byte_out(MOUSE_PORT_CMD, MOUSE_GET_COMPAQ);
  uint8_t status = mouse_read();

  // enable IRQ12, enable mouse clock
  status |= 0x02;
  status &= ~0x20;

  // set compaq status
  mouse_wait_write();
  port_byte_out(MOUSE_PORT_CMD, MOUSE_SET_COMPAQ);
  mouse_wait_write();
  port_byte_out(MOUSE_PORT_DATA, status);

  // default settings
  mouse_write(0xF6);
  mouse_read();

  // enable streaming
  mouse_write(0xF4);
  mouse_read();

  // init context
  mouse_ctx.default_handler = default_mouse_handler;
  mouse_ctx.current_handler = default_mouse_handler;
  mouse_ctx.cycle = 0;

  // default bounds (mode 13h)
  mouse_ctx.min_x = 0;
  mouse_ctx.max_x = 319;
  mouse_ctx.min_y = 0;
  mouse_ctx.max_y = 199;

  // start centered
  mouse_ctx.state.x = 160;
  mouse_ctx.state.y = 100;
  mouse_ctx.state.buttons = 0;

  register_interrupt_handler(IRQ12, mouse_callback);
  kernel_printf("- mouse installed\n");
}

mouse_handler_t register_mouse_handler(mouse_handler_t new_handler) {
  mouse_handler_t old = mouse_ctx.current_handler;
  mouse_ctx.current_handler =
      new_handler ? new_handler : mouse_ctx.default_handler;
  return old;
}

mouse_ctx_t *get_mouse_ctx(void) { return &mouse_ctx; }

mouse_state_t mouse_get_state(void) { return mouse_ctx.state; }

int mouse_left_pressed(void) { return mouse_ctx.state.buttons & 0x01; }

int mouse_right_pressed(void) { return mouse_ctx.state.buttons & 0x02; }

int mouse_middle_pressed(void) { return mouse_ctx.state.buttons & 0x04; }

int mouse_left_clicked(void) {
  return (mouse_ctx.state.buttons & 0x01) &&
         (mouse_ctx.state.buttons_changed & 0x01);
}

int mouse_right_clicked(void) {
  return (mouse_ctx.state.buttons & 0x02) &&
         (mouse_ctx.state.buttons_changed & 0x02);
}