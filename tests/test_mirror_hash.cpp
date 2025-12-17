#include "mirror_hash/mirror_hash.hpp"
#include <iostream>
#include <cassert>
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>
#include <list>
#include <deque>

// ============================================================================
// Test Utilities
// ============================================================================

#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " #name "... "; \
    test_##name(); \
    std::cout << "PASSED\n"; \
} while(0)

int tests_passed = 0;
int tests_failed = 0;

// ============================================================================
// Test Types
// ============================================================================

struct Point {
    int x;
    int y;
    bool operator==(const Point&) const = default;
};

struct Point3D {
    double x, y, z;
    bool operator==(const Point3D&) const = default;
};

struct Person {
    std::string name;
    int age;
    double height;
    bool operator==(const Person&) const = default;
};

struct NestedStruct {
    Point location;
    std::string label;
};

struct DeeplyNested {
    NestedStruct inner;
    int count;
};

struct WithOptional {
    int id;
    std::optional<std::string> nickname;
};

struct WithVector {
    std::vector<int> values;
    std::string name;
};

struct WithArray {
    std::array<double, 3> coords;
    int id;
};

struct Empty {};

struct SingleField {
    int value;
};

enum class Color { Red, Green, Blue };

struct WithEnum {
    Color color;
    int brightness;
};

struct WithPointer {
    int* ptr;
    int value;
};

struct WithPair {
    std::pair<int, std::string> data;
};

struct Complex {
    std::vector<Point> points;
    std::optional<std::string> description;
    std::map<std::string, int> metadata;
};

// Private members test
class PrivateMembers {
    int secret_ = 42;
    std::string name_ = "hidden";
public:
    PrivateMembers() = default;
    PrivateMembers(int s, std::string n) : secret_(s), name_(std::move(n)) {}
};

// ============================================================================
// Basic Type Tests
// ============================================================================

TEST(arithmetic_types) {
    assert(mirror_hash::hash(42) != 0);
    assert(mirror_hash::hash(3.14) != 0);
    assert(mirror_hash::hash(true) != mirror_hash::hash(false));
    assert(mirror_hash::hash(42) != mirror_hash::hash(43));
    assert(mirror_hash::hash(3.14f) != mirror_hash::hash(3.15f));

    // Same values should produce same hashes
    assert(mirror_hash::hash(100) == mirror_hash::hash(100));
    assert(mirror_hash::hash(2.718) == mirror_hash::hash(2.718));
}

TEST(string_types) {
    std::string s1 = "hello";
    std::string s2 = "world";
    std::string s3 = "hello";

    assert(mirror_hash::hash(s1) == mirror_hash::hash(s3));
    assert(mirror_hash::hash(s1) != mirror_hash::hash(s2));

    std::string_view sv1 = "test";
    std::string_view sv2 = "test";
    assert(mirror_hash::hash(sv1) == mirror_hash::hash(sv2));

    // Empty string
    assert(mirror_hash::hash(std::string{}) != 0);
}

TEST(enum_types) {
    assert(mirror_hash::hash(Color::Red) != mirror_hash::hash(Color::Green));
    assert(mirror_hash::hash(Color::Red) == mirror_hash::hash(Color::Red));
}

// ============================================================================
// Struct Tests
// ============================================================================

TEST(simple_struct) {
    Point p1{10, 20};
    Point p2{10, 20};
    Point p3{10, 21};

    assert(mirror_hash::hash(p1) == mirror_hash::hash(p2));
    assert(mirror_hash::hash(p1) != mirror_hash::hash(p3));
}

TEST(struct_with_floats) {
    Point3D p1{1.0, 2.0, 3.0};
    Point3D p2{1.0, 2.0, 3.0};
    Point3D p3{1.0, 2.0, 3.1};

    assert(mirror_hash::hash(p1) == mirror_hash::hash(p2));
    assert(mirror_hash::hash(p1) != mirror_hash::hash(p3));
}

TEST(struct_with_string) {
    Person p1{"Alice", 30, 1.65};
    Person p2{"Alice", 30, 1.65};
    Person p3{"Bob", 30, 1.65};

    assert(mirror_hash::hash(p1) == mirror_hash::hash(p2));
    assert(mirror_hash::hash(p1) != mirror_hash::hash(p3));
}

