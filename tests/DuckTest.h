#pragma once

/*
 * Defines DUCK_TEST(name) macro
 *
 * Usage:
 * DUCK_TEST(TestName) {
 *   // test code here
 *   Vector<T<>> v;
 *   v.push_back(T<>());
 * }
 *
 * The test code will be ran for both almost::vector and std::vector, and the
 * logs of constructor/destructor calls will be compared. You can use the LOG
 * function to log any info you want which will also be compared.
 *
 * Vector<>     will be defined as either almost::vector or std::vector
 *              depending on the context.
 * T<>          will be defined as a type that logs its
 *              constructor/destructor calls, and you can configure which
 *              constructors/assignments are enabled via the Config template
 *              parameter. For example T<{.enableCopyConstructor = false}>
 *              will disable the copy constructor. T<> also has a int
 *              constructor
 * log          is an ostringstream object.
 * LOG(args...) is a function that logs args to log.
 *
 */

#include <gtest/gtest.h>

#include <almost/vector>
#include <vector>

namespace DuckTest {
template <typename DefaultConstructor, typename IntConstructor,
          typename CopyConstructor, typename MoveConstructor,
          typename CopyAssignment, typename MoveAssignment, typename Destructor>
struct Callbacks {
  DefaultConstructor defaultConstructor;
  IntConstructor intConstructor;
  CopyConstructor copyConstructor;
  MoveConstructor moveConstructor;
  CopyAssignment copyAssignment;
  MoveAssignment moveAssignment;
  Destructor destructor;
};

struct Config {
  bool enableDefaultConstructor = true;
  bool defaultConstructorCanThrow = false;
  bool enableIntConstructor = true;
  bool intConstructorCanThrow = false;
  bool enableCopyConstructor = true;
  bool copyConstructorCanThrow = false;
  bool enableMoveConstructor = true;
  bool moveConstructorCanThrow = false;
  bool enableCopyAssignment = true;
  bool copyAssignmentCanThrow = false;
  bool enableMoveAssignment = true;
  bool moveAssignmentCanThrow = false;
  bool enableDestructor = true;
  bool destructorCanThrow = false;
};

template <Callbacks cb, Config config = Config()>
struct A {
  inline static int counter = 1;
  int val;
  int stamp;
  A() noexcept
    requires(config.enableDefaultConstructor &&
             !config.defaultConstructorCanThrow)
  {
    stamp = counter++;
    val = 0;
    cb.defaultConstructor(*this);
  }
  A()
  requires(config.enableDefaultConstructor &&
           config.defaultConstructorCanThrow) {
    stamp = counter++;
    val = 0;
    cb.defaultConstructor(*this);
  }
  A(const int& x) noexcept
    requires(config.enableIntConstructor && !config.intConstructorCanThrow)
  {
    stamp = counter++;
    val = x;
    cb.intConstructor(*this, x);
  }
  A(const int& x)
  requires(config.enableIntConstructor && config.intConstructorCanThrow) {
    stamp = counter++;
    val = x;
    cb.intConstructor(*this, x);
  }
  A(const A& other) noexcept
    requires(config.enableCopyConstructor && !config.copyConstructorCanThrow)
  {
    stamp = counter++;
    val = other.val;
    cb.copyConstructor(*this, other);
  }
  A(const A& other)
  requires(config.enableCopyConstructor && config.copyConstructorCanThrow) {
    stamp = counter++;
    val = other.val;
    cb.copyConstructor(*this, other);
  }
  A(A&& other) noexcept
    requires(config.enableMoveConstructor && !config.moveConstructorCanThrow)
  {
    stamp = counter++;
    val = other.val;
    other.stamp = -std::abs(other.stamp);
    cb.moveConstructor(*this, other);
  }
  A(A&& other)
  requires(config.enableMoveConstructor && config.moveConstructorCanThrow) {
    stamp = counter++;
    val = other.val;
    other.stamp = -std::abs(other.stamp);
    cb.moveConstructor(*this, other);
  }
  A& operator=(const A& other) noexcept
    requires(config.enableCopyAssignment && !config.copyAssignmentCanThrow)
  {
    val = other.val;
    stamp = std::abs(stamp);
    cb.copyAssignment(*this, other);
    return *this;
  }
  A& operator=(const A& other)
    requires(config.enableCopyAssignment && config.copyAssignmentCanThrow)
  {
    val = other.val;
    stamp = std::abs(stamp);
    cb.copyAssignment(*this, other);
    return *this;
  }
  A& operator=(A&& other) noexcept
    requires(config.enableMoveAssignment && !config.moveAssignmentCanThrow)
  {
    val = other.val;
    stamp = std::abs(stamp);
    other.stamp = -std::abs(other.stamp);
    cb.moveAssignment(*this, other);
    return *this;
  }
  A& operator=(A&& other)
    requires(config.enableMoveAssignment && config.moveAssignmentCanThrow)
  {
    val = other.val;
    stamp = std::abs(stamp);
    other.stamp = -std::abs(other.stamp);
    cb.moveAssignment(*this, other);
    return *this;
  }
  ~A() noexcept
    requires(config.enableDestructor && !config.destructorCanThrow)
  {
    cb.destructor(*this);
  }
  ~A()
    requires(config.enableDestructor && config.destructorCanThrow)
  {
    cb.destructor(*this);
  }
  auto operator<=>(const A& other) const = default;
  auto operator==(const A& other) const -> bool = default;
};
template <Callbacks cb, Config config>
std::ostream& operator<<(std::ostream& os, const A<cb, config>& obj) {
  return os << "(" << obj.stamp << ":" << obj.val << ")";
}

template <template <typename T> class Vector, size_t Salt = 0>
struct VectorSequence {
  using Self = VectorSequence<Vector, Salt>;
  inline static std::ostringstream _log;
  std::ostringstream& log = _log;
  static void LOG(auto&&... args) {
    ((Self::_log << args), ...);
    // ((std::cout << ... << args));
  }
  template <Config config = Config()>
  using T =
      A<Callbacks{
            .defaultConstructor =
                [](const auto& obj) { LOG(obj, " DefaultConstructor\n"); },
            .intConstructor =
                [](const auto& obj, const auto& other) {
                  LOG(obj, " IntConstructor from ", other, "\n");
                },
            .copyConstructor =
                [](const auto& obj, const auto& other) {
                  LOG(obj, " CopyConstructor from ", other, "\n");
                },
            .moveConstructor =
                [](const auto& obj, const auto& other) {
                  LOG(obj, " MoveConstructor from ", other, "\n");
                },
            .copyAssignment =
                [](const auto& obj, const auto& other) {
                  LOG(obj, " CopyAssignment from ", other, "\n");
                },
            .moveAssignment =
                [](const auto& obj, const auto& other) {
                  LOG(obj, " MoveAssignment from ", other, "\n");
                },
            .destructor = [](const auto& obj) { LOG(obj, " Destructor\n"); },
        },
        config>;
  auto operator()() {
    run();
    return log.str();
  }
  virtual void run() = 0;
};
}  // namespace DuckTest

#define _DUCK_TEST(suite, name, salt)                                     \
  namespace DuckTest {                                                    \
  template <template <typename T> class Vector>                           \
  struct suite##_##name##Test                                             \
      : public ::DuckTest::VectorSequence<Vector, salt> {                 \
    using Base = ::DuckTest::VectorSequence<Vector, salt>;                \
    template <::DuckTest::Config config = ::DuckTest::Config()>           \
    using T = typename Base::template T<config>;                          \
    using Base::log;                                                      \
    using Base::LOG;                                                      \
    void run() override;                                                  \
  };                                                                      \
  }                                                                       \
  TEST(DuckTest_##suite, name) {                                          \
    auto std_log = DuckTest::suite##_##name##Test<std::vector>{}();       \
    RecordProperty("log", std_log);                                       \
    auto almost_log = DuckTest::suite##_##name##Test<almost::vector>{}(); \
    EXPECT_EQ(std_log, almost_log);                                       \
  }                                                                       \
  template <template <typename T> class Vector>                           \
  void DuckTest::suite##_##name##Test<Vector>::run()

#define DUCK_TEST(suite, name) _DUCK_TEST(suite, name, __COUNTER__)