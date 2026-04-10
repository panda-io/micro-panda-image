#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct DisplayConn DisplayConn;
typedef struct Ssd1306 Ssd1306;
typedef struct GraphicsDriver GraphicsDriver;
typedef struct Graphics Graphics;
typedef struct Point Point;
typedef struct Rect Rect;
typedef struct Allocator Allocator;
typedef struct Node Node;
typedef struct RenderContext RenderContext;

typedef enum {
  DisplayInterface_I2C = 0,
  DisplayInterface_SPI = 1,
} DisplayInterface;

typedef enum {
  DisplayColor_MONO = 0,
  DisplayColor_RGB565 = 1,
} DisplayColor;

typedef enum {
  PixelFormat_Mono = 0,
  PixelFormat_RGB565 = 1,
} PixelFormat;

typedef enum {
  IndexFormat_Index1 = 0,
  IndexFormat_Index2 = 1,
  IndexFormat_Index4 = 2,
  IndexFormat_Index8 = 3,
} IndexFormat;

typedef enum {
  Rotation_R0 = 0,
  Rotation_R90 = 1,
  Rotation_R180 = 2,
  Rotation_R270 = 3,
} Rotation;

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

typedef void (*__Fn_void_void_p_Rotation)(void*, Rotation);
typedef void (*__Fn_void_void_p)(void*);
typedef void (*__Fn_void_void_p_Rect_p_Slice_uint8_t)(void*, Rect*, __Slice_uint8_t);
typedef void (*__Fn_void_RenderContext_p_Point_p_void_p)(RenderContext*, Point*, void*);

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
#include <math.h>
#include <string.h>

struct DisplayConn {
  DisplayInterface interface;
  int32_t device;
  int32_t dc_pin;
  int32_t reset_pin;
  int32_t back_light_pin;
};

struct GraphicsDriver {
  int32_t width;
  int32_t height;
  void* handle;
  __Fn_void_void_p_Rotation set_rotation;
  __Fn_void_void_p wait;
  __Fn_void_void_p_Rect_p_Slice_uint8_t flush;
};

struct Point {
  int32_t x;
  int32_t y;
};

struct Rect {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};

struct Allocator {
  __Slice_uint8_t _memory;
  int32_t _cursor;
};

struct Node {
  void* handle;
  __Fn_void_RenderContext_p_Point_p_void_p renderer;
};

struct RenderContext {
  __Slice_uint8_t buffer;
  PixelFormat format;
  Rect viewpoint;
};

struct Graphics {
  GraphicsDriver* _driver;
  Node* _root;
  uint16_t _background;
  bool _dirty_render;
  Rect _render_window;
  __Slice_uint8_t _strip0;
  __Slice_uint8_t _strip1;
  bool _single_buffer;
  int32_t _front_buffer;
  RenderContext _context;
};

struct Ssd1306 {
  uint8_t strip[512];
  GraphicsDriver driver;
  Graphics gfx;
  int32_t _dev;
  int32_t _width;
  int32_t _height;
  uint8_t _page_buf[1];
};

