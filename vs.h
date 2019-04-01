#pragma once

#include "dfg.h"

void vs_enumerate(const DFG &dfg,
                  int max_num_in,
                  int max_num_out,
                  const std::function<void(const IOSubgraph &)> &output_cb);
