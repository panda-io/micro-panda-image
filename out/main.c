#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct DisplayConn DisplayConn;
typedef struct Ssd1306 Ssd1306;
typedef struct Allocator Allocator;

typedef enum {
  DisplayInterface_I2C = 0,
  DisplayInterface_SPI = 1,
} DisplayInterface;

typedef enum {
  DisplayColor_MONO = 0,
  DisplayColor_RGB565 = 1,
} DisplayColor;

typedef enum {
  GpioMode_INPUT = 1,
  GpioMode_OUTPUT = 3,
  GpioMode_INPUT_PULLUP = 5,
  GpioMode_INPUT_PULLDOWN = 6,
} GpioMode;

typedef enum {
  GpioLevel_LOW = 0,
  GpioLevel_HIGH = 1,
} GpioLevel;

typedef struct { uint8_t* ptr; int32_t size; } __Slice_uint8_t;

#define PWM_MAX_CHANNELS 8
#define I2C_MAX_BUSES 2
#define I2C_MAX_DEVICES 8
#define I2C_TIMEOUT_MS 1000
#define SPI_MAX_BUSES 2
#define SPI_MAX_DEVICES 6
#define SPI_DMA 1
#include "driver/i2c_master.h"

static i2c_master_bus_handle_t __mp_i2c_buses[I2C_MAX_BUSES];
static i2c_master_dev_handle_t __mp_i2c_devs[I2C_MAX_DEVICES];
static uint32_t                __mp_i2c_freq[I2C_MAX_BUSES];

// Initialize I2C bus. Returns bus index or -1 on error.
static inline int32_t __mp_i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz) {
    if (bus < 0 || bus >= I2C_MAX_BUSES) return -1;
    if (__mp_i2c_buses[bus] != NULL) return bus; // already initialized
    i2c_master_bus_config_t cfg = {
        .i2c_port    = (i2c_port_num_t)bus,
        .sda_io_num  = (gpio_num_t)sda,
        .scl_io_num  = (gpio_num_t)scl,
        .clk_source  = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t err = i2c_new_master_bus(&cfg, &__mp_i2c_buses[bus]);
    if (err != ESP_OK) return -1;
    __mp_i2c_freq[bus] = (uint32_t)freq_hz;
    return bus;
}

// Open a device on the bus by 7-bit address. Returns device slot or -1 on error.
static inline int32_t __mp_i2c_open(int32_t bus, int32_t addr) {
    if (bus < 0 || bus >= I2C_MAX_BUSES || __mp_i2c_buses[bus] == NULL) return -1;
    for (int i = 0; i < I2C_MAX_DEVICES; i++) {
        if (__mp_i2c_devs[i] == NULL) {
            i2c_device_config_t dev_cfg = {
                .dev_addr_length = I2C_ADDR_BIT_LEN_7,
                .device_address  = (uint16_t)addr,
                .scl_speed_hz    = __mp_i2c_freq[bus],
            };
            esp_err_t err = i2c_master_bus_add_device(__mp_i2c_buses[bus], &dev_cfg, &__mp_i2c_devs[i]);
            return err == ESP_OK ? i : -1;
        }
    }
    return -1; // no free slot
}

static inline void __mp_i2c_close(int32_t dev) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return;
    i2c_master_bus_rm_device(__mp_i2c_devs[dev]);
    __mp_i2c_devs[dev] = NULL;
}

static inline int32_t __mp_i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_transmit(__mp_i2c_devs[dev], data.ptr, (size_t)len, I2C_TIMEOUT_MS);
}

static inline int32_t __mp_i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_receive(__mp_i2c_devs[dev], buf.ptr, (size_t)len, I2C_TIMEOUT_MS);
}

static inline int32_t __mp_i2c_write_read(int32_t dev,
                                           __Slice_uint8_t tx, int32_t tx_len,
                                           __Slice_uint8_t rx, int32_t rx_len) {
    if (dev < 0 || dev >= I2C_MAX_DEVICES || __mp_i2c_devs[dev] == NULL) return -1;
    return (int32_t)i2c_master_transmit_receive(
        __mp_i2c_devs[dev],
        tx.ptr, (size_t)tx_len,
        rx.ptr, (size_t)rx_len,
        I2C_TIMEOUT_MS);
}
#include "driver/gpio.h"

