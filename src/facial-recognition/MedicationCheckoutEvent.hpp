//
// Created by Marco Stulic on 4/23/26.
//

#pragma once

#include <string>
#include <optional>
#include <cstdint>

#include "PersonEntry.hpp"

struct MedicationCheckoutEvent {
    int64_t timestamp;
    PersonEntry person;
    int box_id;
    std::string shelf_id;
    std::string pill_type;
    int pill_quantity_before;
    int pill_quantity_after;
    int64_t registered_at;
    std::optional<std::string> error_detail;
};
