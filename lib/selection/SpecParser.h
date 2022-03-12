//
// Created by sebastian on 11.03.22.
//

#ifndef CAPI_SPECPARSER_H
#define CAPI_SPECPARSER_H

#include <string>
#include <sstream>
#include <optional>
#include <iostream>

namespace capi {

/*
class SpecString {

  std::string specStr;
public:
  SpecString(std::string specStr) : specStr(std::move(specStr)) {}
};
*/


class CharReader {
  std::string input;
  int pos;
public:
  explicit CharReader(std::string input) : input(std::move(input)) {
    pos = 0;
  }

  bool eof() const {
    return pos == input.size();
  }

  char get() {
    if (eof()) {
      return 0;
    }
    return input[pos++];
  }

  void consume() {
    if (pos < input.size())
      pos++;
  }

  char next() {
    consume();
    return peek();
  }


  char peek(unsigned i = 0) {
    return pos + i < input.size() ? input[pos+i] : 0;
  }
};

struct Token {
  enum Kind {
    UNKNOWN, END_OF_FILE, IDENTIFIER, LITERAL, INT, FLOAT, BOOL, LEFT_PARAN, RIGHT_PARAN, PERCENT, EQUALS, COMMA
  };

  Token(Kind kind, std::string spelling) : kind(kind), spelling(spelling){
  }

  explicit Token(Kind kind) : kind(kind) {
    switch(kind) {
      case LEFT_PARAN:
        spelling = '(';
        break;
      case RIGHT_PARAN:
        spelling = ')';
        break;
      case PERCENT:
        spelling = '%';
        break;
      case EQUALS:
        spelling = "=";
        break;
      case COMMA:
        spelling = ",";
        break;
      default:
        break;
    }
  }

  bool valid() const {
    return kind != UNKNOWN;
  }

  Kind kind;
  std::string spelling;


};

struct LexResult {

  LexResult(Token token) : token(std::move(token)) {}
  LexResult(std::string msg) : msg(std::move(msg)) {}

  operator bool() const {
    return token->valid();
  }

  Token* operator->() {
    return &token.value();
  }

  std::optional<Token> token{};
  std::string msg;
};

class Lexer
{
  CharReader reader;
public:

  constexpr static char QUOTE_CHAR = '\"';

  explicit Lexer(std::string input) : reader(std::move(input))
  {}

  bool eof() const {
    return reader.eof();
  }

  LexResult next() {
    char c = reader.peek();

    if (c == '-' || std::isdigit(c)) {
      return parseNumber();
    }

    switch(c) {
      case '\0':
        return Token(Token::END_OF_FILE);
      case QUOTE_CHAR:
        return parseLiteral();
      case '(':
        reader.consume();
        return Token(Token::LEFT_PARAN);
      case ')':
        reader.consume();
        return Token(Token::RIGHT_PARAN);
      case '%':
        reader.consume();
        return Token(Token::PERCENT);
      case '=':
        reader.consume();
        return Token(Token::EQUALS);
      case ',':
        reader.consume();
        return Token(Token::COMMA);
      default:
        break;
    }

    if (isalpha(c)) {
      return parseIdentifier();
    }

    std::string errMsg = "Encountered unexpected character: ";
    errMsg += c;
    return errMsg;
  }

  void skipWhitespace() {
    char c = reader.peek();
    while (isspace(c)) {
      c = reader.next();
    }
  }

  LexResult parseIdentifier() {
    std::stringstream identifier;
    char c = reader.peek();

    if (!isalpha(c)) {
      return {"Identifiers must start with alphabetic character"};
    }
    identifier << c;

    c = reader.next();
    while (isalpha(c) || c == '_') {
      identifier << c;
      c = reader.next();
    }

    return Token(Token::LITERAL, identifier.str());
  }

  LexResult parseNumber() {

    std::stringstream numberStr;
    bool isFloat{false};

    // TODO: Currently does not allow for scientific notation or space after the minus for negative numbers

    char c = reader.peek();

    while (c == '-' || c == '.' || isdigit(c)) {
      if (c == '.')
        isFloat = true;
      numberStr << c;
      c = reader.next();
    }

    return Token(isFloat ? Token::FLOAT : Token::INT, numberStr.str());

  }


  LexResult parseLiteral() {
    std::stringstream literal;
    char c = reader.get();
    if (c != QUOTE_CHAR) {
      return {"Expected literal"};
    }
    bool done = false;
    do {
      if (reader.eof())
        return {"Unexpected end"};
      c = reader.get();
      if (c == '\\') {
        c = reader.get();
        switch(c) {
          case '\\':
            literal << '\\';
            break;
          case QUOTE_CHAR:
            literal << QUOTE_CHAR;
            break;
          default:
            return {"Escape char \\ must be followed by \\ or \""};
        }
      }
      if (c == QUOTE_CHAR) {
        done = true;
      } else {
        literal << c;
      }
    } while(!done);
    return Token(Token::LITERAL, literal.str());
  }


};

class SpecNode {

  enum Kind {
    SPEC, SELECTOR_DECL, SELECTOR_DEF,
  };

};

class SpecParser
{
  Lexer lexer;
public:
  explicit SpecParser(std::string input) : lexer(std::move(input))
  {}

  bool eof() {
    return lexer.eof();
  }

  void parse() {
    do {
      lexer.skipWhitespace();
      parseSelectorDecl();
      lexer.skipWhitespace();
    } while(eof());
  }



  void parseSelectorDecl() {
    auto t1 = lexer.next();
    if (!t1) {
      std::cerr << "Expected selector declaration\n";
      return;
    }
    auto t2 = lexer.next();
    if (!t2) {
      std::cerr << "Expected '=' or selector definition\n";
    }
    if (t2->kind==Token::EQUALS) {
      // TODO: Parse definition
    }
  }

  void parseSelectorDef() {

  }



};

}

#endif //CAPI_SPECPARSER_H
