#include "mirror_hash/mirror_hash.hpp"
#include <iostream>
#include <unordered_set>
#include <unordered_map>

struct Point {
    int x, y;

    bool operator==(const Point& other) const = default;
};

struct Person {
    std::string name;
    int age;
    std::optional<std::string> email;

    bool operator==(const Person& other) const = default;
};

struct Sensor {
    int id;
    std::string location;
    std::vector<double> readings;
    std::optional<std::string> notes;

    bool operator==(const Sensor& other) const = default;
};

int main() {
    std::cout << "=== mirror_hash Basic Usage ===\n\n";

    // Simple struct hashing
    Point p1{10, 20};
    Point p2{10, 20};
    Point p3{30, 40};

    std::cout << "Point{10, 20} hash: " << mirror_hash::hash(p1) << "\n";
    std::cout << "Point{10, 20} hash: " << mirror_hash::hash(p2) << " (same)\n";
    std::cout << "Point{30, 40} hash: " << mirror_hash::hash(p3) << " (different)\n\n";

    // Using with unordered_set
    std::unordered_set<Point, mirror_hash::std_hash_adapter<Point>> point_set;
    point_set.insert({1, 2});
    point_set.insert({3, 4});
    point_set.insert({1, 2}); // duplicate - won't be added

    std::cout << "Set size after adding 3 points (1 duplicate): " << point_set.size() << "\n\n";

    // Complex struct with optional and vector
    Person alice{"Alice", 30, "alice@example.com"};
    Person bob{"Bob", 25, std::nullopt};

    std::cout << "Alice hash: " << mirror_hash::hash(alice) << "\n";
    std::cout << "Bob hash:   " << mirror_hash::hash(bob) << "\n\n";

    // Using as map key
    std::unordered_map<Person, std::string, mirror_hash::std_hash_adapter<Person>> person_roles;
    person_roles[alice] = "Engineer";
    person_roles[bob] = "Designer";

    std::cout << "Alice's role: " << person_roles[alice] << "\n";
    std::cout << "Bob's role: " << person_roles[bob] << "\n\n";

    // Struct with vector member
    Sensor s1{1, "Room A", {23.5, 24.0, 23.8}, "Temperature sensor"};
    Sensor s2{1, "Room A", {23.5, 24.0, 23.8}, "Temperature sensor"};
    Sensor s3{2, "Room B", {18.0, 18.5}, std::nullopt};

    std::cout << "Sensor 1 hash: " << mirror_hash::hash(s1) << "\n";
    std::cout << "Sensor 2 hash: " << mirror_hash::hash(s2) << " (same as s1)\n";
    std::cout << "Sensor 3 hash: " << mirror_hash::hash(s3) << "\n\n";

    // Hash combine for ad-hoc combinations
    auto combined = mirror_hash::combine(p1, alice, 42);
    std::cout << "Combined hash of (Point, Person, int): " << combined << "\n";

    return 0;
}
