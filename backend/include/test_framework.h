#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <string>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>
#include <chrono>

// 简易测试框架
class TestFramework {
public:
    struct TestResult {
        std::string name;
        bool passed;
        std::string message;
        double duration_ms;
    };
    
    static TestFramework& getInstance() {
        static TestFramework instance;
        return instance;
    }
    
    void addTest(const std::string& name, std::function<bool()> testFunc) {
        tests_.push_back({name, testFunc});
    }
    
    void runAll() {
        results_.clear();
        int passed = 0, failed = 0;
        
        std::cout << "\n" << Color::CYAN 
                  << "╔══════════════════════════════════════════════════════════════╗\n"
                  << "║                    🧪 运行单元测试                            ║\n"
                  << "╚══════════════════════════════════════════════════════════════╝"
                  << Color::RESET << "\n\n";
        
        for (const auto& [name, func] : tests_) {
            auto start = std::chrono::high_resolution_clock::now();
            bool result = false;
            std::string msg;
            
            try {
                result = func();
                msg = result ? "通过" : "失败";
            } catch (const std::exception& e) {
                result = false;
                msg = std::string("异常: ") + e.what();
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            double duration = std::chrono::duration<double, std::milli>(end - start).count();
            
            results_.push_back({name, result, msg, duration});
            
            if (result) {
                ++passed;
                std::cout << Color::GREEN << "  ✓ " << Color::RESET;
            } else {
                ++failed;
                std::cout << Color::RED << "  ✗ " << Color::RESET;
            }
            std::cout << name << " (" << std::fixed << std::setprecision(2) 
                      << duration << "ms)";
            if (!result) {
                std::cout << " - " << Color::RED << msg << Color::RESET;
            }
            std::cout << "\n";
        }
        
        std::cout << "\n" << std::string(60, '-') << "\n";
        std::cout << "总计: " << tests_.size() << " 个测试, "
                  << Color::GREEN << passed << " 通过" << Color::RESET << ", "
                  << Color::RED << failed << " 失败" << Color::RESET << "\n";
        
        if (failed == 0) {
            std::cout << Color::GREEN << "\n✓ 所有测试通过！" << Color::RESET << "\n";
        } else {
            std::cout << Color::RED << "\n✗ 存在失败的测试" << Color::RESET << "\n";
        }
    }
    
    const std::vector<TestResult>& getResults() const { return results_; }
    
    bool allPassed() const {
        for (const auto& r : results_) {
            if (!r.passed) return false;
        }
        return true;
    }
    
private:
    TestFramework() = default;
    std::vector<std::pair<std::string, std::function<bool()>>> tests_;
    std::vector<TestResult> results_;
};

// 断言宏
#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "断言失败: " #expr << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return false; \
    }

#define ASSERT_FALSE(expr) ASSERT_TRUE(!(expr))

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "断言失败: " #a " == " #b << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return false; \
    }

#define ASSERT_NE(a, b) \
    if ((a) == (b)) { \
        std::cerr << "断言失败: " #a " != " #b << " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return false; \
    }

#define ASSERT_GT(a, b) ASSERT_TRUE((a) > (b))
#define ASSERT_GE(a, b) ASSERT_TRUE((a) >= (b))
#define ASSERT_LT(a, b) ASSERT_TRUE((a) < (b))
#define ASSERT_LE(a, b) ASSERT_TRUE((a) <= (b))

#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != nullptr)
#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == nullptr)

#define TEST(name) \
    bool test_##name(); \
    static bool _reg_##name = (TestFramework::getInstance().addTest(#name, test_##name), true); \
    bool test_##name()

#endif // TEST_FRAMEWORK_H
