// CTest unit tests for VaultStore (HashTable-backed storage + ranked
// case-insensitive substring search). Run: ctest --preset debug

#include "storage/VaultStore.hpp"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
int fails = 0;
void report(const char* e, int line) {
  std::cerr << "FAILED: " << e << " (line " << line << ")\n";
  ++fails;
}
#define CHECK(c) if (!(c)) report(#c, __LINE__)

using passwordmagnets::storage::Entry;
using passwordmagnets::storage::VaultStore;

Entry make_entry(std::string service, std::string username,
                 std::string password = "p", std::string notes = "") {
  Entry e;
  e.service = std::move(service);
  e.username = std::move(username);
  e.password = std::move(password);
  e.notes = std::move(notes);
  return e;
}

// Compares search results against an expected list of service names.
void check_order(const char* label, VaultStore& vault, const std::string& q,
                 const std::vector<std::string>& expected) {
  const std::vector<Entry> got = vault.search(q);
  if (got.size() != expected.size()) {
    std::cerr << "FAILED: [" << label << "] size " << got.size()
              << " != expected " << expected.size() << "\n";
    ++fails;
    return;
  }
  for (std::size_t i = 0; i < got.size(); ++i) {
    if (got[i].service != expected[i]) {
      std::cerr << "FAILED: [" << label << "] position " << i << " was '"
                << got[i].service << "', expected '" << expected[i] << "'\n";
      ++fails;
    }
  }
}
}  // namespace

int main() {
  // 1. CRUD basics.
  {
    VaultStore vault;
    CHECK(vault.empty());
    CHECK(vault.size() == 0);

    Entry e = make_entry("github", "octocat", "s3cret");
    CHECK(vault.add(e));
    CHECK(!vault.add(e));  // duplicate service rejected
    CHECK(vault.size() == 1);

    CHECK(vault.contains("github"));
    CHECK(!vault.contains("GitHub"));  // keys stay case-sensitive
    CHECK(vault.get("github").has_value());
    CHECK(vault.get("github")->username == "octocat");
    CHECK(!vault.get("missing").has_value());

    // set() upserts: old value fully replaced.
    Entry updated = make_entry("github", "Mona", "n3w");
    CHECK(vault.set(updated));
    CHECK(vault.size() == 1);
    CHECK(vault.get("github")->username == "Mona");

    CHECK(vault.remove("github"));
    CHECK(!vault.remove("github"));
    CHECK(vault.empty());
  }

  // 2. Search is case-insensitive and hits both service and username.
  {
    VaultStore vault;
    vault.add(make_entry("GitHub", "octocat"));
    vault.add(make_entry("Dropbox", "githubber", "", "sync tool"));
    vault.add(make_entry("Blog", "octocat-the-writer", "", ""));

    CHECK(vault.search("github").size() == 2);   // service + username
    CHECK(vault.search("GITHUB").size() == 2);   // case-insensitive
    CHECK(vault.search("octocat").size() == 2);  // username + username
    CHECK(vault.search("zzzz").empty());         // no match
    CHECK(vault.search("").empty());             // empty query matches none
  }

  // 3. Ranking: exact match first, then earlier match offset wins.
  //    "github" -> "github" (exact), "github.com" (offset 0),
  //                "mygithub" (offset 2), "lab.github.io" (offset 4).
  {
    VaultStore vault;
    vault.add(make_entry("lab.github.io", "a"));
    vault.add(make_entry("mygithub", "b"));
    vault.add(make_entry("github.com", "c"));
    vault.add(make_entry("github", "d"));
    vault.add(make_entry("unrelated", "e"));

    const std::vector<std::string> expected = {"github", "github.com",
                                               "mygithub", "lab.github.io"};
    check_order("exact-then-offset", vault, "github", expected);
  }

  // 4. Tie-breaker: same offset, shorter matched text ranks higher.
  //    "git" -> all prefix matches (offset 0, no exact match):
  //    "gith"(4) < "gitea"(5) < "gitlab"(6) < "github desktop"(14).
  {
    VaultStore vault;
    vault.add(make_entry("github desktop", "a"));
    vault.add(make_entry("gitlab", "b"));
    vault.add(make_entry("gith", "c"));
    vault.add(make_entry("gitea", "d"));

    const std::vector<std::string> expected = {"gith", "gitea", "gitlab",
                                               "github desktop"};
    check_order("shorter-text-first", vault, "git", expected);
  }

  // 5. Ranking on the username field mirrors the same rules.
  //    "octo" -> exact "octo", then prefixes "octopus"(7) and
  //    "octo-mobile"(11) by length, then "xoctopus"(offset 1).
  {
    VaultStore vault;
    vault.add(make_entry("svc-a", "octo"));
    vault.add(make_entry("svc-b", "xoctopus"));
    vault.add(make_entry("svc-c", "octo-mobile"));
    vault.add(make_entry("svc-d", "octopus"));

    const std::vector<std::string> expected = {"svc-a", "svc-d", "svc-c",
                                               "svc-b"};
    check_order("username-ranking", vault, "octo", expected);
  }

  // 6. An entry appears at most once even when both fields match.
  {
    VaultStore vault;
    vault.add(make_entry("github", "github-admin", "p", "n"));

    const std::vector<Entry> got = vault.search("github");
    CHECK(got.size() == 1);
    CHECK(got[0].service == "github");
    CHECK(got[0].username == "github-admin");
  }

  if (fails == 0) std::cout << "vault store tests passed\n";
  return fails == 0 ? 0 : 1;
}
