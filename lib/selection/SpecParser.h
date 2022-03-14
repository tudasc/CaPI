//
// Created by sebastian on 11.03.22.
//

#ifndef CAPI_SPECPARSER_H
#define CAPI_SPECPARSER_H

#include <string>
#include <sstream>
#include <optional>
#include <iostream>
#include <memory>
#include <vector>
#include <algorithm>

#include "SelectionSpecAST.h"
#include "Utils.h"

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
  std::vector<int> markers;
public:
  explicit CharReader(std::string input) : input(std::move(input)) {
    pos = 0;
  }

  std::string getInput() const {
    return input;
  }

  int getPos() const {
    return pos;
  }

  bool eof() const {
    return pos >= input.size();
  }

  char consume() {
    if (eof()) {
      return 0;
    }
    return input[pos++];
  }

  char next() {
    consume();
    return peek();
  }

  char peek(unsigned i = 0) {
    return pos + i < input.size() ? input[pos+i] : 0;
  }

  void pushMarker() {
    markers.push_back(pos);
  }

  void discardMarker() {
    if (markers.empty()) {
      std::cerr << "Cannot discard marker: no markers saved.\n";
    }
    markers.pop_back();
  }

  bool backtrack() {
    if (markers.empty()) {
      std::cerr << "Cannot backtrack: no markers saved.\n";
      return false;
    }
    pos = markers.back();
    markers.pop_back();
    return true;
  }
};

struct Token {
  enum Kind {
    UNKNOWN, END_OF_FILE, IDENTIFIER, STR_LITERAL, INT_LITERAL, FLOAT_LITERAL,
    BOOL_LITERAL, LEFT_PAREN, RIGHT_PAREN, PERCENT, EQUALS, COMMA
  };

  Token(Kind kind, std::string spelling) : kind(kind), spelling(spelling){
  }

