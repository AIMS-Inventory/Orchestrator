//
// Created by Marco Stulic on 3/18/26.
//

#include "KnownBox.hpp"

namespace aims {
    std::vector<std::shared_ptr<KnownBox>> KnownBox::known_boxes;
    std::recursive_mutex KnownBox::static_mutex;

    std::vector<std::shared_ptr<KnownBox>> KnownBox::get_all_boxes() {
        std::lock_guard<std::recursive_mutex> lock(static_mutex);
        return known_boxes;
    }

    void KnownBox::add_known_box(const std::shared_ptr<KnownBox> &box) {
        std::lock_guard<std::recursive_mutex> lock(static_mutex);
        known_boxes.push_back(box);
    }
} // aims