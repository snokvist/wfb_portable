# ── config ───────────────────────────────────────────────
CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 -Iinclude -Ithird_party
LDFLAGS :=

# ── sources ──────────────────────────────────────────────
COMMON_SRC  := src/config.c src/key_loader.c third_party/cJSON.c
MASTER_SRC  := src/master_main.c
OBJ         := $(COMMON_SRC:.c=.o) $(MASTER_SRC:.c=.o)
OUT_DIR     := build
OUT_BIN     := $(OUT_DIR)/broadcast-master
NODE_SRC   := src/node/ws_client.c
NODE_OBJ   := $(NODE_SRC:.c=.o)
OUT_NODE   := $(OUT_DIR)/broadcast-node

# ── build rules ──────────────────────────────────────────
all: $(OUT_BIN)


$(OUT_NODE): $(OUT_DIR) $(COMMON_SRC:.c=.o) $(NODE_OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built $@"


$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

$(OUT_BIN): $(OUT_DIR) $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LDFLAGS)
	@echo "Built $@"

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OUT_DIR) $(OBJ)

.PHONY: all clean
