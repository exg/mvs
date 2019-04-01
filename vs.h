#pragma once

#include "common.h"
#include "dfg.h"

intset config_closure(const DFG &dfg, const intset &config);
std::vector<intset> vs_enum(const DFG &dfg,
                            bool enum_all,
                            int max_num_in,
                            int max_num_out,
                            double &max_weight);
