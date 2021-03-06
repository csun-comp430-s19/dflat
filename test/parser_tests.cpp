//Unit tests for the parser

#include "catch2/catch.hpp"
#include "parser.hpp"
#include "token_helpers.hpp"
#include <iostream>

using namespace dflat;

template <typename T>
T passPrint(T&& asn)
{
    if (asn) 
    {
        asn->toString();
    }
    
    return move(asn);
}

// Needed for nice output.
namespace Catch 
{
    template<>
    struct StringMaker<ASNPtr> 
    {
        static 
        String convert(ASNPtr const& value) 
        {
            return value->toString();
        }
    };
    
    template<>
    struct StringMaker<BlockPtr> 
    {
        static 
        String convert(BlockPtr const& value) 
        {
            return value->toString();
        }
    };
}

BlockPtr emptyBlock()
{
    return std::make_unique<Block>();
}

//Parser( tokens(NumberToken(1), PlusToken(), NumberToken(1)) ).parseAdditive()
#define PT(method, ...) passPrint(Parser(tokens(__VA_ARGS__)).method())

TEST_CASE( "Parser works correctly", "[parser]" )
{
    // Please forgive insane formatting :)
    // == denotes the comparison between input and expected output
    // PT[test function, token input...] == ~[resulting exp, block, or stm]


    /*
     * Tests for success on individual functions:
     */

    REQUIRE( PT(parseNumber, //6 -> NumberExp
        NumberToken(6)
        )
        ==
        ~NumberExp(6)
        );

    REQUIRE( PT(parseBoolTrue, //true -> BoolExp
        TrueToken()
        )
        ==
        ~BoolExp(true)
        );

    REQUIRE( PT(parseBoolFalse, //true -> BoolExp
        FalseToken()
        )
        ==
        ~BoolExp(false)
        );

    REQUIRE( PT(parseExp, //same as above but using parseExp
        NumberToken(6)
        )
        ==
        ~NumberExp(6)
        );

    REQUIRE (PT(parseVariableExp, // "this" as a single token
        ThisToken()
        )
        ==
        ~VariableExp("this")
        );
    
    REQUIRE (PT(parseVariableExp, // this.x -> VariableExp
        ThisToken(),
        MemberToken(),
        NameToken("x")
        )
        ==
        ~VariableExp("this", "x")
        );

    REQUIRE( PT(parseVariableExp, //fun -> VariableExp
        NameToken("fun")
        )
        ==
        ~VariableExp("fun")
        );

    REQUIRE( PT(parseExp,      //same as above but using parseExp
        NameToken("fun")
        )
        ==
        ~VariableExp("fun")
        );
    
    REQUIRE( PT(parseAdditive,  //1 + 1 -> BinopExp(additive)
        NumberToken(1),
        PlusToken(),
        NumberToken(1)
        )
        ==
        ~BinopExp(
            ~NumberExp(1),
            opPlus,
            ~NumberExp(1)
            )
        );

    REQUIRE( PT(parseAdditive, //2 - 5 -> BinopExp(additive)
        NumberToken(2),
        MinusToken(),
        NumberToken(5)
        )
        ==
        ~BinopExp(
            ~NumberExp(2),
            opMinus,
            ~NumberExp(5)
            )
        );

    REQUIRE( PT(parseExp, //same as above but using parseExp
        NumberToken(2),
        MinusToken(),
        NumberToken(5)
        )
        ==
        ~BinopExp(
            ~NumberExp(2),
            opMinus,
            ~NumberExp(5)
            )
        );

    REQUIRE( PT(parseExp, // (2 - 5)  -> BinopExp(additive) from parentheses
        LeftParenToken(),
        NumberToken(2),
        MinusToken(),
        NumberToken(5),
        RightParenToken()
        )
        ==
        ~BinopExp(
            ~NumberExp(2),
            opMinus,
            ~NumberExp(5)
            )
        );

    REQUIRE( PT(parseMultive,     //2 * 3 -> BinopExp(multive)
                NumberToken(2),
                MultiplyToken(),
                NumberToken(3)
                )
             ==
             ~BinopExp(
                ~NumberExp(2),
                opMult,
                ~NumberExp(3)
                 )
             );

    REQUIRE( PT(parseMultive,     //10 / 5 -> BinopExp(multive)
                NumberToken(10),
                DivisionToken(),
                NumberToken(5)
                )
             ==
             ~BinopExp(
                ~NumberExp(10),
                opDiv,
                ~NumberExp(5)
                 )
             );

    REQUIRE( PT(parseExp,         //same as above but using parseExp
                NumberToken(10),
                DivisionToken(),
                NumberToken(5)
                )
             ==
             ~BinopExp(
                ~NumberExp(10),
                opDiv,
                ~NumberExp(5)
                 )
             );

    REQUIRE( PT(parseLogical,         //foo && bar -> BinopExp(logical)
                NameToken("foo"),
                AndToken(),
                NameToken("bar")
                )
             ==
             ~BinopExp(
                ~VariableExp("foo"),
                opAnd,
                ~VariableExp("bar")
                 )
             );

    REQUIRE( PT(parseLogical,         //foo || bar -> BinopExp(logical)
                NameToken("foo"),
                OrToken(),
                NameToken("bar")
                )
             ==
             ~BinopExp(
                ~VariableExp("foo"),
                opOr,
                ~VariableExp("bar")
                 )
             );

    REQUIRE( PT(parseLogical,         //same as above but using parseExp
                NameToken("foo"),
                OrToken(),
                NameToken("bar")
                )
             ==
             ~BinopExp(
                ~VariableExp("foo"),
                opOr,
                ~VariableExp("bar")
                 )
             );

    REQUIRE( PT(parseUnary,    //-1 -> UnaryExp
                MinusToken(),
                NumberToken(1)
                )
             ==
             ~UnopExp(~NumberExp(1), opMinus)
            );

    REQUIRE( PT(parseUnary,    //!var -> UnaryExp
                NotToken(),
                NameToken("var")
                )
             ==
             ~UnopExp(~VariableExp("var"), opNot)
            );

    REQUIRE( PT(parseExp,    //same as above but using parseExp
                NotToken(),
                NameToken("var")
                )
             ==
             ~UnopExp(~VariableExp("var"), opNot)
            );    

    REQUIRE( PT(parseExp,  //BinopExp(additive) with nested BinopExp(multive)
        NumberToken(1),    // 1 + 1 * 4
        PlusToken(),
        NumberToken(1),
        MultiplyToken(),
        NumberToken(4)
        )
        ==
        ~BinopExp(
            ~NumberExp(1),
            opPlus,
            ~BinopExp(
                ~NumberExp(1),
                opMult,
                ~NumberExp(4)
                )
            )
        );

    REQUIRE( PT(parseExp,  //BinopExp(additive) with nested BinopExp(multive)
        NumberToken(1),    // 1 * 1 + 4    ->   (1 * 1) + 4
        MultiplyToken(),
        NumberToken(1),
        PlusToken(),
        NumberToken(4)
        )
        ==
        ~BinopExp(
            ~BinopExp(~NumberExp(1),opMult,~NumberExp(1)),
            opPlus,
            ~NumberExp(4)
            )
        );

    REQUIRE( PT(parseExp,  //BinopExp(additive) with nested UnaryExp
        MinusToken(),      // -1 * 1    ->   (-1) * 1
        NumberToken(1),
        MultiplyToken(),
        NumberToken(1)
        )
        ==
        ~BinopExp(
            ~UnopExp(~NumberExp(1),opMinus),
            opMult,
            ~NumberExp(1)
            )
        );

    REQUIRE( PT(parseBlock,        // { }   ->   empty block
                LeftBraceToken(),
                RightBraceToken()
                )
             ==
             emptyBlock()
             );

    REQUIRE( PT(parseIfStm,         // If (1) { }  ->  IfStm
                IfToken(),
                LeftParenToken(),
                NumberToken(1),
                RightParenToken(),
                LeftBraceToken(),
                RightBraceToken()
                )
             ==
             ~IfStm(~NumberExp(1),
                      emptyBlock(),
                      false,
                      emptyBlock())
             );

    REQUIRE( PT(parseIfStm,         // if(1){} else{}  ->  If else stm
                IfToken(),
                LeftParenToken(),
                NumberToken(1),
                RightParenToken(),
                LeftBraceToken(),
                RightBraceToken(),
                ElseToken(),
                LeftBraceToken(),
                RightBraceToken()
                )
             ==
             ~IfStm(~NumberExp(1),
                    emptyBlock(),
                    true,
                    emptyBlock())
             );

    REQUIRE( PT(parseIfStm,         // if(true){} else{}  ->  If else stm
                IfToken(),
                LeftParenToken(),
                TrueToken(),
                RightParenToken(),
                LeftBraceToken(),
                RightBraceToken(),
                ElseToken(),
                LeftBraceToken(),
                RightBraceToken()
                )
             ==
             ~IfStm(~BoolExp(true),
                    emptyBlock(),
                    true,
                    emptyBlock())
             );

    REQUIRE( PT(parseWhileStm,      //while(1){}  ->  while statement
                WhileToken(),
                LeftParenToken(),
                NumberToken(1),
                RightParenToken(),
                LeftBraceToken(),
                RightBraceToken()
                )
             ==
             ~WhileStm(~NumberExp(1),
                       emptyBlock()
                       )
             );

    REQUIRE( PT(parseWhileStm,      //while(false){}  ->  while statement
                WhileToken(),
                LeftParenToken(),
                FalseToken(),
                RightParenToken(),
                LeftBraceToken(),
                RightBraceToken()
                )
             ==
             ~WhileStm(~BoolExp(false),
                       emptyBlock()
                       )
             );

    REQUIRE( PT(parseAssignStm,         //name = 1;  ->  Assign statement
                NameToken("name"),
                AssignToken(),
                NumberToken(1),
                SemiToken()
                )
             ==
             ~AssignStm(~VariableExp("name"), ~NumberExp(1))
             );

    REQUIRE( PT(parseAssignStm,         //myobj.x = 1;
                NameToken("myobj"),
                MemberToken(),
                NameToken("x"),
                AssignToken(),
                NumberToken(1),
                SemiToken()
                )
             ==
             ~AssignStm(~VariableExp("myobj", "x"), ~NumberExp(1))
             );

    REQUIRE( PT(parseVarDecl,           //type name;  -> variable declaration
                NameToken("type"),
                NameToken("name"),
                SemiToken()
                )
             ==
             ~VarDecStm("type", "name")
             );

    REQUIRE( PT(parseVarDecl,           //int var;  -> variable declaration
                NameToken("int"),
                NameToken("var"),
                SemiToken()
                )
             ==
             ~VarDecStm("int", "var")
             );

    REQUIRE( PT(parseVarAssignDecl,           //type name = 1;  -> variable declaration/assignment
                NameToken("type"),
                NameToken("name"),
                AssignToken(),
                NumberToken(1),
                SemiToken()
                )
             ==
             ~VarDecAssignStm("type", "name", ~NumberExp(1))
             );

    REQUIRE( PT(parseVarAssignDecl,           //bool name = true;  -> variable declaration with bool
                NameToken("bool"),
                NameToken("name"),
                AssignToken(),
                TrueToken(),
                SemiToken()
                )
             ==
             ~VarDecAssignStm("bool", "name", ~BoolExp(true))
             );


    REQUIRE( PT(parseNew,       //new int()  ->  NewExp
                NewToken(),
                NameToken("int"),
                LeftParenToken(),
                RightParenToken()
                )
             ==
             ~NewExp("int",
                      Vector<ASNPtr>{})
             );


    REQUIRE( PT(parseNew,       //new int(3) -> NewExp
                NewToken(),
                NameToken("int"),
                LeftParenToken(),
                NumberToken(3),
                RightParenToken()
                )
             ==
             ~NewExp("int",
                      asns(NumberExp(3)))
             );


    REQUIRE( PT(parseNew,       //new int(3,suh)  -> NewExp
                NewToken(),
                NameToken("int"),
                LeftParenToken(),
                NumberToken(3),
                CommaToken(),
                NameToken("suh"),
                RightParenToken()
                )
             ==
             ~NewExp("int",
                      asns(NumberExp(3), VariableExp("suh")))
             );


    REQUIRE( PT(parseMethodExp,          //obj.meth()  ->  MethodExp
                NameToken("obj"),
                MemberToken(),
                NameToken("meth"),
                LeftParenToken(),
                RightParenToken()
                )
             ==
             ~MethodExp(Variable("obj", "meth"), Vector<ASNPtr>{})
             );
    
    REQUIRE( PT(parseMethodExp,          //meth()  ->  MethodExp with "this"
                NameToken("meth"),
                LeftParenToken(),
                RightParenToken()
                )
             ==
             ~MethodExp(Variable(nullopt, "meth"), Vector<ASNPtr>{})
             );


    REQUIRE( PT(parseMethodExp,            //obj.meth(3)  ->  MethodExp
                NameToken("obj"),
                MemberToken(),
                NameToken("meth"),
                LeftParenToken(),
                NumberToken(3),
                RightParenToken()
                )
             ==
             ~MethodExp(Variable("obj", "meth"), asns(NumberExp(3)))
             );


    REQUIRE( PT(parseMethodExp,            //obj.meth(3,suh)  -> MethodExp
                NameToken("obj"),
                MemberToken(),
                NameToken("meth"),
                LeftParenToken(),
                NumberToken(3),
                CommaToken(),
                NameToken("suh"),
                RightParenToken()
                )
             ==
             ~MethodExp(Variable("obj", "meth"),
                      asns(NumberExp(3), VariableExp("suh")))
             );

    REQUIRE( PT(parseMethodStm,            //obj.meth(3,suh);  -> MethodStm
                NameToken("obj"),
                MemberToken(),
                NameToken("meth"),
                LeftParenToken(),
                NumberToken(3),
                CommaToken(),
                NameToken("suh"),
                RightParenToken(),
                SemiToken()
                )
             ==
             ~MethodStm(
                 ~MethodExp(Variable("obj", "meth"),
                         asns(NumberExp(3), VariableExp("suh"))))
             );

    REQUIRE( PT(parseRetStm,        //return 1;  ->  ReturnStm
                ReturnToken(),
                NumberToken(1),
                SemiToken()
                )
             ==
            ~RetStm(~NumberExp(1))
             );

    REQUIRE( PT(parsePrintStm,        //print(1);  ->  PrintStm
                PrintToken(),
                LeftParenToken(),
                NumberToken(1),
                RightParenToken(),
                SemiToken()
                )
             ==
            ~PrintStm(~NumberExp(1))
             );

    REQUIRE( PT(parsePrintStm,        //print(true);  ->  PrintStm
                PrintToken(),
                LeftParenToken(),
                TrueToken(),
                RightParenToken(),
                SemiToken()
                )
             ==
            ~PrintStm(~BoolExp(true))
             );

    REQUIRE( PT(parseClassDecl, // class MyClass { };  -> ClassDeclaration
                ClassToken(),
                NameToken("MyClass"),
                LeftBraceToken(),
                RightBraceToken(),
                SemiToken()
                )
             ==
             ~ClassDecl("MyClass",Vector<ASNPtr>(),nullptr)
            );

    // TODO: 
    
//    REQUIRE( PT(parseClassDecl, // class MyClass extends BaseClass { };  -> ClassDeclaration
//                ClassToken(),
//                NameToken("MyClass"),
//                ExtendsToken(),
//                NameToken("BaseClass"),
//                LeftBraceToken(),
//                RightBraceToken(),
//                SemiToken()
//                )
//             ==
//             ~ClassDecl("MyClass",Vector<ASNPtr>(),nullptr)
//            );
    /**/

    REQUIRE( PT(parseMethodDecl,            //int func(){ }  -> MethodDef
                NameToken("int"),
                NameToken("func"),
                LeftParenToken(),
                RightParenToken(),
                LeftBraceToken(),
                RightBraceToken()
                )
             ==
             ~MethodDef("int","func",Vector<FormalArg>(), emptyBlock())
             );

    /*
     * nullptr is properly returned for unsuccessful parse:
     */

    REQUIRE( PT(parseNumber,  //parse is not Number
        NameToken("var")
        )
        ==
        nullptr
        );

    REQUIRE( PT(parseVariableExp, //parse is not Variable
        NumberToken(6)
        )
        ==
        nullptr
        );

    REQUIRE( PT(parseAdditive,      //parse is not Additive
                NumberToken(2)
                )
             ==
             nullptr
             );

    REQUIRE( PT(parseAdditive,      //parse is not Additive
                NumberToken(2),
                MultiplyToken(),
                NumberToken(3)
                )
             ==
             nullptr
             );

    REQUIRE( PT(parseMultive,      //parse is not Multive
                NumberToken(2),
                PlusToken(),
                NumberToken(3)
                )
             ==
             nullptr
             );

    REQUIRE( PT(parseUnary,           //parse is not Unary
                NameToken("var")
                )
             ==
             nullptr
            );
}


