// CTest unit tests for the from-scratch HashTable.
// Run: ctest --preset debug

#include "storage/HashTable.hpp"

#include <cstddef>
#include <iostream>
#include <string>

namespace {
int fails = 0;
void report(const char* e, int line) {
  std::cerr << "FAILED: " << e << " (line " << line << ")\n";
  ++fails;
}
#define CHECK(c) if (!(c)) report(#c, __LINE__)
}  // namespace

using passwordmagnets::storage::HashTable;

int main() {
  // 1. Insert, find, update, duplicate handling.
  {
    HashTable<int, std::string> t;
    CHECK(t.empty());
    CHECK(t.capacity() == 8);
    auto [it, ok] = t.insert(1, "one");
    CHECK(ok);
    CHECK(it->second == "one");
    CHECK(t.size() == 1);
    auto [it2, ok2] = t.insert(1, "uno");
    CHECK(!ok2);
    CHECK(t.size() == 1);
    CHECK(t.find(1) != t.end());
    CHECK(t.find(2) == t.end());
    t.find(1)->second = "ONE";
    int seen = 0;
    for (auto& kv : t) {
      ++seen;
      CHECK(kv.second == "ONE");
    }
    CHECK(seen == 1);
    CHECK(t.remove(1));
    CHECK(!t.remove(1));
    CHECK(t.empty());
    CHECK(t.capacity() == 8);  // no shrink
  }

  // 2. Collision handling: keys k*128 always land in bucket 0, so every
  //    insert exercises the same chain even across rehashes.
  {
    HashTable<int, int> t(1);
    const int n = 64;
    for (int i = 1; i <= n; ++i) t.insert(i * 128, i);
    CHECK(t.size() == 64);
    CHECK(t.capacity() > 1);
    for (int i = 1; i <= n; ++i) {
      auto f = t.find(i * 128);
      CHECK(f != t.end());
      CHECK(f->second == i);
    }
    CHECK(t.remove(64 * 128));
    CHECK(t.find(64 * 128) == t.end());
    CHECK(t.size() == 63);

    int cnt = 0;
    long sum = 0;
    for (const auto& [k, v] : t) {
      ++cnt;
      sum += k;
    }
    CHECK(cnt == 63);
    CHECK(sum == 128L * 63 * 64 / 2);  // 128 + 256 + ... + 128*63
  }

  // 3. Dynamic resizing with unique keys.
  {
    HashTable<int, int> t;
    const std::size_t cap0 = t.capacity();
    for (int i = 0; i < 200; ++i) t.insert(i, i * 10);
    CHECK(t.size() == 200);
    CHECK(t.capacity() > cap0);
    int cnt = 0;
    for (const auto& [k, v] : t) {
      ++cnt;
      CHECK(v == k * 10);
    }
    CHECK(cnt == 200);
    t.clear();
    CHECK(t.empty());
    CHECK(t.find(5) == t.end());
  }

  // 4. Copy and move semantics (deep copy and ownership transfer; any leak
  //    or double free would trip these under valgrind/ASan).
  {
    HashTable<int, int> a;
    a.insert(1, 100);
    a.insert(2, 200);
    HashTable<int, int> b(a);
    CHECK(b.size() == 2);
    b.remove(1);
    CHECK(a.contains(1));
    CHECK(!b.contains(1));
    HashTable<int, int> c;
    c = a;
    CHECK(c.size() == 2);
    HashTable<int, int> d(std::move(c));
    CHECK(d.size() == 2);
    CHECK(c.empty());
    CHECK(c.insert(9, 9).second);  // moved-from object stays usable
  }

  if (fails == 0) std::cout << "hash table tests passed\n";
  return fails == 0 ? 0 : 1;
}
