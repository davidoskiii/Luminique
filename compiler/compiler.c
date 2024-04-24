#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../object/object.h"
#include "../common.h"
#include "compiler.h"
#include "../memory/memory.h"
#include "../scanner/scanner.h"
#include "../string/string.h"

#ifdef DEBUG_PRINT_CODE
#include "../debug/debug.h"
#endif

#define MAX_CASES 256

typedef struct {
  Token current;
  Token previous;
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
  TYPE_INITIALIZER,
  TYPE_METHOD,
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
} Compiler;

typedef struct ClassCompiler {
  struct ClassCompiler* enclosing;
} ClassCompiler;

Parser parser;
Compiler* current = NULL;
ClassCompiler* currentClass = NULL;

static void emitConstant(Value value);
static uint8_t makeConstant(Value value);

static Chunk* currentChunk() {
  return &current->function->chunk;
}

static void errorAt(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  fprintf(stderr, "[line %d] Error", token->line);

  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end");
  } else if (token->type == TOKEN_ERROR) {
    // Nothing.
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }

  fprintf(stderr, ": %s\n", message);
  parser.hadError = true;
}

static void error(const char* message) {
  errorAt(&parser.previous, message);
}

static void errorAtCurrent(const char* message) {
  errorAt(&parser.current, message);
}

static void advance() {
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

  errorAtCurrent(message);
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

static void emitLoop(int loopStart) {
  emitByte(OP_LOOP);

  int offset = currentChunk()->count - loopStart + 2;
  if (offset > UINT16_MAX) error("Loop body too large.");

  emitByte((offset >> 8) & 0xff);
  emitByte(offset & 0xff);
}

static int emitJump(uint8_t instruction) {
  emitByte(instruction);
  emitByte(0xff);
  emitByte(0xff);
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
  emitBytes(OP_CONSTANT, makeConstant(value));
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

uint8_t identifierConstant(Token* name) {
  return makeConstant(OBJ_VAL(copyString(name->start, name->length)));
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
    }
    else {
      offset += opCodeOffset(chunk, offset);
    }
  }
}

