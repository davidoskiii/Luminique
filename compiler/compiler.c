#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../object/object.h"
#include "../common.h"
#include "../memory/memory.h"
#include "../scanner/scanner.h"
#include "../native/native.h"
#include "../hash/hash.h"
#include "../string/string.h"
#include "../dsa/dsa.h"
#include "compiler.h"

#ifdef DEBUG_PRINT_CODE
#include "../debug/debug.h"
#endif

#define MAX_CASES 256

typedef struct {
  Token pprevious;
  Token previous;
  Token current;
  Token next;
  Token rootClass;
  bool hadError;
  bool panicMode;
} Parser;

typedef void (*ParseFn)(bool canAssign);

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_BIT_AND,     // &
  PREC_BIT_XOR,     // ^
  PREC_BIT_OR,      // !
  PREC_SHIFT,       // << >> 
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY
} Precedence;

typedef struct {
  ParseFn prefix;
  ParseFn infix;
  Precedence precedence;
} ParseRule;

typedef struct {
  Token name;
  int depth;
  bool isCaptured;
  bool isMutable;
} Local;

typedef struct {
  uint8_t index;
  bool isLocal;
  bool isMutable;
} Upvalue;

typedef enum {
  TYPE_FUNCTION,
  TYPE_ABSTRACT,
  TYPE_INITIALIZER,
  TYPE_METHOD,
  TYPE_GETTER,
  TYPE_SETTER,
  TYPE_LAMBDA,
  TYPE_SCRIPT
} FunctionType;

typedef struct Compiler {
  struct Compiler* enclosing;
  ObjFunction* function;
  FunctionType type;

  Local locals[UINT8_COUNT];
  int localCount;
  Upvalue upvalues[UINT8_COUNT];

  int scopeDepth;
  int innermostLoopStart;
  int innermostLoopScopeDepth;
  bool isAsync;
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
  Token name;
  Token superclass;
  bool isStaticMethod;
} ClassCompiler;

static Stack namespaceStack;
Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

static void emitConstant(Value value);
static uint16_t makeConstant(Value value);

static Chunk* currentChunk() {
  return &current->function->chunk;
}

char* stripSpaces(const char* line) {
  while (*line == ' ') line++;
  return strdup(line);
}

char* stringPrecision(const char* str, int precision) {
  if (str == NULL || precision <= 0) return NULL;

  size_t length = strlen(str);
  char* newStr = (char*)malloc((precision + 1) * sizeof(char));
  if (!newStr) return NULL;

  int i;
  for (i = 0; i < length && i < precision; i++) {
    newStr[i] = str[i];
  }
  newStr[i] = '\0';

  return newStr;
}


int findPosition(const char* haystack, const char* needle) {
  if (haystack == NULL || needle == NULL) {
    return -1;
  }

  char* found = strstr(haystack, needle);
  if (found == NULL) {
    return -1;
  }

  return found - haystack;
}

char* tokenString(const Token* token) {
  if (token == NULL || token->length <= 0) return NULL;

  char* str = (char*)malloc(token->length + 1);
  if (!str) return NULL;

  for (int i = 0; i < token->length; i++) {
    str[i] = '~';
  }

  if (token->length > 0) str[0] = '^';

  str[token->length] = '\0';

  return str;
}

char* arrowsTokenString(Token* token, const char* line) {
  int length = strlen(line);
  char* arrows = (char*)malloc(length + 1);
  memset(arrows, ' ', length);
  arrows[length] = '\0';

  int start = token->startColumn - 1;
  int end = token->endColumn;
  if (start < length) {
    arrows[start] = '^';
  }
  for (int i = start + 1; i < end && i < length; i++) {
    arrows[i] = '~';
  }

  return arrows;
}

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;

  fprintf(stderr, "\n\033[1m%s:%d", vm.currentModule->path->chars, token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end: ");
    fprintf(stderr, "%s\n", message);
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s': \033[0m", token->length, token->start);
    fprintf(stderr, "%s\n", message);

    char* line;
    if (vm.repl) {
      line = stripSpaces(vm.currentModule->source);
    } else {
      line = stripSpaces(getLine(vm.currentModule->source, token->line));
    }

    char* arrows = arrowsTokenString(token, line);

    int lineNumberWidth = digitsInNumber(token->line);
    char* spaces = (char*)malloc(lineNumberWidth + 1);
    memset(spaces, ' ', lineNumberWidth);
    spaces[lineNumberWidth] = '\0';

    fprintf(stderr, "   %d | %s\n   %s | \033[31;1m%s\033[0m\n   %s |\n", token->line, line, spaces, arrows, spaces);

    free(arrows);
    free(spaces);
  }

  parser.hadError = true;
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void errorAtPrevious(const char* message) {
  errorAt(&parser.previous, message);
}

static void advance() {
  parser.pprevious = parser.previous;
  parser.previous = parser.current;
  parser.current = parser.next;

  for (;;) {
    parser.next = scanToken();
    if (parser.next.type != TOKEN_ERROR) break;

    errorAtCurrent(parser.next.start);
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }

  errorAtPrevious(message);
}

static bool check(TokenType type) {
  return parser.current.type == type;
}

bool checkNext(TokenType type) {
  return parser.next.type == type;
}

bool checkPrevious(TokenType type) {
  return parser.previous.type == type;
}

static bool match(TokenType type) {
  if (!check(type)) return false;
  advance();
  return true;
}

static void emitByte(uint8_t byte) {
  writeChunk(currentChunk(), byte, parser.previous.line);
}

static void emitBytes(uint8_t byte1, uint8_t byte2) {
  emitByte(byte1);
  emitByte(byte2);
}

static void emitShort(uint16_t s) {
  emitByte((s >> 8) & 0xff);
  emitByte(s & 0xff);
}

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);
  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitShort(offset);
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitShort(0xffff);
  return currentChunk()->count - 2;
}

static void emitReturn(uint8_t depth) {
  if (current->type == TYPE_INITIALIZER) {
    emitBytes(OP_GET_LOCAL, 0);
  } else {
    emitByte(OP_NIL);
  }

  if (depth == 0) {
    emitByte(OP_RETURN);
  } else {
    emitBytes(OP_RETURN_NONLOCAL, depth);
  }
}

static void emitConstant(Value value) {
  uint16_t constant = makeConstant(value);

  if (constant < UINT8_MAX) {
    emitByte(OP_CONSTANT);
    emitByte((uint8_t)constant);
  } else if (constant < UINT16_MAX) {
    emitByte(OP_CONSTANT_16);
    emitShort(constant);
  }
}

static void patchJump(int offset) {
  // -2 to adjust for the bytecode for the jump offset itself.
  int jump = currentChunk()->count - offset - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  currentChunk()->code[offset] = (jump >> 8) & 0xff;
  currentChunk()->code[offset + 1] = jump & 0xff;
}

uint16_t identifierConstant(Token* name) {
  const char* start = name->start[0] != '`' ? name->start : name->start + 1;
  int length = name->start[0] != '`' ? name->length : name->length - 2;
  return makeConstant(OBJ_VAL(copyString(start, length)));
}

static void emitIdentifier(Token* token) {
  emitBytes(OP_CONSTANT, identifierConstant(token));
}

static void patchAddress(int offset) {
  currentChunk()->code[offset] = (currentChunk()->count >> 8) & 0xff;
  currentChunk()->code[offset + 1] = currentChunk()->count & 0xff;
}

static void endLoop() {
  int offset = current->innermostLoopStart;
  Chunk* chunk = currentChunk();
  while (offset < chunk->count) {
    if (chunk->code[offset] == OP_END) {
      chunk->code[offset] = OP_JUMP;
      patchJump(offset + 1);
    } else {
      offset += opCodeOffset(chunk, offset);
    }
  }
}

static void initCompiler(Compiler* compiler, FunctionType type, bool isAsync) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->isAsync = isAsync;
  compiler->function = newFunction();
  compiler->function->isAsync = isAsync;
  compiler->scopeDepth = 0;
  compiler->innermostLoopStart = -1;
  compiler->innermostLoopScopeDepth = 0;
  current = compiler;

  initStack(&namespaceStack);

  if (type != TYPE_SCRIPT) {
    if (parser.previous.length == 8 && memcmp(parser.previous.start, "function", 8) == 0) {
      compiler->function->name = copyString("", 0);
    } else {
      compiler->function->name = copyString(parser.previous.start, parser.previous.length);
    }
  }

  Local* local = &current->locals[current->localCount++];
  local->depth = 0;
  local->isCaptured = false;
  
  if (type != TYPE_FUNCTION && type != TYPE_LAMBDA) {
    local->name.start = "this";
    local->name.length = 4;
  } else {
    local->name.start = "";
    local->name.length = 0;
  }
}

