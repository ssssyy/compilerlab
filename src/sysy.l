%option noyywrap
%option nounput
%option noinput

%{

#include <cstdlib>
#include <string>
#include <iostream>

// 因为 Flex 会用到 Bison 中关于 token 的定义
// 所以需要 include Bison 生成的头文件
#include "sysy.tab.hpp"

using namespace std;

%}

/* 空白符和注释 */
WhiteSpace    [ \t\n\r]*
LineComment   "//".*$
/* 由于太菜了写不出正确的块注释表达式，我在此处参考了网络上的如何使用lex匹配C注释，下方的行为也同样 */
%x COMMENT
BlockCommentHead    "/*"
BlockCommentLine    .|\n
BlockCommentTail    "*/"

/* 标识符 */
Identifier    [a-zA-Z_][a-zA-Z0-9_]*

/* 整数字面量 */
Decimal       [1-9][0-9]*
Octal         0[0-7]*
Hexadecimal   0[xX][0-9a-fA-F]+

%%

{WhiteSpace}    { /* 忽略, 不做任何操作 */ }
{LineComment}   { /* cout<<"Line comment."<<endl; */ }
{BlockCommentHead}  { /* cout<<"Block begin."<<endl; */ BEGIN COMMENT;  }
<COMMENT>{BlockCommentLine}  {/* Do nothing. Just keep the compiler happy. */ }
<COMMENT>{BlockCommentTail}  {BEGIN INITIAL; /* cout<<"Block end."<<endl; */ }

"int"           { return INT; }
"return"        { return RETURN; } 
"<="            { return LE; }
">="            { return GE; }
"=="            { return EQ; }
"!="            { return NE; }
"&&"            { return LAND; }
"||"            { return LOR; }

{Identifier}    { yylval.str_val = new string(yytext); return IDENT; }

{Decimal}       { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Octal}         { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }
{Hexadecimal}   { yylval.int_val = strtol(yytext, nullptr, 0); return INT_CONST; }

.               { return yytext[0]; }   /* 匹配任意的单个字符，用在一元运算符上 */

%%