struct DisplayConn {
  DisplayInterface interface;
  int32_t device;
  int32_t dc_pin;
  int32_t reset_pin;
  int32_t back_light_pin;
};

struct Ssd1306 {
  uint8_t heap[1];
  uint8_t strip[512];
  GraphicsDriver driver;
  Graphics gfx;
  int32_t _dev;
  int32_t _width;
  int32_t _height;
  __Slice_uint8_t _page_buf;
};

struct Allocator {
  __Slice_uint8_t _memory;
  int32_t _cursor;
};

void main__main(void);
static int32_t drivers__ssd1306___cmd1(int32_t dev, int32_t c);
static int32_t drivers__ssd1306___cmd2(int32_t dev, int32_t c, int32_t v);
static void drivers__ssd1306___hw_init_128x32(int32_t dev);
static void drivers__ssd1306___hw_init_128x64(int32_t dev);
static void drivers__ssd1306___attach(Ssd1306* d, DisplayConn* conn, int32_t w, int32_t h);
Ssd1306* drivers__ssd1306__attach_ssd1306_128x32(Ssd1306* d, DisplayConn* conn);
Ssd1306* drivers__ssd1306__attach_ssd1306_128x64(Ssd1306* d, DisplayConn* conn);
static void Ssd1306__set_rotation(Ssd1306* this, Rotation r);
static void Ssd1306__wait(Ssd1306* this);
static void Ssd1306__blit(Ssd1306* this, Rect* rect, __Slice_uint8_t src);
static void Ssd1306__flush(Ssd1306* this, Rect* rect, __Slice_uint8_t buf);
static void Ssd1306__send_page(Ssd1306* this);
int32_t i2c__i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz);
int32_t i2c__i2c_open(int32_t bus, int32_t addr);
void i2c__i2c_close(int32_t dev);
int32_t i2c__i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len);
int32_t i2c__i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len);
int32_t i2c__i2c_write_read(int32_t dev, __Slice_uint8_t tx, int32_t tx_len, __Slice_uint8_t rx, int32_t rx_len);
void gpio__gpio_mode(int32_t pin, GpioMode mode);
static inline void gpio__gpio_write(int32_t pin, GpioLevel value);
static inline GpioLevel gpio__gpio_read(int32_t pin);
void Allocator_init(Allocator* this, __Slice_uint8_t mem);
static inline void* Allocator_allocate(Allocator* this, size_t __sizeof_T);
static inline void Allocator_reset(Allocator* this);
static inline __Slice_uint8_t Allocator_allocate_array_uint8_t(Allocator* this, int32_t length);

int32_t drivers__ssd1306__HEAP_SIZE = 1040;

static int32_t __mp_argc = 0;
static char** __mp_argv = NULL;

static int32_t drivers__ssd1306___cmd1(int32_t dev, int32_t c) {
  uint8_t buf[2];
  (buf[0] = 0x00);
  (buf[1] = ((uint8_t)(c)));
  return i2c__i2c_write(dev, (__Slice_uint8_t){buf, 2}, 2);
}

static int32_t drivers__ssd1306___cmd2(int32_t dev, int32_t c, int32_t v) {
  uint8_t buf[3];
  (buf[0] = 0x00);
  (buf[1] = ((uint8_t)(c)));
  (buf[2] = ((uint8_t)(v)));
  return i2c__i2c_write(dev, (__Slice_uint8_t){buf, 3}, 3);
}

static void drivers__ssd1306___hw_init_128x32(int32_t dev) {
  drivers__ssd1306___cmd1(dev, 0xAE);
  drivers__ssd1306___cmd2(dev, 0xD5, 0x80);
  drivers__ssd1306___cmd2(dev, 0xA8, 0x1F);
  drivers__ssd1306___cmd2(dev, 0xD3, 0x00);
  drivers__ssd1306___cmd1(dev, 0x40);
  drivers__ssd1306___cmd2(dev, 0x8D, 0x14);
  drivers__ssd1306___cmd2(dev, 0x20, 0x00);
  drivers__ssd1306___cmd1(dev, 0xA1);
  drivers__ssd1306___cmd1(dev, 0xC8);
  drivers__ssd1306___cmd2(dev, 0xDA, 0x02);
  drivers__ssd1306___cmd2(dev, 0x81, 0xCF);
  drivers__ssd1306___cmd2(dev, 0xD9, 0xF1);
  drivers__ssd1306___cmd2(dev, 0xDB, 0x40);
  drivers__ssd1306___cmd1(dev, 0xA4);
  drivers__ssd1306___cmd1(dev, 0xA6);
  drivers__ssd1306___cmd1(dev, 0xAF);
}