static uint16_t makeConstant(Value value) {
  uint16_t constant = addConstant(currentChunk(), value);
  if (constant > UINT16_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint16_t)constant;
}


static ObjString* identifierName(uint8_t arg) {
  return AS_STRING(currentChunk()->constants.values[arg]);
}

static ObjFunction* endCompiler() {
  emitReturn(0);
  ObjFunction* function = current->function;

#ifdef DEBUG_PRINT_CODE
 if (!parser.hadError) {
   disassembleChunk(currentChunk(), function->name != NULL
       ? function->name->chars : "<script>");
 }
#endif
  
  current = current->enclosing;
  return function;
}

static void beginScope() {
  current->scopeDepth++;
}

static void endScope() {
  current->scopeDepth--;

  while (current->localCount > 0 &&
         current->locals[current->localCount - 1].depth >
            current->scopeDepth) {
    if (current->locals[current->localCount - 1].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    } else {
      emitByte(OP_POP);
    }
    current->localCount--;
  }
}

static void expression();
static void block();
static void function(FunctionType type, bool isAsync);
static void statement();
static void declaration();
static ParseRule* getRule(TokenType type);
static void parsePrecedence(Precedence precedence);

static bool identifiersEqual(Token* a, Token* b) {
  if (a->length != b->length) return false;
  return memcmp(a->start, b->start, a->length) == 0;
}


static Token syntheticToken(const char* text) {
  Token token;
  token.start = text;
  token.length = (int)strlen(text);
  return token;
}


static uint16_t propretyConstant(const char* message) {
  switch (parser.current.type) {
    case TOKEN_IDENTIFIER:
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_AMP:
    case TOKEN_PIPE:
    case TOKEN_CARRET:
    case TOKEN_SHOWEL_R:
    case TOKEN_SHOWEL_L:
    case TOKEN_GREATER:
    case TOKEN_LESS:
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_MODULO:
    case TOKEN_POWER:
      advance();
      return identifierConstant(&parser.previous);
    case TOKEN_LEFT_BRAKE:
      advance();
      if (match(TOKEN_RIGHT_BRAKE)) {
        Token token = syntheticToken(match(TOKEN_EQUAL) ? "[]=" : "[]");
        return identifierConstant(&token);
      } else {
        errorAtCurrent(message);
        return -1;
      }
    case TOKEN_LEFT_PAREN:
      advance();
      if (match(TOKEN_RIGHT_PAREN)) {
        Token token = syntheticToken("()");
        return identifierConstant(&token);
      } else {
        errorAtCurrent(message);
        return -1;
      }
    default:
      errorAtCurrent(message);
      return -1;
  }
}

static int resolveLocal(Compiler* compiler, Token* name) {
  for (int i = compiler->localCount - 1; i >= 0; i--) {
    Local* local = &compiler->locals[i];
    if (identifiersEqual(name, &local->name)) {
      if (local->depth == -1) {
        error("Can't read local variable in its own initializer.");
      }
      return i;
    }
  }

  return -1;
}

static int addUpvalue(Compiler* compiler, uint8_t index, bool isLocal, bool isMutable) {
  int upvalueCount = compiler->function->upvalueCount;

  for (int i = 0; i < upvalueCount; i++) {
    Upvalue* upvalue = &compiler->upvalues[i];
    if (upvalue->index == index && upvalue->isLocal == isLocal) {
      return i;
    }
  }

  if (upvalueCount == UINT8_COUNT) {
    error("Too many closure variables in function.");
    return 0;
  }

  compiler->upvalues[upvalueCount].isLocal = isLocal;
  compiler->upvalues[upvalueCount].index = index;
  compiler->upvalues[upvalueCount].isMutable = isMutable;
  return compiler->function->upvalueCount++;
}

static int resolveUpvalue(Compiler* compiler, Token* name) {
  if (compiler->enclosing == NULL) return -1;

  int local = resolveLocal(compiler->enclosing, name);
  if (local != -1) {
    compiler->enclosing->locals[local].isCaptured = true;
    return addUpvalue(compiler, (uint8_t)local, true, compiler->enclosing->locals[local].isMutable);
  }

  int upvalue = resolveUpvalue(compiler->enclosing, name);
  if (upvalue != -1) {
    return addUpvalue(compiler, (uint8_t)upvalue, false, compiler->enclosing->upvalues[upvalue].isMutable);
  }

  return -1;
}

static int addLocal(Token name) {
  if (current->localCount == UINT8_COUNT) {
    error("Too many local variables in function.");
    return -1;
  }

  Local* local = &current->locals[current->localCount++];
  local->name = name;
  local->depth = -1;
  local->isCaptured = false;

  local->isMutable = true;

  return current->localCount - 1;
}

static void getLocal(int slot) {
  emitByte(OP_GET_LOCAL);
  emitByte(slot);
}

static void setLocal(int slot) {
  emitByte(OP_SET_LOCAL);
  emitByte(slot);
}

static int discardLocals() {
  int i = current->localCount - 1;
  for (; i >= 0 && current->locals[i].depth > current->innermostLoopScopeDepth; i--) {
    if (current->locals[i].isCaptured) {
      emitByte(OP_CLOSE_UPVALUE);
    }
    else {
      emitByte(OP_POP);
    }
  }
  return current->localCount - i - 1;
}

static void invokeMethod(int args, const char* name, int length) {
  uint16_t slot = makeConstant(OBJ_VAL(copyString(name, length)));
  emitByte(OP_INVOKE);
  emitShort(slot);
  emitByte(args);
}

static void declareVariable() {
  if (current->scopeDepth == 0) return;

  Token* name = &parser.previous;

  for (int i = current->localCount - 1; i >= 0; i--) {
    Local* local = &current->locals[i];
    if (local->depth != -1 && local->depth < current->scopeDepth) {
      break; 
    }

    if (identifiersEqual(name, &local->name)) {
      error("Already a variable with this name in this scope.");
    }
  }

  addLocal(*name);
}

static uint16_t parseVariable(const char* errorMessage) {
  consume(TOKEN_IDENTIFIER, errorMessage);

  declareVariable();
  if (current->scopeDepth > 0) return 0;

  return identifierConstant(&parser.previous);
}

static void markInitialized(bool isMutable) {
  if (current->scopeDepth == 0) return;
  current->locals[current->localCount - 1].depth = current->scopeDepth;
  current->locals[current->localCount - 1].isMutable = isMutable;
}

static void defineVariable(uint16_t global, bool isMutable) {
  if (current->scopeDepth > 0) {
    markInitialized(isMutable);
    return;
  } else {
    ObjString* name = identifierName(global);
    Value value;
    if (tableGet(&vm.currentNamespace->compilerValues, name, &value) || tableGet(&vm.currentNamespace->compilerGlobals, name, &value)) {
      error("Cannot redeclare global variable.");
    }

    if (isMutable) {
      tableSet(&vm.currentNamespace->compilerGlobals, name, NIL_VAL);
      emitByte(OP_DEFINE_GLOBAL);
      emitShort(global);
    } else {
      tableSet(&vm.currentNamespace->compilerValues, name, NIL_VAL);
      emitByte(OP_DEFINE_CONST);
      emitShort(global);
    }
  }
}

static int argumentList() {
  int argCount = 0;
  if (!check(TOKEN_RIGHT_PAREN)) {
    do {
      expression();
      if (argCount == 255) {
        error("Can't have more than 255 arguments.");
      }
      argCount++;
    } while (match(TOKEN_COMMA));
  }
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
  return argCount;
}

static void parameterList() {
  if (match(TOKEN_DOT_DOT_DOT)) {
    current->function->arity = -1;
    uint16_t constant = parseVariable("Expect variadic parameter name.");
    uint32_t hashedName = hashString(parser.previous.start, parser.previous.length);
    current->function->parameters[0].hash = hashedName;
    current->function->parameters[0].isConst = false;
    defineVariable(constant, false);
    return;
  }

  do {
    current->function->arity++;
    if (current->function->arity > 255) {
      errorAtCurrent("Can't have more than 255 parameters.");
    }

    if (match(TOKEN_CONST)) {
      uint16_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant, false);
      uint32_t hashedName = hashString(parser.previous.start, parser.previous.length);
      current->function->parameters[current->function->arity - 1].hash = hashedName;
      current->function->parameters[0].isConst = true;
    } else {
      uint16_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant, true);
      uint32_t hashedName = hashString(parser.previous.start, parser.previous.length);
      current->function->parameters[current->function->arity - 1].hash = hashedName;
      current->function->parameters[0].isConst = false;
    }
  } while (match(TOKEN_COMMA));
}