TEST(nested_struct) {
    NestedStruct n1{{10, 20}, "home"};
    NestedStruct n2{{10, 20}, "home"};
    NestedStruct n3{{10, 21}, "home"};

    assert(mirror_hash::hash(n1) == mirror_hash::hash(n2));
    assert(mirror_hash::hash(n1) != mirror_hash::hash(n3));
}

TEST(deeply_nested) {
    DeeplyNested d1{{{10, 20}, "test"}, 5};
    DeeplyNested d2{{{10, 20}, "test"}, 5};
    DeeplyNested d3{{{10, 20}, "test"}, 6};

    assert(mirror_hash::hash(d1) == mirror_hash::hash(d2));
    assert(mirror_hash::hash(d1) != mirror_hash::hash(d3));
}

TEST(empty_struct) {
    Empty e1, e2;
    assert(mirror_hash::hash(e1) == mirror_hash::hash(e2));
}

TEST(single_field) {
    SingleField s1{42};
    SingleField s2{42};
    SingleField s3{43};

    assert(mirror_hash::hash(s1) == mirror_hash::hash(s2));
    assert(mirror_hash::hash(s1) != mirror_hash::hash(s3));
}

TEST(private_members) {
    PrivateMembers pm1;
    PrivateMembers pm2;
    PrivateMembers pm3(100, "different");

    assert(mirror_hash::hash(pm1) == mirror_hash::hash(pm2));
    assert(mirror_hash::hash(pm1) != mirror_hash::hash(pm3));
}

// ============================================================================
// Container Tests
// ============================================================================

TEST(vector) {
    std::vector<int> v1{1, 2, 3};
    std::vector<int> v2{1, 2, 3};
    std::vector<int> v3{1, 2, 4};
    std::vector<int> v4{1, 2};

    assert(mirror_hash::hash(v1) == mirror_hash::hash(v2));
    assert(mirror_hash::hash(v1) != mirror_hash::hash(v3));
    assert(mirror_hash::hash(v1) != mirror_hash::hash(v4));
}

TEST(array) {
    std::array<int, 3> a1{1, 2, 3};
    std::array<int, 3> a2{1, 2, 3};
    std::array<int, 3> a3{1, 2, 4};

    assert(mirror_hash::hash(a1) == mirror_hash::hash(a2));
    assert(mirror_hash::hash(a1) != mirror_hash::hash(a3));
}

TEST(list) {
    std::list<int> l1{1, 2, 3};
    std::list<int> l2{1, 2, 3};
    std::list<int> l3{1, 2, 4};

    assert(mirror_hash::hash(l1) == mirror_hash::hash(l2));
    assert(mirror_hash::hash(l1) != mirror_hash::hash(l3));
}

TEST(deque) {
    std::deque<int> d1{1, 2, 3};
    std::deque<int> d2{1, 2, 3};
    std::deque<int> d3{1, 2, 4};

    assert(mirror_hash::hash(d1) == mirror_hash::hash(d2));
    assert(mirror_hash::hash(d1) != mirror_hash::hash(d3));
}

TEST(set) {
    std::set<int> s1{3, 1, 2};
    std::set<int> s2{1, 2, 3};
    std::set<int> s3{1, 2, 4};

    assert(mirror_hash::hash(s1) == mirror_hash::hash(s2));
    assert(mirror_hash::hash(s1) != mirror_hash::hash(s3));
}

TEST(struct_with_vector) {
    WithVector w1{{1, 2, 3}, "test"};
    WithVector w2{{1, 2, 3}, "test"};
    WithVector w3{{1, 2, 4}, "test"};

    assert(mirror_hash::hash(w1) == mirror_hash::hash(w2));
    assert(mirror_hash::hash(w1) != mirror_hash::hash(w3));
}

TEST(struct_with_array) {
    WithArray w1{{1.0, 2.0, 3.0}, 1};
    WithArray w2{{1.0, 2.0, 3.0}, 1};
    WithArray w3{{1.0, 2.0, 3.1}, 1};

    assert(mirror_hash::hash(w1) == mirror_hash::hash(w2));
    assert(mirror_hash::hash(w1) != mirror_hash::hash(w3));
}

// ============================================================================
// Optional Tests
// ============================================================================

