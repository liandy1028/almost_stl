#include "DuckTest.h"

DUCK_TEST(DefaultConstructor) { Vector<T<>> v; }

DUCK_TEST(SizedConstructor) { Vector<T<>> v(3); }

// DUCK_TEST(CountValueConstructor) { Vector<T<>> v(3, 0); }

DUCK_TEST(Resize) {
  Vector<T<>> v;
  v.resize(3);
}

DUCK_TEST(Sizeof) {
  LOG("sizeof(vector) = ", sizeof(Vector<T<>>), "\n");
  log << "you can also use stream operators to log other info\n";
}

DUCK_TEST(FailedTest) {
  LOG(typeid(Vector<int>).name(), "\n");
}