static void initCompiler(Compiler* compiler, FunctionType type) {
  compiler->enclosing = current;
  compiler->function = NULL;
  compiler->type = type;
  compiler->localCount = 0;
  compiler->function = newFunction();
  compiler->scopeDepth = 0;
  compiler->innermostLoopStart = -1;
  compiler->innermostLoopScopeDepth = 0;
  current = compiler;

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

static uint8_t makeConstant(Value value) {
  int constant = addConstant(currentChunk(), value);
  if (constant > UINT8_MAX) {
    error("Too many constants in one chunk.");
    return 0;
  }

  return (uint8_t)constant;
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
static void function(FunctionType type);
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


static uint8_t propretyConstant(const char* message) {
  switch (parser.current.type) {
    case TOKEN_IDENTIFIER:
    case TOKEN_EQUAL_EQUAL:
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
    return addUpvalue(compiler, (uint8_t)upvalue, false, compiler->enclosing->locals[local].isMutable);
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
  int slot = makeConstant(OBJ_VAL(copyString(name, length)));
  emitByte(OP_INVOKE);
  emitByte(slot);
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

static uint8_t parseVariable(const char* errorMessage) {
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

static void defineVariable(uint8_t global, bool isMutable) {
  if (current->scopeDepth > 0) {
    markInitialized(isMutable);
    return;
  } else {
    ObjString* name = identifierName(global);
    Value value;
    if (tableGet(&vm.globals, name, &value)) {
      error("Cannot redeclare global variable.");
    }

    if (isMutable) {
      tableSet(&vm.globals, name, NIL_VAL);
      emitBytes(OP_DEFINE_GLOBAL, global);
    }
    else {
      tableSet(&vm.rootNamespace->values, name, NIL_VAL);
      emitBytes(OP_DEFINE_CONST, global);
    }
  }
}

static uint8_t argumentList() {
  uint8_t argCount = 0;
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
    uint8_t constant = parseVariable("Expect variadic parameter name.");
    defineVariable(constant, false);
    return;
  }

  do {
    current->function->arity++;
    if (current->function->arity > 255) {
      errorAtCurrent("Can't have more than 255 parameters.");
    }

    if (match(TOKEN_CONST)) {
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant, false);
    } else {
      uint8_t constant = parseVariable("Expect parameter name.");
      defineVariable(constant, true);
    }
  } while (match(TOKEN_COMMA));
}

static void and_(bool canAssign) {
  int endJump = emitJump(OP_JUMP_IF_FALSE);

  emitByte(OP_POP);
  parsePrecedence(PREC_AND);

  patchJump(endJump);
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
    default: return; // Unreachable.
  }
}

static void call(bool canAssign) {
  uint8_t argCount = argumentList();
  emitBytes(OP_CALL, argCount);
}

static void dot(bool canAssign) {
  uint8_t name = propretyConstant("Expect property name after '.'.");

  if (canAssign && match(TOKEN_EQUAL)) {
    expression();
    emitBytes(OP_SET_PROPERTY, name);
  } else if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    emitBytes(OP_INVOKE, name);
    emitByte(argCount);
  } else {
    emitBytes(OP_GET_PROPERTY, name);
  }
}

static void coloncolon(bool canAssign) {
  uint8_t name = propretyConstant("Expect property name after '::'.");

  emitBytes(OP_GET_NAMESPACE, name);
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
  int value = strtol(parser.previous.start, NULL, 10);
  emitConstant(INT_VAL(value));
}

static void number(bool canAssign) {
  double value = strtod(parser.previous.start, NULL);
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
    invokeMethod(0, "toString", 8);

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
    case OP_SET_GLOBAL:
      ObjString* name = identifierName(arg);
      Value value;
      if (tableGet(&vm.rootNamespace->values, name, &value)) { 
        error("Cannot assign to immutable global variables.");
      }
      break;
    default:
      break;
  }
}

static void closure(bool canAssign) {
  function(TYPE_FUNCTION);
}

static void lambda(bool canAssign) {
  function(TYPE_LAMBDA);
}

static void namedVariable(Token name, bool canAssign) {
  uint8_t getOp, setOp;
  int arg = resolveLocal(current, &name);
  if (arg != -1) {
    getOp = OP_GET_LOCAL;
    setOp = OP_SET_LOCAL;
  } else if ((arg = resolveUpvalue(current, &name)) != -1) {
    getOp = OP_GET_UPVALUE;
    setOp = OP_SET_UPVALUE;
  } else {
    arg = identifierConstant(&name);
    getOp = OP_GET_GLOBAL;
    setOp = OP_SET_GLOBAL;
  }

  if (canAssign && match(TOKEN_EQUAL)) {
    checkMutability(arg, setOp);
    expression();
    emitBytes(setOp, (uint8_t)arg);
  } else {
    emitBytes(getOp, (uint8_t)arg);
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

  variable(false);
}

static void namespace_(bool canAssign) {
  consume(TOKEN_IDENTIFIER, "Expect Namespace identifier.");
  ObjString* name = copyString(parser.previous.start, parser.previous.length);
  emitConstant(OBJ_VAL(name));
}

static void super_(bool canAssign) {
  if (currentClass == NULL) {
    error("Can't use 'super' outside of a class.");
  }

  consume(TOKEN_DOT, "Expect '.' after 'super'.");
  consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
  uint8_t name = identifierConstant(&parser.previous);

  namedVariable(syntheticToken("this"), false);
  if (match(TOKEN_LEFT_PAREN)) {
    uint8_t argCount = argumentList();
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_SUPER_INVOKE, name);
    emitByte(argCount);
  } else {
    namedVariable(syntheticToken("super"), false);
    emitBytes(OP_GET_SUPER, name);
  }
}

