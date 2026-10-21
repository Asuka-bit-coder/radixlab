#pragma once

#include <exception>
#include <iostream>
#include <string_view>

namespace radixlab::test {

class Suite {
public:
    explicit Suite(std::string_view name) {
        std::cout << "=== " << name << " ===\n\n";
    }

    void check(std::string_view name, bool condition) {
        if (condition) {
            ++passed_;
            std::cout << "[PASS] " << name << '\n';
        } else {
            ++failed_;
            std::cerr << "[FAIL] " << name << '\n';
        }
    }

    template <typename Exception, typename Action>
    void expectThrows(std::string_view name, Action&& action) {
        try {
            action();
            check(name, false);
        } catch (const Exception&) {
            check(name, true);
        } catch (const std::exception& error) {
            std::cerr << "  wrong exception: " << error.what() << '\n';
            check(name, false);
        } catch (...) {
            check(name, false);
        }
    }

    int finish() const {
        std::cout << "\nPassed: " << passed_
                  << "\nFailed: " << failed_ << '\n';
        return failed_ == 0 ? 0 : 1;
    }

private:
    int passed_ = 0;
    int failed_ = 0;
};

} // namespace radixlab::test
