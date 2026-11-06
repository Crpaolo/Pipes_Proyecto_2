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
  uint64_t p; uint64_t* b; uint64_t* fresh; uint64_t id;
  out = malloc(8); b = malloc(8);
  check(pipe_read(1, b, 1) == MINUS1);
  check(pipe_destroy(1) == MINUS1);
  p = pipe_create();
  check(pipe_read(p, (uint64_t*) 8, 0) == 0);
  check(pipe_read(p, b, 1) == 0);
  check(pipe_write(p, "abcdefghijklmnop", 16) == 16);
  check(pipe_write(p, (uint64_t*) 8, 0) == 0);
  check(pipe_write(p, b, 1) == 0);
  check(pipe_read(p, (uint64_t*) 8, 1) == MINUS1);
  check(pipe_read(p, b, 8) == 8);
  check(*b == 7523094288207667809);
  check(pipe_read(p, b, 8) == 8);
  check(pipe_write(0, b, 1) == MINUS1);
  check(pipe_write(16, b, 1) == MINUS1);
  check(pipe_write(MINUS1, b, 1) == MINUS1);
  check(pipe_read(18446744073709551615, b, 1) == MINUS1);
  check(pipe_read(0, b, 0) == MINUS1);
  check(pipe_write(16, b, 0) == MINUS1);
  check(pipe_write(p, (uint64_t*) 18446744073709551615, 1) == MINUS1);
  check(pipe_write(p, (uint64_t*) 4294967296, 1) == MINUS1);
  check(pipe_write(p, (uint64_t*) 65536, 1) == MINUS1);
  fresh = malloc(16384);
  check(pipe_write(p, fresh + 1024, 8) == 8);
  check(pipe_read(p, b, 8) == 8);
  check(*b == 0);
  check(pipe_destroy(p) == 0);
  check(pipe_read(p, b, 0) == MINUS1);
  id = pipe_create();
  check(id == p);
  check(pipe_read(id, b, 1) == 0);
  check(pipe_destroy(id) == 0);
  return failures;
}