void main__main(void);
static void drivers__ssd1306___ssd1306_set_rotation(void* handle, Rotation r);
static void drivers__ssd1306___ssd1306_wait(void* handle);
static void drivers__ssd1306___ssd1306_flush(void* handle, Rect* rect, __Slice_uint8_t buf);
static int32_t drivers__ssd1306___cmd1(int32_t dev, int32_t c);
static int32_t drivers__ssd1306___cmd2(int32_t dev, int32_t c, int32_t v);
static void drivers__ssd1306___hw_init(int32_t dev, int32_t rows);
static void drivers__ssd1306___attach(Ssd1306* display, DisplayConn* conn, int32_t width, int32_t height);
void drivers__ssd1306__attach_ssd1306_128x32(Ssd1306* display, DisplayConn* conn);
void drivers__ssd1306__attach_ssd1306_128x64(Ssd1306* display, DisplayConn* conn);
static void Ssd1306__set_rotation(Ssd1306* this, Rotation r);
static void Ssd1306__wait(Ssd1306* this);
static void Ssd1306__blit(Ssd1306* this, Rect* rect, __Slice_uint8_t src, int32_t page_start, int32_t page_count);
static void Ssd1306__flush(Ssd1306* this, Rect* rect, __Slice_uint8_t buf);
static void Ssd1306__send_pages(Ssd1306* this, int32_t page_start, int32_t page_count);
void Graphics_set_root(Graphics* this, Node* node);
void Graphics_init(Graphics* this, GraphicsDriver* driver, __Slice_uint8_t buffer0, __Slice_uint8_t buffer1, PixelFormat pixel_format, uint16_t background, Rotation rotation, bool dirty_render);
void Graphics_mark_dirty(Graphics* this, Rect* rect);
void Graphics_render(Graphics* this);
static __Slice_uint8_t Graphics__front_strip(Graphics* this);
static __Slice_uint8_t Graphics__back_strip(Graphics* this);
static void Graphics__clear_strip(Graphics* this, __Slice_uint8_t buffer);
static inline void Point_copy(Point* this, Point* point);
static inline void Rect_copy(Rect* this, Rect* rect);
static inline bool Rect_intersect(Rect* this, Rect* rect);
static inline bool Rect_contains(Rect* this, Point* point);
static inline void Rect_merge(Rect* this, Rect* rect);
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
static inline int32_t Allocator_available(Allocator* this);
static inline void Allocator_reset(Allocator* this);
static inline int32_t math__floor_q16(int32_t value);
static inline int32_t math__ceil_q16(int32_t value);
static inline int32_t math__round_q16(int32_t value);
static inline int32_t math__floor_fixed(int32_t value);
static inline int32_t math__ceil_fixed(int32_t value);
static inline int32_t math__round_fixed(int32_t value);
static inline bool RenderContext_intersect(RenderContext* this, Rect* rect);
void RenderContext_set_pixel(RenderContext* this, Point* point, uint16_t color);
void RenderContext_fill_rect(RenderContext* this, Rect* rect, uint16_t color);
static inline void RenderContext__set_pixel_mono(RenderContext* this, int32_t x, int32_t y, uint16_t color);
static inline void RenderContext__set_pixel_rgb565(RenderContext* this, int32_t x, int32_t y, uint16_t color);
static inline void memory__memory_set(__Slice_uint8_t dst, uint8_t value);
static inline void memory__memory_copy(__Slice_uint8_t dst, __Slice_uint8_t src, int32_t size);
static inline void memory__memory_move(__Slice_uint8_t dst, __Slice_uint8_t src, int32_t size);
static inline void memory__memory_zero(__Slice_uint8_t dst);
static inline int32_t math__min_int32_t(int32_t a, int32_t b);
static inline int32_t math__max_int32_t(int32_t a, int32_t b);

int32_t drivers__ssd1306__BUF_SIZE = 528;
const float math__PI = 3.14159265358979323846f;
const float math__TAU = 6.28318530717958647692f;
const float math__E = 2.71828182845904523536f;

static int32_t __mp_argc = 0;
static char** __mp_argv = NULL;

static void drivers__ssd1306___ssd1306_set_rotation(void* handle, Rotation r) {
  Ssd1306__set_rotation(((Ssd1306*)(handle)), r);
}

static void drivers__ssd1306___ssd1306_wait(void* handle) {
}

static void drivers__ssd1306___ssd1306_flush(void* handle, Rect* rect, __Slice_uint8_t buf) {
  Ssd1306__flush(((Ssd1306*)(handle)), rect, buf);
}

static int32_t drivers__ssd1306___cmd1(int32_t dev, int32_t c) {
  uint8_t buf[2];
  (buf[0] = 0x00);
  (buf[1] = ((uint8_t)(c)));
  return i2c__i2c_write(dev, (__Slice_uint8_t){(&buf[0]), 2}, 2);
}

