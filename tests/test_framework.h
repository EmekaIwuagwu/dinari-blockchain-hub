#ifndef DINARI_TEST_FRAMEWORK_H
#define DINARI_TEST_FRAMEWORK_H

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <exception>

namespace dinari {
namespace test {

/**
 * @brief Simple test framework for unit testing
 */
class TestFramework {
public:
    struct TestCase {
        std::string name;
        std::function<void()> testFunc;
        bool passed;
        std::string error;

        TestCase(const std::string& n, std::function<void()> f)
            : name(n), testFunc(f), passed(false) {}
    };

    static TestFramework& Instance() {
        static TestFramework instance;
        return instance;
    }

    void RegisterTest(const std::string& name, std::function<void()> testFunc) {
        tests.emplace_back(name, testFunc);
    }

    int RunAllTests() {
        std::cout << "Running " << tests.size() << " tests...\n\n";

        int passed = 0;
        int failed = 0;

        for (auto& test : tests) {
            std::cout << "Running: " << test.name << "... ";
            try {
                test.testFunc();
                test.passed = true;
                std::cout << "PASSED\n";
                passed++;
            } catch (const std::exception& e) {
                test.passed = false;
                test.error = e.what();
                std::cout << "FAILED: " << e.what() << "\n";
                failed++;
            } catch (...) {
                test.passed = false;
                test.error = "Unknown exception";
                std::cout << "FAILED: Unknown exception\n";
                failed++;
            }
        }

        std::cout << "\n========================================\n";
        std::cout << "Test Results:\n";
        std::cout << "  Passed: " << passed << "\n";
        std::cout << "  Failed: " << failed << "\n";
        std::cout << "  Total:  " << tests.size() << "\n";
        std::cout << "========================================\n";

        return failed;
    }

private:
    TestFramework() = default;
    std::vector<TestCase> tests;
};

// Test registration macro
#define TEST(name) \
    static void Test_##name(); \
    static struct TestRegistrar_##name { \
        TestRegistrar_##name() { \
            dinari::test::TestFramework::Instance().RegisterTest(#name, Test_##name); \
        } \
    } testRegistrar_##name; \
    static void Test_##name()

// Assertion macros
#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error("Assertion failed: !" #condition); \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        throw std::runtime_error("Assertion failed: " #a " == " #b); \
    }

#define ASSERT_NE(a, b) \
    if ((a) == (b)) { \
        throw std::runtime_error("Assertion failed: " #a " != " #b); \
    }

#define ASSERT_THROW(statement) \
    { \
        bool threw = false; \
        try { statement; } catch (...) { threw = true; } \
        if (!threw) { \
            throw std::runtime_error("Expected exception not thrown"); \
        } \
    }

} // namespace test
} // namespace dinari

#endif // DINARI_TEST_FRAMEWORK_H
