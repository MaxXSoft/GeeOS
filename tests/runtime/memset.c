#include <stddef.h>
#include <stdint.h>

extern void *memset(void *dst, int value, size_t count);

static unsigned char buffer[4128] __attribute__((aligned(16)));

static int check(size_t offset, size_t count, int value) {
  const size_t begin = 16 + offset;
  const size_t end = begin + count;
  const size_t limit = end + 8;
  const unsigned char byte = (unsigned char)value;
  for (size_t i = 0; i < limit; ++i) {
    buffer[i] = (unsigned char)~byte;
  }
  if (memset(buffer + begin, value, count) != buffer + begin) {
    return 1;
  }
  for (size_t i = 0; i < limit; ++i) {
    const unsigned char expected = i >= begin && i < end
                                       ? byte : (unsigned char)~byte;
    if (buffer[i] != expected) {
      return 2;
    }
  }
  return 0;
}

int test_main(void) {
  static const int values[] = {0, 1, 5, 0x80, 0xff, 0x123, -1, -128};
  static const size_t lengths[] = {95, 127, 128, 129, 255, 256, 257,
                                   4095, 4096, 4097};
  for (size_t offset = 0; offset < 8; ++offset) {
    for (size_t v = 0; v < sizeof(values) / sizeof(values[0]); ++v) {
      for (size_t count = 0; count <= 65; ++count) {
        if (check(offset, count, values[v])) return 1;
      }
      for (size_t n = 0; n < sizeof(lengths) / sizeof(lengths[0]); ++n) {
        if (check(offset, lengths[n], values[v])) return 2;
      }
    }
  }
  return 0;
}