TEST(optional) {
    std::optional<int> o1 = 42;
    std::optional<int> o2 = 42;
    std::optional<int> o3 = 43;
    std::optional<int> o4 = std::nullopt;

    assert(mirror_hash::hash(o1) == mirror_hash::hash(o2));
    assert(mirror_hash::hash(o1) != mirror_hash::hash(o3));
    assert(mirror_hash::hash(o1) != mirror_hash::hash(o4));
}

TEST(struct_with_optional) {
    WithOptional w1{1, "nick"};
    WithOptional w2{1, "nick"};
    WithOptional w3{1, std::nullopt};
    WithOptional w4{1, "other"};

    assert(mirror_hash::hash(w1) == mirror_hash::hash(w2));
    assert(mirror_hash::hash(w1) != mirror_hash::hash(w3));
    assert(mirror_hash::hash(w1) != mirror_hash::hash(w4));
}

// ============================================================================
// Pair and Variant Tests
// ============================================================================

TEST(pair) {
    std::pair<int, std::string> p1{1, "hello"};
    std::pair<int, std::string> p2{1, "hello"};
    std::pair<int, std::string> p3{1, "world"};

    assert(mirror_hash::hash(p1) == mirror_hash::hash(p2));
    assert(mirror_hash::hash(p1) != mirror_hash::hash(p3));
}

TEST(variant) {
    std::variant<int, std::string> v1 = 42;
    std::variant<int, std::string> v2 = 42;
    std::variant<int, std::string> v3 = std::string("hello");
    std::variant<int, std::string> v4 = 43;

    assert(mirror_hash::hash(v1) == mirror_hash::hash(v2));
    assert(mirror_hash::hash(v1) != mirror_hash::hash(v3));
    assert(mirror_hash::hash(v1) != mirror_hash::hash(v4));
}

TEST(struct_with_pair) {
    WithPair w1{{1, "hello"}};
    WithPair w2{{1, "hello"}};
    WithPair w3{{1, "world"}};

    assert(mirror_hash::hash(w1) == mirror_hash::hash(w2));
    assert(mirror_hash::hash(w1) != mirror_hash::hash(w3));
}

// ============================================================================
// Enum Tests
// ============================================================================

