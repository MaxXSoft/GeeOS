#include <stdbool.h>
#include <stddef.h>

extern int memcmp(const void *, const void *, size_t);
extern void test_check(bool condition, const char *message);

int test_main(void) {
  unsigned char a, b;
  test_check(memcmp(NULL, NULL, 0) == 0, "empty comparison does not read");
  // Check the sign for every unsigned-byte pair, including 0x80..0xff.
  for (unsigned lhs = 0; lhs < 256; ++lhs) {
    for (unsigned rhs = 0; rhs < 256; ++rhs) {
      a = (unsigned char)lhs;
      b = (unsigned char)rhs;
      int result = memcmp(&a, &b, 1);
      test_check((result > 0) - (result < 0) == (lhs > rhs) - (lhs < rhs),
                 "unsigned byte comparison sign");
    }
  }
  volatile unsigned char lhs[32], rhs[32];
  for (unsigned offset = 0; offset < 4; ++offset) {
    for (unsigned length = 1; length <= 16; ++length) {
      for (unsigned i = 0; i < 32; ++i) {
        lhs[i] = rhs[i] = (unsigned char)(i + 11);
      }
      test_check(memcmp((const void *)(lhs + offset),
                        (const void *)(rhs + offset), length) == 0,
                 "equal range at each alignment");
      lhs[offset + length - 1] = 0;
      rhs[offset + length - 1] = 255;
      test_check(memcmp((const void *)(lhs + offset),
                        (const void *)(rhs + offset), length) < 0,
                 "difference after equal prefix");
      test_check(memcmp((const void *)(lhs + offset),
                        (const void *)(rhs + offset), length - 1) == 0,
                 "bytes past count ignored");
      if (length > 1) {
        lhs[offset] = 255;
        rhs[offset] = 0;
        test_check(memcmp((const void *)(lhs + offset),
                          (const void *)(rhs + offset), length) > 0,
                   "first difference determines sign");
      }
    }
  }
  return 0;
}
