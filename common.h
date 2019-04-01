#pragma once

#include "intset.h"
#include <string>
#include <vector>

bool parse_integer(const std::string &str, int &v, int a, int b);
std::vector<std::string> split(const std::string &s, char c);
double get_time();
bool fp_eq(double x, double y, double eps);
void dump_intset(const intset &s, FILE *f);
