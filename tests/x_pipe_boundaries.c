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
  uint64_t p; uint64_t* block; uint64_t page; uint64_t addr;
  uint64_t* dest; uint64_t* tail; uint64_t i;
  out = malloc(8); dest = malloc(32); block = malloc(16384);
  p = pipe_create();
  page = ((uint64_t) block / 4096 + 1) * 4096;
  *((uint64_t*) (page - 8)) = 578437695752307201;
  *((uint64_t*) page) = 1157159078456920585;
  check(pipe_write(p, (uint64_t*) (page - 3), 6) == 6);
  *dest = 0;
  check(pipe_read(p, dest, 6) == 6);
  check(*dest == 12137729099526);
  check(pipe_write(p, "abcdefgh", 8) == 8);
  *((uint64_t*) (page + 4096 - 8)) = 0;
  *((uint64_t*) (page + 4096)) = 0;
  check(pipe_read(p, (uint64_t*) (page + 4096 - 3), 8) == 8);
  check(byte_at((uint64_t*) (page + 4096 - 8), 4) == 0);
  check(byte_at((uint64_t*) (page + 4096 - 8), 5) == 'a');
  check(byte_at((uint64_t*) (page + 4096), 0) == 'd');
  check(byte_at((uint64_t*) (page + 4096), 4) == 'h');
  check(byte_at((uint64_t*) (page + 4096), 5) == 0);
  tail = malloc(8);
  *tail = 'X' + 'Y' * 256 + 'Z' * 65536 + 'W' * 16777216 + 'A' * 4294967296 + 'B' * 1099511627776 + 'C' * 281474976710656 + 'D' * 72057594037927936;
  addr = (uint64_t) tail + 6;
  check(pipe_write(p, (uint64_t*) addr, 4) == 2);
  *dest = 0;
  check(pipe_read(p, dest, 8) == 2);
  check(*dest == 'C' + 'D' * 256);
  check(pipe_write(p, "WXYZ", 4) == 4);
  check(pipe_read(p, (uint64_t*) addr, 4) == 2);
  check(byte_at(tail, 5) == 'B');
  check(byte_at(tail, 6) == 'W');
  check(byte_at(tail, 7) == 'X');
  *dest = 0;
  check(pipe_read(p, dest, 8) == 2);
  check(*dest == 'Y' + 'Z' * 256);
  check(pipe_read(p, dest, 1) == 0);
  check(pipe_destroy(p) == 0);
  return failures;
}
