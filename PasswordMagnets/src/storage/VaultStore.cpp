// VaultStore implementation: CRUD over the custom HashTable plus a
// case-insensitive substring search (Knuth-Morris-Pratt) with explicit,
// deterministic match ranking.
#include "VaultStore.hpp"

#include <algorithm>
#include <vector>

namespace passwordmagnets::storage {

namespace {

// ---------------------------------------------------------------------------
// ASCII lower-casing helper (only plain-ASCII letters are folded, which is
// unambiguous and cheap; multi-byte UTF-8 passes through untouched).
// ---------------------------------------------------------------------------
std::string ascii_lower(const std::string& s) {
  std::string out;
  out.reserve(s.size());
  for (char c : s) {
    out.push_back((c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a')
                                         : c);
  }
  return out;
}

// ---------------------------------------------------------------------------
// Knuth-Morris-Pratt substring search.
//
// The prefix table pi[i] is the length of the longest proper prefix of
// pattern[0..i] that is also a suffix of pattern[0..i]. It lets the search
// skip re-comparing characters that cannot start a new match, giving an
// O(|text| + |pattern|) worst case instead of the O(|text| * |pattern|) of a
// naive scan. Both strings are already lower-cased by the caller, so all
// comparisons are plain byte equality.
// ---------------------------------------------------------------------------
std::vector<int> prefix_table(const std::string& p) {
  std::vector<int> pi(p.size(), 0);
  int matched = 0;
  for (std::size_t i = 1; i < p.size(); ++i) {
    while (matched > 0 && p[i] != p[static_cast<std::size_t>(matched)]) {
      matched = pi[static_cast<std::size_t>(matched) - 1];
    }
    if (p[i] == p[static_cast<std::size_t>(matched)]) ++matched;
    pi[i] = matched;
  }
  return pi;
}

// Returns the offset of the first occurrence of `pattern` in `text`, or -1.
// The pattern is assumed non-empty and its prefix table is supplied by the
// caller (built once per search, not once per scanned field).
int kmp_first(const std::string& text, const std::string& pattern,
              const std::vector<int>& pi) {
  int matched = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    while (matched > 0 && text[i] != pattern[static_cast<std::size_t>(matched)]) {
      matched = pi[static_cast<std::size_t>(matched) - 1];
    }
    if (text[i] == pattern[static_cast<std::size_t>(matched)]) ++matched;
    if (matched == static_cast<int>(pattern.size())) {
      return static_cast<int>(i) - matched + 1;
    }
  }
  return -1;
}

// ---------------------------------------------------------------------------
// Match + ranking model (the DSA showcase of this module).
//
// A match is m = (T, o) where T is the (lower-cased) text of one searchable
// field and o is the 0-based offset of the FIRST occurrence of the query in
// T. A field contributes at most one match: the earliest occurrence, which is
// also always the best possible occurrence of that field under this model.
//
// Ranking is the lexicographic comparison of the key
//
//      key(m) = ( exact(m),  -o,  -|T| )          ("higher" = "better")
//
// with the components read left to right:
//
//   1. exact(m) : 1 when the whole field equals the query (case-insensitively),
//      0 otherwise. An exact match is the strongest possible signal and always
//      outranks partial matches, whatever their offset.
//
//   2. -o : between two partial matches, the earlier occurrence wins. A match
//      at offset 0 (a prefix) beats offset 1, which beats offset 2, and so on.
//
//   3. -|T| : tie-breaker requested by the spec ("shorter total string
//      length"). If two matches are equally exact and start at the same
//      offset, the one embedded in the shorter field text ranks higher.
//
// Both searchable fields are weighted identically. For each entry the SINGLE
// strongest of its two field matches is kept and weaker matches never
// influence ordering (an entry appears at most once in the results).
//
// Offsets and field lengths are unbounded, so folding the triple into one
// integer would require an arbitrary cap that could silently corrupt the
// order for very long strings. Comparing the cascade directly is exact,
// deterministic, and equivalent to sorting by score descending; see
// compare_matches() below.
// ---------------------------------------------------------------------------
struct Match {
  bool matched = false;
  bool exact = false;
  std::size_t offset = 0;
  std::size_t text_length = 0;  // |T|, used for the shorter-text tie-breaker
};

// Returns -1 when a is ranked below b, 0 when tied, 1 when a outranks b.
int compare_matches(const Match& a, const Match& b) {
  if (!a.matched && !b.matched) return 0;
  if (!a.matched) return -1;
  if (!b.matched) return 1;
  if (a.exact != b.exact) return a.exact ? 1 : -1;         // 1) exact first
  if (a.offset != b.offset)                                // 2) earlier first
    return (a.offset < b.offset) ? 1 : -1;
  if (a.text_length != b.text_length)                      // 3) shorter text
    return (a.text_length < b.text_length) ? 1 : -1;
  return 0;
}

}  // namespace

// --- Basic CRUD ------------------------------------------------------------

bool VaultStore::add(const Entry& entry) {
  if (entries_.contains(entry.service)) return false;
  return entries_.insert(entry.service, entry).second;
}

bool VaultStore::set(const Entry& entry) {
  entries_.remove(entry.service);  // no-op when absent
  return entries_.insert(entry.service, entry).second;
}

bool VaultStore::remove(const std::string& service) {
  return entries_.remove(service);
}

bool VaultStore::contains(const std::string& service) const {
  return entries_.contains(service);
}

std::optional<Entry> VaultStore::get(const std::string& service) const {
  const auto it = entries_.find(service);
  if (it == entries_.end()) return std::nullopt;
  return it->second;  // copy keeps the result valid across future rehashes
}

VaultStore::size_type VaultStore::size() const noexcept {
  return entries_.size();
}

bool VaultStore::empty() const noexcept { return entries_.empty(); }

void VaultStore::clear() { entries_.clear(); }

// --- Search ------------------------------------------------------------------

std::vector<Entry> VaultStore::search(const std::string& query) const {
  std::vector<Entry> results;

  const std::string needle = ascii_lower(query);
  if (needle.empty()) return results;  // empty query matches nothing
  results.reserve(entries_.size());

  // The needle's prefix table is query-wide, so compute it once and reuse it
  // for every field scan below.
  const std::vector<int> needle_pi = prefix_table(needle);

  // Evaluate one field (already lower-cased haystack) against the needle.
  const auto match_field = [&needle, &needle_pi](const std::string& field)
      -> Match {
    Match m;
    const std::string text = ascii_lower(field);
    const int offset = kmp_first(text, needle, needle_pi);
    if (offset < 0) return m;  // stays unmatched
    m.matched = true;
    m.exact = (offset == 0) && (text.size() == needle.size());
    m.offset = static_cast<std::size_t>(offset);
    m.text_length = text.size();
    return m;
  };

  struct Candidate {
    Entry entry;  // copy: results stay valid regardless of later mutations
    Match match;  // the stronger of the two field matches
  };

  std::vector<Candidate> candidates;
  candidates.reserve(entries_.size());

  // Scan every entry once: O(1) amortized hash lookups via the underlying
  // table, plus O(|T| + |q|) KMP per searchable field.
  for (const auto& kv : entries_) {
    const Entry& e = kv.second;
    const Match on_service = match_field(e.service);
    const Match on_username = match_field(e.username);
    const Match best =
        (compare_matches(on_service, on_username) >= 0) ? on_service
                                                        : on_username;
    if (best.matched) candidates.push_back({e, best});
  }

  std::sort(candidates.begin(), candidates.end(),
            [](const Candidate& a, const Candidate& b) {
              const int c = compare_matches(a.match, b.match);
              if (c != 0) return c > 0;  // higher-ranked match first
              // Final deterministic tie-break: service names are unique, so
              // an alphabetical fallback yields a total order.
              return a.entry.service < b.entry.service;
            });

  for (const Candidate& c : candidates) results.push_back(c.entry);
  return results;
}

}  // namespace passwordmagnets::storage
