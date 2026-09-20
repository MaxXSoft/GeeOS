#include <stdbool.h>

extern int test_main(void);
extern void runtime_exit(int code) __attribute__((noreturn));

static void print(const char *text) {
  volatile unsigned char *uart = (volatile unsigned char *)0x10000000;
  while (*text) {
    while (!(uart[5] & 0x40)) {}
    uart[0] = (unsigned char)*text++;
  }
}

void test_check(bool condition, const char *message) {
  if (!condition) {
    print("FAIL: ");
    print(message);
    print("\n");
    runtime_exit(1);
  }
}

int runtime_run(void) {
  int result = test_main();
  if (!result) print("PASS\n");
  return result;
}