static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
}

static void bitand(bool canAssign) {
  parsePrecedence(PREC_BIT_AND);
  emitByte(OP_BITAND);
}

static void bitor(bool canAssign) {
  parsePrecedence(PREC_BIT_OR);
  emitByte(OP_BITOR);
}

static void bitxor(bool canAssign) {
  parsePrecedence(PREC_BIT_XOR);
  emitByte(OP_BITXOR);
}

static void typeof_(bool canAssign) {
  expression();
  emitByte(OP_TYPEOF);
}

static void instanceof(bool canAssign) {
  expression();
  emitByte(OP_INSTANCEOF);
}

static void showel(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  parsePrecedence(PREC_SHIFT);

  switch (operatorType) {
    case TOKEN_SHOWEL_L: emitByte(OP_SHOWEL_L); break;
    case TOKEN_SHOWEL_R: emitByte(OP_SHOWEL_R); break;
    default: return;
  }
}

static void binary(bool canAssign) {
  TokenType operatorType = parser.previous.type;
  ParseRule* rule = getRule(operatorType);
  parsePrecedence((Precedence)(rule->precedence + 1));

  switch (operatorType) {
    case TOKEN_BANG_EQUAL:    emitBytes(OP_EQUAL, OP_NOT); break;
    case TOKEN_EQUAL_EQUAL:   emitByte(OP_EQUAL); break;
    case TOKEN_GREATER:       emitByte(OP_GREATER); break;
    case TOKEN_GREATER_EQUAL: emitBytes(OP_LESS, OP_NOT); break;
    case TOKEN_LESS:          emitByte(OP_LESS); break;
    case TOKEN_LESS_EQUAL:    emitBytes(OP_GREATER, OP_NOT); break;
    case TOKEN_PLUS:          emitByte(OP_ADD); break;
    case TOKEN_MINUS:         emitByte(OP_SUBTRACT); break;
    case TOKEN_STAR:          emitByte(OP_MULTIPLY); break;
    case TOKEN_SLASH:         emitByte(OP_DIVIDE); break;
    case TOKEN_MODULO:        emitByte(OP_MODULO); break;
    case TOKEN_POWER:         emitByte(OP_POWER); break;
    case TOKEN_DOT_DOT_DOT:   emitByte(OP_RANGE); break;
    default: return; // Unreachable.
  }
}

static void call(bool canAssign) {
  int argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign) {
  uint16_t name = propretyConstant("Expect property name after '.'.");

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitByte(OP_SET_PROPERTY);
    emitShort(name);
  } else if (match(TOKEN_LEFT_PAREN)) {
    int argCount = argumentList();
    emitByte(OP_INVOKE);
    emitShort(name);
    emitByte(argCount);
  } else {
    emitByte(OP_GET_PROPERTY);
    emitShort(name);
  }
}

static void coloncolon(bool canAssign) {
  uint16_t name = propretyConstant("Expect property name after '::'.");

  emitByte(OP_GET_COLON_PROPERTY);
  emitShort(name);
}

static void literal(bool canAssign) {
  switch (parser.previous.type) {
    case TOKEN_FALSE: emitByte(OP_FALSE); break;
    case TOKEN_NIL: emitByte(OP_NIL); break;
    case TOKEN_TRUE: emitByte(OP_TRUE); break;
    default: return; // Unreachable.
  }
}

static void grouping(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void integer(bool canAssign) {
  long long int value = strtoll(parser.previous.start, NULL, 10);
  emitConstant(INT_VAL(value));
}

static void hex(bool canAssign) {
  char* endptr;
  long long int value = strtoll(parser.previous.start + 2, &endptr, 16);
  emitConstant(INT_VAL(value));
}

static void bin(bool canAssign) {
  char* endptr;
  long long int value = strtoll(parser.previous.start + 2, &endptr, 2);
  emitConstant(INT_VAL(value));
}

static void octal(bool canAssign) {
  char* endptr;
  long long int value = strtoll(parser.previous.start + 2, &endptr, 8);
  emitConstant(INT_VAL(value));
}

static void number(bool canAssign) {
  long double value = strtold(parser.previous.start, NULL);
  emitConstant(NUMBER_VAL(value));
}

static void or_(bool canAssign) {
  int elseJump = emitJump(OP_JUMP_IF_FALSE);
  int endJump = emitJump(OP_JUMP);

  patchJump(elseJump);
  emitByte(OP_POP);

  parsePrecedence(PREC_OR);
  patchJump(endJump);
}

static void array(bool canAssign) {
  uint8_t elementCount = 0;
  if (!check(TOKEN_RIGHT_BRAKE)) {
    do {
      expression();
      if (elementCount == UINT8_MAX) {
        error("Cannot have more than 255 elements.");
      }
      elementCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_BRAKE, "Expect ']' after elements.");
  emitBytes(OP_ARRAY, elementCount);
}

static void dictionary(bool canAssign) {
  uint8_t entryCount = 0;
  if (!check(TOKEN_RIGHT_BRACE)) {
    do {
      expression();
      consume(TOKEN_COLON, "Expect ':' after entry key.");
      expression();

      if (entryCount == UINT8_MAX) {
        error("Cannot have more than 255 entries.");
      }
      entryCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after entries.");
  emitBytes(OP_DICTIONARY, entryCount);
}

static void preinc(bool canAssign) {
  if (parser.previous.type == TOKEN_IDENTIFIER) {
    uint8_t incrementOp;
    bool isConstant = false;
    Token name = parser.previous;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
      incrementOp = OP_INCREMENT_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
      incrementOp = OP_INCREMENT_UPVALUE;
    } else {
      arg = identifierConstant(&name);
      isConstant = true;
      incrementOp = OP_INCREMENT_GLOBAL;
    }

    emitByte(incrementOp);
    if (isConstant) {
      emitShort((uint16_t)arg);
    } else {
      emitByte((uint8_t)arg);
    }
    emitByte(1);
  } else {
    error("Unexpected use of pre-increment operator.");
  }
}

static void predec(bool canAssign) {
  if (parser.previous.type == TOKEN_IDENTIFIER) {
    uint8_t decrementOp;
    bool isConstant = false;
    Token name = parser.previous;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
      decrementOp = OP_DECREMENT_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
      decrementOp = OP_DECREMENT_UPVALUE;
    } else {
      arg = identifierConstant(&name);
      isConstant = true;
      decrementOp = OP_DECREMENT_GLOBAL;
    }

    emitByte(decrementOp);
    if (isConstant) {
      emitShort((uint16_t)arg);
    } else {
      emitByte((uint8_t)arg);
    }
    emitByte(1);
  } else {
    error("Unexpected use of pre-decrement operator.");
  }
}

static void postinc(bool canAssign) {
  if (canAssign && (parser.pprevious.type == TOKEN_IDENTIFIER)) {
    uint8_t incrementOp;
    bool isConstant = false;
    Token name = parser.pprevious;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
      incrementOp = OP_INCREMENT_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
      incrementOp = OP_INCREMENT_UPVALUE;
    } else {
      arg = identifierConstant(&name);
      isConstant = true;
      incrementOp = OP_INCREMENT_GLOBAL;
    }

    emitByte(incrementOp);
    if (isConstant) {
      emitShort((uint16_t)arg);
    } else {
      emitByte((uint8_t)arg);
    }
    emitByte(0);
  } else {
    error("Unexpected use of post-increment operator.");
  }
}

static void postdec(bool canAssign) {
  if (canAssign && (parser.pprevious.type == TOKEN_IDENTIFIER)) {
    uint8_t decrementOp;
    bool isConstant = false;
    Token name = parser.pprevious;
    int arg = resolveLocal(current, &name);
    if (arg != -1) {
      decrementOp = OP_DECREMENT_LOCAL;
    } else if ((arg = resolveUpvalue(current, &name)) != -1) {
      decrementOp = OP_DECREMENT_UPVALUE;
    } else {
      arg = identifierConstant(&name);
      isConstant = true;
      decrementOp = OP_DECREMENT_GLOBAL;
    }

    emitByte(decrementOp);
    if (isConstant) {
      emitShort((uint16_t)arg);
    } else {
      emitByte((uint8_t)arg);
    }
    emitByte(0);
  } else {
    error("Unexpected use of post-decrement operator.");
  }
}

static void subscript(bool canAssign) {
  expression();
  consume(TOKEN_RIGHT_BRAKE, "Expect ']' after subscript.");

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitByte(OP_SET_SUBSCRIPT);
  }
  else {
    emitByte(OP_GET_SUBSCRIPT);
  }
}

