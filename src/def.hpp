#include<string>
#include"koopa.h"
std::string txt2mmry(std::string);
void Visit(const koopa_raw_program_t &program, std::string &ans);

void Visit(const koopa_raw_slice_t &slice, std::string &ans);

void Visit(const koopa_raw_function_t &func, std::string &ans);

void Visit(const koopa_raw_basic_block_t &bb, std::string &ans);

void Visit(const koopa_raw_value_t &value, std::string &ans);

void Visit(const koopa_raw_return_t &ret, std::string &ans);

void Visit(const koopa_raw_integer_t &integer, std::string &ans);

void Visit(const koopa_raw_binary_t &op, std::string &ans);
