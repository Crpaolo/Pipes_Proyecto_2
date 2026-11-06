uint64_t* out;
uint64_t MINUS1 = 4294967295;
uint64_t failures;
uint64_t checks;

void put(uint64_t c) { *out = c; write(1, out, 1); }
void check(uint64_t pass) {
  checks = checks + 1;
  if (pass) { put('o'); put('k'); }
  else { put('F'); put('A'); put('I'); put('L'); failures = failures + 1; }
  put(' '); put('0' + checks / 10); put('0' + checks % 10); put(10);
}
void spin(uint64_t n) { uint64_t i; i = 0; while (i < n) i = i + 1; }
uint64_t byte_at(uint64_t* b, uint64_t index) {
  uint64_t word; uint64_t i;
  word = *(b + index / 8); i = 0;
  while (i < index % 8) { word = word / 256; i = i + 1; }
  return word % 256;
}
uint64_t main() {
  uint64_t p; uint64_t* b; uint64_t* dst;
  out = malloc(8); b = malloc(8); dst = malloc(16); p = pipe_create();
  *b = 0 + 1 * 256 + 127 * 65536 + 128 * 16777216 + 255 * 4294967296 + 'Q' * 1099511627776;
  *dst = 18446744073709551615;
  *(dst + 1) = 18446744073709551615;
  check(pipe_write(p, b, 6) == 6);
  check(pipe_read(p, (uint64_t*) ((uint64_t) dst + 1), 6) == 6);
  check(byte_at(dst, 0) == 255);
  check(byte_at(dst, 1) == 0);
  check(byte_at(dst, 2) == 1);
  check(byte_at(dst, 3) == 127);
  check(byte_at(dst, 4) == 128);
  check(byte_at(dst, 5) == 255);
  check(byte_at(dst, 6) == 'Q');
  check(byte_at(dst, 7) == 255);
  check(*(dst + 1) == 18446744073709551615);
  check(pipe_destroy(p) == 0);
  return failures;
}