static void unary(bool canAssign) {
  TokenType operatorType = parser.previous.type;

  // Compile the operand.
  parsePrecedence(PREC_UNARY);

  // Emit the operator instruction.
  switch (operatorType) {
    case TOKEN_BANG: emitByte(OP_NOT); break;
    case TOKEN_MINUS: emitByte(OP_NEGATE); break;
    default: return; // Unreachable.
  }
}

ParseRule rules[] = {
  [TOKEN_LEFT_BRAKE]    = {array,         subscript, PREC_CALL}, 
  [TOKEN_RIGHT_BRAKE]   = {NULL,          NULL,      PREC_NONE},
  [TOKEN_LEFT_PAREN]    = {grouping,      call,      PREC_CALL},
  [TOKEN_RIGHT_PAREN]   = {NULL,          NULL,      PREC_NONE},
  [TOKEN_LEFT_BRACE]    = {dictionary,    NULL,      PREC_NONE}, 
  [TOKEN_RIGHT_BRACE]   = {NULL,          NULL,      PREC_NONE},
  [TOKEN_COMMA]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_DOT]           = {NULL,          dot,       PREC_CALL},
  [TOKEN_COLON_COLON]   = {NULL,          coloncolon,PREC_CALL},
  [TOKEN_DOT_DOT_DOT]   = {NULL,          binary,    PREC_NONE},
  [TOKEN_MINUS_MINUS]   = {NULL,          NULL,      PREC_CALL},
  [TOKEN_MINUS]         = {unary,         binary,    PREC_TERM},
  [TOKEN_PLUS_PLUS]     = {NULL,          NULL,      PREC_CALL},
  [TOKEN_PLUS]          = {NULL,          binary,    PREC_TERM},
  [TOKEN_SEMICOLON]     = {NULL,          NULL,      PREC_NONE},
  [TOKEN_SLASH]         = {NULL,          binary,    PREC_FACTOR},
  [TOKEN_STAR]          = {NULL,          binary,    PREC_FACTOR},
  [TOKEN_MODULO]        = {NULL,          binary,    PREC_FACTOR},
  [TOKEN_POWER]         = {NULL,          binary,    PREC_FACTOR},
  [TOKEN_BANG]          = {unary,         NULL,      PREC_NONE},
  [TOKEN_BANG_EQUAL]    = {NULL,          binary,    PREC_EQUALITY},
  [TOKEN_EQUAL]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_EQUAL_EQUAL]   = {NULL,          binary,    PREC_EQUALITY},
  [TOKEN_GREATER]       = {NULL,          binary,    PREC_COMPARISON},
  [TOKEN_GREATER_EQUAL] = {NULL,          binary,    PREC_COMPARISON},
  [TOKEN_LESS]          = {NULL,          binary,    PREC_COMPARISON},
  [TOKEN_LESS_EQUAL]    = {NULL,          binary,    PREC_COMPARISON},
  [TOKEN_IDENTIFIER]    = {variable,      NULL,      PREC_NONE},
  [TOKEN_STRING]        = {string,        NULL,      PREC_NONE},
  [TOKEN_INTERPOLATION] = {interpolation, NULL,      PREC_NONE},
  [TOKEN_NUMBER]        = {number,        NULL,      PREC_NONE},
  [TOKEN_INT]           = {integer,       NULL,      PREC_NONE},
  [TOKEN_AND]           = {NULL,          and_,      PREC_AND},
  [TOKEN_CLASS]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_ELSE]          = {NULL,          NULL,      PREC_NONE},
  [TOKEN_FALSE]         = {literal,       NULL,      PREC_NONE},
  [TOKEN_FOR]           = {NULL,          NULL,      PREC_NONE},
  [TOKEN_FUN]           = {closure,       NULL,      PREC_NONE},
  [TOKEN_LAMBDA]        = {lambda,        NULL,      PREC_NONE},
  [TOKEN_IF]            = {NULL,          NULL,      PREC_NONE},
  [TOKEN_NIL]           = {literal,       NULL,      PREC_NONE},
  [TOKEN_OR]            = {NULL,          or_,       PREC_OR},
  [TOKEN_REQUIRE]       = {NULL,          NULL,      PREC_NONE},
  [TOKEN_RETURN]        = {NULL,          NULL,      PREC_NONE},
  [TOKEN_SUPER]         = {super_,        NULL,      PREC_NONE},
  [TOKEN_THIS]          = {this_,         NULL,      PREC_NONE},
  [TOKEN_TRUE]          = {literal,       NULL,      PREC_NONE},
  [TOKEN_VAR]           = {NULL,          NULL,      PREC_NONE},
  [TOKEN_CONST]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_WHILE]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_ERROR]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_THROW]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_TRY]           = {NULL,          NULL,      PREC_NONE},
  [TOKEN_CATCH]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_FINALLY]       = {NULL,          NULL,      PREC_NONE},
  [TOKEN_AS]            = {NULL,          NULL,      PREC_NONE},
  [TOKEN_NAMESPACE]     = {NULL,          NULL,      PREC_NONE},
  [TOKEN_USING]         = {NULL,          NULL,      PREC_NONE},
  [TOKEN_EOF]           = {NULL,          NULL,      PREC_NONE}
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