static int32_t drivers__ssd1306___cmd2(int32_t dev, int32_t c, int32_t v) {
  uint8_t buf[3];
  (buf[0] = 0x00);
  (buf[1] = ((uint8_t)(c)));
  (buf[2] = ((uint8_t)(v)));
  return i2c__i2c_write(dev, (__Slice_uint8_t){(&buf[0]), 3}, 3);
}

static void drivers__ssd1306___hw_init(int32_t dev, int32_t rows) {
  drivers__ssd1306___cmd1(dev, 0xAE);
  drivers__ssd1306___cmd2(dev, 0xD5, 0x80);
  drivers__ssd1306___cmd2(dev, 0xA8, (rows - 1));
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

static void drivers__ssd1306___attach(Ssd1306* display, DisplayConn* conn, int32_t width, int32_t height) {
  (display->_dev = conn->device);
  (display->_width = width);
  (display->_height = height);
  (display->_page_buf[0] = 0x40);
  if ((conn->reset_pin >= 0)) {
    gpio__gpio_mode(conn->reset_pin, GpioMode_OUTPUT);
    gpio__gpio_write(conn->reset_pin, GpioLevel_LOW);
    gpio__gpio_write(conn->reset_pin, GpioLevel_HIGH);
  }
  (display->driver.width = width);
  (display->driver.height = height);
  (display->driver.handle = ((void*)(display)));
  (display->driver.set_rotation = drivers__ssd1306___ssd1306_set_rotation);
  (display->driver.wait = drivers__ssd1306___ssd1306_wait);
  (display->driver.flush = drivers__ssd1306___ssd1306_flush);
}

void drivers__ssd1306__attach_ssd1306_128x32(Ssd1306* display, DisplayConn* conn) {
  drivers__ssd1306___attach(display, conn, 128, 32);
  drivers__ssd1306___hw_init(conn->device, 32);
}

void drivers__ssd1306__attach_ssd1306_128x64(Ssd1306* display, DisplayConn* conn) {
  drivers__ssd1306___attach(display, conn, 128, 64);
  drivers__ssd1306___hw_init(conn->device, 64);
}

static void Ssd1306__set_rotation(Ssd1306* this, Rotation r) {
}

static void Ssd1306__wait(Ssd1306* this) {
}

static void Ssd1306__blit(Ssd1306* this, Rect* rect, __Slice_uint8_t src, int32_t page_start, int32_t page_count) {
  int32_t stride = (this->_width >> 3);
  for (int32_t ry = 0; ry < rect->height; ry++) {
    int32_t abs_y = (rect->y + ry);
    if (((abs_y >= 0) && (abs_y < this->_height))) {
      int32_t page = (abs_y >> 3);
      if (((page >= page_start) && (page < (page_start + page_count)))) {
        int32_t bit = (abs_y & 7);
        int32_t row = (abs_y * stride);
        int32_t local_page = (page - page_start);
        for (int32_t rx = 0; rx < rect->width; rx++) {
          int32_t abs_x = (rect->x + rx);
          if (((abs_x >= 0) && (abs_x < this->_width))) {
            int32_t pixel = ((((int32_t)(src.ptr[(row + (abs_x >> 3))])) >> (7 - (abs_x & 7))) & 1);
            int32_t p = ((1 + (local_page * this->_width)) + abs_x);
            if ((pixel != 0)) {
              (this->_page_buf[p] = (this->_page_buf[p] | ((uint8_t)((1 << bit)))));
            } else {
              (this->_page_buf[p] = (this->_page_buf[p] & ((uint8_t)((~(1 << bit))))));
            }
          }
        }
      }
    }
  }
}

static void Ssd1306__flush(Ssd1306* this, Rect* rect, __Slice_uint8_t buf) {
  int32_t total_pages = (this->_height >> 3);
  int32_t half = 4;
  int32_t passes = (total_pages / half);
  for (int32_t pass = 0; pass < passes; pass++) {
    int32_t ps = (pass * half);
    Rect hr = {0};
    (hr.x = 0);
    (hr.y = (ps << 3));
    (hr.width = this->_width);
    (hr.height = 32);
    for (int32_t i = 1; i < (1 + (half * this->_width)); i++) {
      (this->_page_buf[i] = 0x00);
    }
    Ssd1306__blit(this, (&hr), buf, ps, half);
    Ssd1306__send_pages(this, ps, half);
  }
}

static void Ssd1306__send_pages(Ssd1306* this, int32_t page_start, int32_t page_count) {
  uint8_t col[4];
  (col[0] = 0x00);
  (col[1] = 0x21);
  (col[2] = 0x00);
  (col[3] = ((uint8_t)((this->_width - 1))));
  i2c__i2c_write(this->_dev, (__Slice_uint8_t){col, 4}, 4);
  uint8_t page[4];
  (page[0] = 0x00);
  (page[1] = 0x22);
  (page[2] = ((uint8_t)(page_start)));
  (page[3] = ((uint8_t)(((page_start + page_count) - 1))));
  i2c__i2c_write(this->_dev, (__Slice_uint8_t){page, 4}, 4);
  i2c__i2c_write(this->_dev, this->_page_buf, (1 + (page_count * this->_width)));
}

void Graphics_set_root(Graphics* this, Node* node) {
  (this->_root = node);
}

void Graphics_init(Graphics* this, GraphicsDriver* driver, __Slice_uint8_t buffer0, __Slice_uint8_t buffer1, PixelFormat pixel_format, uint16_t background, Rotation rotation, bool dirty_render) {
  (this->_driver = driver);
  (this->_strip0 = buffer0);
  (this->_strip1 = buffer1);
  (this->_context.format = pixel_format);
  (this->_background = background);
  (this->_dirty_render = dirty_render);
  if ((buffer1.size == 0)) {
    (this->_single_buffer = true);
  }
  (this->_render_window.x = 0);
  (this->_render_window.y = 0);
  if (this->_dirty_render) {
    (this->_render_window.width = 0);
    (this->_render_window.height = 0);
  }
  this->_driver->set_rotation(this->_driver->handle, rotation);
  if (((rotation == Rotation_R0) || (rotation == Rotation_R180))) {
    if ((!this->_dirty_render)) {
      (this->_render_window.width = this->_driver->width);
      (this->_render_window.height = this->_driver->height);
    }
  } else {
    if ((!this->_dirty_render)) {
      (this->_render_window.width = this->_driver->height);
      (this->_render_window.height = this->_driver->width);
    }
  }
  (this->_front_buffer = 0);
}

void Graphics_mark_dirty(Graphics* this, Rect* rect) {
  int32_t aligned_x = rect->x;
  int32_t aligned_width = rect->width;
  if ((this->_context.format == PixelFormat_Mono)) {
    int32_t right = (((rect->x + rect->width) + 7) & (~7));
    (aligned_x = (rect->x & (~7)));
    (aligned_width = (right - aligned_x));
  }
  Rect aligned = (Rect){aligned_x, rect->y, aligned_width, rect->height};
  if (((this->_render_window.width == 0) && (this->_render_window.height == 0))) {
    Rect_copy((&this->_render_window), (&aligned));
  } else {
    Rect_merge((&this->_render_window), (&aligned));
  }
}

void Graphics_render(Graphics* this) {
  if (((this->_render_window.width == 0) || (this->_render_window.height == 0))) {
    return;
  }
  (this->_context.buffer = Graphics__back_strip(this));
  (this->_context.viewpoint.x = this->_render_window.x);
  (this->_context.viewpoint.width = this->_render_window.width);
  int32_t row_bytes = 0;
  if ((this->_context.format == PixelFormat_Mono)) {
    (row_bytes = ((this->_context.viewpoint.width + 7) / 8));
  } else {
    (row_bytes = (this->_context.viewpoint.width * 2));
  }
  int32_t strip_rows = (((this->_context.buffer.size + row_bytes) - 1) / row_bytes);
  int32_t strip_count = (((this->_render_window.height + strip_rows) - 1) / strip_rows);
  Point offset = (Point){0, 0};
  for (int32_t strip_index = 0; strip_index < strip_count; strip_index++) {
    (this->_context.viewpoint.y = (this->_render_window.y + (strip_index * strip_rows)));
    (this->_context.viewpoint.height = math__min_int32_t(strip_rows, ((this->_render_window.y + this->_render_window.height) - this->_context.viewpoint.y)));
    Graphics__clear_strip(this, this->_context.buffer);
    this->_root->renderer((&this->_context), (&offset), this->_root->handle);
    this->_driver->wait(this->_driver->handle);
    (this->_front_buffer = (1 - this->_front_buffer));
    this->_driver->flush(this->_driver->handle, (&this->_context.viewpoint), Graphics__front_strip(this));
  }
  if (this->_dirty_render) {
    (this->_render_window.x = 0);
    (this->_render_window.y = 0);
    (this->_render_window.width = 0);
    (this->_render_window.height = 0);
  }
}

static __Slice_uint8_t Graphics__front_strip(Graphics* this) {
  if ((this->_single_buffer || (this->_front_buffer == 0))) {
    return this->_strip0;
  }
  return this->_strip1;
}

static __Slice_uint8_t Graphics__back_strip(Graphics* this) {
  if ((this->_single_buffer || (this->_front_buffer == 1))) {
    return this->_strip0;
  }
  return this->_strip1;
}

static void Graphics__clear_strip(Graphics* this, __Slice_uint8_t buffer) {
  if ((this->_context.format == PixelFormat_Mono)) {
    if ((this->_background == 0)) {
      memory__memory_zero(buffer);
    } else {
      memory__memory_set(buffer, 0xFF);
    }
  } else {
    if ((this->_background == 0)) {
      memory__memory_zero(buffer);
    } else {
      int32_t i = 0;
      int32_t size = buffer.size;
      while ((i < size)) {
        (buffer.ptr[i] = ((uint8_t)((this->_background >> 8))));
        (buffer.ptr[(i + 1)] = ((uint8_t)(this->_background)));
        (i += 2);
      }
    }
  }
}

static inline void Point_copy(Point* this, Point* point) {
  (this->x = point->x);
  (this->y = point->y);
}

static inline void Rect_copy(Rect* this, Rect* rect) {
  (this->x = rect->x);
  (this->y = rect->y);
  (this->width = rect->width);
  (this->height = rect->height);
}

static inline bool Rect_intersect(Rect* this, Rect* rect) {
  return ((((rect->x < (this->x + this->width)) && ((rect->x + rect->width) > this->x)) && (rect->y < (this->y + this->height))) && ((rect->y + rect->height) > this->y));
}

static inline bool Rect_contains(Rect* this, Point* point) {
  return ((((point->x >= this->x) && (point->x < (this->x + this->width))) && (point->y >= this->y)) && (point->y < (this->y + this->height)));
}

static inline void Rect_merge(Rect* this, Rect* rect) {
  int32_t right = math__max_int32_t((this->x + this->width), (rect->x + rect->width));
  int32_t bottom = math__max_int32_t((this->y + this->height), (rect->y + rect->height));
  (this->x = math__min_int32_t(this->x, rect->x));
  (this->y = math__min_int32_t(this->y, rect->y));
  (this->width = (right - this->x));
  (this->height = (bottom - this->y));
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

static inline int32_t Allocator_available(Allocator* this) {
  return (this->_memory.size - this->_cursor);
}

static inline void Allocator_reset(Allocator* this) {
  (this->_cursor = 0);
}

static inline int32_t math__floor_q16(int32_t value) {
  return (value & (-65536));
}

static inline int32_t math__ceil_q16(int32_t value) {
  int32_t result = (value & (-65536));
  if ((value != result)) {
    return (result + 65536);
  }
  return result;
}

static inline int32_t math__round_q16(int32_t value) {
  return ((value + 32768) & (-65536));
}

static inline int32_t math__floor_fixed(int32_t value) {
  return (value & (-65536));
}

static inline int32_t math__ceil_fixed(int32_t value) {
  int32_t result = (value & (-65536));
  if ((value != result)) {
    return (result + 65536);
  }
  return result;
}

static inline int32_t math__round_fixed(int32_t value) {
  return ((value + 32768) & (-65536));
}

static inline bool RenderContext_intersect(RenderContext* this, Rect* rect) {
  return Rect_intersect((&this->viewpoint), rect);
}

void RenderContext_set_pixel(RenderContext* this, Point* point, uint16_t color) {
  if ((!Rect_contains((&this->viewpoint), point))) {
    return;
  }
  int32_t px = (point->x - this->viewpoint.x);
  int32_t py = (point->y - this->viewpoint.y);
  if ((this->format == PixelFormat_Mono)) {
    RenderContext__set_pixel_mono(this, px, py, color);
  } else {
    RenderContext__set_pixel_rgb565(this, px, py, color);
  }
}

void RenderContext_fill_rect(RenderContext* this, Rect* rect, uint16_t color) {
  int32_t py = rect->y;
  for (int32_t py = rect->y; py < (rect->y + rect->height); py++) {
    if (((py >= this->viewpoint.y) && (py < (this->viewpoint.y + this->viewpoint.height)))) {
      for (int32_t px = rect->x; px < (rect->x + rect->width); px++) {
        if ((this->format == PixelFormat_Mono)) {
          RenderContext__set_pixel_mono(this, px, (py - this->viewpoint.y), color);
        } else {
          RenderContext__set_pixel_rgb565(this, px, (py - this->viewpoint.y), color);
        }
        (px += 1);
      }
    }
  }
}

static inline void RenderContext__set_pixel_mono(RenderContext* this, int32_t x, int32_t y, uint16_t color) {
  int32_t index = (((y * (this->viewpoint.width + 7)) / 8) + (x / 8));
  if ((color == 0)) {
    (this->buffer.ptr[index] = (this->buffer.ptr[index] & ((uint8_t)((~(0x80 >> (x & 7)))))));
  } else {
    (this->buffer.ptr[index] = (this->buffer.ptr[index] | ((uint8_t)((0x80 >> (x & 7))))));
  }
}

static inline void RenderContext__set_pixel_rgb565(RenderContext* this, int32_t x, int32_t y, uint16_t color) {
  int32_t index = (((y * this->viewpoint.width) + x) * 2);
  (this->buffer.ptr[index] = ((uint8_t)((color >> 8))));
  (this->buffer.ptr[(index + 1)] = ((uint8_t)(color)));
}

static inline void memory__memory_set(__Slice_uint8_t dst, uint8_t value) {
  memset(dst.ptr, value, dst.size);
}

static inline void memory__memory_copy(__Slice_uint8_t dst, __Slice_uint8_t src, int32_t size) {
  memcpy(dst.ptr, src.ptr, size);
}

static inline void memory__memory_move(__Slice_uint8_t dst, __Slice_uint8_t src, int32_t size) {
  memmove(dst.ptr, src.ptr, size);
}

static inline void memory__memory_zero(__Slice_uint8_t dst) {
  memory__memory_set(dst, ((uint8_t)(0)));
}

static inline int32_t math__min_int32_t(int32_t a, int32_t b) {
  if ((a < b)) {
    return a;
  }
  return b;
}

static inline int32_t math__max_int32_t(int32_t a, int32_t b) {
  if ((a > b)) {
    return a;
  }
  return b;
}

int main(int argc, char** argv) { __mp_argc = argc; __mp_argv = argv; main__main(); return 0; }

