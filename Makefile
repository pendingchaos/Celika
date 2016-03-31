PROJNAME = unnamedgame
CFLAGS = -pedantic -Wall -std=c11 -g `sdl2-config --cflags` -DPROJNAME=\"$(PROJNAME)\"
LDFLAGS = `sdl2-config --libs` -lGL -lm
OUTPUT = $(PROJNAME)
RUN_OPT =
PREFIX = /usr/local

TARGET = $@
PREREQ1 = $<

src = $(wildcard src/*.c)
obj = $(join $(dir $(src:.c=.o)), $(addprefix ., $(notdir $(src:.c=.o))))
dep = $(obj:.o=.d)
prefix = $(DESTDIR)$(PREFIX)

.PHONY: all
all: $(OUTPUT)

$(OUTPUT): $(obj)
	@$(CC) $(obj) $(LDFLAGS) -o $(TARGET)

-include $(dep)

.%.d: %.c
	@# -M:  generate Makefile rule
	@# -MT: specify the rule's target
	@$(CPP) $(CFLAGS) $(PREREQ1) -M -MT $(@:.d=.o) >$(TARGET)

.%.o: %.c Makefile
	@$(CC) -c $(CFLAGS) $(PREREQ1) -o $(TARGET)

.PHONY: clean
clean:
	@rm -f $(dep) $(obj) $(OUTPUT)

.PHONY: install
install: $(OUTPUT)

.PHONY: uninstall
uninstall:

.PHONY: valgrind
valgrind: $(OUTPUT)
	@valgrind --track-origins=yes --leak-check=full ./$(OUTPUT) $(RUN_OPT)

.PHONY: apitrace
apitrace: $(OUTPUT)
	@apitrace trace ./$(OUTPUT)
	@qapitrace $(OUTPUT).trace
	@rm $(OUTPUT).trace

.PHONY: run
run: $(OUTPUT)
	@./$(OUTPUT) $(RUN_OPT)
