//
// Created by xenon on 19.11.17.
//

#include <sstream>
#include "CanonicalLR.h"

std::string toPythonString(char c) {
    std::stringstream ss;
    ss << "chr(" << static_cast<int>(c) << ")";
    return ss.str();
}