static void drivers__ssd1306___hw_init_128x64(int32_t dev) {
  drivers__ssd1306___cmd1(dev, 0xAE);
  drivers__ssd1306___cmd2(dev, 0xD5, 0x80);
  drivers__ssd1306___cmd2(dev, 0xA8, 0x3F);
  drivers__ssd1306___cmd2(dev, 0xD3, 0x00);
  drivers__ssd1306___cmd1(dev, 0x40);
  drivers__ssd1306___cmd2(dev, 0x8D, 0x14);
  drivers__ssd1306___cmd2(dev, 0x20, 0x00);
  drivers__ssd1306___cmd1(dev, 0xA1);
  drivers__ssd1306___cmd1(dev, 0xC8);
  drivers__ssd1306___cmd2(dev, 0xDA, 0x12);
  drivers__ssd1306___cmd2(dev, 0x81, 0xCF);
  drivers__ssd1306___cmd2(dev, 0xD9, 0xF1);
  drivers__ssd1306___cmd2(dev, 0xDB, 0x40);
  drivers__ssd1306___cmd1(dev, 0xA4);
  drivers__ssd1306___cmd1(dev, 0xA6);
  drivers__ssd1306___cmd1(dev, 0xAF);
}

static void drivers__ssd1306___attach(Ssd1306* d, DisplayConn* conn, int32_t w, int32_t h) {
  Allocator alloc = {0};
  Allocator_init((&alloc), d->heap);
  (d->_dev = conn->device);
  (d->_width = w);
  (d->_height = h);
  int32_t pages = (h >> 3);
  int32_t buf_size = (1 + (pages * w));
  (d->_page_buf = Allocator_allocate_array_uint8_t((&alloc), buf_size));
  (d->_page_buf.ptr[0] = 0x40);
  if ((conn->reset_pin >= 0)) {
    gpio__gpio_mode(conn->reset_pin, GpioMode_OUTPUT);
    gpio__gpio_write(conn->reset_pin, GpioLevel_LOW);
    gpio__gpio_write(conn->reset_pin, GpioLevel_HIGH);
  }
  (d->driver.width = w);
  (d->driver.height = h);
  (d->driver.set_rotation = d->_set_rotation);
  (d->driver.wait = d->_wait);
  (d->driver.flush = d->_flush);
}

Ssd1306* drivers__ssd1306__attach_ssd1306_128x32(Ssd1306* d, DisplayConn* conn) {
  drivers__ssd1306___attach(d, conn, 128, 32);
  drivers__ssd1306___hw_init_128x32(conn->device);
  return d;
}

Ssd1306* drivers__ssd1306__attach_ssd1306_128x64(Ssd1306* d, DisplayConn* conn) {
  drivers__ssd1306___attach(d, conn, 128, 64);
  drivers__ssd1306___hw_init_128x64(conn->device);
  return d;
}

static void Ssd1306__set_rotation(Ssd1306* this, Rotation r) {
}

static void Ssd1306__wait(Ssd1306* this) {
}

static void Ssd1306__blit(Ssd1306* this, Rect* rect, __Slice_uint8_t src) {
  int32_t stride = ((rect->width + 7) / 8);
  for (int32_t ry = 0; ry < rect->height; ry++) {
    int32_t abs_y = (rect->y + ry);
    if (((abs_y >= 0) && (abs_y < this->_height))) {
      int32_t page = (abs_y >> 3);
      int32_t bit = (abs_y & 7);
      int32_t row = (ry * stride);
      for (int32_t rx = 0; rx < rect->width; rx++) {
        int32_t abs_x = (rect->x + rx);
        if (((abs_x >= 0) && (abs_x < this->_width))) {
          int32_t pixel = ((((int32_t)(src.ptr[(row + (rx >> 3))])) >> (7 - (rx & 7))) & 1);
          int32_t p = ((1 + (page * this->_width)) + abs_x);
          if ((pixel != 0)) {
            (this->_page_buf.ptr[p] = (this->_page_buf.ptr[p] | ((uint8_t)((1 << bit)))));
          } else {
            (this->_page_buf.ptr[p] = (this->_page_buf.ptr[p] & ((uint8_t)((~(1 << bit))))));
          }
        }
      }
    }
  }
}