static int hexDigit(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;

  error("Invalid hex escape sequence.");
  return -1;
}

static int hexEscape(const char* source, int startIndex, int length) {
  int value = 0;
  for (int i = 0; i < length; i++) {
    int index = startIndex + i + 2;
    if (source[index] == '"' || source[index] == '\0') {
      error("Incomplete hex escape sequence.");
      break;
    }

    int digit = hexDigit(source[index]);
    if (digit == -1) break;
    value = (value * 16) | digit;
  }
  return value;
}

static int unicodeEscape(const char* source, char* target, int base, int startIndex, int currentLength) {
  int value = hexEscape(source, startIndex, 4);
  int numBytes = utf8NumBytes(value);
  if (numBytes < 0) error("Negative unicode character specified.");
  if (value > 0xffff) numBytes++;

  if (numBytes > 0) {
    char* utfChar = utf8Encode(value);
    if (utfChar == NULL) error("Invalid unicode character specified.");
    else {
      memcpy(target + currentLength, utfChar, (size_t)numBytes + 1);
      free(utfChar);
    }
  }
  return numBytes;
}

char* parseString() {
  int maxLength = parser.previous.length - 2;
  int length = 0;
  const char* source = parser.previous.start + 1;
  char* target = (char*)malloc((size_t)maxLength + 1);
  if (target == NULL) {
    fprintf(stderr, "Not enough memory to allocate string in compiler. \n");
    exit(74);
  }

  for (int i = 0; i < maxLength; i++, length++) {
    if (source[i] == '\\') {
      switch (source[i + 1]) {
        case 'a': {
          target[length] = '\a';
          i++;
          break;
        }
        case 'b': {
          target[length] = '\b';
          i++;
          break;
        }
        case 'f': {
          target[length] = '\f';
          i++;
          break;
        }
        case 'n': {
          target[length] = '\n';
          i++;
          break;
        }
        case 'r': {
          target[length] = '\r';
          i++;
          break;
        }
        case 't': {
          target[length] = '\t';
          i++;
          break;
        }
        case 'v': {
          target[length] = '\v';
          i++;
          break;
        }
        case 'x': {
          target[length] = hexEscape(source, i, 2);
          i += 3;
          break;
        }
        case 'u': {
          int numBytes = unicodeEscape(source, target, 4, i, length);  
          length += numBytes > 4 ? numBytes - 2 : numBytes - 1;
          i += numBytes > 4 ? 6 : 5;
          break;
        }
        case 'U': {
          int numBytes = unicodeEscape(source, target, 8, i, length);
          length += numBytes > 4 ? numBytes - 2 : numBytes - 1;
          i += 9;
          break;
        }
        case '"': {
          target[length] = '"';
          i++;
          break;
        }
        case '\\': {
          target[length] = '\\';
          i++;
          break;
        }
        default: target[length] = source[i];
      }
    }
    else target[length] = source[i];
  }

  target = (char*)reallocate(target, (size_t)maxLength + 1, (size_t)length + 1);
  target[length] = '\0';
  return target;
}

static void string(bool canAssign) {
  char* string = parseString();
  int length = (int)strlen(string);
  emitConstant(OBJ_VAL(takeString(string, length)));
}

static void interpolation(bool canAssign) {
  int count = 0;
  do {
    bool concatenate = false;
    bool isString = false;

    if (parser.previous.length > 2) {
      string(canAssign);
      concatenate = true;
      isString = true;
      if (count > 0) emitByte(OP_ADD);
    }

    expression();
    invokeMethod(0, "__str__", 7);

    if (concatenate || (count >= 1 && !isString)) {
      emitByte(OP_ADD);
    }

    count++;
  } while (match(TOKEN_INTERPOLATION));

  consume(TOKEN_STRING, "Expect end of string interpolation.");
  if (parser.previous.length > 2) {
    string(canAssign);
    emitByte(OP_ADD);
  }
}


static void checkMutability(int arg, uint8_t opCode) { 
  switch (opCode) {
    case OP_SET_LOCAL: 
      if (!current->locals[arg].isMutable) { 
        error("Cannot assign to immutable local variable.");
      }
      break;
    case OP_SET_UPVALUE: 
      if (!current->upvalues[arg].isMutable) { 
        error("Cannot assign to immutable upvalue.");
      }
      break;
    case OP_SET_GLOBAL: {
      ObjString* name = identifierName(arg);
      Value value;
      if (tableGet(&vm.currentNamespace->compilerValues, name, &value)) { 
        error("Cannot assign to immutable global variables.");
      }
      break;
    }
    default:
      break;
  }
}

static void assertStatement() {
  expression();

  if (match(TOKEN_COMMA)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after assert statement.");

  emitByte(OP_ASSERT);
}

static void closure(bool canAssign) {
  function(TYPE_FUNCTION, false);
}

static void lambda(bool canAssign) {
  function(TYPE_LAMBDA, false);
}

static void getVariable(Token name) {
  uint8_t getOp;
  bool isConstant = false;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
  } else {
    isConstant = true;
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
  }

  emitByte(getOp);
  if (isConstant) {
    emitShort((uint16_t)arg);
  } else {
    emitByte((uint8_t) arg);
  }
}

static void setVariable(Token name, uint8_t byte) {
  uint8_t setOp;
  bool isConstant = false;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    setOp = OP_SET_UPVALUE;
  } else {
    isConstant = true;
    arg = identifierConstant(&name);
    setOp = OP_SET_GLOBAL;
  }

  checkMutability(arg, setOp);
  emitByte(byte);
  emitByte(setOp);
  if (isConstant) {
    emitShort((uint16_t)arg);
  } else {
    emitByte((uint8_t)arg);
  }
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  bool isConstant = false;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    isConstant = true;
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    checkMutability(arg, setOp);
    expression();
    emitByte(setOp);
    if (isConstant) {
      emitShort((uint16_t)arg);
    } else {
      emitByte((uint8_t) arg);
    }
  } else {
    emitByte(getOp);
    if (isConstant) {
      emitShort((uint16_t)arg);
    } else {
      emitByte((uint8_t) arg);
    }
  }
}

static void variable(bool canAssign) {
  namedVariable(parser.previous, canAssign);
}

static void this_(bool canAssign) {
  if (currentClass == NULL) {
    error("Can't use 'this' outside of a class.");
    return;
  }

  if (currentClass->isStaticMethod == true) {
    error("Can't use 'this' inside of a static method.");
    return;
  }

  variable(false);
}

static void super_(bool canAssign) {
  if (currentClass == NULL) {
    error("Can't use 'super' outside of a class.");
  }

  consume(TOKEN_DOT, "Expect '.' after 'super'.");
  consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
  uint16_t name = identifierConstant(&parser.previous);

  namedVariable(syntheticToken("this"), false);
  if (match(TOKEN_LEFT_PAREN)) {
    int argCount = argumentList();
    namedVariable(currentClass->superclass, false);
    emitByte(OP_SUPER_INVOKE);
    emitShort(name);
    emitByte(argCount);
  } else {
    namedVariable(currentClass->superclass, false);
    emitByte(OP_GET_SUPER);
    emitShort(name);
  }
}

static void yield(bool canAssign) {
  if (current->type == TYPE_SCRIPT) {
    error("Can't yield from top-level code.");
  } else if (current->type == TYPE_INITIALIZER) {
    error("Cannot yield from an initializer.");
  }

  current->function->isGenerator = true;

  if (match(TOKEN_RIGHT_PAREN) || match(TOKEN_RIGHT_BRAKE) || match(TOKEN_RIGHT_BRACE)
      || match(TOKEN_COMMA) || match(TOKEN_SEMICOLON)) {
    emitBytes(OP_NIL, OP_YIELD);
  } else if (match(TOKEN_FROM)) {
    expression();
    emitByte(OP_YIELD_FROM);
  } else {
    expression();
    emitByte(OP_YIELD);
  }
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT); break;
    case TOKEN_PLUS: emitByte(OP_AFFERM); break;
    case TOKEN_TILDA: emitByte(OP_BITNOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    case TOKEN_MINUS_MINUS: predec(canAssign); break;
    case TOKEN_PLUS_PLUS: preinc(canAssign); break;
    default: return; // Unreachable.
  }
}

