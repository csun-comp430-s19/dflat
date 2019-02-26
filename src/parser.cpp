#include "parser.hpp"
#include <iostream>

namespace dflat
{

using namespace std;

#define TRACE //std::cout << __func__ << "\n";
#define SUCCESS //std::cout << "SUCCESS: " << __func__ << "\n";
#define FAILURE //std::cout << "FAILURE: " << __func__ << "\n";
#define ENABLE_ROLLBACK auto rollbacker = Rollbacker(*this, _tokenPos)
#define CANCEL_ROLLBACK rollbacker.disable()

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
//  On failure, the present function returns early with nullptr.
#define MATCH(var, type) \
    type const* var##__ = match<type>(); \
    if (!var##__) { FAILURE; return nullptr; } \
    type const& var = *var##__ \
    /*end MATCH*/

// Matches the current token but doesn't store it in a var.
#define MATCH_(type) \
    if (!match<type>()) { FAILURE; return nullptr; } \
    /*end MATCH_*/

// Calls <parser> and stores the result in <var>.
//  On success, var holds the result.
//  On failure, the present function returns early with nullptr.
#define PARSE(var, parser) \
    auto var = parser; \
    if (!var) { FAILURE; return nullptr; } \
    /*end PARSE*/

OpType Parser::parseUnaryOp()
{
    TRACE;
    return advancing([this]()
    {
        switch (cur()->getType())
        {
            case tokNot:    SUCCESS; return opNot;
            case tokMinus:  SUCCESS; return opMinus;
            default:        FAILURE; return opNull;
        }
    });
}

OpType Parser::parseMultiveOp()
{
    TRACE;
    return advancing([this]()
    {
        switch (cur()->getType())
        {
            case tokMult:   SUCCESS; return opMult;
            case tokDiv:    SUCCESS; return opDiv;
            default:        FAILURE; return opNull;
        }
    });
}

OpType Parser::parseAdditiveOp()
{
    TRACE;
    return advancing([this]()
    {
        switch (cur()->getType())
        {
            case tokPlus:   SUCCESS; return opPlus;
            case tokMinus:  SUCCESS; return opMinus;
            default:        FAILURE; return opNull;
        }
    });
}

OpType Parser::parseLogicalOp()
{
    TRACE;
    return advancing([this]()
    {
        switch (cur()->getType())
        {
            case tokAnd:    return opAnd;
            case tokOr:     return opOr;
            case tokEq:     return opLogEq;
            case tokNotEq:  return opLogNotEq;
            default:        return opNull;
        }
    });
}

// EXPRESSION PARSERS

ASNPtr Parser::parseVariable()
{
    TRACE;
    MATCH(var, VariableToken);
    SUCCESS;
    return make_unique<VariableExp>(var.name);
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
    PARSE(prim, parsePrimary());

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<UnopExp>(move(prim), op);
}

ASNPtr Parser::parseMethodCall()
{
    TRACE;
    FAILURE;
    return nullptr; //TODO
}

ASNPtr Parser::parseNew()
{
    TRACE;
    // new + type + ( + exp + exp* + )
    FAILURE;
    return nullptr; //TODO
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
    MATCH_(RightParenToken);
    
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
    else if (result = parseMethodCall())
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
    PARSE(right, parseMultiveOrPrimary());
    
    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<BinopExp>(move(left), op, move(right));
}

ASNPtr Parser::parseMultiveOrPrimary()
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

    PARSE(left, parseMultiveOrPrimary());
    PARSE(op, parseAdditiveOp());
    PARSE(right, parseAdditiveOrPrimary());

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<BinopExp>(move(left), op, move(right));
}

ASNPtr Parser::parseAdditiveOrPrimary()
{
    TRACE;
    ASNPtr result;

    if (result = parseAdditive())
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

ASNPtr Parser::parseLogical()
{
    TRACE;
    ENABLE_ROLLBACK;

    PARSE(left, parseAdditiveOrPrimary());
    PARSE(op, parseLogicalOp());
    PARSE(right, parseLogicalOrPrimary());
    
    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<BinopExp>(move(left), op, move(right));
}

ASNPtr Parser::parseLogicalOrPrimary()
{
    TRACE;
    ASNPtr result;

    if (result = parseLogical())
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

ASNPtr Parser::parseExp()
{
    TRACE;
    return parseLogicalOrPrimary();
}

// STATEMENT PARSERS

ASNPtr Parser::parseVarDecl()
{
    TRACE;
    FAILURE;
    return nullptr; //TODO
}

ASNPtr Parser::parseAssignStmt()
{
    TRACE;
    FAILURE;
    return nullptr; //TODO
}

ASNPtr Parser::parseMemberAssignStmt()
{
    TRACE;
    FAILURE;
    return nullptr; //TODO
}

ASNPtr Parser::parseIfStmt()
{
    TRACE;
    //  incomplete
    //  only one statement accepted in each brace
    /*
    if(exp)
    {
        exp
    }
    else
    {
        exp
    }
    */
    
    ENABLE_ROLLBACK;
    MATCH_(IfToken);
    MATCH_(LeftParenToken);
    PARSE(logicExp, parseExp());
    MATCH_(RightParenToken);
    MATCH_(LeftBraceToken);
    PARSE(trueStatements, parseBlock());
    MATCH_(RightBraceToken);
    
    if( match<ElseToken>() )
    {
        MATCH_(LeftBraceToken);
        PARSE(falseStatements, parseBlock());
        MATCH_(RightBraceToken);

        CANCEL_ROLLBACK;
    SUCCESS;
        return make_unique<IfBlock>(
            move(logicExp), 
            move(trueStatements), 
            move(falseStatements)
            );
    }

    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<IfBlock>(
        move(logicExp), 
        move(trueStatements), 
        make_unique<Block>(Vector<ASNPtr>{})
        );
}

ASNPtr Parser::parseWhileStmt()
{
    TRACE;
    ENABLE_ROLLBACK;

    MATCH_(WhileToken);
    MATCH_(LeftParenToken);
    PARSE(cond, parseExp());
    MATCH_(RightParenToken);
    PARSE(body, parseBlock());
    
    CANCEL_ROLLBACK;
    SUCCESS;
    return make_unique<WhileBlock>(move(cond), move(body));
}

ASNPtr Parser::parseForStmt()
{
    TRACE;
    FAILURE;
    return nullptr; //TODO
}

ASNPtr Parser::parseStmt()
{
    TRACE;
    ASNPtr result;

    if (result = parseVarDecl())
    {
    SUCCESS;
        return result;
    }
    else if (result = parseAssignStmt())
    {
    SUCCESS;
        return result;
    }
    else if (result = parseMemberAssignStmt())
    {
    SUCCESS;
        return result;
    }
    else if (result = parseIfStmt())
    {
    SUCCESS;
        return result;
    }
    else if (result = parseWhileStmt())
    {
    SUCCESS;
        return result;
    }
    else if (result = parseForStmt())
    {
    SUCCESS;
        return result;
    }
    else if (result = parseExp())
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

// COMPOUND PARSERS

ASNPtr Parser::parseBlock()
{
    TRACE;
    FAILURE;
    return nullptr; //TODO
}

ASNPtr Parser::parseProgram()
{
    TRACE;
    FAILURE;
    return nullptr; // TODO
}

Parser::Parser(Vector<TokenPtr> const& tokens)
    : _tokens(tokens)
    , _tokenPos(0)
    , _end(make_unique<EndToken>())
{}

} //namespace dflat
