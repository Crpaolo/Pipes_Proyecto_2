uint64_t main() {
  uint64_t* b;
  b = malloc(8);
  *b = 'A' + 'B' * 65536 + 255 * 16777216;
  if (write(1, b, 4) != 4) return 1;
  if (write(1, (uint64_t*) 8, 0) != 0) return 2;
  return 0;
}
