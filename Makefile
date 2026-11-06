CC = gcc
CFLAGS = -w -O2 -m64
CPPFLAGS = -D'uint64_t=unsigned long'
PYTHON = python3

.PHONY: all verify sanitize clean
all: selfie

selfie: selfie.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $< -o $@

verify:
	$(PYTHON) verify.py
	$(PYTHON) verify_extra.py

sanitize:
	$(PYTHON) verify_sanitizers.py

clean:
	rm -f selfie
	rm -rf build __pycache__
