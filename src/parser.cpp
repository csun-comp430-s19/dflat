#include "parser.hpp"
#include "config.hpp"
#include <iostream>

namespace dflat
{

using namespace std;

#define TRACE _tracer.push(__func__ + String(" ") + cur()->toString() + " (" + to_string(_tokenPos) + ")")
#define SUCCESS _tracer.pop(traceSuccess)
#define FAILURE _tracer.pop(traceFailure)

#define ENABLE_ROLLBACK auto rollbacker = Rollbacker(*this, _tokenPos)
#define CANCEL_ROLLBACK rollbacker.disable()


Vector<ASNPtr> parse(Vector<TokenPtr> const& tokens)
{
    Parser p(tokens);
    return p.parseProgram();
}

ParserException::ParserException(String msg) noexcept
{
    message = "Parser Exception:\n" + msg;
}

const char* ParserException::what() const noexcept
{
    return message.c_str();
}

/**
 * @brief Returns the current token or end of program token.
 * @return TokenPtr
 */
TokenPtr const& Parser::cur() const
{
    if (_tokenPos >= _tokens.size())
    {
        return _end;
    }

    return _tokens[_tokenPos];
}

/**
 * @brief Advances the reader if not at the end of the program.
 */
void Parser::next()
{
    if (_tokenPos >= _tokens.size())
    {
        return;
    }

    ++_tokenPos;
}

// Matches the current token against a given token type.
//  On success, var is a reference to the current token,
//              and the current token advances.
//  On failure, the present function returns early with nullptr:
#define MATCH(var, type) \
    type const* var##__ = match<type>(); \
    if (!var##__) { FAILURE; return nullptr; } \
    type const& var = *var##__ \
    /*end MATCH*/

//Same as MATCH, but instead of returning a nullptr, throws an exception
//expected is a string saying what should be matched:
#define MUST_MATCH(var, type, expected) \
    type const* var##__ = match<type>(); \
    if (!var##__) { FAILURE; \
        throw ParserException("Expected " + String(expected) \
            + " at position: " + to_string(_tokenPos)); } \
    type const& var = *var##__ \
    /*end MUST_MATCH*/

// Matches the current token but doesn't store it in a var:
#define MATCH_(type) \
    if (!match<type>()) { FAILURE; return nullptr; } \
    /*end MATCH_*/

//Same as MATCH_, but instead of returning a nullptr, throws an exception:
#define MUST_MATCH_(type) \
    if (!match<type>()) { FAILURE; \
    throw ParserException("Expected '" + type().toString() \
        + "' at position: " + to_string(_tokenPos)\
        + "\nPossible invalid expression or statement."); } \
    /*end MUST_MATCH_*/

ASNPtr deref(ASNPtr& p)
{
    return move(p);
}

template <typename T>
decltype(auto) deref(T& t)
{
    return *t;
}

// Calls <parser> and stores the result in <var>.
//  On success, var holds the result.
//  On failure, the present function returns early with nullptr:
#define PARSE(var, parser) \
    auto var##__ = parser; \
    if (!var##__) { FAILURE; return nullptr; } \
    decltype(auto) var = deref(var##__)
    /*end PARSE*/

// Same as PARSE, but throws a specified exception message if it fails:
#define MUST_PARSE(var, parser, exceptionMsg) \
    auto var##__ = parser; \
    if (!var##__) { FAILURE; \
    throw ParserException( String(exceptionMsg) \
        + " at position: " + to_string(_tokenPos)); } \
    decltype(auto) var = deref(var##__)
    /*end MUST_PARSE*/

// Parses a NameToken as a string.
Optional<String> Parser::parseName()
{
    TRACE;
    if (NameToken const* tok = match<NameToken>())
    {
        SUCCESS;
        return tok->name;
    }
    else
    {
        FAILURE;
        return failure;
    }
}

Optional<OpType> Parser::parseUnaryOp()
{
    TRACE;
    switch (cur()->getType())
    {
        case tokNot:    SUCCESS; next(); return opNot;
        case tokMinus:  SUCCESS; next(); return opMinus;
        default:        FAILURE; return failure;
    }
}

Optional<OpType> Parser::parseMultiveOp()
{
    TRACE;
    switch (cur()->getType())
    {
        case tokMult:   SUCCESS; next(); return opMult;
        case tokDiv:    SUCCESS; next(); return opDiv;
        default:        FAILURE; return failure;
    }
}

Optional<OpType> Parser::parseAdditiveOp()
{
    TRACE;
    switch (cur()->getType())
    {
        case tokPlus:   SUCCESS; next(); return opPlus;
        case tokMinus:  SUCCESS; next(); return opMinus;
        default:        FAILURE; return failure;
    }
}

Optional<OpType> Parser::parseLogicalOp()
{
    TRACE;
    switch (cur()->getType())
    {
        case tokAnd:    SUCCESS; next(); return opAnd;
        case tokOr:     SUCCESS; next(); return opOr;
        case tokEq:     SUCCESS; next(); return opLogEq;
        case tokNotEq:  SUCCESS; next(); return opLogNotEq;
        default:        FAILURE; return failure;
    }
}

// EXPRESSION PARSERS

ASNPtr Parser::parseVariable()
{
    TRACE;
    PARSE(var, parseName());
    SUCCESS;
    return make_unique<VariableExp>(var);
}

ASNPtr Parser::parseTypeVariable()
{
    TRACE;
    ENABLE_ROLLBACK;
    PARSE(type, parseName());
    PARSE(var, parseName());
    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<TypeVariableExp>(type, var);
}

ASNPtr Parser::parseNumber()
{
    TRACE;
    MATCH(num, NumberToken);
    SUCCESS;
    return make_unique<NumberExp>(num.num);
}

ASNPtr Parser::parseUnary()
{
    TRACE;
    // op prim
    ENABLE_ROLLBACK;

    PARSE(op, parseUnaryOp());
    MUST_PARSE(prim, parsePrimary(), "Expected expression after unary operator");

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<UnopExp>(move(prim), op);
}

ASNPtr Parser::parseMethodExp()
{
    // functionName + ( + exp + (, + exp)* + )
    TRACE;
    ENABLE_ROLLBACK;

    Vector<ASNPtr> exps;
    ASNPtr temp;
    String object, method;

    // "this." can be omitted.
    PARSE(name1, parseName());

    if (match<MemberToken>())
    {
        object = name1;
        PARSE(name2, parseName());
        method = name2;
    }
    else
    {
        object = "this";
        method = name1;
    }

    MATCH_(LeftParenToken);
    temp = parseExp();
    if(temp)
    {
        exps.push_back(move(temp));
        while(match<CommaToken>())
        {
            MUST_PARSE(temp1, parseExp(), "Expected expression after ','");
            exps.push_back(move(temp1));
        }
    }
    MUST_MATCH_(RightParenToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<MethodExp>(object, method, move(exps));
}

ASNPtr Parser::parseNew()
{
    // new + type + ( + exp + (, + exp)* + )
    TRACE;
    ENABLE_ROLLBACK;

    Vector<ASNPtr> exps;
    ASNPtr temp;

    MATCH_(NewToken);
    MUST_PARSE(var, parseName(), "type declaration for new");
    MATCH_(LeftParenToken);
    temp = parseExp();
    if(temp)
    {
        exps.push_back(move(temp));
        while(match<CommaToken>())
        {
            MUST_PARSE(temp1, parseExp(), "Expected expression after ','");
            exps.push_back(move(temp1));
        }
    }
    MUST_MATCH_(RightParenToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<NewExp>(var, move(exps));
}

/**
 * @brief parseParensExp | Tries to parse everything inside of parenthesis.
 * @return ASNPtr
 */
ASNPtr Parser::parseParensExp()
{
    TRACE;
    ENABLE_ROLLBACK;

    MATCH_(LeftParenToken);
    PARSE(exp, parseExp());
    MUST_MATCH_(RightParenToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return exp;
}

ASNPtr Parser::parsePrimary()
{
    TRACE;
    ASNPtr result;

    if (result = parseNumber())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseParensExp())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseUnary())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseMethodExp())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseNew())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseVariable())
    {
        SUCCESS;
        return result;
    }
    else
    {
        FAILURE;
        return nullptr;
    }
}

ASNPtr Parser::parseMultive()
{
    TRACE;
    ENABLE_ROLLBACK;

    PARSE(left, parsePrimary());
    PARSE(op, parseMultiveOp());
    MUST_PARSE(right, parseMultiveDown(), "Expected expression after multive operator");

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<BinopExp>(move(left), op, move(right));
}

ASNPtr Parser::parseMultiveDown()
{
    TRACE;
    ASNPtr result;

    if (result = parseMultive())
    {
        SUCCESS;
        return result;
    }
    else if (result = parsePrimary())
    {
        SUCCESS;
        return result;
    }
    else
    {
        FAILURE;
        return nullptr;
    }
}

ASNPtr Parser::parseAdditive()
{
    TRACE;
    ENABLE_ROLLBACK;

    PARSE(left, parseMultiveDown());
    PARSE(op, parseAdditiveOp());
    MUST_PARSE(right, parseAdditiveDown(), "Expected expression after additive operator");

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<BinopExp>(move(left), op, move(right));
}

ASNPtr Parser::parseAdditiveDown()
{
    TRACE;
    ASNPtr result;

    if (result = parseAdditive())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseMultiveDown())
    {
        SUCCESS;
        return result;
    }
    else
    {
        FAILURE;
        return nullptr;
    }
}

ASNPtr Parser::parseLogical()
{
    TRACE;
    ENABLE_ROLLBACK;

    PARSE(left, parseAdditiveDown());
    PARSE(op, parseLogicalOp());
    MUST_PARSE(right, parseLogicalDown(), "Expected expression after logical operator");

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<BinopExp>(move(left), op, move(right));
}

ASNPtr Parser::parseLogicalDown()
{
    TRACE;
    ASNPtr result;

    if (result = parseLogical())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseAdditiveDown())
    {
        SUCCESS;
        return result;
    }
    else
    {
        FAILURE;
        return nullptr;
    }
}

ASNPtr Parser::parseExp()
{
    TRACE;
    ASNPtr result;

    if (result = parseLogicalDown())
    {
        SUCCESS;
        return result;
    }
    else
    {
        FAILURE;
        return nullptr;
    }
}

// STATEMENT PARSERS

ASNPtr Parser::parseVarDecl()
{
    TRACE;
    ENABLE_ROLLBACK;

    PARSE(varType, parseName());
    PARSE(varName, parseName());
    MATCH_(AssignToken);
    MUST_PARSE(exp, parseExp(), "Expected expression in assignment");
    MUST_MATCH_(SemiToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<VarDecStm>(varType, varName, move(exp));
}

ASNPtr Parser::parseAssignStm()
{
    TRACE;
    ENABLE_ROLLBACK;

    PARSE(varName, parseName());
    MATCH_(AssignToken);
    MUST_PARSE(exp, parseExp(), "Expected expression in assignment");
    MUST_MATCH_(SemiToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<AssignStm>(varName, move(exp));
}

ASNPtr Parser::parseMethodStm()
{
    // functionName + ( + exp + (, + exp)* + ) + ;
    TRACE;
    ENABLE_ROLLBACK;

    PARSE(exp, parseMethodExp());
    MATCH_(SemiToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<MethodStm>(move(exp));
}

ASNPtr Parser::parseMemberAssignStm()
{
    TRACE;
    ENABLE_ROLLBACK;

    PARSE(object, parseName());
    MATCH_(MemberToken);
    PARSE(member, parseName());
    MATCH_(AssignToken);
    PARSE(value, parseExp());
    MATCH_(SemiToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<MemberAssignStm>(object, member, move(value));
}

ASNPtr Parser::parseIfStm()
{
    TRACE;

    ENABLE_ROLLBACK;
    MATCH_(IfToken);
    MUST_MATCH_(LeftParenToken);
    MUST_PARSE(logicExp, parseExp(), "Expected expression in if statement");
    MUST_MATCH_(RightParenToken);
    MUST_PARSE(trueStatements, parseBlock(), "Expected block{} after if statement");
    ASNPtr elseBlock;
    if( match<ElseToken>() )
    {
        MUST_PARSE(falseStatements, parseBlock(), "Expected block{} after else statement");
        elseBlock = move(falseStatements);
    }
    else
    {
        elseBlock = make_unique<Block>(Vector<ASNPtr>{});
    }

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<IfStm>(
        move(logicExp),
        move(trueStatements),
        move(elseBlock)
        );
}

ASNPtr Parser::parseWhileStm()
{
    TRACE;
    ENABLE_ROLLBACK;

    MATCH_(WhileToken);
    MUST_MATCH_(LeftParenToken);
    MUST_PARSE(cond, parseExp(), "Expected expression in while statement");
    MUST_MATCH_(RightParenToken);
    MUST_PARSE(body, parseBlock(), "Expected block{} after while statement");

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<WhileStm>(move(cond), move(body));
}

ASNPtr Parser::parseStm()
{
    TRACE;
    ASNPtr result;

    if (result = parseVarDecl())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseAssignStm())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseMemberAssignStm())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseMethodStm())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseIfStm())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseWhileStm())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseMethodDecl())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseRetStm())
    {
        SUCCESS;
        return result;
    }