static void functionParameters() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

  if (!check(TOKEN_RIGHT_PAREN)) {
    parameterList();
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
}

static void lambdaParameters() {
  consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

  if (!check(TOKEN_RIGHT_PAREN)) {
    parameterList();
  }

  consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
  consume(TOKEN_COLON, "Expect ':' after ')'.");
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

static void function(FunctionType type) {
  Compiler compiler;
  initCompiler(&compiler, type);
  beginScope(); 

  if (type == TYPE_LAMBDA) {
    lambdaParameters();
    
    consume(TOKEN_LEFT_PAREN, "Expect '(' after ':'.");

    while (!check(TOKEN_RIGHT_PAREN) && !check(TOKEN_EOF)) {
      declaration();
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after lambda body.");
  } else {
    functionParameters();
    block();
  }

  ObjFunction* function = endCompiler();
  emitBytes(OP_CLOSURE, makeConstant(OBJ_VAL(function)));

  for (int i = 0; i < function->upvalueCount; i++) {
    emitByte(compiler.upvalues[i].isLocal ? 1 : 0);
    emitByte(compiler.upvalues[i].index);
  }
}

static void method() {
  consume(TOKEN_FUN, "Expect 'function' keyword");

  uint8_t constant = propretyConstant("Expect method name.");

  FunctionType type = TYPE_METHOD;
  if (parser.previous.length == 8 &&
      memcmp(parser.previous.start, "__init__", 8) == 0) {
    type = TYPE_INITIALIZER;
  }

  function(type);
  emitBytes(OP_METHOD, constant);
}

static void classDeclaration() {
  consume(TOKEN_IDENTIFIER, "Expect class name.");
  Token className = parser.previous;
  uint8_t nameConstant = identifierConstant(&parser.previous);
  declareVariable();

  emitBytes(OP_CLASS, nameConstant);

  ClassCompiler classCompiler;
  classCompiler.enclosing = currentClass;
  currentClass = &classCompiler;


  if (match(TOKEN_COLON)) {
    consume(TOKEN_IDENTIFIER, "Expect superclass name.");
    variable(false);

    if (identifiersEqual(&className, &parser.previous)) {
      error("A class can't inherit from itself.");
    }
  } else {
    namedVariable(parser.rootClass, false);
    if (identifiersEqual(&className, &parser.rootClass)) {
      error("Cannot redeclare root class Object.");
    }
  }

  beginScope();
  addLocal(syntheticToken("super"));
  defineVariable(0, false);
  namedVariable(className, false);
  emitByte(OP_INHERIT);

  namedVariable(className, false);
  consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");

  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    method();
  }

  consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");
  emitByte(OP_POP);
  endScope();

  currentClass = currentClass->enclosing;
}

static void varDeclaration(bool isMutable) {
  uint8_t global = parseVariable("Expect variable name.");

  if (!isMutable && !check(TOKEN_EQUAL)) {
    error("Const variable must be initialized upon declaration.");
  } else if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emitByte(OP_NIL);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");

  defineVariable(global, isMutable);
}

static void funDeclaration() {
  uint8_t global = parseVariable("Expect function name.");
  markInitialized(false);
  function(TYPE_FUNCTION);
  defineVariable(global, false);
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
    int exitJump = emitJump(OP_JUMP_IF_FALSE);

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

static void namespaceDeclaration() {
  uint8_t namespaceDepth = 0;
  do {
    if (namespaceDepth > UINT4_MAX) {
      errorAtCurrent("Can't have more than 15 levels of namespace depth.");
    }
    namespace_(false);
    namespaceDepth++;
  } while (match(TOKEN_DOT));

  consume(TOKEN_SEMICOLON, "Expect semicolon after namespace declaration.");
  emitBytes(OP_NAMESPACE, namespaceDepth);
}

static void usingStatement() {
  uint8_t namespaceDepth = 0;
  do { 
    consume(TOKEN_IDENTIFIER, "Expect namespace identifier.");
    emitIdentifier(&parser.previous);
    namespaceDepth++;
  } while (match(TOKEN_DOT));

  emitBytes(OP_USING, namespaceDepth);
  uint8_t alias = makeConstant(OBJ_VAL(newString("")));

  if (match(TOKEN_AS)) {
    consume(TOKEN_IDENTIFIER, "Expect alias after 'as'.");
    Token name = parser.previous;
    alias = identifierConstant(&name);
  }
  consume(TOKEN_SEMICOLON, "Expect ';' after using statement.");
  emitBytes(OP_SUBNAMESPACE, alias);
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
  if(current->type == TYPE_LAMBDA) depth = lambdaDepth();

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

static void synchronize() {
  parser.panicMode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_CLASS:
      case TOKEN_NAMESPACE:
      case TOKEN_USING:
      case TOKEN_SWITCH:
      case TOKEN_THROW:
      case TOKEN_FUN:
      case TOKEN_CONST:
      case TOKEN_VAR:
      case TOKEN_FOR:
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_RETURN:
        return;

      default:
        ; // Do nothing.
    }

    advance();
  }
}

static void declaration() {
  if (match(TOKEN_CLASS)) {
    classDeclaration();
  } else if (match(TOKEN_FUN)) {
    funDeclaration();
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
  if (match(TOKEN_BREAK)) {
    breakStatement();
  } else if (match(TOKEN_CONTINUE)) {
    continueStatement();
  } else if (match(TOKEN_FOR)) {
    forStatement();
  } else if (match(TOKEN_SWITCH)) {
    switchStatement();
  } else if (match(TOKEN_IF)) {
    ifStatement();
  } else if (match(TOKEN_THROW)) {
    throwStatement();
  } else if (match(TOKEN_TRY)) {
    tryStatement();
  } else if (match(TOKEN_NAMESPACE)) {
    namespaceDeclaration();
  } else if (match(TOKEN_USING)) {
    usingStatement();
  } else if (match(TOKEN_REQUIRE)) {
    requireStatement();
  } else if (match(TOKEN_RETURN)) {
    returnStatement();
  } else if (match(TOKEN_WHILE)) {
    whileStatement();
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
  initCompiler(&compiler, TYPE_SCRIPT);

  parser.rootClass = syntheticToken("Object");
  parser.hadError = false;
  parser.panicMode = false;

  advance();
  advance();

  while (!match(TOKEN_EOF)) {
    declaration();
  }

  ObjFunction* function = endCompiler();
  return parser.hadError ? NULL : function;
}

void markCompilerRoots() {
  Compiler* compiler = current;
  while (compiler != NULL) {
    markObject((Obj*)compiler->function);
    compiler = compiler->enclosing;
  }
}
