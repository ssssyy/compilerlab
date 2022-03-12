#include<iostream>
#include<string>
#include<memory>
#include<cstdio>
#include<sstream>
#include<unordered_map>
#include<cassert>
#include"koopa.h"
#include"def.hpp"
#include"symtab.hpp"

using namespace std;


string txt2mmry(string s)
{
    cout<<"Assembly:\n";
    koopa_program_t program;
    const char * str= s.c_str();
    koopa_error_code_t ret = koopa_parse_from_string(str,&program);
    assert(ret==KOOPA_EC_SUCCESS);
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    koopa_raw_program_t raw = koopa_build_raw_program(builder,program);
    koopa_delete_program(program);
    string ans = "";
    Visit(raw,ans);
    koopa_delete_raw_program_builder(builder);
    return ans;
}

