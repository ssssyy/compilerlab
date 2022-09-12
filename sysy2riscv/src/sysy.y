%code requires {
  #include <memory>
  #include <string>
  #include "ast.hpp"
}

%{

#include <iostream>
#include <memory>
#include <string>

// 声明 lexer 函数和错误处理函数
class BaseAST;
int yylex();
void yyerror(std::unique_ptr<BaseAST> &ast, const char *s);

using namespace std;

%}

// 定义 parser 函数和错误处理函数的附加参数
// 我们需要返回一个字符串作为 AST, 所以我们把附加参数定义成字符串的智能指针
// 解析完成后, 我们要手动修改这个参数, 把它设置成解析得到的字符串
%parse-param { std::unique_ptr<BaseAST> &ast }

// yylval 的定义, 我们把它定义成了一个联合体 (union)
// 因为 token 的值有的是字符串指针, 有的是整数
// 之前我们在 lexer 中用到的 str_val 和 int_val 就是在这里被定义的
// 至于为什么要用字符串指针而不直接用 string 或者 unique_ptr<string>?
// 请自行 STFW 在 union 里写一个带析构函数的类会出现什么情况
%union {
  std::string *str_val;
  int int_val;
  BaseAST *ast_val;
}

// lexer 返回的所有 token 种类的声明
// 注意 IDENT 和 INT_CONST 会返回 token 的值, 分别对应 str_val 和 int_val
%token INT VOID RETURN '+' '-' '!' '*' '/' '%' '<' '>' LE GE EQ NE LAND LOR
%token CONST ';' IF ELSE WHILE CONTINUE BREAK
%token <str_val> IDENT
%token <int_val> INT_CONST

// 非终结符的类型定义
%type <ast_val> CompUnit FuncDef Block Stmt Exp UnaryExp PrimaryExp UnaryOp AddExp MulExp LOrExp LAndExp EqExp RelExp
%type <ast_val> BlockItem BlockItemArr Decl ConstDecl ConstDefArr ConstDef ConstInitVal LVal ConstExp
%type <ast_val> VarDecl VarDefArr VarDef InitVal Condition FuncFParams FuncFParam FuncRParams
%type <ast_val> ConstArr ExpArr InitValArr ConstInitValArr
%type <int_val> Number

%%

// 开始符, CompUnit ::= FuncDef, 大括号后声明了解析完成后 parser 要做的事情
// 之前我们定义了 FuncDef 会返回一个 str_val, 也就是字符串指针
// 而 parser 一旦解析完 CompUnit, 就说明所有的 token 都被解析了, 即解析结束了
// 此时我们应该把 FuncDef 返回的结果收集起来, 作为 AST 传给调用 parser 的函数
// $1 指代规则里第一个符号的返回值, 也就是 FuncDef 的返回值

FileUnit
  : CompUnit{
    auto file_unit = make_unique<FileUnitAST>();
    file_unit->comp_unit = unique_ptr<BaseAST>($1);
    ast = move(file_unit);
  }
  ;
  