static void await(bool canAssign) {
  if (current->type == TYPE_SCRIPT) current->isAsync = true;
  else if (!current->isAsync) error("Cannot use await unless in top level code or inside async functions/methods.");
  expression();
  emitByte(OP_AWAIT);
}

static void async(bool canAssign) {
  if (match(TOKEN_FUN)) {
    function(TYPE_FUNCTION, true);
  } else if (match(TOKEN_LAMBDA)) {
    function(TYPE_LAMBDA, true);
  } else {
    error("Can only use async as expression modifier for anonymous functions or lambda.");
  }
}

ParseRule rules[] = {
  [TOKEN_LEFT_BRAKE]    = {array,         subscript,    PREC_CALL}, 
  [TOKEN_RIGHT_BRAKE]   = {NULL,          NULL,         PREC_NONE},
  [TOKEN_LEFT_PAREN]    = {grouping,      call,         PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,          NULL,         PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {dictionary,    NULL,         PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,          NULL,         PREC_NONE},
  [TOKEN_COMMA]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_DOT]           = {NULL,          dot,          PREC_CALL},
  [TOKEN_COLON_COLON]   = {NULL,          coloncolon,   PREC_CALL},
  [TOKEN_DOT_DOT_DOT]   = {NULL,          binary,       PREC_CALL},
  [TOKEN_INSTANCEOF]    = {NULL,          instanceof,   PREC_CALL},
  [TOKEN_MINUS_MINUS]   = {unary,         postdec,      PREC_CALL},
  [TOKEN_MINUS]         = {unary,         binary,       PREC_TERM},
  [TOKEN_PLUS_PLUS]     = {unary,         postinc,      PREC_CALL},
  [TOKEN_PLUS]          = {unary,         binary,       PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,          NULL,         PREC_NONE},
  [TOKEN_SLASH]         = {NULL,          binary,       PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,          binary,       PREC_FACTOR},
  [TOKEN_MODULO]        = {NULL,          binary,       PREC_FACTOR},
  [TOKEN_POWER]         = {NULL,          binary,       PREC_FACTOR},
  [TOKEN_BANG]          = {unary,         NULL,         PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,          binary,       PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,          binary,       PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,          binary,       PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,          binary,       PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,          binary,       PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,          binary,       PREC_COMPARISON},
  [TOKEN_TYPEOF]        = {typeof_,       NULL,         PREC_NONE},
  [TOKEN_IDENTIFIER]    = {variable,      NULL,         PREC_NONE},
  [TOKEN_STRING]        = {string,        NULL,         PREC_NONE},
  [TOKEN_INTERPOLATION] = {interpolation, NULL,         PREC_NONE},
  [TOKEN_NUMBER]        = {number,        NULL,         PREC_NONE},
  [TOKEN_INT]           = {integer,       NULL,         PREC_NONE},
  [TOKEN_HEX]           = {hex,           NULL,         PREC_NONE},
  [TOKEN_BIN]           = {bin,           NULL,         PREC_NONE},
  [TOKEN_OCT]           = {octal,         NULL,         PREC_NONE},
  [TOKEN_AND]           = {NULL,          and_,         PREC_AND},
  [TOKEN_AMP]           = {NULL,          bitand,       PREC_BIT_AND},
  [TOKEN_SHOWEL_L]      = {NULL,          showel,       PREC_SHIFT},
  [TOKEN_SHOWEL_R]      = {NULL,          showel,       PREC_SHIFT},
  [TOKEN_CLASS]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_ELSE]          = {NULL,          NULL,         PREC_NONE},
  [TOKEN_FALSE]         = {literal,       NULL,         PREC_NONE},
  [TOKEN_FOR]           = {NULL,          NULL,         PREC_NONE},
  [TOKEN_ASSERT]        = {NULL,          NULL,         PREC_NONE},
  [TOKEN_ASYNC]         = {async,         NULL,         PREC_NONE},
  [TOKEN_AWAIT]         = {await,         NULL,         PREC_NONE},
  [TOKEN_FUN]           = {closure,       NULL,         PREC_NONE},
  [TOKEN_LAMBDA]        = {lambda,        NULL,         PREC_NONE},
  [TOKEN_IF]            = {NULL,          NULL,         PREC_NONE},
  [TOKEN_NIL]           = {literal,       NULL,         PREC_NONE},
  [TOKEN_OR]            = {NULL,          or_,          PREC_OR},
  [TOKEN_PIPE]          = {NULL,          bitor,        PREC_BIT_OR},
  [TOKEN_CARRET]        = {NULL,          bitxor,       PREC_BIT_XOR},
  [TOKEN_TILDA]         = {unary,         NULL,         PREC_NONE},
  [TOKEN_REQUIRE]       = {NULL,          NULL,         PREC_NONE},
  [TOKEN_RETURN]        = {NULL,          NULL,         PREC_NONE},
  [TOKEN_SUPER]         = {super_,        NULL,         PREC_NONE},
  [TOKEN_THIS]          = {this_,         NULL,         PREC_NONE},
  [TOKEN_TRUE]          = {literal,       NULL,         PREC_NONE},
  [TOKEN_VAR]           = {NULL,          NULL,         PREC_NONE},
  [TOKEN_CONST]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_WHILE]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_FROM]          = {NULL,          NULL,         PREC_NONE},
  [TOKEN_YIELD]         = {yield,         NULL,         PREC_NONE},
  [TOKEN_ERROR]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_THROW]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_TRY]           = {NULL,          NULL,         PREC_NONE},
  [TOKEN_CATCH]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_FINALLY]       = {NULL,          NULL,         PREC_NONE},
  [TOKEN_AS]            = {NULL,          NULL,         PREC_NONE},
  [TOKEN_NAMESPACE]     = {NULL,          NULL,         PREC_NONE},
  [TOKEN_USING]         = {NULL,          NULL,         PREC_NONE},
  [TOKEN_EOF]           = {NULL,          NULL,         PREC_NONE}
};

static ParseRule* getRule(TokenType type) {
  return &rules[type];
}

