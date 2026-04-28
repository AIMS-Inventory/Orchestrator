//
// Created by Marco Stulic on 4/26/26.
//

#pragma once

#include <string>
#include <vector>

namespace aims {
    struct BoxContents {
        std::string placed_by;
        std::vector<std::string> pills;
    };

    struct Box {
        std::string id;
        BoxContents contents;
    };
}

