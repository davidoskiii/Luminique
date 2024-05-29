#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "../common.h"
#include "scanner.h"

typedef struct {
  const char* start;
  const char* current;
  int line;
  int column;
  int interpolationDepth;
} Scanner;

Scanner scanner;

void initScanner(const char* source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
  scanner.column = 1;
}

static bool isAlpha(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
          c == '_';
}

static bool isDigit(char c) {
  return c >= '0' && c <= '9';
}

static bool isHex(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
    (c >= 'A' && c <= 'F');
}

static bool isBinary(char c) {
  return (c == '0' || c == '1');
}

static bool isOct(char c) {
  return (c >= '0' && c <= '7');
}

static bool isAtEnd() {
  return *scanner.current == '\0';
}

static char advance() {
  scanner.current++;
  scanner.column++;
  return scanner.current[-1];
}

static char peek() {
  return *scanner.current;
}

static char peekNext() {
  if (isAtEnd()) return '\0';
  return scanner.current[1];
}

static char peekPrevious() {
  return scanner.current[-1];
}

static bool match(char expected) {
  if (isAtEnd()) return false;
  if (*scanner.current != expected) return false;
  scanner.current++;
  scanner.column++;
  return true;
}

static bool matchNext(char expected) {
  if (isAtEnd() || peekNext() != expected) return false;
  scanner.current += 2;
  scanner.column += 2;
  return true;
}

static Token makeToken(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (int)(scanner.current - scanner.start);
  token.line = scanner.line;
  token.startColumn = scanner.column - token.length;
  token.endColumn = scanner.column - 1;
  return token;
}

static Token errorToken(const char* message) {
  Token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = (int)strlen(message);
  token.line = scanner.line;
  token.startColumn = scanner.column;
  token.endColumn = scanner.column + token.length - 1;
  scanner.interpolationDepth = 0;
  return token;
}

static void skipWhitespace() {
  for (;;) {
    char c = peek();
    switch (c) {
      case ' ':
      case '\r':
      case '\t':
        advance();
        break;
      case '\n':
        scanner.line++;
        scanner.column = 1;
        advance();
        break;
      case '/':
        if (peekNext() == '/') {
          // Single-line comment: skip until the end of the line.
          while (peek() != '\n' && !isAtEnd()) advance();
        } else if (peekNext() == '*') {
          // Multi-line comment: skip until the closing token '*/'.
          advance();
          advance();
          while (!(peek() == '*' && peekNext() == '/') && !isAtEnd()) {
            if (peek() == '\n') {
              scanner.line++;
              scanner.column = 1;
            }
            advance();
          }
          advance();
          advance();
        } else {
          return;
        }
        break;
      default:
        return;
    }
  }
}

