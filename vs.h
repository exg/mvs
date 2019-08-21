#pragma once

#include "common.h"
#include "dfg.h"

std::vector<io_config> vs_enum(const DFG &dfg,
                               bool enum_all,
                               int max_num_in,
                               int max_num_out,
                               double &max_weight);
