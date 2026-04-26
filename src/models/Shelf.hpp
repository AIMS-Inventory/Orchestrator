//
// Created by Marco Stulic on 4/26/26.
//

#pragma once

#include <string>
#include <map>
#include <vector>
#include "Box.hpp"

namespace aims {
    struct Shelf {
        std::string name;
        std::map<std::string, int> codes;
        std::map<std::string, Box> boxes; // mapped by position
    };
}

