//
// Created by Marco Stulic on 4/26/26.
//

#pragma once

#include <string>
#include <vector>

namespace aims {
    struct Box {
        std::string id;
        std::vector<std::string> pills;
        std::string crew;
    };
}

