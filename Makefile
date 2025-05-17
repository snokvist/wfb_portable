COMMON_SRC := src/config.c src/key_loader.c
COMMON_OBJ := $(COMMON_SRC:.c=.o)

# add cJSON.c to third_party/ before compiling
$(COMMON_OBJ): CFLAGS += -Iinclude -Ithird_party
