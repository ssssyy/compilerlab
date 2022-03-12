#include <cassert>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <sstream>
#include <stack>
#include <fstream>
#include <vector>
#include <unordered_map>
#include "def.hpp"
#include "ast.hpp"


using namespace std;

string name_table[20] = {"e0","e1","e2","e3","e4","e5","e6","e7","e8","e9","e10",
"e11","e12","e13","e14","e15","e16","e17","e18","e19"};
int name_count = 0;     //用于生成临时变量的名字的变量，每生成一个表达式都++
int tpname_count = 0;    //对临时名字进行计数
//对if、else和then的基本块的标号计数
int if_count = 0;
int else_count = 0;
int then_count = 0;
int while_count = 0;
int while_not_count = 0;
int current_while = -1;
int shortcut_count = 0;
int cal_val = 0;
bool is_in_shortcut = false;
bool ret_in_branch = false;
bool is_ret_inside = false;
bool last_ret = false;
bool inside_if_else = false;
bool exp_is_const = false;
string start_name;

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
class Symtab;

Symtab* roottab = new Symtab(0);
vector<int> symtabnum={0};

int main(int argc, const char *argv[]) {
  // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
  // compiler 模式 输入文件 -o 输出文件
  assert(argc == 5);
  auto mode = argv[1];
  auto input = argv[2];
  auto output = argv[4];


  // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
  yyin = fopen(input, "r");
  assert(yyin);
  //cout<<1<<endl;

  // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
  unique_ptr<BaseAST> ast;
  auto ret = yyparse(ast);
  //cout<<2<<endl;
  assert(!ret);

  // 输出解析得到的 AST, 其实就是个字符串
  
  stack<string> aststack;
  string IR = "";
  ast->dfs(IR,aststack,roottab);
  cout<<endl;
  
  ofstream ofs(output);
  if(mode[1]=='k') ofs<<IR;
  else if(mode[1]=='r'||mode[1]=='p') ofs<<txt2mmry(IR);
/*
  ofstream off;
  off.open("text.txt",ios::out|ios::app);
  ifstream iff(input);
  ostringstream buf;
  char ch;
  while(buf&&iff.get(ch)) buf.put(ch);
  off<<buf.str();
  off<<"\n";
  off.close();
*/

  return 0;
}



