CC      := gcc
CFLAGS  := -std=c11 -Wall -Wextra -Wpedantic -O2 -march=native
DBGFLAGS:= -std=c11 -Wall -Wextra -g3 -fsanitize=address,undefined -O0
LDFLAGS := -lm

# ── Targets ───────────────────────────────────────────────────────────
TARGET  := converter
SRC     := main.c
HDR     := converter.h

# ── Colours for terminal output ───────────────────────────────────────
GRN := \033[38;5;82m
CYN := \033[38;5;51m
RED := \033[38;5;196m
RST := \033[0m

# ══ DEFAULT: release build ════════════════════════════════════════════
.PHONY: all
all: $(TARGET)
	@printf "$(GRN)  ✓  Release build complete → ./$(TARGET)$(RST)\n"

$(TARGET): $(SRC) $(HDR)
	@printf "$(CYN)  ▶  Compiling $(TARGET) ...$(RST)\n"
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LDFLAGS)

# ══ DEBUG: AddressSanitizer + UBSan ═══════════════════════════════════
.PHONY: debug
debug: $(SRC) $(HDR)
	@printf "$(CYN)  ▶  Debug build (ASan + UBSan) ...$(RST)\n"
	$(CC) $(DBGFLAGS) -o $(TARGET)_debug $(SRC) $(LDFLAGS)
	@printf "$(GRN)  ✓  Debug build → ./$(TARGET)_debug$(RST)\n"

# ══ PROFILE: gprof-ready ══════════════════════════════════════════════
.PHONY: profile
profile: $(SRC) $(HDR)
	$(CC) -std=c11 -Wall -O2 -pg -o $(TARGET)_prof $(SRC) $(LDFLAGS)
	@printf "$(GRN)  ✓  Profile build → ./$(TARGET)_prof$(RST)\n"
	@printf "     Run it, then: gprof $(TARGET)_prof gmon.out | less\n"

# ══ STATIC: single portable binary ════════════════════════════════════
.PHONY: static
static: $(SRC) $(HDR)
	$(CC) $(CFLAGS) -static -o $(TARGET)_static $(SRC) $(LDFLAGS)
	@printf "$(GRN)  ✓  Static binary → ./$(TARGET)_static$(RST)\n"

# ══ INSTALL: copy to /usr/local/bin ═══════════════════════════════════
.PHONY: install
install: all
	@printf "$(CYN)  ▶  Installing to /usr/local/bin/$(TARGET) ...$(RST)\n"
	install -m 755 $(TARGET) /usr/local/bin/$(TARGET)
	@printf "$(GRN)  ✓  Installed. Run: $(TARGET) --help$(RST)\n"

.PHONY: uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@printf "$(RED)  ✗  Uninstalled $(TARGET)$(RST)\n"

# ══ CLEAN ══════════════════════════════════════════════════════════════
.PHONY: clean
clean:
	rm -f $(TARGET) $(TARGET)_debug $(TARGET)_prof $(TARGET)_static \
	      gmon.out *.log *.bin *.csv
	@printf "$(RED)  ✗  Clean complete$(RST)\n"

.PHONY: distclean
distclean: clean
	rm -f converter.ini wallet.bin rates_cache.bin
	@printf "$(RED)  ✗  Dist-clean complete$(RST)\n"

# ══ TEST: automated smoke tests ════════════════════════════════════════
.PHONY: test
test: all
	@printf "$(CYN)  ▶  Running smoke tests ...$(RST)\n"
	@echo "Testing scripting mode (USD→INR 100):"
	@./$(TARGET) USD INR 100
	@echo ""
	@echo "Testing scripting mode (EUR→GBP 50):"
	@./$(TARGET) EUR GBP 50
	@echo ""
	@echo "Testing scripting mode (BTC→USD 0.5):"
	@./$(TARGET) BTC USD 0.5
	@printf "$(GRN)  ✓  All smoke tests passed$(RST)\n"

# ══ VALGRIND: memory-leak check ════════════════════════════════════════
.PHONY: memcheck
memcheck: debug
	valgrind --leak-check=full --show-leak-kinds=all \
	         --track-origins=yes --error-exitcode=1 \
	         ./$(TARGET)_debug USD INR 100

# ══ LINES OF CODE: project stats ══════════════════════════════════════
.PHONY: stats
stats:
	@printf "$(CYN)  Project Statistics:$(RST)\n"
	@wc -l $(SRC) $(HDR)
	@printf "  Features  : 100\n"
	@printf "  Currencies: 120+\n"
	@printf "  Standard  : C11\n"

# ══ HELP ═══════════════════════════════════════════════════════════════
.PHONY: help
help:
	@printf "$(GRN)\n  Makefile Targets:\n$(RST)"
	@printf "$(CYN)  make          $(RST)— Release build (-O2)\n"
	@printf "$(CYN)  make debug    $(RST)— Debug build (ASan + UBSan)\n"
	@printf "$(CYN)  make profile  $(RST)— gprof profiling build\n"
	@printf "$(CYN)  make static   $(RST)— Static portable binary\n"
	@printf "$(CYN)  make test     $(RST)— Run smoke tests\n"
	@printf "$(CYN)  make memcheck $(RST)— Valgrind leak check\n"
	@printf "$(CYN)  make install  $(RST)— Install to /usr/local/bin\n"
	@printf "$(CYN)  make stats    $(RST)— Lines of code + project info\n"
	@printf "$(CYN)  make clean    $(RST)— Remove build artifacts\n"
	@printf "$(CYN)  make distclean$(RST)— Remove builds + data files\n\n"
	@printf "  Scripting mode (no interactive prompt):\n"
	@printf "    ./converter <FROM> <TO> <AMOUNT>\n"
	@printf "    ./converter USD INR 100\n\n"