static void parsePrecedence(Precedence precedence) {
  advance();
  ParseFn prefixRule = getRule(parser.previous.type)->prefix;
  if (prefixRule == NULL) {
    error("Expect expression.");
    return;
  }

  bool canAssign = precedence <= PREC_ASSIGNMENT;
  prefixRule(canAssign);

  while (precedence <= getRule(parser.current.type)->precedence) {
    advance();
    ParseFn infixRule = getRule(parser.previous.type)->infix;
    infixRule(canAssign);
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static void expression() {
  parsePrecedence(PREC_ASSIGNMENT);

  if (match(TOKEN_QUESTION)) {
    int elseJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP);
    expression();
    consume(TOKEN_COLON, "Expect ':' after true branch");
    int endJump = emitJump(OP_JUMP);

    patchJump(elseJump);
    emitByte(OP_POP);
    expression();

    patchJump(endJump);
  }
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void abstractParameters() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after abstract method name.");
  if (!check(TOKEN_RIGHT_PAREN)) {
    if (match(TOKEN_DOT_DOT_DOT)) {
      current->function->arity = -1;
      consume(TOKEN_IDENTIFIER, "Expect variadic parameter name.");
      uint32_t hashedName = hashString(parser.previous.start, parser.previous.length);
      current->function->parameters[0].hash = hashedName;
      current->function->parameters[0].isConst = false;
      consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
      consume(TOKEN_SEMICOLON, "Expect ';' after abstract method declaration.");
      return;
    }

    do {
      current->function->arity++;
      if (current->function->arity > 255) {
        errorAtCurrent("Can't have more than 255 parameters.");
      }

      if (match(TOKEN_CONST)) {
        consume(TOKEN_IDENTIFIER, "Expect parameter name.");
        uint32_t hashedName = hashString(parser.previous.start, parser.previous.length);
        current->function->parameters[current->function->arity - 1].hash = hashedName;
        current->function->parameters[current->function->arity - 1].isConst = true;
      } else {
        consume(TOKEN_IDENTIFIER, "Expect parameter name.");
        uint32_t hashedName = hashString(parser.previous.start, parser.previous.length);
        current->function->parameters[current->function->arity - 1].hash = hashedName;
        current->function->parameters[current->function->arity - 1].isConst = false;
      }
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_SEMICOLON, "Expect ';' after abstract method declaration.");
}

static void functionParameters(Compiler* compiler) {
  compiler->function->isMutable = true;
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

  if (!check(TOKEN_RIGHT_PAREN)) {
    parameterList();
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  if (match(TOKEN_CONST)) {
    compiler->function->isMutable = false;
  }
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
}

static void lambdaParameters() {
  consume(TOKEN_LEFT_BRAKE, "Expect '[' after function name.");

  if (!check(TOKEN_RIGHT_BRAKE)) {
    parameterList();
  }

  consume(TOKEN_RIGHT_BRAKE, "Expect ']' after parameters.");
}

static uint8_t lambdaDepth() {
  uint8_t depth = 1;
  Compiler* compiler = current->enclosing;
  while (compiler->type == TYPE_LAMBDA) {
    depth++;
    compiler = compiler->enclosing;
  }
  return depth;
}

static void function(FunctionType type, bool isAsync) {
  Compiler compiler;
  initCompiler(&compiler, type, isAsync);
  beginScope();

  if (type == TYPE_LAMBDA) {
    lambdaParameters();

    consume(TOKEN_LEFT_BRACE, "Expect lambda body after ']'.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
      declaration();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after lambda body.");
  } else if (type == TYPE_ABSTRACT) {
    current->function->isAbstract = true;
    abstractParameters();
  } else if (type == TYPE_GETTER) {
    current->function->arity = 0;
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();
  } else if (type == TYPE_SETTER) {
    current->function->arity = 1;
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    if (match(TOKEN_CONST)) {
      uint16_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant, false);
    } else {
      uint16_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant, true);
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();
  } else {
    functionParameters(compiler.enclosing);
    block();
  }

  ObjFunction* function = endCompiler();
  uint16_t constant = makeConstant(OBJ_VAL(function));
  emitByte(OP_CLOSURE);
  emitShort(constant);

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void getter(bool isAsync) {
  FunctionType type = TYPE_GETTER;
  beginScope();
  
  function(type, isAsync);
  
  endScope();
}

static void setter(bool isAsync) {
  FunctionType type = TYPE_SETTER;
  beginScope();
  
  function(type, isAsync);
  
  endScope();
}

static void abstractMethod() {
  bool isAsync = false;
  if (match(TOKEN_ASYNC)) isAsync = true;
  uint16_t constant = propretyConstant("Expect method name.");

  function(TYPE_ABSTRACT, isAsync);
  emitByte(OP_ABSTRACT_METHOD);
  emitShort(constant);
}

static void method() {
  bool isAsync = false;
  if (match(TOKEN_ASYNC)) isAsync = true;
  uint8_t opCode = OP_METHOD;
  currentClass->isStaticMethod = false;

  if (match(TOKEN_STATIC)) {
    currentClass->isStaticMethod = true;
  } else if (match(TOKEN_GET)) {
    opCode = OP_GETTER;
  } else if (match(TOKEN_SET)) {
    opCode = OP_SETTER;
  } else {
    consume(TOKEN_FUN, "Expect 'function' keyword");
  }

  uint16_t constant = propretyConstant("Expect method name.");

  FunctionType type = TYPE_METHOD;
  if (parser.previous.length == 8 &&
      memcmp(parser.previous.start, "__init__", 8) == 0) {
    type = TYPE_INITIALIZER;
  }

  if (opCode == OP_GETTER) getter(isAsync);
  else if (opCode == OP_SETTER) setter(isAsync);
  else function(type, isAsync);

  emitByte(currentClass->isStaticMethod ? OP_TRUE : OP_FALSE);
  emitByte(opCode);
  emitShort(constant);
}

// TODO implement immutable class propreties
static void classPropretyDeclaration(Token* className) {
  uint16_t globals[UINT8_MAX];
  int propCount = 0;

  do {
    if (propCount >= UINT8_MAX) {
      error("Can't have more than 255 properties in a single declaration.");
      return;
    }

    consume(TOKEN_IDENTIFIER, "Expect property name.");
    Token name = parser.previous;
    globals[propCount] = identifierConstant(&name);
    propCount++;

  } while (match(TOKEN_COMMA));

  if (match(TOKEN_EQUAL)) {
    int exprCount = 0;
    do {
      if (exprCount >= propCount) {
        error("Too many initializers.");
        return;
      }
      expression();
      exprCount++;
    } while (match(TOKEN_COMMA));

    if (exprCount < propCount) {
      error("Too few initializers.");
      return;
    }

  } else {
    for (int i = 0; i < propCount; i++) {
      emitByte(OP_NIL);
    }
  }

  consume(TOKEN_SEMICOLON, "Expect ';' after property declaration.");

  for (int i = 0; i < propCount; i++) {
    getVariable(*className);
    emitByte(OP_CLASS_PROPRETY);
    emitShort(globals[i]);
  }
}

static void decorateFunction(Token functionName) {
  uint8_t setOp;
  bool isConstant = false;
  int arg = resolveLocal(current, &functionName);
  if (arg != -1) {
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &functionName)) != -1) {
    setOp = OP_SET_UPVALUE;
  } else {
    isConstant = true;
    arg = identifierConstant(&functionName);
    setOp = OP_SET_GLOBAL;
  }

  checkMutability(arg, setOp);
  emitByte(setOp);
  if (isConstant) {
    emitShort((uint16_t)arg);
  } else {
    emitByte((uint8_t)arg);
  }
}

static void decoratorDeclaration(bool isMethod) {
  consume(TOKEN_IDENTIFIER, "Expect decorator name after '@'.");
  Token decoratorName = parser.previous;

  if (!isMethod) {
    consume(TOKEN_FUN, "Expect a function after decorator declaration.");
    uint16_t global = parseVariable("Expect function name.");
    Token functionName = parser.previous;

    markInitialized(current->function->isMutable);
    function(TYPE_FUNCTION, false);
    defineVariable(global, current->function->isMutable);

    getVariable(decoratorName);
    getVariable(functionName);
    emitByte(OP_DECORATOR);

    decorateFunction(functionName);
    emitByte(OP_POP);
    emitByte(OP_POP);
  } else {
    // TODO complete the implementation for methods
  }
}


static void classBody(Token* className) {
  if (match(TOKEN_VAR)) {
    classPropretyDeclaration(className);
  } else if (match(TOKEN_AT)) {
    decoratorDeclaration(true);
  } else if (match(TOKEN_ABSTRACT)) {
    abstractMethod();
  } else {
    method();
  }
}

static void enumDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect enum name.");
  Token enumName = parser.previous;
  uint16_t nameConstant = identifierConstant(&enumName);
  declareVariable();

  emitByte(OP_ENUM);
  emitShort(nameConstant);
  defineVariable(nameConstant, false);

  beginScope();
  namedVariable(enumName, false);
  consume(TOKEN_LEFT_BRACE, "Expect '{' before enum body.");

  uint8_t elementCount = 0;
  if (!check(TOKEN_RIGHT_BRACE)) {
    do {
      consume(TOKEN_IDENTIFIER, "Expect enum element name.");
      Token elementName = parser.previous;

      uint16_t elementConstant = identifierConstant(&elementName);
      emitByte(OP_ENUM_ELEMENT);
      emitShort(elementConstant);

      if (elementCount == UINT8_MAX) {
        error("Cannot have more than 255 enum elements.");
      }
      elementCount++;
    } while (match(TOKEN_COMMA));
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after enum body.");
  emitByte(OP_POP);
  endScope();
}

static void classDeclaration(bool isAbstract) {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  bool doesInhert = false;
  Token className = parser.previous;
  Token superclassName;
  uint16_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

  isAbstract ? emitByte(OP_ABSTRACT_CLASS) : emitByte(OP_CLASS);
  emitShort(nameConstant);
  defineVariable(nameConstant, false);


  ClassCompiler* enclosingClass = currentClass;
  ClassCompiler classCompiler = { .name = className, .enclosing = enclosingClass, .superclass = parser.rootClass, .isStaticMethod = false };
  currentClass = &classCompiler;

  if (match(TOKEN_COLON)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    doesInhert = true;
    superclassName = parser.previous;
    classCompiler.superclass = superclassName;
    namedVariable(className, false);

    if (identifiersEqual(&className, &superclassName)) {
      error("A class can't inherit from itself.");
      doesInhert = false;
    }
  } else {
    namedVariable(className, false);
    if (identifiersEqual(&className, &parser.rootClass)) {
      error("Cannot redeclare root class Object.");
    }
  }

  beginScope();
  addLocal(syntheticToken("super"));
  defineVariable(0, false);
  if (doesInhert) {
    namedVariable(superclassName, false);
  } else {
    namedVariable(parser.rootClass, false);
  }
  emitByte(OP_INHERIT);

  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    classBody(&className);
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
  endScope();

  currentClass = currentClass->enclosing;
}

static void varDeclaration(bool isMutable) {
  uint16_t globals[UINT8_MAX];
  int varCount = 0;

  do {
    if (varCount >= UINT8_MAX) {
      error("Can't have more than 255 variables in a single declaration.");
      return;
    }
    globals[varCount] = parseVariable("Expect variable name.");
    varCount++;
  } while (match(TOKEN_COMMA));

  if (!isMutable && !check(TOKEN_EQUAL)) {
    error("Const variable must be initialized upon declaration.");
    return;
  } else if (match(TOKEN_SEMICOLON)) {
    for (int i = varCount - 1; i >= 0; i--) {
      emitByte(OP_NIL);
    }
  } else if (match(TOKEN_EQUAL)) {
    int exprCount = 0;
    if (check(TOKEN_COMMA) || !check(TOKEN_SEMICOLON)) {
      do {
        if (exprCount >= varCount) {
          error("Too many initializers.");
          return;
        }
        expression();
        exprCount++;
      } while (match(TOKEN_COMMA));
    } else {
      expression();
      exprCount = varCount;
    }

    if (exprCount < varCount) {
      error("Too few initializers.");
      return;
    }

    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
  }


  for (int i = varCount - 1; i >= 0; i--) {
    defineVariable(globals[i], isMutable);
  }
}

static void funDeclaration(bool isAsync) {
  uint16_t global = parseVariable("Expect function name.");
  markInitialized(current->function->isMutable);
  function(TYPE_FUNCTION, isAsync);
  defineVariable(global, current->function->isMutable);
}


static void expressionStatement() {
  expression();

  if (current->type == TYPE_LAMBDA && !check(TOKEN_SEMICOLON)) {
    emitByte(OP_RETURN);
  } else {
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    emitByte(OP_POP);
  }
}

static void awaitStatement() {
  if (current->type == TYPE_SCRIPT) {
    current->isAsync = true;
    current->function->isAsync = true;
  }
  else if (!current->isAsync) error("Can only use 'await' in async methods or top level code.");
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after await value.");
  emitBytes(OP_AWAIT, OP_POP);
}

static void breakStatement() {
  if (current->innermostLoopStart == -1) {
    error("Cannot use 'break' outside of a loop.");
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after 'break'.");

  discardLocals();
  emitJump(OP_END);
}

static void continueStatement() {
  if (current->innermostLoopStart == -1) {
    error("Cannot use 'continue' outside of a loop.");
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after 'continue'.");

  discardLocals();
  emitLoop(current->innermostLoopStart);
}

static void forStatement() {
  beginScope();

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
  if (match(TOKEN_VAR)) {
    varDeclaration(true);
  } else if (match(TOKEN_SEMICOLON)) {
    // No initializer.
  } else if (match(TOKEN_IDENTIFIER)) {
    Token valueToken, indexToken;
    if (check(TOKEN_COMMA)) {
      valueToken = parser.previous;
      advance();
      consume(TOKEN_IDENTIFIER, "Expect variable name after ','.");
      indexToken = parser.previous;
    } else { 
      indexToken = syntheticToken("index ");
      valueToken = parser.previous;
    }

    consume(TOKEN_COLON, "Expect ':' after variable name.");

    expression();

    if (current->localCount + 3 > UINT8_MAX) {
      error("For loop can only contain up to 252 variables.");
    }

    int collectionSlot = addLocal(syntheticToken("collection "));
    emitByte(OP_NIL);
    int indexSlot = addLocal(indexToken);
    markInitialized(true);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after loop expression.");

    int loopStart = current->innermostLoopStart;
    int scopeDepth = current->innermostLoopScopeDepth;
    current->innermostLoopStart = currentChunk()->count;
    current->innermostLoopScopeDepth = current->scopeDepth;

    getLocal(collectionSlot);
    getLocal(indexSlot);
    invokeMethod(1, "next", 4);
    setLocal(indexSlot);
    emitByte(OP_POP);
    int exitJump = emitJump(OP_JUMP_IF_EMPTY);

    getLocal(collectionSlot);
    getLocal(indexSlot);
    invokeMethod(1, "nextValue", 9);

    beginScope();
    int valueSlot = addLocal(valueToken);
    markInitialized(true);
    setLocal(valueSlot);
    statement();
    endScope();

    emitLoop(current->innermostLoopStart);
    patchJump(exitJump);
    endLoop();
    emitByte(OP_POP);
    emitByte(OP_POP);

    current->localCount -= 2;
    current->innermostLoopStart = loopStart;
    current->innermostLoopScopeDepth = scopeDepth;
    endScope();

    return;
  } else {
    expressionStatement();
  }

  int surroundingLoopStart = current->innermostLoopStart;
  int surroundingLoopScopeDepth = current->innermostLoopScopeDepth;
  current->innermostLoopStart = currentChunk()->count;
  current->innermostLoopScopeDepth = current->scopeDepth;

  int exitJump = -1;
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");

    // Jump out of the loop if the condition is false.
    exitJump = emitJump(OP_JUMP_IF_FALSE);
    emitByte(OP_POP); // Condition.
  }

  if (!match(TOKEN_RIGHT_PAREN)) {
    int bodyJump = emitJump(OP_JUMP);

    int incrementStart = currentChunk()->count;
    expression();
    emitByte(OP_POP);
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    emitLoop(current->innermostLoopStart);
    current->innermostLoopStart = incrementStart;
    patchJump(bodyJump);
  }

  statement();

  emitLoop(current->innermostLoopStart);

  if (exitJump != -1) {
    patchJump(exitJump);
    emitByte(OP_POP); // Condition.
  }

  endLoop();
  current->innermostLoopStart = surroundingLoopStart;
  current->innermostLoopScopeDepth = surroundingLoopScopeDepth;

  endScope();
}

static void switchStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'switch'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after value.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before switch cases.");

  int state = 0; // 0: before all cases, 1: before default, 2: after default.
  int caseEnds[MAX_CASES];
  int caseCount = 0;
  int previousCaseSkip = -1;

  while (!match(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    if (match(TOKEN_CASE) || match(TOKEN_DEFAULT)) {
      TokenType caseType = parser.previous.type;

      if (state == 2) {
        error("Can't have another case or default after the default case.");
      }

      if (state == 1) {
        // At the end of the previous case, jump over the others.
        caseEnds[caseCount++] = emitJump(OP_JUMP);

        // Patch its condition to jump to the next case (this one).
        patchJump(previousCaseSkip);
        emitByte(OP_POP);
      }

      if (caseType == TOKEN_CASE) {
        state = 1;

        // See if the case is equal to the value.
        emitByte(OP_DUP);
        expression();

        consume(TOKEN_COLON, "Expect ':' after case value.");

        emitByte(OP_EQUAL);
        previousCaseSkip = emitJump(OP_JUMP_IF_FALSE);

        // Pop the comparison result.
        emitByte(OP_POP);
      } else {
        state = 2;
        consume(TOKEN_COLON, "Expect ':' after default.");
        previousCaseSkip = -1;
      }
    } else {
      // Otherwise, it's a statement inside the current case.
      if (state == 0) {
        error("Can't have statements before any case.");
      }
      statement();
    }
  }

  // If we ended without a default case, patch its condition jump.
  if (state == 1) {
    patchJump(previousCaseSkip);
    emitByte(OP_POP);
  }

  // Patch all the case jumps to the end.
  for (int i = 0; i < caseCount; i++) {
    patchJump(caseEnds[i]);
  }

  emitByte(OP_POP); // The switch value.
}

static void ifStatement() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition."); 

  int thenJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();

  int elseJump = emitJump(OP_JUMP);

  patchJump(thenJump);
  emitByte(OP_POP);

  if (match(TOKEN_ELSE)) statement();
  patchJump(elseJump);
}

static void throwStatement() {
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after thrown exception object.");
  emitByte(OP_THROW);
}

static void tryStatement() {
  emitByte(OP_TRY);
  int exceptionType = currentChunk()->count;
  emitByte(0xff);
  int handlerAddress = currentChunk()->count;
  emitBytes(0xff, 0xff);
  int finallyAddress = currentChunk()->count;
  emitBytes(0xff, 0xff);
  statement();
  emitByte(OP_CATCH);
  int catchJump = emitJump(OP_JUMP);

  if (match(TOKEN_CATCH)) {
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after catch");
    consume(TOKEN_IDENTIFIER, "Expect type name to catch");
    uint8_t name = identifierConstant(&parser.previous);
    currentChunk()->code[exceptionType] = name;
    patchAddress(handlerAddress);

    consume(TOKEN_COLON, "Expect ': after name to catch");

    if (check(TOKEN_IDENTIFIER)) {
      consume(TOKEN_IDENTIFIER, "Expect identifier after exception type.");
      addLocal(parser.previous);
      markInitialized(false);
      uint8_t variable = resolveLocal(current, &parser.previous);
      emitBytes(OP_SET_LOCAL, variable);
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after catch statement");
    emitByte(OP_CATCH);
    statement();
    endScope();
  } else {
    errorAtCurrent("Must have a catch statement following a try statement.");
  }

  patchJump(catchJump);

  if (match(TOKEN_FINALLY)) {
    emitByte(OP_FALSE);
    patchAddress(finallyAddress);
    statement();

    int finallyJump = emitJump(OP_JUMP_IF_FALSE);

    emitByte(OP_POP);
    emitByte(OP_FINALLY);
    patchJump(finallyJump);
    emitByte(OP_POP);
  }
}

// TODO fix the whole namespace declaration (VM + compiler)
static void namespaceDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect namespace name.");

  Token namespaceName = parser.previous;
  uint16_t nameConstant = identifierConstant(&namespaceName);
  ObjString* name = copyString(parser.previous.start, parser.previous.length);

  declareVariable();

  ObjNamespace* namespaceObj = defineNativeNamespace(name->chars, vm.currentNamespace);
  stackPush(&namespaceStack, vm.currentNamespace);  // Push the current namespace onto the stack
  vm.previousNamespace = vm.currentNamespace;
  vm.currentNamespace = namespaceObj;

  emitByte(OP_BEGIN_NAMESPACE);
  emitShort(nameConstant);
  defineVariable(nameConstant, false);
  namedVariable(namespaceName, false);

  consume(TOKEN_LEFT_BRACE, "Expect '{' after namespace declaration.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after namespace block.");

  vm.currentNamespace = stackPop(&namespaceStack);  // Pop the previous namespace from the stack
  emitByte(OP_END_NAMESPACE);

  emitByte(OP_POP);
}

static void usingStatement() {
  uint8_t namespaceDepth = 0;
  Token namep;
  do { 
    consume(TOKEN_IDENTIFIER, "Expect namespace identifier.");
    namep = parser.previous;
    emitIdentifier(&namep);
    namespaceDepth++;
  } while (match(TOKEN_COLON_COLON));

  emitBytes(OP_USING, namespaceDepth);
  uint16_t alias = makeConstant(OBJ_VAL(newString("")));

  if (match(TOKEN_AS)) {
    consume(TOKEN_IDENTIFIER, "Expect alias after 'as'.");
    Token name = parser.previous;
    if (strcmp(stringPrecision(namep.start, namep.length), stringPrecision(name.start, name.length)) == 0) {
      error("Namespace name and alias can't be the same.");
    }
    alias = identifierConstant(&name);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after using statement.");
  emitByte(OP_SUBNAMESPACE);
  emitShort(alias);
}

static void requireStatement() {
  if (current->type != TYPE_SCRIPT) {
    error("Can only require source files from top-level code.");
  }
  
  expression();
  consume(TOKEN_SEMICOLON, "Expect ';' after required file path.");
  emitByte(OP_REQUIRE);
}

static void returnStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Can't return from top-level code.");
  }

  uint8_t depth = 0;
  if (current->type == TYPE_LAMBDA) depth = lambdaDepth();

  if (match(TOKEN_SEMICOLON)) {
    emitReturn(depth);
  } else {
    if (current->type == TYPE_INITIALIZER) {
      error("Can't return a value from an initializer.");
    }

    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after return value.");

    if (current->type == TYPE_LAMBDA) {
      emitBytes(OP_RETURN_NONLOCAL, depth);
    }
    else {
      emitByte(OP_RETURN);
    }
  }
}

static void whileStatement() {
  int loopStart = current->innermostLoopStart;
  int scopeDepth = current->innermostLoopScopeDepth;
  current->innermostLoopStart = currentChunk()->count;
  current->innermostLoopScopeDepth = current->scopeDepth;

  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);
  statement();
  emitLoop(current->innermostLoopStart);

  patchJump(exitJump);
  emitByte(OP_POP);

  endLoop();
  current->innermostLoopStart = loopStart;
  current->innermostLoopScopeDepth = scopeDepth;
}