TEST(struct_with_enum) {
    WithEnum w1{Color::Red, 100};
    WithEnum w2{Color::Red, 100};
    WithEnum w3{Color::Blue, 100};

    assert(mirror_hash::hash(w1) == mirror_hash::hash(w2));
    assert(mirror_hash::hash(w1) != mirror_hash::hash(w3));
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST(complex_struct) {
    Complex c1{{{1, 2}, {3, 4}}, "desc", {{"a", 1}, {"b", 2}}};
    Complex c2{{{1, 2}, {3, 4}}, "desc", {{"a", 1}, {"b", 2}}};
    Complex c3{{{1, 2}, {3, 5}}, "desc", {{"a", 1}, {"b", 2}}};

    assert(mirror_hash::hash(c1) == mirror_hash::hash(c2));
    assert(mirror_hash::hash(c1) != mirror_hash::hash(c3));
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST(hash_consistency) {
    Point p1{10, 20};
    Point p2{10, 20};
    Point p3{10, 21};

    // Same value produces same hash
    assert(mirror_hash::hash(p1) == mirror_hash::hash(p2));
    // Different value produces different hash
    assert(mirror_hash::hash(p1) != mirror_hash::hash(p3));
    // Multiple calls are consistent
    assert(mirror_hash::hash(p1) == mirror_hash::hash(p1));
}

// ============================================================================
// std::hash Adapter Tests
// ============================================================================

TEST(std_hash_adapter) {
    std::unordered_set<Point, mirror_hash::std_hash_adapter<Point>> point_set;
    point_set.insert({1, 2});
    point_set.insert({3, 4});
    point_set.insert({1, 2}); // duplicate

    assert(point_set.size() == 2);
    assert(point_set.count({1, 2}) == 1);
    assert(point_set.count({5, 6}) == 0);
}

TEST(std_hash_adapter_map) {
    std::unordered_map<Person, int, mirror_hash::std_hash_adapter<Person>> person_map;
    Person alice{"Alice", 30, 1.65};
    Person bob{"Bob", 25, 1.80};
    person_map[alice] = 1;
    person_map[bob] = 2;

    assert(person_map.size() == 2);
    assert((person_map[alice] == 1));
}

// ============================================================================
// Hash Combine Tests
// ============================================================================

TEST(combine) {
    auto h1 = mirror_hash::combine(1, 2, 3);
    auto h2 = mirror_hash::combine(1, 2, 3);
    auto h3 = mirror_hash::combine(1, 2, 4);

    assert(h1 == h2);
    assert(h1 != h3);
}

// ============================================================================
// Distribution Tests
// ============================================================================

TEST(distribution_quality) {
    // Test that hashes are well-distributed
    std::unordered_set<std::size_t> hashes;
    const int N = 1000;

    for (int i = 0; i < N; ++i) {
        Point p{i, i * 2};
        hashes.insert(mirror_hash::hash(p));
    }

    // With good distribution, we should have very few collisions
    // Allow up to 1% collision rate
    assert(hashes.size() > N * 0.99);
}

TEST(avalanche_effect) {
    // Test that small changes produce very different hashes
    Point p1{1000, 2000};
    Point p2{1000, 2001}; // Single bit change in value

    auto h1 = mirror_hash::hash(p1);
    auto h2 = mirror_hash::hash(p2);

    // Count differing bits - should be roughly half for good avalanche
    auto diff = h1 ^ h2;
    int differing_bits = __builtin_popcountll(diff);

    // Expect at least 20% of bits to differ (conservative)
    assert(differing_bits >= sizeof(std::size_t) * 8 * 0.2);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(zero_values) {
    Point p1{0, 0};
    Point p2{0, 0};
    Point p3{0, 1};

    assert(mirror_hash::hash(p1) == mirror_hash::hash(p2));
    assert(mirror_hash::hash(p1) != mirror_hash::hash(p3));
}

TEST(negative_values) {
    Point p1{-10, -20};
    Point p2{-10, -20};
    Point p3{10, 20};

    assert(mirror_hash::hash(p1) == mirror_hash::hash(p2));
    assert(mirror_hash::hash(p1) != mirror_hash::hash(p3));
}

TEST(empty_containers) {
    std::vector<int> v1;
    std::vector<int> v2;

    assert(mirror_hash::hash(v1) == mirror_hash::hash(v2));
}

TEST(large_values) {
    Point3D p1{1e100, 1e100, 1e100};
    Point3D p2{1e100, 1e100, 1e100};
    Point3D p3{1e100, 1e100, 1e101};

    assert(mirror_hash::hash(p1) == mirror_hash::hash(p2));
    assert(mirror_hash::hash(p1) != mirror_hash::hash(p3));
}

// ============================================================================
// Main
// ============================================================================

int main() {
    std::cout << "=== mirror_hash Test Suite ===\n\n";

    // Basic types
    RUN_TEST(arithmetic_types);
    RUN_TEST(string_types);
    RUN_TEST(enum_types);

    // Structs
    RUN_TEST(simple_struct);
    RUN_TEST(struct_with_floats);
    RUN_TEST(struct_with_string);
    RUN_TEST(nested_struct);
    RUN_TEST(deeply_nested);
    RUN_TEST(empty_struct);
    RUN_TEST(single_field);
    RUN_TEST(private_members);

    // Containers
    RUN_TEST(vector);
    RUN_TEST(array);
    RUN_TEST(list);
    RUN_TEST(deque);
    RUN_TEST(set);
    RUN_TEST(struct_with_vector);
    RUN_TEST(struct_with_array);

    // Optional
    RUN_TEST(optional);
    RUN_TEST(struct_with_optional);

    // Pair and Variant
    RUN_TEST(pair);
    RUN_TEST(variant);
    RUN_TEST(struct_with_pair);

    // Enum
    RUN_TEST(struct_with_enum);

    // Complex types
    RUN_TEST(complex_struct);

    // Consistency
    RUN_TEST(hash_consistency);

    // std::hash adapter
    RUN_TEST(std_hash_adapter);
    RUN_TEST(std_hash_adapter_map);

    // Combine
    RUN_TEST(combine);

    // Distribution
    RUN_TEST(distribution_quality);
    RUN_TEST(avalanche_effect);

    // Edge cases
    RUN_TEST(zero_values);
    RUN_TEST(negative_values);
    RUN_TEST(empty_containers);
    RUN_TEST(large_values);

    std::cout << "\n=== All tests passed! ===\n";
    return 0;
}