static TokenType checkKeyword(int start, int length,
    const char* rest, TokenType type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

static TokenType identifierType() {
  switch (scanner.start[0]) {
    case 'a':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'n': return checkKeyword(2, 1, "d", TOKEN_AND);
          case 's': return checkKeyword(2, 0, "", TOKEN_AS);
        }
      }
      break;
    case 'b': return checkKeyword(1, 4, "reak", TOKEN_BREAK);
    case 'd': return checkKeyword(1, 6, "efault", TOKEN_DEFAULT);
    case 'c':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'l': return checkKeyword(2, 3, "ass", TOKEN_CLASS);
          case 'a': 
            if (scanner.current - scanner.start > 2) {
              switch (scanner.start[2]) {
                case 's': return checkKeyword(3, 1, "e", TOKEN_CASE);
                case 't': return checkKeyword(3, 2, "ch", TOKEN_CATCH);
              }
            }
          case 'o': 
            if (scanner.current - scanner.start > 2) {
              switch (scanner.start[2]) {
                case 'n': 
                  if (scanner.current - scanner.start > 3) {
                    switch (scanner.start[3]) {
                      case 's': return checkKeyword(4, 1, "t", TOKEN_CONST);
                      case 't': return checkKeyword(4, 4, "inue", TOKEN_CONTINUE);
                    }
                  }
              }
            }
        }
      }
      break;
    case 'e': return checkKeyword(1, 3, "lse", TOKEN_ELSE);
    case 'f':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'a': return checkKeyword(2, 3, "lse", TOKEN_FALSE);
          case 'o': return checkKeyword(2, 1, "r", TOKEN_FOR);
          case 'i': return checkKeyword(2, 5, "nally", TOKEN_FINALLY);
          case 'u': return checkKeyword(2, 6, "nction", TOKEN_FUN);
        }
      }
      break;
    case 'i': return checkKeyword(1, 1, "f", TOKEN_IF);
    case 'l':
        if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'e': return checkKeyword(2, 1, "t", TOKEN_VAR);
          case 'a': return checkKeyword(2, 4, "mbda", TOKEN_LAMBDA);
        }
      }
      break;
    case 'n': 
        if (scanner.current - scanner.start > 1) {
          switch (scanner.start[1]) {
            case 'a': return checkKeyword(2, 7, "mespace", TOKEN_NAMESPACE);
            case 'i': return checkKeyword(2, 1, "l", TOKEN_NIL);
          }
        }
    case 'o': return checkKeyword(1, 1, "r", TOKEN_OR);
    case 'r': 
      if (scanner.current - scanner.start > 2) {
        switch (scanner.start[2]) {
          case 'q': return checkKeyword(3, 4, "uire", TOKEN_REQUIRE);
          case 't': return checkKeyword(3, 3, "urn", TOKEN_RETURN);
        }
      }
      break;
    case 's':
        if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'u': return checkKeyword(2, 3, "per", TOKEN_SUPER);
          case 't': return checkKeyword(2, 4, "atic", TOKEN_STATIC);
          case 'w': return checkKeyword(2, 4, "itch", TOKEN_SWITCH);
        }
      }
      break;
    case 't':
      if (scanner.current - scanner.start > 1) {
        switch (scanner.start[1]) {
          case 'y': return checkKeyword(2, 4, "peof", TOKEN_TYPEOF);
          case 'h':
            if (scanner.current - scanner.start > 2) {
              switch (scanner.start[2]) {
                case 'i': return checkKeyword(3, 1, "s", TOKEN_THIS);
                case 'r': return checkKeyword(3, 2, "ow", TOKEN_THROW);
              }
            }
          case 'r':
            if (scanner.current - scanner.start > 2) {
              switch (scanner.start[2]) {
                case 'u': return checkKeyword(3, 1, "e", TOKEN_TRUE);
                case 'y': return checkKeyword(3, 0, "", TOKEN_TRY);
              }
            }
          }
        }
      break;
    case 'u': return checkKeyword(1, 4, "sing", TOKEN_USING);
    case 'w': return checkKeyword(1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

static Token identifier() {
  while (isAlpha(peek()) || isDigit(peek())) advance();
  return makeToken(identifierType());
}

static Token number(char cur) {
  if (cur == '0' && (peek() == 'c' || peek() == 'C')) {
    advance();
    while (isOct(peek())) advance();
    return makeToken(TOKEN_OCT);
  } else if (cur == '0' && (peek() == 'x' || peek() == 'X')) {
    advance();
    while (isHex(peek())) advance();
    return makeToken(TOKEN_HEX);
  } else if (cur == '0' && (peek() == 'b' || peek() == 'B')) {
    advance();
    while (isBinary(peek())) advance();
    return makeToken(TOKEN_BIN);
  } else {
    while (isDigit(peek())) advance();

    if (peek() == '.' && isDigit(peekNext())) {
      advance();

      while (isDigit(peek())) advance();
      return makeToken(TOKEN_NUMBER);
    }

    return makeToken(TOKEN_INT);
  }
}

static Token string() {
  while ((peek() != '"' || peekPrevious() == '\\') && !isAtEnd()) {
    if (peek() == '\n') scanner.line++;
    else if (peek() == '$' && peekNext() == '{') {
      if (scanner.interpolationDepth >= 15) {
        return errorToken("Interpolation may only nest 15 levels deep.");
      }
      scanner.interpolationDepth++;
      advance();
      Token token = makeToken(TOKEN_INTERPOLATION);
      advance();
      return token;
    }
    advance();
  }

  if (isAtEnd()) return errorToken("Unterminated string.");
  advance();
  return makeToken(TOKEN_STRING);
}

Token scanToken() {
  skipWhitespace();
  scanner.start = scanner.current;

  if (isAtEnd()) return makeToken(TOKEN_EOF);

  char c = advance();
  if (isAlpha(c)) return identifier();
  if (isDigit(c)) return number(c);

  switch (c) {
    case '(': return makeToken(TOKEN_LEFT_PAREN);
    case ')': return makeToken(TOKEN_RIGHT_PAREN);
    case '[': return makeToken(TOKEN_LEFT_BRAKE);
    case ']': return makeToken(TOKEN_RIGHT_BRAKE);
    case '{': return makeToken(TOKEN_LEFT_BRACE);
    case '}': 
      if (scanner.interpolationDepth > 0) {
        scanner.interpolationDepth--;
        return string();
      }
      return makeToken(TOKEN_RIGHT_BRACE);
    case ';': return makeToken(TOKEN_SEMICOLON);
    case ',': return makeToken(TOKEN_COMMA);
    case '.': {
      if (match('.')) {
        if (match('.')) {
          return makeToken(TOKEN_DOT_DOT_DOT);
        }
        
        return errorToken("Expected another '.'");
      }
      
      return makeToken(TOKEN_DOT);
    }
    case '^': return makeToken(TOKEN_CARRET);
    case '~': return makeToken(TOKEN_TILDA);
    case '-': return makeToken(
          match('-') ? TOKEN_MINUS_MINUS : TOKEN_MINUS);
    case '+': return makeToken(
          match('+') ? TOKEN_PLUS_PLUS : TOKEN_PLUS);
    case '/': return makeToken(TOKEN_SLASH);
    case '*': return makeToken(
          match('*') ? TOKEN_POWER : TOKEN_STAR); 
    case '%': return makeToken(TOKEN_MODULO);
    case ':': return makeToken(
          match(':') ? TOKEN_COLON_COLON : TOKEN_COLON);
    case '?': return makeToken(TOKEN_QUESTION);

    case '!':
      return makeToken(
          match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
    case '=':
      return makeToken(
          match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
    case '<':
      if (match('=')) return makeToken(TOKEN_LESS_EQUAL);
      else if (match('<')) return makeToken(TOKEN_SHOWEL_L);
      else return makeToken(TOKEN_LESS);
    case '>': {
      if (match('=')) return makeToken(TOKEN_GREATER_EQUAL);
      else if (match('>')) return makeToken(TOKEN_SHOWEL_R);
      else return makeToken(TOKEN_GREATER);
    }
    case '&':
      if (match('&')) {
        return makeToken(TOKEN_AND);
      } else {
        return makeToken(TOKEN_AMP);
      }
    case '|':
      if (match('|')) {
        return makeToken(TOKEN_OR);
      } else {
        return makeToken(TOKEN_PIPE);
      }

    case '"': return string();
  }

  return errorToken("Unexpected character.");
}