static void doWhileStatement() {
  int loopStart = current->innermostLoopStart;
  int scopeDepth = current->innermostLoopScopeDepth;

  current->innermostLoopStart = currentChunk()->count;
  current->innermostLoopScopeDepth = current->scopeDepth;

  statement();

  consume(TOKEN_WHILE, "Expect 'while' after body.");
  consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
  expression();
  consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

  int exitJump = emitJump(OP_JUMP_IF_FALSE);
  emitByte(OP_POP);

  emitLoop(current->innermostLoopStart);

  patchJump(exitJump);
  emitByte(OP_POP);

  endLoop();
  current->innermostLoopStart = loopStart;
  current->innermostLoopScopeDepth = scopeDepth;
}

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_AWAIT:
      case TOKEN_ASYNC:
      case TOKEN_ASSERT:
      case TOKEN_NAMESPACE:
      case TOKEN_USING:
      case TOKEN_SWITCH:
      case TOKEN_THROW:
      case TOKEN_DO:
      case TOKEN_FUN:
      case TOKEN_CONST:
      case TOKEN_VAR:
      case TOKEN_ENUM:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_RETURN:
      case TOKEN_FROM:
      case TOKEN_YIELD:
        return;

      default:
        ; // Do nothing.
    }

    advance();
  }
}

