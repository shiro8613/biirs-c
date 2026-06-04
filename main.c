#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define FILE_NAME "./test.bii"

// opcodes, registers, literals, max 15 chars + nul char(1byte)
#define LIT_BUFF_SIZE 15 + 1

// opcodes
#define OP_ORG 0x01
#define OP_LD 0x02
#define OP_MOV_REG 0x03
#define OP_MOV_PRI 0x04
#define OP_ADD 0x05
#define OP_SUB 0x06
#define OP_MUL 0x07
#define OP_DIV 0x08
#define OP_AND 0x09
#define OP_OR 0x0A
#define OP_XOR 0x0B
#define OP_PUSH 0x0C
#define OP_POP 0x0D
#define OP_SHOW 0x0E
#define OP_HLT 0x0F

// registers
#define RAX 0x00
#define RBX 0x01
#define RCX 0x02
#define RDX 0x03
#define R5 0x05
#define R6 0x06
#define R7 0x07
#define R8 0x08

void die(const char *msg) {
  printf("Error: %s\n", msg);
  exit(1);
}

typedef struct var_buffer {
  long *ptr;
  size_t capa;
  size_t len;
} var_buffer;

var_buffer *init_var_buffer() {
  var_buffer *instance = (var_buffer *)malloc(sizeof(var_buffer));
  if (instance) {
    instance->len = 0;
    instance->capa = 2;
    instance->ptr = (long *)malloc(sizeof(long) * instance->capa);
  }

  return instance;
}

void free_var_buffer(var_buffer *instance) {
  free(instance->ptr);
  free(instance);
}

void append_var_buffer(var_buffer *instance, long value) {
  if (instance->len >= instance->capa) {
    instance->capa *= 2;
    instance->ptr =
        (long *)realloc(instance->ptr, sizeof(long) * instance->capa);
  }

  instance->ptr[instance->len++] = value;
}

long get_var_buffer(var_buffer *instance, size_t idx) {
  if (instance->len < idx) {
    return -1;
  }

  return instance->ptr[idx];
}

typedef enum Mode {
  SKIP = 0,
  LIT = 1,
  COMMENT = 2,
} Mode;

bool is_valid_char(char c) {
  return isprint((unsigned char)c) && !isspace((unsigned char)c);
}

bool is_comment(char *buf, size_t len, size_t pos) {
  return buf[pos] == '/' && pos < len && buf[pos + 1] == '/';
}

int parse_text(char *buf, ssize_t len) {

  Mode mode = SKIP;
  size_t lit_len = 0;
  char lit_buf[LIT_BUFF_SIZE];

  for (ssize_t i = 0; i < len; i++) {
    char c = buf[i];

    switch (mode) {
    case SKIP:
      if (is_comment(buf, len, i)) {
        mode = COMMENT;
        continue;
      }
      if (is_valid_char(c)) {
        mode = LIT;
        lit_buf[lit_len++] = c;
      }
      break;
    case LIT:
      if (is_valid_char(c)) {
        lit_buf[lit_len++] = c;
      } else {
        lit_buf[lit_len++] = '\0';
        // todo parse lit
        printf("%s\n", lit_buf);
        mode = SKIP;
        lit_len = 0;
      }
      break;
    case COMMENT:
      if (c == '\n') {
        mode = SKIP;
      }
      break;
    }
  }

  return 0;
}

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
