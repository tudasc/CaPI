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
    BOOL_LITERAL, LEFT_PAREN, RIGHT_PAREN, LEFT_BRACKET, RIGHT_BRACKET, PERCENT, EQUALS, COMMA, EXCLAM, PIPE,
    UNION_OP, INTERSECT_OP, DIFF_OP
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
      case LEFT_BRACKET:
        spelling = "[";
        break;
      case RIGHT_BRACKET:
        spelling = "]";
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
        spelling = "|>";
        break;
      case UNION_OP:
        spelling = "|";
        break;
      case INTERSECT_OP:
        spelling = "&";
        break;
      case DIFF_OP:
        spelling = "-";
        break;
      default:
        break;
    }
  }

  bool valid() const {
    return kind != UNKNOWN;
  }

  bool isSetOperator() const {
    return kind == UNION_OP || kind == INTERSECT_OP || kind == DIFF_OP;
  }

  Kind kind;
  std::string spelling;
};

inline std::optional<OperatorType> getOperator(Token t) {
  switch(t.kind) {
    case Token::UNION_OP:
      return OperatorType::UNION;
    case Token::INTERSECT_OP:
      return OperatorType::INTERSECT;
    case Token::DIFF_OP:
      return OperatorType::DIFF;
    default:
      break;
  }
  return {};
}

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

    // Try to parse '-' as number first
    if (std::isdigit(c) || (c == '-' && std::isdigit(reader.peek(1)))) {
      return parseNumber();
    }

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
      case '[':
        reader.consume();
        return Token(Token::LEFT_BRACKET);
      case ']':
        reader.consume();
        return Token(Token::RIGHT_BRACKET);
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
        c = reader.next();
        if (c == '>') {
          reader.consume();
          return Token(Token::PIPE);
        }
        return Token(Token::UNION_OP);
      case '&':
        reader.consume();
        return Token(Token::INTERSECT_OP);
      case '-':
        reader.consume();
        return Token(Token::DIFF_OP);
      default:
        break;
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

  void moveMarker() {
    reader.discardMarker();
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

struct ParserTrace {

  struct TraceEntry {
    int depth;
    std::string method;
    std::string info;
  };

  std::vector<TraceEntry> trace;

  int depth = 0;

  void push(const std::string& method) {
    trace.push_back({depth++, method});
  }

  void pop() {
    depth--;
  }

  void amend(std::string info) {
    trace.back().info += info;
  }

  void dump(std::ostream& os) {
    for (auto& entry: trace) {
      for (int i = 0; i < entry.depth; i++) {
        os << "  ";
      }
      os << entry.method;
      if (!entry.info.empty()) {
        os << ": " << entry.info;
      }
      os << "\n";
    }
  }
};

struct ParserTraceRAII {
  ParserTrace& trace;
  ParserTraceRAII(ParserTrace& trace, const std::string& method) : trace(trace) {
    trace.push(method);
  }

  ~ParserTraceRAII() {
    trace.pop();
  }
};

class QueryParser {
  Lexer lexer;
  ParserTrace trace;
public:
  explicit QueryParser(std::string input) : lexer(std::move(input))
  {}

  bool eof() {
    return lexer.eof();
  }

  ASTPtr parse() {
    MarkerConsistencyChecker checker(lexer, "parse");
    ParserTraceRAII ptr(trace, "parse");
    std::vector<NodePtr> stmts;
    do {
      auto stmt = parseStmt();
      if (stmt) {
        stmts.emplace_back(std::move(stmt));
      } else {
        printErrorMessage("Parsing failed.");
        std::cerr << "Parser trace:\n";
        std::cerr << "--------------\n";
        trace.dump(std::cerr);
        std::cerr << "--------------\n";
        return nullptr;
      }
      lexer.skipWhitespace();
    } while(!eof());

    return std::make_unique<QueryAST>(std::move(stmts));
  }

protected:
 PipelineRefPtr parsePipelineRef() {
   MarkerConsistencyChecker checker(lexer, "parsePipelineRef");
   ParserTraceRAII ptr(trace, "parsePipelineRef");
   auto t = lexer.next();
   if (!t) {
     printErrorMessage(t.msg);
     printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline ref");
     return {};
   }
   if (t->kind != Token::PERCENT) {
     printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline ref (starting with '%')");
     return {};
   }

   auto idToken = lexer.next();
   if (!idToken) {
     printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline identifier");
     return {};
   }
   if (idToken->kind != Token::IDENTIFIER && idToken->kind != Token::PERCENT) {
     printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline identifier", idToken->spelling);
     return {};
   }
   trace.amend(idToken->spelling);
   return std::make_unique<PipelineRef>(idToken->spelling);
 }

  NodePtr parseParam() {

    // BNF: param := string | int | float | bool | pipelineRef

    MarkerConsistencyChecker checker(lexer, "parseParam");
    ParserTraceRAII ptr(trace, "parseParam");

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
    ParserTraceRAII ptr(trace, "parseSelectorDef");

    auto selectorToken = lexer.next();
    if (!selectorToken) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a selector identifier");
      return {};
    }
    if (selectorToken->kind != Token::IDENTIFIER) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "an identifier", selectorToken->spelling);
      return {};
    }
    trace.amend(selectorToken->spelling);
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

    return std::make_unique<SelectorDef>(selectorToken->spelling, std::move(params));
  }

  std::vector<NodePtr> parseParams() {

    MarkerConsistencyChecker checker(lexer, "parseParams");
    ParserTraceRAII ptr(trace, "parseParams");

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
    ParserTraceRAII ptr(trace, "parseStmt");

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
    ParserTraceRAII ptr(trace, "parseDirective");

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
    trace.amend(name->spelling);

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


  std::vector<PipelineOpPtr> parsePipelineOps() {

    MarkerConsistencyChecker checker(lexer, "parsePipelineOps");
    ParserTraceRAII ptr(trace, "parsePipelineOps");

    // Note: Assumes that the leading bracket has already been parsed.

    LexResult nextToken("placeholder");
    std::vector<PipelineOpPtr> ops;
    do {
      auto op = parsePipelineOp();
      if (!op) {
        printErrorMessage("Could not parse pipeline input.");
        return {};
      }
      ops.push_back(std::move(op));
      nextToken = lexer.next();
      if (!nextToken) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "',' or ']'");
        return {};
      }
    } while (nextToken->kind == Token::COMMA);

    trace.amend(std::to_string(ops.size()) + " ops parsed");

    // Make sure that the last token was ')'
    if (nextToken->kind != nextToken->RIGHT_BRACKET) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "']'", nextToken->spelling);
      return {};
    }

    return ops;
  }

  PipelineOpPtr parsePipelineOp() {
    MarkerConsistencyChecker checker(lexer, "parsePipelineOp");
    ParserTraceRAII ptr(trace, "parsePipelineOp");

    // BNF: <pipelineOp> ::= <pipelineOp> ('|' | '&' | '-') <pipelineExpr>
    //                     | <pipelineExpr>
    // Parsed as: <pipelineOp>  ::= <pipelineExpr> <pipelineOp'>
    //            <pipelineOp'> ::= ('|' | '&' | '-') <pipelineExpr> <pipelineOp'> | eps

    PipelineExprPtr lhsExpr = parsePipelineExpr();
    if (!lhsExpr) {
      return nullptr;
    }
    lexer.pushMarker();
    auto t = lexer.next();
    if (!t) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline definition");
      return {};
    }
    PipelineOpPtr lhs = std::make_unique<PipelineOp>(std::move(lhsExpr));

    while (t->isSetOperator()) {
      auto rhs = parsePipelineExpr();
      if (!rhs) {
        return nullptr;
      }
      auto opType = getOperator(t.get());
      assert(opType && "Must be an operator");
      trace.amend("operator " + getOperatorName(opType.value()) + " ");
      lhs = std::make_unique<PipelineOp>(std::move(lhs), std::move(rhs), opType.value());
      lexer.moveMarker();
      t = lexer.next();
      logInfo() << "Next op token is: " << t->spelling << "\n";
    }
    // Last token wasn't an operator, so backtrack
    lexer.backtrack();
    return lhs;
  }

  TermPtr parseTerm() {
    MarkerConsistencyChecker checker(lexer, "parseTerm");
    ParserTraceRAII ptr(trace, "parseTerm");
    lexer.pushMarker();
    auto t = lexer.next();
    if (!t) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline term");
      return {};
    }
    if (t->kind == Token::PERCENT) {
      lexer.backtrack();
      auto ref = parsePipelineRef();
      if (!ref) {
        printErrorMessage("Could not parse pipeline ref");
        return nullptr;
      }
      trace.amend("%"+ref->getIdentifier());
      return  std::make_unique<Term>(std::move(ref));
    } else if (t->kind == Token::LEFT_PAREN) {
      lexer.discardMarker();
      trace.amend("'(' <expr> ')'");
      auto op = parsePipelineOp();
      if (!op) {
        printErrorMessage("Could not parse pipeline operation");
        return nullptr;
      }
      t = lexer.next();
      if (t->kind != Token::RIGHT_PAREN) {
        printErrorPosition("Expected closing parenthesis", lexer.getPos());
        return nullptr;
      }
      return std::make_unique<Term>(std::move(op));
    } else if (t->kind == Token::LEFT_BRACKET) {
      trace.amend("tuple");
      lexer.discardMarker();
      auto inputs = parsePipelineOps();
      if (inputs.empty()) {
        printErrorMessage("Tuple cannot be empty");
        return nullptr;
      }
      return std::make_unique<Term>(std::make_unique<ExprTuple>(std::move(inputs)));
    }
    lexer.backtrack();
    printErrorMessage("Expected a pipeline ref, an expression enclosed in parentheses or an expression tuple");
    printErrorPosition(lexer.getInput(), lexer.getPos());
    return nullptr;
  }



  PipelineExprPtr parsePipelineExpr() {
    MarkerConsistencyChecker checker(lexer, "parsePipelineExpr");
    ParserTraceRAII ptr(trace, "parsePipelineExpr");

    // BNF: <pipelineExpr>     ::= <term> | <selectorDef> | <pipelineExpr> '|>' <selectorDef>
    // Parsed as: <pipelineExpr>  ::= <term> <pipelineExpr'>
    //            <pipelineExpr'> ::= '|>' <selectorDef> <pipelineExpr'> | eps

    bool impliedInput = false;

    TermPtr lhsTerm;
    // Saving for error message
    int pipelineStartPos = lexer.getPos();
    lexer.pushMarker();
    auto t = lexer.next();
    if (!t) {
      printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline expression");
      return {};
    }

    if (t->kind == Token::IDENTIFIER) {
      trace.amend("%% is implied");
      // No ref given - %% is implied as input
      lexer.backtrack();
      auto inputRef = std::make_unique<PipelineRef>("%");
      lhsTerm = std::make_unique<Term>(std::move(inputRef));
      impliedInput = true;
    } else {
      lexer.backtrack();
      lhsTerm = parseTerm();
    }

    if (!lhsTerm) {
      printErrorMessage("Could not parse term");
      return nullptr;
    }

    // If input is not implied, parse first pipe (if there is one)
    if (!impliedInput) {
      lexer.pushMarker();
      t = lexer.next();
      if (!t) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline expression");
        return {};
      }
      if (t->kind == Token::PIPE) {
        lexer.discardMarker();
      } else {
        // Simple term without pipe operator
        lexer.backtrack();
        // Pipelines must have a single output.
        // Example: 'a = [%b, %c]' is valid syntax but not allowed.
        if (lhsTerm->isTuple()) {
          logError() << "Pipeline is invalid: all pipelines must have a single output\n";
          printErrorPosition(lexer.getInput(), pipelineStartPos);
          return nullptr;
        }
        return std::make_unique<PipelineExpr>(std::move(lhsTerm));
      }
    }


    PipelineExprPtr pipelineExpr = std::make_unique<PipelineExpr>(std::move(lhsTerm));
    lexer.pushMarker();
    do {
      auto selDef = parseSelectorDef();
      if (!selDef) {
        printErrorMessage("Could not parse selector");
        return nullptr;
      }
      pipelineExpr = std::make_unique<PipelineExpr>(std::move(pipelineExpr), std::move(selDef));
      lexer.moveMarker();
      t = lexer.next();
      if (!t) {
        printErrorMessageExpected(lexer.getPos(), lexer.getInput(), "a pipeline expression");
        return {};
      }
    } while(t->kind == Token::PIPE);
    // The last token was not a pipe operator -> backtrack
    lexer.backtrack();
    return pipelineExpr;
  }


  PipelineDeclPtr parsePipelineDecl() {

    MarkerConsistencyChecker checker(lexer, "parsePipelineDecl");
    ParserTraceRAII ptr(trace, "parsePipelineDecl");

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

    trace.amend(pipelineId);

    auto pipeline = parsePipelineOp();
    if (!pipeline) {
      printErrorMessage("Failed to parse pipeline definition.\n");
      return nullptr;
    }
    return  std::make_unique<PipelineDecl>(std::move(pipelineId), std::move(pipeline));
  }



};

}

#endif //CAPI_QUERYPARSER_H
