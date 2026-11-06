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
  uint64_t p; uint64_t* b; uint64_t* pattern; uint64_t i; uint64_t round; uint64_t good;
  out = malloc(8); b = malloc(16); pattern = (uint64_t*) "ABCDEFGHIJKLMNOP";
  p = pipe_create(); round = 0; good = 1;
  while (round < 64) {
    if (pipe_write(p, pattern, 16) != 16) good = 0;
    if (pipe_read(p, b, 10) != 10) good = 0;
    i = 0; while (i < 10) { if (byte_at(b, i) != 'A' + i) good = 0; i = i + 1; }
    if (pipe_write(p, pattern, 10) != 10) good = 0;
    if (pipe_read(p, b, 16) != 16) good = 0;
    i = 0; while (i < 6) { if (byte_at(b, i) != 'K' + i) good = 0; i = i + 1; }
    i = 6; while (i < 16) { if (byte_at(b, i) != 'A' + i - 6) good = 0; i = i + 1; }
    round = round + 1;
  }
  check(good);
  check(pipe_read(p, b, 1) == 0);
  check(pipe_destroy(p) == 0);
  return failures;
}
