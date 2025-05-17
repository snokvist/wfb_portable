# ── compiler flags ──────────────────────────────────────
CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -O2 -Iinclude -Ithird_party -DNO_SSL -DUSE_IPV6
LDFLAGS := -lpthread -ldl

# ── source lists ────────────────────────────────────────
COMMON_SRC  := \
    src/config.c \
    src/key_loader.c \
    src/ws_proto.c \
    src/proc_mgr.c \
    third_party/cJSON.c \
    $(CIVET_SRC)

MASTER_SRC  := src/master_main.c src/master/ws_server.c
NODE_SRC    := src/node/ws_client.c

COMMON_OBJ  := $(COMMON_SRC:.c=.o)
MASTER_OBJ  := $(MASTER_SRC:.c=.o)
NODE_OBJ    := $(NODE_SRC:.c=.o)

CIVET_SRC := third_party/civetweb.c
COMMON_SRC += $(CIVET_SRC)

# ── output locations ───────────────────────────────────
OUT_DIR  := build
OUT_BIN  := $(OUT_DIR)/broadcast-master
OUT_NODE := $(OUT_DIR)/broadcast-node

# ── default target builds both binaries ─────────────────
all: $(OUT_BIN) $(OUT_NODE)

# ── linking rules ───────────────────────────────────────
$(OUT_BIN): $(OUT_DIR) $(COMMON_OBJ) $(MASTER_OBJ)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(LDFLAGS)

$(OUT_NODE): $(OUT_DIR) $(COMMON_OBJ) $(NODE_OBJ)
	$(CC) $(CFLAGS) -o $@ $(filter %.o,$^) $(LDFLAGS)

# ── generic pattern for .o ──────────────────────────────
%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# ── output dir helper ───────────────────────────────────
$(OUT_DIR):
	@mkdir -p $(OUT_DIR)

clean:
	rm -rf $(OUT_DIR) $(COMMON_OBJ) $(MASTER_OBJ) $(NODE_OBJ)

.PHONY: all clean
