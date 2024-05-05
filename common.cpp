/* Copyright (C) 2014-2019 Emanuele Giaquinta

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the
   Free Software Foundation; either version 2, or (at your option) any
   later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#include "cluster.h"
#include "dfg.h"
#include "intset.h"
#include "nlohmann/json.hpp"
#include <cmath>
#include <cstdlib>
#include <string>
#include <vector>

bool parse_integer(const std::string &str, int &v, int a, int b)
{
    long lv = strtol(str.c_str(), nullptr, 10);
    if (lv < a || lv > b)
        return false;
    v = lv;
    return true;
}

std::vector<std::string> split(const std::string &s, char c)
{
    std::string::size_type i = 0;
    std::string::size_type j = s.find(c);
    std::vector<std::string> fields;

    while (j != std::string::npos) {
        fields.push_back(s.substr(i, j - i));
        i = ++j;
        j = s.find(c, j);
    }

    fields.push_back(s.substr(i, s.length()));
    return fields;
}

// Knuth's floating point comparison, see The Art of Computer Programming Volume
// 2, pp. 217-219
bool fp_eq(double x, double y, double eps)
{
    double diff = x - y;
    int e;

    frexp(fabs(x) > fabs(y) ? x : y, &e);
    eps = ldexp(eps, e);

    return diff >= -eps && diff <= eps;
}

void to_json(nlohmann::json &j, const SCluster &cluster)
{
    j = {
        {"nodes", cluster.nodes()},
        {"edges", cluster.edges()},
        {"src", cluster.src()},
        {"dst", cluster.dst()},
    };
}

void to_json(nlohmann::json &j, const IOSubgraph &config)
{
    j = {
        {"nodes", config.nodes()},
        {"inputs", config.inputs()},
        {"outputs", config.outputs()},
    };
}

void to_json(nlohmann::json &j, const intset &s)
{
    j = nlohmann::json::array();
    for (const auto &i : s) {
        j += i;
    }
}