CompUnit
  : CompUnit FuncDef {
    auto ast = new CompUnitAST();
    ast->comp_unit = unique_ptr<BaseAST>($1);
    ast->func_def = unique_ptr<BaseAST>($2);
    ast->decl = nullptr;
    $$ = ast;
  }
  | FuncDef {
    auto ast = new CompUnitAST();
    ast->comp_unit = nullptr;
    ast->func_def = unique_ptr<BaseAST>($1);
    ast->decl = nullptr;
    $$ = ast;
  } 
  | CompUnit Decl{
    auto ast = new CompUnitAST();
    ast->comp_unit = unique_ptr<BaseAST>($1);
    ast->func_def = nullptr;
    ast->decl = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | Decl{
    auto ast = new CompUnitAST();
    ast->comp_unit = nullptr;
    ast->func_def = nullptr;
    ast->decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

// FuncDef ::= FuncType IDENT '(' ')' Block;
// 我们这里可以直接写 '(' 和 ')', 因为之前在 lexer 里已经处理了单个字符的情况
// 解析完成后, 把这些符号的结果收集起来, 然后拼成一个新的字符串, 作为结果返回
// $$ 表示非终结符的返回值, 我们可以通过给这个符号赋值的方法来返回结果
// 你可能会问, FuncType, IDENT 之类的结果已经是字符串指针了
// 为什么还要用 unique_ptr 接住它们, 然后再解引用, 把它们拼成另一个字符串指针呢
// 因为所有的字符串指针都是我们 new 出来的, new 出来的内存一定要 delete
// 否则会发生内存泄漏, 而 unique_ptr 这种智能指针可以自动帮我们 delete
// 虽然此处你看不出用 unique_ptr 和手动 delete 的区别, 但当我们定义了 AST 之后
// 这种写法会省下很多内存管理的负担

FuncDef
  : INT IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>(new string("int"));
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_params = nullptr;
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | INT IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>(new string("int"));
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_params = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  | VOID IDENT '(' ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>(new string("void"));
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_params = nullptr;
    ast->block = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  | VOID IDENT '(' FuncFParams ')' Block {
    auto ast = new FuncDefAST();
    ast->func_type = *unique_ptr<string>(new string("void"));
    ast->ident = *unique_ptr<string>($2);
    ast->func_f_params = unique_ptr<BaseAST>($4);
    ast->block = unique_ptr<BaseAST>($6);
    $$ = ast;
  }
  ;

FuncFParams
  : FuncFParam {
    auto ast = new FuncFParamsAST();
    ast->func_f_param = unique_ptr<BaseAST>($1);
    ast->func_f_params = nullptr;
    $$ = ast;
  }
  | FuncFParams ',' FuncFParam {
    auto ast = new FuncFParamsAST();
    ast->func_f_param = unique_ptr<BaseAST>($3);
    ast->func_f_params = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

FuncFParam
  : INT IDENT {
    auto ast = new FuncFParamAST();
    ast->ident = *unique_ptr<string>($2);
    ast->const_arr = nullptr;
    $$ = ast;
  }
  | INT IDENT '[' ']' ConstArr {
    auto ast = new FuncFParamAST();
    ast->ident = *unique_ptr<string>($2);
    ast->const_arr = unique_ptr<BaseAST>($5);
    $$ = ast;
  }
  ;

FuncRParams
  : Exp {
    auto ast = new FuncRParamsAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->func_r_params = nullptr;
    $$ = ast;
  }
  |
  FuncRParams ',' Exp {
    auto ast = new FuncRParamsAST();
    ast->exp = unique_ptr<BaseAST>($3);
    ast->func_r_params =unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Block
  : '{' BlockItemArr '}' {
    auto ast = new BlockAST();
    ast->block_item_arr = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

BlockItemArr
  : BlockItemArr BlockItem{
    auto ast = new BlockItemArrAST();
    ast->block_item_arr = unique_ptr<BaseAST>($1);
    ast->block_item = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  | BlockItem{
    auto ast = new BlockItemArrAST();
    ast->block_item_arr = nullptr;
    ast->block_item = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | {
    auto ast = new BlockItemArrAST();
    ast->block_item_arr = nullptr;
    ast->block_item = nullptr;
    $$ = ast;
  }
  ;

BlockItem
  : Decl {
    auto ast = new BlockItemAST();
    ast->decl = unique_ptr<BaseAST>($1);
    ast->stmt = nullptr;
    $$ = ast;
  }
  | Stmt {
    auto ast = new BlockItemAST();
    ast->stmt = unique_ptr<BaseAST>($1);
    ast->decl = nullptr;
    $$ = ast;
  }
  ;

Decl
  : ConstDecl {
    auto ast = new DeclAST();
    ast->const_decl = unique_ptr<BaseAST>($1);
    ast->var_decl = nullptr;
    $$ = ast;
  }
  | VarDecl {
    auto ast = new DeclAST();
    ast->const_decl = nullptr;
    ast->var_decl = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDecl
  : INT VarDefArr ';' {
    auto ast = new VarDeclAST();
    ast->var_def_arr = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

VarDefArr
  : VarDefArr ',' VarDef {
    auto ast = new VarDefArrAST();
    ast->var_def_arr = unique_ptr<BaseAST>($1);
    ast->var_def = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | VarDef {
    auto ast = new VarDefArrAST();
    ast->var_def_arr = nullptr;
    ast->var_def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

VarDef
  : IDENT ConstArr '=' InitVal {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_arr = unique_ptr<BaseAST>($2);
    ast->init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  | IDENT ConstArr {
    auto ast = new VarDefAST();
    ast->ident = *unique_ptr<string>($1);
    ast->const_arr = unique_ptr<BaseAST>($2);
    ast->init_val = nullptr;
    $$ = ast;
  }
  ;

InitVal
  : Exp {
    auto ast = new InitValAST();
    ast->exp = unique_ptr<BaseAST>($1);
    ast->init_val_arr = nullptr;
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new InitValAST();
    ast->exp = nullptr;
    ast->init_val_arr = nullptr;
    $$ = ast;
  }
  | '{' InitValArr '}' {
    auto ast = new InitValAST();
    ast->exp = nullptr;
    ast->init_val_arr = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

InitValArr
 : InitValArr ',' InitVal {
   auto ast = new InitValArrAST();
   ast->init_val_arr = unique_ptr<BaseAST>($1);
   ast->init_val = unique_ptr<BaseAST>($3);
   $$ = ast;
 }
 | InitVal {
   auto ast = new InitValArrAST();
   ast->init_val_arr = nullptr;
   ast->init_val = unique_ptr<BaseAST>($1);
   $$ = ast;
 }
 ;

ConstDecl
  : CONST INT ConstDefArr ';' {
    auto ast = new ConstDeclAST();
    ast->const_def_arr = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

ConstDefArr
  : ConstDefArr ',' ConstDef {
    auto ast = new ConstDefArrAST();
    ast->const_def_arr = unique_ptr<BaseAST>($1);
    ast->const_def = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | ConstDef{
    auto ast = new ConstDefArrAST();
    ast->const_def_arr = nullptr;
    ast->const_def = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

ConstDef
  : IDENT ConstArr '=' ConstInitVal{
    auto ast = new ConstDefAST();
    ast->const_arr = unique_ptr<BaseAST>($2);
    ast->ident = *unique_ptr<string>($1);
    ast->const_init_val = unique_ptr<BaseAST>($4);
    $$ = ast;
  }
  ;

ConstArr
  : ConstArr '[' ConstExp ']' {
    auto ast = new ConstArrAST();
    ast->const_arr = unique_ptr<BaseAST>($1);
    ast->const_exp = unique_ptr<BaseAST>($3);
    $$ = ast; 
  }
  | {
    auto ast = new ConstArrAST();
    ast->const_arr = nullptr;
    ast->const_exp = nullptr;
    $$ = ast;
  }
  ;

ConstInitVal
  : ConstExp { 
    auto ast = new ConstInitValAST();
    ast->const_exp = unique_ptr<BaseAST>($1);
    ast->const_init_val_arr = nullptr;
    $$ = ast;
  }
  | '{' '}' {
    auto ast = new ConstInitValAST();
    ast->const_exp = nullptr;
    ast->const_init_val_arr = nullptr;
    $$ = ast;
  }
  | '{' ConstInitValArr '}' 
  {
    auto ast = new ConstInitValAST();
    ast->const_exp = nullptr;
    ast->const_init_val_arr = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

ConstInitValArr
 : ConstInitValArr ',' ConstInitVal {
   auto ast = new ConstInitValArrAST();
   ast->const_init_val_arr = unique_ptr<BaseAST>($1);
   ast->const_init_val = unique_ptr<BaseAST>($3);
   $$ = ast;
 }
 | ConstInitVal {
   auto ast = new ConstInitValArrAST();
   ast->const_init_val_arr = nullptr;
   ast->const_init_val = unique_ptr<BaseAST>($1);
   $$ = ast;
 }
 ;

ConstExp
  : Exp {
    auto ast = new ConstExpAST();
    ast->exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

Stmt
  : RETURN Exp ';' {
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->stmt = *unique_ptr<string>(new string("return"));
    ast->exp = unique_ptr<BaseAST>($2);
    ast->block = nullptr;
    ast->condition = nullptr;
    ast->stmt1 = nullptr;
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  | RETURN ';' {
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->stmt = *unique_ptr<string>(new string("return"));
    ast->exp = nullptr;
    ast->block = nullptr;
    ast->condition = nullptr;
    ast->stmt1 = nullptr;
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  | LVal '=' Exp ';' {
    auto ast = new StmtAST();
    ast->lval = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    ast->block = nullptr;
    ast->condition = nullptr;
    ast->stmt1 = nullptr;
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  | Block {
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->exp = nullptr;
    ast->block = unique_ptr<BaseAST>($1);
    ast->condition = nullptr;
    ast->stmt1 = nullptr;
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  | Exp ';' {
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->exp = unique_ptr<BaseAST>($1);
    ast->block = nullptr;
    ast->condition = nullptr;
    ast->stmt1 = nullptr;
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  | ';' {
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->block = nullptr;
    ast->exp = nullptr;
    ast->stmt = "";
    ast->condition = nullptr;
    ast->stmt1 = nullptr;
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  | IF '(' Condition ')' Stmt{
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->block = nullptr;
    ast->exp = nullptr;
    ast->stmt = *unique_ptr<string>(new string("if"));;
    ast->stmt1 = unique_ptr<BaseAST>($5);
    ast->condition = unique_ptr<BaseAST>($3);
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  | IF '(' Condition ')' Stmt ELSE Stmt{
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->block = nullptr;
    ast->exp = nullptr;
    ast->stmt = *unique_ptr<string>(new string("ifelse"));;
    ast->stmt1 = unique_ptr<BaseAST>($5);
    ast->stmt2 = unique_ptr<BaseAST>($7);
    ast->condition = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | WHILE '(' Exp ')' Stmt{
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->block = nullptr;
    ast->exp = unique_ptr<BaseAST>($3);
    ast->stmt = "while";
    ast->stmt1 = unique_ptr<BaseAST>($5);
    ast->stmt2 = nullptr;
    ast->condition = nullptr;
    $$ = ast;
  }
  | BREAK ';' {
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->block = nullptr;
    ast->exp = nullptr;
    ast->stmt = "break";
    ast->condition = nullptr;
    ast->stmt1 = nullptr;
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  | CONTINUE ';' {
    auto ast = new StmtAST();
    ast->lval = nullptr;
    ast->block = nullptr;
    ast->exp = nullptr;
    ast->stmt = "continue";
    ast->condition = nullptr;
    ast->stmt1 = nullptr;
    ast->stmt2 = nullptr;
    $$ = ast;
  }
  ;

Condition
  : LOrExp{
    auto ast = new ConditionAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }

Exp
  : LOrExp {
    auto ast = new ExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  ;

UnaryExp
  : PrimaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = unique_ptr<BaseAST>($1);
    ast->unary_op = nullptr;
    ast->unary_exp = nullptr;
    ast->func_r_params = nullptr;
    ast->ident = "";
    $$ = ast;
  }
  | UnaryOp UnaryExp {
    auto ast = new UnaryExpAST();
    ast->primary_exp = nullptr;
    ast->unary_op = unique_ptr<BaseAST>($1);
    ast->unary_exp = unique_ptr<BaseAST>($2);
    ast->func_r_params = nullptr;
    ast->ident = "";
    $$ = ast;
  }
  | IDENT '(' ')' {
    //cout<<"call without params"<<endl;
    auto ast = new UnaryExpAST();
    ast->primary_exp = nullptr;
    ast->unary_op = nullptr;
    ast->unary_exp = nullptr;
    ast->func_r_params = nullptr;
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  | IDENT '(' FuncRParams ')' {
    //cout<<"call with params"<<endl;
    auto ast = new UnaryExpAST();
    ast->primary_exp = nullptr;
    ast->unary_op = nullptr;
    ast->unary_exp = nullptr;
    ast->func_r_params = unique_ptr<BaseAST>($3);
    ast->ident = *unique_ptr<string>($1);
    $$ = ast;
  }
  ;

PrimaryExp
  : '(' Exp ')' {
    auto ast = new PrimaryExpAST();
    ast->exp = unique_ptr<BaseAST>($2);
    ast->lval = nullptr;
    $$ = ast;
  }
  | LVal {
    auto ast = new PrimaryExpAST();
    ast->exp = nullptr;
    ast->lval = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | Number {
    auto ast = new PrimaryExpAST();
    ast->exp = nullptr;
    ast->lval = nullptr;
    ast->number = *unique_ptr<int>(new int($1));
    $$ = ast;
  }
  ;

LVal
  : IDENT ExpArr{
    auto ast = new LValAST();
    ast->ident = *unique_ptr<string>($1);
    ast->exp_arr = unique_ptr<BaseAST>($2);
    $$ = ast;
  }
  ;

ExpArr
  : ExpArr '[' Exp ']' {
    auto ast = new ExpArrAST();
    ast->exp_arr = unique_ptr<BaseAST>($1);
    ast->exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | {
    auto ast = new ExpArrAST();
    ast->exp_arr = nullptr;
    ast->exp = nullptr;
    $$ = ast;
  }
  ;

UnaryOp
  : '+' {
    //cout<<"UnaryOp +"<<endl;
    auto ast = new UnaryOpAST();
    ast->unary_op = *unique_ptr<string>(new string("+"));
    $$ = ast;
  }
  | '-' {
    //cout<<"UnaryOp -"<<endl;
    auto ast = new UnaryOpAST();
    ast->unary_op = *unique_ptr<string>(new string("-"));
    $$ = ast;
  }
  | '!' {
    //cout<<"UnaryOp !"<<endl;
    auto ast = new UnaryOpAST();
    ast->unary_op = *unique_ptr<string>(new string("!"));
    $$ = ast;
  }
  ;

AddExp
  : MulExp{
    //cout<<"AddExp 1"<<endl;
    auto ast = new AddExpAST();
    ast->add_exp = nullptr;
    ast->mul_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | AddExp '+' MulExp{
    //cout<<"AddExp 2"<<endl;
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->add_op = *unique_ptr<string>(new string("+"));
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | AddExp '-' MulExp{
    //cout<<"AddExp 3"<<endl;
    auto ast = new AddExpAST();
    ast->add_exp = unique_ptr<BaseAST>($1);
    ast->add_op = *unique_ptr<string>(new string("-"));
    ast->mul_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

MulExp
  : UnaryExp{
    //cout<<"MulExp 1"<<endl;
    auto ast = new MulExpAST();
    ast->mul_exp = nullptr;
    ast->unary_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | MulExp '*' UnaryExp{
    //cout<<"MulExp 2"<<endl;
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = *unique_ptr<string>(new string("*"));
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '/' UnaryExp{
    //cout<<"MulExp 3"<<endl;
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = *unique_ptr<string>(new string("/"));
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | MulExp '%' UnaryExp{
    //cout<<"MulExp 4"<<endl;
    auto ast = new MulExpAST();
    ast->mul_exp = unique_ptr<BaseAST>($1);
    ast->mul_op = *unique_ptr<string>(new string("%"));
    ast->unary_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LOrExp
  : LAndExp{
    //cout<<"LOrExp 1"<<endl;
    auto ast = new LOrExpAST();
    ast->lor_exp = nullptr;
    ast->land_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LOrExp LOR LAndExp{
    //cout<<"LOrExp 2"<<endl;
    auto ast = new LOrExpAST();
    ast->lor_exp = unique_ptr<BaseAST>($1);
    ast->land_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

LAndExp
  : EqExp{
    //cout<<"LAndExp 1"<<endl;
    auto ast = new LAndExpAST();
    ast->land_exp = nullptr;
    ast->eq_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | LAndExp LAND EqExp{
    //cout<<"LAndExp 2"<<endl;
    auto ast = new LAndExpAST();
    ast->land_exp = unique_ptr<BaseAST>($1);
    ast->eq_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

EqExp
  : RelExp{
    //cout<<"EqExp 1"<<endl;
    auto ast = new EqExpAST();
    ast->eq_exp = nullptr;
    ast->rel_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | EqExp EQ RelExp{
    //cout<<"EqExp 2"<<endl;
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->eq_op = *unique_ptr<string>(new string("=="));
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | EqExp NE RelExp{
    //cout<<"EqExp 3"<<endl;
    auto ast = new EqExpAST();
    ast->eq_exp = unique_ptr<BaseAST>($1);
    ast->eq_op = *unique_ptr<string>(new string("!="));
    ast->rel_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

RelExp
  : AddExp{
    //cout<<"RelExp 1"<<endl;
    auto ast = new RelExpAST();
    ast->rel_exp = nullptr;
    ast->add_exp = unique_ptr<BaseAST>($1);
    $$ = ast;
  }
  | RelExp '<' AddExp{
    //cout<<"RelExp 2"<<endl;
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = *unique_ptr<string>(new string("<"));
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp '>' AddExp{
    //cout<<"RelExp 3"<<endl;
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = *unique_ptr<string>(new string(">"));
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp LE AddExp{
    //cout<<"RelExp 4"<<endl;
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = *unique_ptr<string>(new string("<="));
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  | RelExp GE AddExp{
    //cout<<"RelExp 5"<<endl;
    auto ast = new RelExpAST();
    ast->rel_exp = unique_ptr<BaseAST>($1);
    ast->rel_op = *unique_ptr<string>(new string(">="));
    ast->add_exp = unique_ptr<BaseAST>($3);
    $$ = ast;
  }
  ;

Number
  : INT_CONST {
    $$ = int($1);
  }
  ;

%%

// 定义错误处理函数, 其中第二个参数是错误信息
// parser 如果发生错误 (例如输入的程序出现了语法错误), 就会调用这个函数
void yyerror(unique_ptr<BaseAST> &ast, const char *s) {
  cerr << "error: " << s << endl;
}