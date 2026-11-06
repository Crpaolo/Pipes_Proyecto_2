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
uint64_t x;
uint64_t main() {
  uint64_t p; uint64_t pid; uint64_t previous; uint64_t i;
  uint64_t* b; uint64_t* heap; uint64_t got; uint64_t sum; uint64_t rounds;
  uint64_t local;
  out = malloc(8); b = malloc(8); heap = malloc(16384);
  *heap = 77; *(heap + 1536) = 66; local = 88; x = 99; p = pipe_create(); previous = 0; i = 1;
  while (i <= 3) {
    pid = fork();
    if (pid == 0) {
      *heap = i; *(heap + 1536) = i; *(heap + 1024) = i; local = i; x = i; *b = i;
      if (pipe_write(p, b, 1) != 1) exit(10);
      exit(0);
    }
    check(pid > previous);
    check(pid != MINUS1);
    previous = pid; i = i + 1;
  }
  got = 0; sum = 0; rounds = 0;
  while (got < 3) {
    if (pipe_read(p, b, 1) == 1) { sum = sum + *b % 256; got = got + 1; }
    spin(1000); rounds = rounds + 1;
    if (rounds > 10000) return 9;
  }
  check(sum == 6);
  check(*heap == 77);
  check(*(heap + 1536) == 66);
  check(*(heap + 1024) == 0);
  check(local == 88);
  check(x == 99);
  check(pipe_destroy(p) == 0);
  return failures;
}
