#include <array>

struct point {
    std::array<float, 2> position;
    std::array<float, 4> color;
};

struct indexed_point {
    int index;
    point p;
};

struct point_ref {
    int less_index;
    int more_index;
    
    bool operator==(const point_ref &other) const {
        return (less_index == other.less_index && more_index == other.more_index);
    }
};
