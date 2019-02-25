#ifndef TOKEN_HPP
#define TOKEN_HPP

#include "string.hpp"
#include <memory>

namespace dflat
{
    enum TokType { tokNum, tokVar, tokIf, tokElse, tokPlus, tokMinus,
                    tokDiv, tokAssign, tokMult, tokRBrace, tokLBrace,
                    tokLParen, tokRParen, tokNewLine, tokFor, tokWhile,
                    tokAnd, tokOr, tokEq, tokNotEq, tokNot };

    class Token
    {
    public:
        virtual ~Token();
        virtual TokType getType() const = 0;
        virtual String toString() const = 0;

        template <typename T>
        T* as()
        {
            return dynamic_cast<T*>(this);
        }
        
        template <typename T>
        T const* as() const
        {
            return dynamic_cast<T const*>(this);
        }
    };

    using TokenPtr = std::unique_ptr<Token>;


    class NumberToken : public Token
    {
    public:
        int num;
        NumberToken(int);
        TokType getType() const { return tokNum; }
        String toString() const { return to_string(num); }
    };

    class VariableToken : public Token
    {
    public:
        String name;
        VariableToken(String const&);
        TokType getType() const { return tokVar; }
        String toString() const { return name; }
    };

    class IfToken : public Token
    {
    public:
        TokType getType() const { return tokIf; }
        String toString() const { return "if"; }
    };

    class ElseToken : public Token
    {
    public:
        TokType getType() const { return tokElse; }
        String toString() const { return "else"; }
    };

    class PlusToken : public Token
    {
    public:
        TokType getType() const { return tokPlus; }
        String toString() const { return "+"; }
    };

    class MinusToken : public Token
    {
    public:
        TokType getType() const { return tokMinus; }
        String toString() const { return "-"; }
    };

    class MultiplyToken : public Token
    {
    public:
        TokType getType() const { return tokMult; }
        String toString() const { return "*"; }
    };

    class DivisionToken : public Token
    {
    public:
        TokType getType() const { return tokDiv; }
        String toString() const { return "/"; }
    };

    class AssignToken : public Token
    {
    public:
        TokType getType() const { return tokAssign; }
        String toString() const { return "="; }
    };

    class LeftBraceToken : public Token
    {
    public:
        TokType getType() const { return tokLBrace; }
        String toString() const { return "{"; }
    };

    class RightBraceToken : public Token
    {
    public:
        TokType getType() const { return tokRBrace; }
        String toString() const { return "}"; }
    };

    class LeftParenToken : public Token
    {
    public:
        TokType getType() const { return tokLParen; }
        String toString() const { return "("; }
    };

    class RightParenToken : public Token
    {
    public:
        TokType getType() const { return tokRParen; }
        String toString() const { return ")"; }
    };

    class NewlineToken : public Token
    {
    public:
        TokType getType() const { return tokNewLine; }
        String toString() const { return "\\n"; }
    };

    class ForToken : public Token
    {
    public:
        TokType getType() const { return tokFor; }
        String toString() const { return "for"; }
    };

    class WhileToken : public Token
    {
    public:
        TokType getType() const { return tokWhile; }
        String toString() const { return "while"; }
    };

    class AndToken : public Token
    {
    public:
        TokType getType() const { return tokAnd; }
        String toString() const { return "&&"; }
    };

	class OrToken : public Token
	{
	public:
        TokType getType() const { return tokOr; }
        String toString() const { return "||"; }
	};
	
    class EqToken : public Token
	{
	public:
        TokType getType() const { return tokEq; }
        String toString() const { return "=="; }
	};
	
    class NotEqToken : public Token
	{
	public:
        TokType getType() const { return tokNotEq; }
        String toString() const { return "!="; }
	};

    class NotToken : public Token //!
    {
    public:
        TokType getType() const { return tokNot; }
        String toString() const { return "!"; }
    };

} //namespace dflat

#endif // TOKEN_HPP
