#pragma once

#include "cluster.h"
#include "dfg.h"
#include "intset.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

bool parse_integer(const std::string &str, int &v, int a, int b);
std::vector<std::string> split(const std::string &s, char c);
double get_time();
bool fp_eq(double x, double y, double eps);
void to_json(nlohmann::json &j, const s_cluster &cluster);
void to_json(nlohmann::json &j, const io_config &config);
void to_json(nlohmann::json &j, const intset &s);
