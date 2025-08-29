//
// Created by sebastian on 11.03.22.
//

#ifndef CAPI_QUERYPARSER_H
#define CAPI_QUERYPARSER_H

#include <string>
#include <sstream>
#include <optional>
#include <memory>
#include <vector>
#include <algorithm>
#include <cctype>

#include "capi/selection/SelectionQueryAST.h"
#include "capi/support/Logging.h"

namespace capi {

std::string stripComments(const std::string& input);


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


  int getNumMarkers() const {
    return markers.size();
  }


  bool hasMarkers() const {
    return !markers.empty();
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
    BOOL_LITERAL, LEFT_PAREN, RIGHT_PAREN, PERCENT, EQUALS, COMMA, EXCLAM, PIPE, PLUS, MINUS
  };

  Token(Kind kind, std::string spelling) : kind(kind), spelling(std::move(spelling)){
  }

  explicit Token(Kind kind) : kind(kind) {
    switch(kind) {
      case UNKNOWN:
        spelling = "UNKNOWN";
        break;
      case END_OF_FILE:
        spelling = "EOF";
        break;
      case LEFT_PAREN:
        spelling = "(";
        break;
      case RIGHT_PAREN:
        spelling = ")";
        break;
      case PERCENT:
        spelling = "%";
        break;
      case EQUALS:
        spelling = "=";
        break;
      case COMMA:
        spelling = ",";
        break;
      case EXCLAM:
        spelling = "!";
        break;
      case PIPE:
        spelling = "|";
        break;
      case PLUS:
        spelling = "+";
        break;
      case MINUS:
        spelling = "-";
        break;
      default:
        break;
    }
  }

  bool valid() const {
    return kind != UNKNOWN;
  }

  bool isOperator() const {
    return kind == PIPE || kind == PLUS || kind == MINUS;
  }

  Kind kind;
  std::string spelling;
};

struct LexResult {

  LexResult(Token token) : token(std::move(token)) {}
  LexResult(std::string msg) : msg(std::move(msg)) {}

  operator bool() const {
    return token && token->valid();
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
      case '!':
        reader.consume();
        return Token(Token::EXCLAM);
      case '|':
        reader.consume();
        return Token(Token::PIPE);
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

  void skipLine() {
    char c;
    do {
      c = reader.next();
    } while(c != '\n');
  }

  void skipWhitespace() {
    char c = reader.peek();
    while (isspace(c) || c == '\n') {
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

    char c = reader.peek();

    while (c == '-' || c == '.' || isdigit(c) || std::tolower(c) == 'e') {
      if (c == '.' || std::tolower(c) == 'e')
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

  bool hasMarkers() const {
    return reader.hasMarkers();
  }

  int getNumMarkers() const {
    return reader.getNumMarkers();
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
  for (int i = 1; i < pos; ++i) {
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

struct MarkerConsistencyChecker {
  MarkerConsistencyChecker(const Lexer& lexer, const std::string& where) : lexer(lexer), where(where) {
    countBefore = lexer.getNumMarkers();
  }
  ~MarkerConsistencyChecker() {
    if (lexer.getNumMarkers() != countBefore) {
      logWarn() << "Detected inconsistencies in function '" << where << "' - this is likely an implementation bug. The resulting AST might be incorrect.\n";
    }
  }
 private:
  const Lexer& lexer;
  std::string where;
  int countBefore;
};

class QueryParser {
  Lexer lexer;
public:
  explicit QueryParser(std::string input) : lexer(std::move(input))
  {}

  bool eof() {
    return lexer.eof();
  }

  ASTPtr parse() {
    MarkerConsistencyChecker checker(lexer, "parse");
    std::vector<NodePtr> stmts;
    do {
      auto stmt = parseStmt();
      if (stmt) {
        stmts.emplace_back(std::move(stmt));
      } else {
        printErrorMessage("Parsing failed.");
        return nullptr;
      }
      lexer.skipWhitespace();
    } while(!eof());

    return std::make_unique<QueryAST>(std::move(stmts));
  }

protected:
 PipelineRefPtr parseSelectorRef() {
   MarkerConsistencyChecker checker(lexer, "parseSelectorRef");
   auto t = lexer.next();
   if (!t) {
     printErrorMessage(t.msg);
     printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector ref");
     return {};
   }
   if (t->kind != Token::PERCENT) {
     printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector ref (starting with '%')");
     return {};
   }

   auto idToken = lexer.next();
   if (!idToken) {
     printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector identifier");
     return {};
   }
   if (idToken->kind != Token::IDENTIFIER && idToken->kind != Token::PERCENT) {
     printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector identifier", idToken->spelling);
     return {};
   }
   return std::make_unique<PipelineRef>(idToken->spelling);
 }

  NodePtr parseParam() {

    // BNF: param := string | int | float | bool | selectorRef

    MarkerConsistencyChecker checker(lexer, "parseParam");

    lexer.pushMarker();

    auto nextToken = lexer.next();
    if (!nextToken) {
      printErrorMessage(nextToken.msg);
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
    case Token::IDENTIFIER: // TODO: refs and defs are not allowed as selector parameters anymore. Change this accordingly.
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
      return std::make_unique<PipelineRef>(idToken->spelling);
    }
    default:
      break;
    }

    printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector parameter", nextToken->spelling);
    return {};
  }

  SelectorDefPtr parseSelectorDef() {

    // BNF: selectorDef := selectorType '(' params ')' | selectorType '(' ')' | <selectorType>
    //      params := param | params ',' param

    MarkerConsistencyChecker checker(lexer, "parseSelectorDef");

    auto selectorToken = lexer.next();
    if (!selectorToken) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector identifier");
      return {};
    }
    if (selectorToken->kind != Token::IDENTIFIER) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "an identifier", selectorToken->spelling);
      return {};
    }
    lexer.pushMarker();
    auto leftParen = lexer.next();
    if (!leftParen) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "'('");
      return {};
    }
    std::vector<NodePtr> params;
    if (leftParen->kind == Token::LEFT_PAREN) {
      lexer.discardMarker();
      params = parseParams();
    } else {
      // Selector with no parameters
      lexer.backtrack();
    }

    std::cout << "Parsing: " << selectorToken->spelling << "\n";

    return std::make_unique<SelectorDef>(selectorToken->spelling, std::move(params));
  }

  std::vector<NodePtr> parseParams() {

    MarkerConsistencyChecker checker(lexer, "parseParams");

    // Note: Assumes that the leading parenthesis has already been parsed.

    lexer.pushMarker();
    auto nextToken = lexer.next();
    if (!nextToken) {
      printErrorMessage(nextToken.msg);
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "parameters or ')'");
      return {};
    }
    std::vector<NodePtr> params;
    if (nextToken->kind != Token::RIGHT_PAREN) {
      // Backtrack so that the next token is the start of the first argument
      lexer.backtrack();
      do {
        auto arg = parseParam();
        if (!arg) {
          printErrorMessage("Could not parse parameter.");
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
      if (nextToken->kind != Token::RIGHT_PAREN) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "')'", nextToken->spelling);
        return {};
      }
    } else {
      lexer.discardMarker();
    }
    return params;
  }

