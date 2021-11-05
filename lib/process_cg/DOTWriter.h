//
// Created by sebastian on 28.10.21.
//

#ifndef INSTCONTROL_CGTRAITS_H
#define INSTCONTROL_CGTRAITS_H

#include "../../../../../../usr/include/c++/11.1.0/ostream"

#include "CallGraph.h"

bool writeDOT(const CallGraph& cg, std::ostream& out);


#endif //INSTCONTROL_CGTRAITS_H
