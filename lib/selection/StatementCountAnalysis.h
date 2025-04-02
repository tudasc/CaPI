//
// Created by sebastian on 02.04.25.
//

#ifndef CAPI_STATEMENTCOUNTANALYSIS_H
#define CAPI_STATEMENTCOUNTANALYSIS_H

#include "TraversalHelper.h"

#include "metadata/TransientMD.h"


namespace capi {

struct ISCKey {
  static constexpr const char* key = "isc";
};
using ISCMD = TransientMD<long, ISCKey>;

class StatementCountAnalysis {
 public:
      bool run(TraversalHelper& helper);
};

}

#endif  // CAPI_STATEMENTCOUNTANALYSIS_H