  NodePtr parseStmt() {
    MarkerConsistencyChecker checker(lexer, "parseStmt");

    lexer.pushMarker();
    auto t = lexer.next();
    if (!t) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a directive or selector declaration");
      return {};
    }
    if (t->kind == Token::EXCLAM) {
      lexer.discardMarker();
      return parseDirective();
    } else {
      lexer.backtrack();
      return parsePipelineDecl();
    }
  }

  DirectivePtr parseDirective() {

    MarkerConsistencyChecker checker(lexer, "parseDirective");

    // BNF: <directive> ::= '!' <directiveType> | '!' <directiveType> '(' <directiveParams> ')'
    // Note: The '!' is already parsed here.

    auto name = lexer.next();
    if (!name) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a directive");
      return {};
    }

    if (name->kind != Token::IDENTIFIER) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "an identifier", name->spelling);
      return {};
    }

    lexer.pushMarker();
    auto t = lexer.next();
    if (!t) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "directive parameters or a new statement");
      return {};
    }
    std::vector<NodePtr> params {};
    if (t->kind == Token::LEFT_PAREN) {
      lexer.discardMarker();
      params = parseParams();
    } else {
      // Token is start of new statement
      lexer.backtrack();
    }

    return std::make_unique<Directive>(name->spelling, std::move(params));

  }


  std::vector<PipelineExprPtr> parsePipelineExprs() {

    MarkerConsistencyChecker checker(lexer, "parsePipelineExprs");

    // Note: Assumes that the leading parenthesis has already been parsed.

    LexResult nextToken("placeholder");
    std::vector<PipelineExprPtr> exprs;
    do {
      auto expr = parsePipelineExpr();
      if (!expr) {
        printErrorMessage("Could not parse pipeline input.");
        return {};
      }
      exprs.push_back(std::move(expr));
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

    return exprs;
  }

  PipelineExprTuplePtr parsePipelineExprTuple() {
    MarkerConsistencyChecker checker(lexer, "parsePipelineExprTuple");

    // BNF: <pipelineExprTuple> ::= <pipelineExpr> | '(' <pipelineExprs> ')'
    lexer.pushMarker();
    auto t = lexer.next();
    if (!t) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a tuple of pipeline expressions");
      return {};
    }
    std::vector<PipelineExprPtr> inputs;
    if (t->kind == Token::LEFT_PAREN) {
      std::cout << "Tuple consists of multiple exprs\n";
      lexer.discardMarker();
      inputs = parsePipelineExprs();
    } else {
      lexer.backtrack();
      inputs.push_back(parsePipelineExpr());
    }
    return std::make_unique<PipelineExprTuple>(std::move(inputs));
  }


  PipelineExprPtr parsePipelineExpr() {
    MarkerConsistencyChecker checker(lexer, "parsePipelineExpr");

    // BNF: <pipelineExpr> ::= <pipelineRef>
    //                     | <pipelineExprTuple> '|' <pipedDefs>
    //                     | <pipelineExpr> '+' <pipelineExpr>
    //                     | <pipelineExpr> '-' <pipelineExpr>

    // Find out if the expression is a single ref
    //  (1) Check if there is a leading '%'
    //  (2) If yes, go back and parse the ref
    //  (3) Check if the next token is '|'
    //  (4) If no, then this is a single ref. If yes, continue parsing the pipeline.

    std::cout << "Starting to parse expression\n";

    bool firstPipeImplied = false;
    PipelineExprTuplePtr inputTuple;
    // Saving for error message
    int pipelineStartPos = lexer.getPos();
    lexer.pushMarker();
    auto t = lexer.next();
    if (!t) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline expression");
      return {};
    }
    if (t->kind == Token::PERCENT) {
      lexer.backtrack();
      auto ref = parseSelectorRef();
      std::cout << "  Pipeline uses single input: " << ref->getIdentifier() << "\n";
      lexer.pushMarker();
      t = lexer.next();
      if (t->isOperator()) {
        std::cout << "  Next token is an operator " << t->spelling << "\n";
        // This expression is a full pipeline with selectors
        std::vector<PipelineExprPtr> inputExprs;
        inputExprs.push_back(std::make_unique<PipelineExpr>(std::move(ref)));
        inputTuple = std::make_unique<PipelineExprTuple>(std::move(inputExprs));
        lexer.backtrack();
      } else {
        // This expression is a single ref
        std::cout << "  Expression is a single ref\n";
        lexer.backtrack();
        return std::make_unique<PipelineExpr>(std::move(ref));
      }
    } else if (t->kind == Token::IDENTIFIER) {
      std::cout << "Token is identifier\n";
      // No ref given - %% is implied as input
      lexer.backtrack();
      std::vector<PipelineExprPtr> inputExprs;
      inputExprs.push_back(std::make_unique<PipelineExpr>(std::make_unique<PipelineRef>("%")));
      inputTuple = std::make_unique<PipelineExprTuple>(std::move(inputExprs));
      firstPipeImplied = true;
    } else {
        lexer.backtrack();
    }

    // Parse the input tuple if not already done
    if (!inputTuple) {
      std::cout << "  Starting to parse expr tuple\n";
      inputTuple = parsePipelineExprTuple();
      if (!inputTuple) {
        return nullptr;
      }
    }

    if (!firstPipeImplied) {
      t = lexer.next();
      if (!t) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector pipeline");
        return {};
      }
    }
    lexer.pushMarker();
    std::vector<SelectorDefPtr> defs;
    while (firstPipeImplied || t->kind == Token::PIPE) {
      firstPipeImplied = false;
      auto def = parseSelectorDef();
      if (!def) {
        printErrorMessage("Could not parse selector definition");
        return {};
      }
      defs.push_back(std::move(def));
      lexer.discardMarker();
      lexer.pushMarker();
      t = lexer.next();
    }
    lexer.backtrack();

    // Pipelines must have a single output.
    // Example: 'a = (%b, %c)' is valid syntax but not allowed.
    if (defs.empty() && inputTuple->getNumChildren() > 1) {
      logError() << "Pipeline is invalid: all pipelines must have a single output\n";
      printErrorPosition(lexer.getInput(), pipelineStartPos);
      return nullptr;
    }

    return std::make_unique<PipelineExpr>(std::move(inputTuple), std::move(defs));
  }

  PipelineDeclPtr parsePipelineDecl() {

    MarkerConsistencyChecker checker(lexer, "parsePipelineDecl");

    // BNF: selectorDecl := selectorName '=' selectorPipeline | selectorPipeline

    lexer.pushMarker();

    auto t1 = lexer.next();
    if (!t1) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline declaration");
      return {};
    }
    auto t2 = lexer.next();
    if (!t2) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "'=' or a pipeline expression");
      return {};
    }

    std::string pipelineId = "";

    if (t2->kind==Token::EQUALS) {
      // Name is in t1
      lexer.discardMarker();
      if (t1->kind != Token::IDENTIFIER) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "an identifier", t1->spelling);
      }
      pipelineId = t1->spelling;
    } else {
      // Pipeline is not named -> backtrack to start
      lexer.backtrack();
    }

    std::cout << "Starting to parse decl: " << pipelineId << "\n";

    auto pipeline = parsePipelineExpr();
    if (!pipeline) {
      printErrorMessage("Failed to parse pipeline definition.\n");
      return nullptr;
    }
    return  std::make_unique<PipelineDecl>(std::move(pipelineId), std::move(pipeline));
  }



};

}

#endif //CAPI_QUERYPARSER_H