  explicit Token(Kind kind) : kind(kind) {
    switch(kind) {
      case LEFT_PAREN:
        spelling = '(';
        break;
      case RIGHT_PAREN:
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

  Token get() {
    return token.value();
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

  std::string getInput() const {
    return reader.getInput();
  }

  int getPos() const {
    return reader.getPos();
  }

  LexResult next() {
    skipWhitespace();

    char c = reader.peek();

    switch(c) {
      case '\0':
        return Token(Token::END_OF_FILE);
      case QUOTE_CHAR:
        return parseStringLiteral();
      case '(':
        reader.consume();
        return Token(Token::LEFT_PAREN);
      case ')':
        reader.consume();
        return Token(Token::RIGHT_PAREN);
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


    if (c == '-' || std::isdigit(c)) {
      return parseNumber();
    }

    if (isalpha(c)) {
      auto id = parseIdentifier();
      if (!id) {
        return {"Unable to parse identifier"};
      }
      if (id->spelling == "true") {
        return Token(Token::BOOL_LITERAL, "true");
      }
      if (id->spelling == "false") {
        return Token(Token::BOOL_LITERAL, "false");
      }
      return id;
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
    while (isalpha(c) || isdigit(c) || c == '_') {
      identifier << c;
      c = reader.next();
    }

    return Token(Token::IDENTIFIER, identifier.str());
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

    return Token(isFloat ? Token::FLOAT_LITERAL : Token::INT_LITERAL, numberStr.str());

  }


  LexResult parseStringLiteral() {
    std::stringstream literal;
    char c = reader.consume();
    if (c != QUOTE_CHAR) {
      return {"Expected literal"};
    }
    bool done = false;
    do {
      if (reader.eof())
        return {"Unexpected end"};
      c = reader.consume();
      if (c == '\\') {
        c = reader.consume();
        switch(c) {
          case '\\':
            literal << '\\';
            break;
          case QUOTE_CHAR:
            literal << QUOTE_CHAR;
            break;
          default:
            return {R"(Escape char \ must be followed by \ or ")"};
        }
      }
      if (c == QUOTE_CHAR) {
        done = true;
      } else {
        literal << c;
      }
    } while(!done);
    return Token(Token::STR_LITERAL, literal.str());
  }

  void pushMarker() {
    reader.pushMarker();
  }

  void discardMarker() {
    reader.discardMarker();
  }

  bool backtrack() {
    return reader.backtrack();
  }


};

inline void printErrorMessage(std::string msg) {
  std::cerr << "[Error] " << msg << "\n";
}

inline void printErrorPosition(std::string input, int pos) {
  std::cerr << "[Error] " << input << "\n[Error] ";
  for (int i = 0; i < pos; ++i) {
    std::cerr << " ";
  }
  std::cerr << "^\n";
}

inline void printErrorMessageExpected(int pos, std::string input, std::string expected) {
  std::cerr << "[Error] At index " << pos << ": Expected " << expected << ".\n";
  printErrorPosition(input, pos);
}

inline void printErrorMessageExpected(int pos, std::string input, std::string expected, std::string actual) {
  std::cerr << "[Error] At index " << pos << ": Expected " << expected << ", got " << actual << " instead.\n";
  printErrorPosition(input, pos);
}

class SpecParser
{
  Lexer lexer;
public:
  explicit SpecParser(std::string input) : lexer(std::move(input))
  {}

  bool eof() {
    return lexer.eof();
  }

  ASTPtr parse() {
    std::vector<DeclPtr> decls;
    do {
      auto decl = parseSelectorDecl();
      if (decl) {
        decls.emplace_back(std::move(decl));
      } else {
        printErrorMessage("Parsing failed.");
        return nullptr;
      }
    } while(!eof());
    return std::make_unique<SpecAST>(std::move(decls));
  }

protected:

  NodePtr parseParam() {

    // BNF: param := string | int | float | bool | selectorRef | selectorDef

    lexer.pushMarker();

    auto nextToken = lexer.next();
    if (!nextToken) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector parameter");
      return {};
    }

    switch(nextToken->kind) {
    case Token::STR_LITERAL:
      lexer.discardMarker();
      return std::make_unique<StringLiteral>(nextToken->spelling);
    case Token::INT_LITERAL:
      lexer.discardMarker();
      return IntLiteral::fromString(nextToken->spelling);
    case Token::FLOAT_LITERAL:
      lexer.discardMarker();
      return FloatLiteral::fromString(nextToken->spelling);
    case Token::BOOL_LITERAL:
      lexer.discardMarker();
      return BoolLiteral::fromString(nextToken->spelling);
    case Token::IDENTIFIER:
      // Go back to the last token
      lexer.backtrack();
      return parseSelectorDef();
    case Token::PERCENT:
    {
      lexer.discardMarker();
      auto idToken = lexer.next();
      if (!idToken) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector identifier");
        return {};
      }
      if (idToken->kind != Token::IDENTIFIER && idToken->kind != Token::PERCENT) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector identifier", idToken->spelling);
        return {};
      }
      return std::make_unique<SelectorRef>(idToken->spelling);
    }
    default:
      break;
    }

    printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector parameter", nextToken->spelling);
    return {};
  }

  DefPtr parseSelectorDef() {

    // BNF: selectorDef := selectorType '(' params ')' | selectorType '(' ')'
    //      params := param | params ',' param

    auto selectorToken = lexer.next();
    if (!selectorToken) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector identifier");
      return {};
    }
    if (selectorToken->kind != Token::IDENTIFIER) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "an identifier", selectorToken->spelling);
      return {};
    }
    auto leftParen = lexer.next();
    if (!leftParen) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "'('");
      return {};
    }
    if (leftParen->kind != Token::LEFT_PAREN) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "'('", leftParen->spelling);
      return {};
    }

    lexer.pushMarker();
    auto nextToken = lexer.next();
    if (!nextToken) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "selector parameters or ')'");
      return {};
    }
    SelectorDef::Params params;
    if (nextToken->kind != nextToken->RIGHT_PAREN) {
      // Backtrack so that the next token is the start of the first argument
      lexer.backtrack();
      do {
        auto arg = parseParam();
        if (!arg) {
          printErrorMessage("Could not parse selector parameter.");
          return {};
        }
        params.emplace_back(std::move(arg));
        nextToken = lexer.next();
        if (!nextToken) {
          printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "',' or ')'");
          return {};
        }
      } while (nextToken->kind == Token::COMMA);

      // Make sure that the last token was ')'
      if (nextToken->kind != nextToken->RIGHT_PAREN) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "')'", nextToken->spelling);
        return {};
      }
    } else {
      lexer.discardMarker();
    }

    return std::make_unique<SelectorDef>(selectorToken->spelling, std::move(params));
  }



  DeclPtr parseSelectorDecl() {

    // BNF: selectorDecl := selectorName '=' selectorDef | selectorDef

    lexer.pushMarker();

    auto t1 = lexer.next();
    if (!t1) {
      std::cerr << "Expected selector declaration.\n";
      return nullptr;
    }
    auto t2 = lexer.next();
    if (!t2) {
      std::cerr << "Expected '=' or selector definition.\n";
      return nullptr;
    }

    std::string selectorId = "";

    if (t2->kind==Token::EQUALS) {
      // Name is in t1
      lexer.discardMarker();
      if (t1->kind != Token::IDENTIFIER) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "an identifier", t1->spelling);
      }
      selectorId = t1->spelling;
    } else {
      // Selector is not named -> backtrack to start
      lexer.backtrack();
    }

    auto def = parseSelectorDef();
    if (!def) {
      std::cerr << "Failed to parse selector definition.\n";
      return nullptr;
    }
    return  std::make_unique<SelectorDecl>(std::move(selectorId), std::move(def));
  }



};

}

#endif //CAPI_SPECPARSER_H
