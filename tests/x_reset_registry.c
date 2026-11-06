uint64_t main() {
  uint64_t p;
  p = pipe_create();
  if (p != 1) return 1;
  if (pipe_write(p, "Q", 1) != 1) return 2;
  return 0;
}
