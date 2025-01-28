# csti - contest system terminal interface
# See LICENSE file for copyright and license details.

include config.mk

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

.PHONY: all clean $(BUILD_DIR)

all: $(BUILD_DIR) $(EXEC)

$(BUILD_DIR):
	mkdir -p $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c ${CFLAGS} $< -o $@ ${LDFLAGS}

$(EXEC): $(OBJS)
	$(CC) -o $(BUILD_DIR)/$@ $(OBJS) ${LDFLAGS}

${OBJS}: config.h config.mk

config.h:
	cp -n $(SRC_DIR)/config.def.h $(SRC_DIR)/$@

clean:
	rm -rf $(BUILD_DIR) $(EXEC) $(SRC_DIR)/config.h

install: $(EXEC)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f $(BUILD_DIR)/$(EXEC) $(DESTDIR)${PREFIX}/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/$(EXEC)
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(EXEC)
