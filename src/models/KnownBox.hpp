//
// Created by Marco Stulic on 3/18/26.
//

#pragma once
#include <memory>
#include <mutex>
#include <vector>

namespace aims {
    class KnownBox {
    public:
        KnownBox(std::string name, int id);

        static std::vector<std::shared_ptr<KnownBox>> get_all_boxes();
        static void add_known_box(const std::shared_ptr<KnownBox>& box);
    protected:
        static std::recursive_mutex static_mutex;
        mutable std::recursive_mutex mutex;

        std::string box_name;
        int box_id;

        static std::vector<std::shared_ptr<KnownBox>> known_boxes;
    };
}