static void Ssd1306__flush(Ssd1306* this, Rect* rect, __Slice_uint8_t buf) {
  Ssd1306__blit(this, rect, buf);
  Ssd1306__send_page(this);
}

static void Ssd1306__send_page(Ssd1306* this) {
  int32_t pages = (this->_height >> 3);
  uint8_t col[4];
  (col[0] = 0x00);
  (col[1] = 0x21);
  (col[2] = 0x00);
  (col[3] = ((uint8_t)((this->_width - 1))));
  i2c__i2c_write(this->_dev, (__Slice_uint8_t){col, 4}, 4);
  uint8_t page[4];
  (page[0] = 0x00);
  (page[1] = 0x22);
  (page[2] = 0x00);
  (page[3] = ((uint8_t)((pages - 1))));
  i2c__i2c_write(this->_dev, (__Slice_uint8_t){page, 4}, 4);
  i2c__i2c_write(this->_dev, this->_page_buf, (1 + (pages * this->_width)));
}

int32_t i2c__i2c_init(int32_t bus, int32_t sda, int32_t scl, int32_t freq_hz) {
  return __mp_i2c_init(bus, sda, scl, freq_hz);
}

int32_t i2c__i2c_open(int32_t bus, int32_t addr) {
  return __mp_i2c_open(bus, addr);
}

void i2c__i2c_close(int32_t dev) {
  __mp_i2c_close(dev);
}

int32_t i2c__i2c_write(int32_t dev, __Slice_uint8_t data, int32_t len) {
  return __mp_i2c_write(dev, data, len);
}

int32_t i2c__i2c_read(int32_t dev, __Slice_uint8_t buf, int32_t len) {
  return __mp_i2c_read(dev, buf, len);
}

int32_t i2c__i2c_write_read(int32_t dev, __Slice_uint8_t tx, int32_t tx_len, __Slice_uint8_t rx, int32_t rx_len) {
  return __mp_i2c_write_read(dev, tx, tx_len, rx, rx_len);
}

void gpio__gpio_mode(int32_t pin, GpioMode mode) {
  gpio_reset_pin(pin);
  if ((mode == GpioMode_INPUT_PULLUP)) {
    gpio_set_direction(pin, ((int32_t)(GpioMode_INPUT)));
    gpio_set_pull_mode(pin, 0);
  } else if ((mode == GpioMode_INPUT_PULLDOWN)) {
    gpio_set_direction(pin, ((int32_t)(GpioMode_INPUT)));
    gpio_set_pull_mode(pin, 1);
  } else {
    gpio_set_direction(pin, ((int32_t)(mode)));
  }
}

static inline void gpio__gpio_write(int32_t pin, GpioLevel value) {
  gpio_set_level(pin, ((int32_t)(value)));
}

static inline GpioLevel gpio__gpio_read(int32_t pin) {
  return ((GpioLevel)(gpio_get_level(pin)));
}

void Allocator_init(Allocator* this, __Slice_uint8_t mem) {
  (this->_memory = mem);
  (this->_cursor = 0);
}

static inline void* Allocator_allocate(Allocator* this, size_t __sizeof_T) {
  uint64_t size = __sizeof_T;
  if (((this->_cursor + size) > this->_memory.size)) {
    return NULL;
  }
  void* ptr = (void*)((&this->_memory.ptr[this->_cursor]));
  (this->_cursor = (((this->_cursor + size) + 3) & (~((int32_t)(3)))));
  return ptr;
}

static inline void Allocator_reset(Allocator* this) {
  (this->_cursor = 0);
}

static inline __Slice_uint8_t Allocator_allocate_array_uint8_t(Allocator* this, int32_t length) {
  uint64_t size = (sizeof(uint8_t) * length);
  if (((this->_cursor + size) > this->_memory.size)) {
    return (__Slice_uint8_t){NULL, 0};
  }
  uint8_t* ptr = ((uint8_t*)((&this->_memory.ptr[this->_cursor])));
  (this->_cursor = (((this->_cursor + size) + 3) & (~((int32_t)(3)))));
  return (__Slice_uint8_t){ptr, length};
}

int main(int argc, char** argv) { __mp_argc = argc; __mp_argv = argv; main__main(); return 0; }