//    else if (result = parseExp())     //only for testing
//    {
//        SUCCESS;
//        return result;
//    }
    else
    {
        FAILURE;
        return nullptr;
    }
}

// COMPOUND PARSERS

ASNPtr Parser::parseBlock()
{
    TRACE;
    ENABLE_ROLLBACK;

    Vector<ASNPtr> stm;
    ASNPtr curstm = nullptr;

    MATCH_(LeftBraceToken);

    while((curstm = parseStm()))
    {
        stm.push_back(move(curstm));
    }

    MUST_MATCH_(RightBraceToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<Block>(move(stm));
}

ASNPtr Parser::parseMethodDecl()
{
    /*
    type + name + ( + exp + (, + exp)* + ) + ;
    */
    TRACE;
    ENABLE_ROLLBACK;

    Vector<ASNPtr> exps;
    ASNPtr temp = nullptr;

    PARSE(typeName, parseName());
    PARSE(functionName, parseName());
    MATCH_(LeftParenToken)
    temp = parseTypeVariable();
    if(temp)
    {
        exps.push_back(move(temp));
        while(match<CommaToken>())
        {
            MUST_PARSE(temp1, parseTypeVariable(), "Expected type variable after ','");
            exps.push_back(move(temp1));
        }
    }
    MUST_MATCH_(RightParenToken);
    MUST_PARSE(body, parseBlock(), "Expected block{} after while statement");

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<MethodDef>(typeName, functionName,
                                  move(exps), move(body));
}

ASNPtr Parser::parseClassDecl()
{
    //distinguish variable from methods
    /*
    class name {
        block
        block
    }\n
    */
    TRACE;
    ENABLE_ROLLBACK;

    Vector<ASNPtr> stm;
    ASNPtr curstm = nullptr;

    MATCH_(ClassToken);
    MUST_PARSE(className, parseName(), "class name");
    MUST_MATCH_(LeftBraceToken)

    while(curstm = parseClassStm())
    {
        stm.push_back(move(curstm));
    }
    MUST_MATCH_(RightBraceToken);
    MUST_MATCH_(SemiToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<ClassDecl>(className, move(stm));
}

ASNPtr Parser::parseClassStm()
{
    TRACE;
    ASNPtr result;

    if (result = parseVarDecl())
    {
        SUCCESS;
        return result;
    }
    else if (result = parseMethodDecl())
    {
        SUCCESS;
        return result;
    }
    else
    {
        FAILURE;
        return nullptr;
    }
}

ASNPtr Parser::parseRetStm()
{
    TRACE;
    ENABLE_ROLLBACK;

    MATCH_(ReturnToken);
    MUST_PARSE(exp, parseExp(), "Expected expression for return statement");
    MUST_MATCH_(SemiToken);

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<RetStm>(move(exp));
}

Vector<ASNPtr> Parser::parseProgram()
{
    TRACE;
    Vector<ASNPtr> prog;
    bool run = true;
    ASNPtr result;

    while(run)
    {
        if(result = parseClassDecl())
        {
            prog.push_back(move(result));
        }
        else
        {
            run = false;
        }
    }

    if(!match<EndToken>())
    {
        FAILURE;
        String msg = "Unable to parse at position: " + to_string(_tokenPos)
                + "\nUnexpected: " + _tokens[_tokenPos]->toString();
        throw ParserException(msg);
    }

    return prog;
}

Parser::Parser(Vector<TokenPtr> const& tokens)
    : _tokens(tokens)
    , _tokenPos(0)
    , _end(make_unique<EndToken>())
    , _tracer("Parser(" + to_string(tokens) + ")", config::traceIndent)
{
}

Parser::~Parser()
{
    if (config::trace)
    {
        std::cout << "\n";
        _tracer.finalize();
        _tracer.print();
    }
}

} //namespace dflat
