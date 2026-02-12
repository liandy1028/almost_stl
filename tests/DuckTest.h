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
 *              constructor; the int value is discarded.
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
  bool enableIntConstructor = true;
  bool enableCopyConstructor = true;
  bool enableMoveConstructor = true;
  bool enableCopyAssignment = true;
  bool enableMoveAssignment = true;
};

template <Callbacks cb, Config config = Config()>
struct A {
  static size_t cnt;
  size_t val;
  template <bool B = config.enableDefaultConstructor,
            typename = std::enable_if_t<B>>
  A() {
    val = cnt++;
    cb.defaultConstructor(*this);
  }
  template <bool B = config.enableIntConstructor,
            typename = std::enable_if_t<B>>
  A(int) {
    val = cnt++;
    cb.intConstructor(*this);
  }
  template <bool B = config.enableCopyConstructor,
            typename = std::enable_if_t<B>>
  A(const A& other) {
    val = cnt++;
    cb.copyConstructor(*this);
  }
  template <bool B = config.enableMoveConstructor,
            typename = std::enable_if_t<B>>
  A(A&& other) {
    val = cnt++;
    cb.moveConstructor(*this);
  }
  template <bool B = config.enableCopyAssignment,
            typename = std::enable_if_t<B>>
  A& operator=(const A& other) {
    val = cnt++;
    cb.copyAssignment(*this);
    return *this;
  }
  template <bool B = config.enableMoveAssignment,
            typename = std::enable_if_t<B>>
  A& operator=(A&& other) {
    val = cnt++;
    cb.moveAssignment(*this);
    return *this;
  }
  ~A() { cb.destructor(*this); }
};
template <Callbacks cb, Config config>
size_t A<cb, config>::cnt = 0;  // not reset between tests

template <template <typename T> class Vector>
struct VectorSequence {
  static std::ostringstream _log;
  std::ostringstream& log = _log;
  static void LOG(auto... args) {
    ((VectorSequence<Vector>::_log << args), ...);
  }
  template <Config config = Config()>
  using T = A<
      Callbacks{
          .defaultConstructor =
              [](const auto& obj) {
                LOG("(", obj.val, ") DefaultConstructor\n");
              },
          .intConstructor =
              [](const auto& obj) { LOG("(", obj.val, ") IntConstructor\n"); },
          .copyConstructor =
              [](const auto& obj) { LOG("(", obj.val, ") CopyConstructor\n"); },
          .moveConstructor =
              [](const auto& obj) { LOG("(", obj.val, ") MoveConstructor\n"); },
          .copyAssignment =
              [](const auto& obj) { LOG("(", obj.val, ") CopyAssignment\n"); },
          .moveAssignment =
              [](const auto& obj) { LOG("(", obj.val, ") MoveAssignment\n"); },
          .destructor =
              [](const auto& obj) { LOG("(", obj.val, ") Destructor\n"); },
      },
      config>;
  auto operator()() {
    // reset
    log = std::ostringstream();
    run();
    return log.str();
  }
  virtual void run() = 0;
};
template <template <typename T> class Vector>
std::ostringstream VectorSequence<Vector>::_log;
}  // namespace DuckTest

#define DUCK_TEST(name)                                               \
  namespace DuckTest {                                                \
  template <template <typename T> class Vector>                       \
  struct name##Test : public ::DuckTest::VectorSequence<Vector> {     \
    template <::DuckTest::Config config = ::DuckTest::Config()>       \
    using T = typename ::DuckTest::VectorSequence<Vector>::T<config>; \
    using ::DuckTest::VectorSequence<Vector>::log;                    \
    using ::DuckTest::VectorSequence<Vector>::LOG;                    \
    void run() override;                                              \
  };                                                                  \
  }                                                                   \
  TEST(DuckTest, name) {                                              \
    auto almost_log = DuckTest::name##Test<almost::vector>{}();       \
    auto std_log = DuckTest::name##Test<std::vector>{}();             \
    EXPECT_EQ(almost_log, std_log);                                   \
    RecordProperty("log", std_log);                                   \
  }                                                                   \
  template <template <typename T> class Vector>                       \
  void DuckTest::name##Test<Vector>::run()
