#include "mirror_hash/mirror_hash.hpp"
#include <iostream>

struct GameEntity {
    int id;
    std::string name;
    double x, y, z;
    std::vector<int> inventory;

    bool operator==(const GameEntity&) const = default;
};

int main() {
    std::cout << "=== mirror_hash Example ===\n\n";

    GameEntity entity{42, "Player", 100.5, 200.0, 50.0, {1, 2, 3, 4, 5}};

    auto h = mirror_hash::hash(entity);
    std::cout << "GameEntity hash: " << h << "\n\n";

    // Same entity hashes the same
    GameEntity entity2{42, "Player", 100.5, 200.0, 50.0, {1, 2, 3, 4, 5}};
    std::cout << "Same entity hash: " << mirror_hash::hash(entity2) << "\n";
    std::cout << "Hashes equal: " << (h == mirror_hash::hash(entity2) ? "yes" : "no") << "\n\n";

    // Different entity hashes differently
    GameEntity entity3{43, "Enemy", 50.0, 50.0, 0.0, {10}};
    std::cout << "Different entity hash: " << mirror_hash::hash(entity3) << "\n";

    return 0;
}
