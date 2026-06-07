#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILE_NAME "./test.bii"

// opcodes, registers, literals, max 15 chars + nul char(1byte)
#define LIT_BUFF_SIZE 15 + 1

#define UNKNOWN 0xFF
#define OPCODES_LEN 16
#define OPCODES_TYPE_LEN 2

typedef struct opcode_t {
  const char *name;
  const size_t len;
  const uint8_t types[OPCODES_TYPE_LEN];
} opcode_t;

#define INIT_OPCODE(name, len, ...)                                            \
  {                                                                            \
    name, len, { __VA_ARGS__ }                                                 \
  }

// opcodes
const opcode_t OPCODES[OPCODES_LEN] = {
    INIT_OPCODE("org", 1, 0x01),       INIT_OPCODE("ld", 1, 0x02),
    INIT_OPCODE("mov", 2, 0x03, 0x04), INIT_OPCODE("str", 1, 0x05),
    INIT_OPCODE("add", 2, 0x06),       INIT_OPCODE("sub", 2, 0x07),
    INIT_OPCODE("mul", 2, 0x08),       INIT_OPCODE("div", 2, 0x09),
    INIT_OPCODE("and", 2, 0x0a),       INIT_OPCODE("or", 2, 0x0b),
    INIT_OPCODE("xor", 2, 0x0c),       INIT_OPCODE("not", 2, 0x0d),
    INIT_OPCODE("push", 1, 0x0e),      INIT_OPCODE("pop", 1, 0x0f),
    INIT_OPCODE("show", 1, 0x10),      INIT_OPCODE("halt", 0, 0x11),
};

struct opcode_i {
  size_t idx;
  size_t len;
};

void opcode_clear(struct opcode_i *p) {
  p->idx = 0;
  p->len = 0;
}

int opcode_find(struct opcode_i *op, char *buf) {
  for (size_t i = 0; i < OPCODES_LEN; i++) {
    const opcode_t opcode = OPCODES[i];
    if (strcmp(buf, opcode.name) == 0) {
      op->idx = i;
      op->len = opcode.len;
      return 0;
    }
  }

  return -1;
}

#define CREATE_REGISTER(buf, name, val)                                        \
  if (strcmp(buf, name) == 0)                                                  \
    return val;

// registers
uint8_t to_register(char *buf) {
  CREATE_REGISTER(buf, "rax", 0x01)
  CREATE_REGISTER(buf, "rbx", 0x02)
  CREATE_REGISTER(buf, "rcx", 0x03)
  CREATE_REGISTER(buf, "rdx", 0x04)
  CREATE_REGISTER(buf, "r5", 0x05)
  CREATE_REGISTER(buf, "r6", 0x06)
  CREATE_REGISTER(buf, "r7", 0x07)
  CREATE_REGISTER(buf, "r8", 0x08)
  return UNKNOWN;
}

void die(const char *msg) {
  printf("Error: %s\n", msg);
  exit(1);
}

void to_lower(char *buf) {
  if (!buf) {
    return;
  }

  for (size_t i = 0; buf[i] != '\0'; i++) {
    buf[i] = tolower(buf[i]);
  }
}

typedef struct var_buffer {
  uint8_t *ptr;
  size_t capa;
  size_t len;
} var_buffer;

var_buffer *init_var_buffer() {
  var_buffer *instance = (var_buffer *)malloc(sizeof(var_buffer));
  if (instance) {
    instance->len = 0;
    instance->capa = 2;
    instance->ptr = (uint8_t *)malloc(sizeof(uint8_t) * instance->capa);
  }

  return instance;
}

void free_var_buffer(var_buffer *instance) {
  free(instance->ptr);
  free(instance);
}

void append_var_buffer(var_buffer *instance, uint8_t value) {
  if (instance->len >= instance->capa) {
    instance->capa *= 2;
    instance->ptr =
        (uint8_t *)realloc(instance->ptr, sizeof(uint8_t) * instance->capa);
  }

  instance->ptr[instance->len++] = value;
}

uint8_t get_var_buffer(var_buffer *instance, size_t idx) {
  if (instance->len < idx) {
    return -1;
  }

  return instance->ptr[idx];
}

bool is_valid_char(char c) {
  return isprint((unsigned char)c) && !isspace((unsigned char)c);
}

bool is_comment(char *buf, size_t len, size_t pos) {
  return buf[pos] == '/' && pos < len && buf[pos + 1] == '/';
}

int parse_text(char *buf, ssize_t len) { return 0; }

int main(void) {
  FILE *fp = fopen(FILE_NAME, "r");
  if (!fp) {
    die("File open");
    return 1;
  }

  fseek(fp, 0, SEEK_END);
  ssize_t len = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  char *buf = (char *)malloc(sizeof(char) * len);
  if (!buf) {
    die("Buffer alloc");
    return 1;
  }

  if (!fread(buf, sizeof(char), len, fp)) {
    die("File read");
    return 1;
  }

  var_buffer *output = init_var_buffer();

  int result = parse_text(buf, len);

  free_var_buffer(output);
  free(buf);

  if (result) {
    die("compile was not complited");
  }

  return 0;
}