static void yieldStatement() {
  if (current->type == TYPE_SCRIPT) {
    error("Can't yield from top-level code.");
  } else if (current->type == TYPE_INITIALIZER) {
    error("Cannot yield from an initializer.");
  }

  current->function->isGenerator = true;
  if (match(TOKEN_SEMICOLON)) {
    emitBytes(OP_YIELD, OP_POP);
  } else if (match(TOKEN_FROM)) {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after yield value.");
    emitByte(OP_YIELD_FROM);
  } else {
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after yield value.");
    emitBytes(OP_YIELD, OP_POP);
  }
}

static void declaration() {
  if (match(TOKEN_AT)) {
    decoratorDeclaration(false);
  } else if (check(TOKEN_ASYNC) && checkNext(TOKEN_FUN)) {
    advance();
    advance();
    funDeclaration(true);
  } else if (match(TOKEN_ABSTRACT)) {
    consume(TOKEN_CLASS, "Expect 'class' after 'abstract'.");
    classDeclaration(true);
  } else if (match(TOKEN_CLASS)) {
    classDeclaration(false);
  } else if (match(TOKEN_NAMESPACE)) {
    namespaceDeclaration();
  } else if (match(TOKEN_ENUM)) {
    enumDeclaration();
  } else if (match(TOKEN_FUN)) {
    funDeclaration(false);
  } else if (match(TOKEN_VAR)) {
    varDeclaration(true);
  } else if (match(TOKEN_CONST)) {
    varDeclaration(false);
  } else {
    statement();
  }

  if (parser.panicMode) synchronize();
}

static void statement() {
  if (match(TOKEN_AWAIT)) {
    awaitStatement();
  } else if (match(TOKEN_BREAK)) {
    breakStatement();
  } else if (match(TOKEN_CONTINUE)) {
    continueStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_SWITCH)) {
    switchStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_ASSERT)) {
    assertStatement();
  } else if (match(TOKEN_THROW)) {
    throwStatement();
  } else if (match(TOKEN_TRY)) {
    tryStatement();
  } else if (match(TOKEN_USING)) {
    usingStatement();
  } else if (match(TOKEN_REQUIRE)) {
    requireStatement();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
  } else if (match(TOKEN_YIELD)) {
    yieldStatement();
  } else if (match(TOKEN_DO)) {
    doWhileStatement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    beginScope();
    block();
    endScope();
  } else {
    expressionStatement();
  }
}

ObjFunction* compile(const char* source) {
  initScanner(source);
  Compiler compiler;
  initCompiler(&compiler, TYPE_SCRIPT, false);

  parser.rootClass = syntheticToken("Object");
  parser.hadError = false;
  parser.panicMode = false;

  advance();
  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction* function = endCompiler();
  freeStack(&namespaceStack);
  return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
