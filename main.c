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
#define OPCODES_LEN 2
#define OPCODES_REG 0
#define OPCODES_LIT 1

typedef struct OpCode {
  const char *name;
  const uint8_t types[OPCODES_LEN];
} OpCode;

#define INIT_OPCODE(name, ...)                                                 \
  {                                                                            \
    name, { __VA_ARGS__ }                                                      \
  }

// opcodes
const OpCode OPCODES[16] = {
    INIT_OPCODE("org", 0x01),       INIT_OPCODE("ld", 0x02),
    INIT_OPCODE("mov", 0x03, 0x04), INIT_OPCODE("str", 0x05),
    INIT_OPCODE("add", 0x06),       INIT_OPCODE("sub", 0x07),
    INIT_OPCODE("mul", 0x08),       INIT_OPCODE("div", 0x09),
    INIT_OPCODE("and", 0x0a),       INIT_OPCODE("or", 0x0b),
    INIT_OPCODE("xor", 0x0c),       INIT_OPCODE("not", 0x0d),
    INIT_OPCODE("push", 0x0e),      INIT_OPCODE("pop", 0x0f),
    INIT_OPCODE("show", 0x10),      INIT_OPCODE("halt", 0x11),
};

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

  for (int i = 0; buf[i] != '\0'; i++) {
    buf[i] = tolower(buf[i]);
  }
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

typedef enum ParseState {
  WAIT,
  LITERAL,
  OK,
} ParseState;

typedef enum ParseResult {
  CONTINUE,
  SUCCESS,
  FAIL,
} ParseResult;

struct op_parser {
  ParseState state;
  char lit_buf[LIT_BUFF_SIZE];
  uint8_t reg;
  long from;
};

void parser_clear(struct op_parser *p) {
  p->state = WAIT;
  p->reg = 0x00;
  p->from = 0x00;
}

// todo: opcodes毎に変数長が違う問題を何とかする
// 現在は初手レジスタ固定だが、その限りでは無いものの対応をする

ParseResult parser_parse(struct op_parser *p, char *buf) {
  if (p->reg == 0x00) {
    if (p->state == WAIT) {
      strcpy(p->lit_buf, buf);
      p->state = LITERAL;
    } else if (p->state == LITERAL) {
      uint8_t r = to_register(buf);
      if (r == UNKNOWN) {
        return FAIL;
      }

      p->reg = r;
    }

    return CONTINUE;
  } else if (p->from == 0x00) {
    long l;
    char *endptr;
    l = strtol(buf, &endptr, 0);
    if (buf != endptr) {
      p->from = l;
    } else {
      uint8_t r = to_register(buf);
      if (r == UNKNOWN) {
        return FAIL;
      }
      p->from = r;
    }

    return SUCCESS;
  }

  return CONTINUE;
}

void parser_print(struct op_parser *p) {
  printf("%s %d %ld", p->lit_buf, p->reg, p->from);
}

typedef enum Mode {
  SKIP,
  LIT,
  COMMENT,
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
  struct op_parser parser = {0};
  parser_clear(&parser);

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
        to_lower(lit_buf);
        ParseResult res = parser_parse(&parser, lit_buf);
        if (res == FAIL) {
          return -1;
        } else if (res == SUCCESS) {
          parser_print(&parser);
          parser_clear(&parser);
        }

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