TEST_CASE( "Parser properly throws exceptions", "[parser]" )
{
    /*
     * exception is thrown for bad parse:
     */

    REQUIRE_THROWS_AS( PT(parseExp, // (2 - 5  -> missing )
        LeftParenToken(),
        NumberToken(2),
        MinusToken(),
        NumberToken(5)
        ),
        ParserException
        );

    REQUIRE_THROWS_AS( PT(parseMethodExp,          //obj.meth(  ->  missing )
                NameToken("obj"),
                MemberToken(),
                NameToken("meth"),
                LeftParenToken()
                ),
             ParserException
             );

    REQUIRE_THROWS_AS( PT(parseMethodExp,     //obj.meth(3,)  -> expected stm after ,
                NameToken("obj"),
                MemberToken(),
                NameToken("meth"),
                LeftParenToken(),
                NumberToken(3),
                CommaToken(),
                RightParenToken()
                ),
            ParserException
             );

    REQUIRE_THROWS_AS( PT(parseNew,       //new ()  ->  missing type
                NewToken(),
                LeftParenToken(),
                RightParenToken()
                ),
             ParserException
             );

    REQUIRE_THROWS_AS( PT(parseVarDecl, //int var  -> expected semicolon
                NameToken("int"),
                NameToken("var"),
                AssignToken()
                ),
             ParserException
             );

    REQUIRE_THROWS_AS( PT(parseVarAssignDecl, //type name = ;  -> expected expression
                NameToken("type"),
                NameToken("name"),
                AssignToken()
                ),
             ParserException
             );

    REQUIRE_THROWS_AS( PT(parseVarAssignDecl, //type name = 1 +;  -> expected expression
                NameToken("type"),
                NameToken("name"),
                AssignToken(),
                NumberToken(1),
                PlusToken()
                ),
             ParserException
             );

    REQUIRE_THROWS_AS( PT(parseAdditive,  //1 + -> expected expression after '+'
        NumberToken(1),
        PlusToken()
        ),
        ParserException
        );

    REQUIRE_THROWS_AS( PT(parseUnary,  //- -> expected expression after unary -
                MinusToken()
                ),
            ParserException
            );

    REQUIRE_THROWS_AS( PT(parseAdditive,  //1 + - -> expected expresion after unary -
        NumberToken(1),
        PlusToken(),
        MinusToken()
        ),
        ParserException
        );

    REQUIRE_THROWS_AS( PT(parseIfStm,      // if(1){} else{}  ->  expected block after if
                IfToken(),
                LeftParenToken(),
                NumberToken(1),
                RightParenToken()
                ),
            ParserException
             );

    REQUIRE_THROWS_AS( PT(parseIfStm,   // if(1){} else{}  -> expected block after else
                IfToken(),
                LeftParenToken(),
                NumberToken(1),
                RightParenToken(),
                LeftBraceToken(),
                RightBraceToken(),
                ElseToken()
                ),
            ParserException
             );

    REQUIRE_THROWS_AS( PT(parseBlock,   // {  -> missing right bracket for block
                LeftBraceToken()
                ),
            ParserException
            );

    REQUIRE_THROWS_AS( PT(parseClassDecl, // class -> expected class name
                ClassToken()
                ),
            ParserException
            );
}
