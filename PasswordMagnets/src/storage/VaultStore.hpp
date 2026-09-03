// VaultStore: a password-vault storage layer built on the from-scratch
// HashTable (see HashTable.hpp). Entries are keyed by their unique service
// name and can be searched case-insensitively across the service and
// username fields, with results ranked by match quality.
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

#include "HashTable.hpp"

namespace passwordmagnets::storage {

// One stored credential. `service` is the primary key and is unique in a
// VaultStore; the other fields are free-form text.
struct Entry {
  std::string service;
  std::string username;
  std::string password;
  std::string notes;
};

class VaultStore {
 public:
  using size_type = std::size_t;

  VaultStore() = default;

  // --- Basic CRUD ----------------------------------------------------------
  // Add a new entry. Returns false (and leaves the vault untouched) when an
  // entry with the same service name already exists.
  bool add(const Entry& entry);

  // Insert or fully replace the entry identified by entry.service.
  bool set(const Entry& entry);

  // Remove the entry for `service`; returns true if one was removed.
  bool remove(const std::string& service);

  bool contains(const std::string& service) const;

  // Copy of the stored entry, or std::nullopt when absent.
  std::optional<Entry> get(const std::string& service) const;

  size_type size() const noexcept;
  bool empty() const noexcept;
  void clear();

  // --- Search ----------------------------------------------------------------
  // Case-insensitive substring search over the `service` and `username`
  // fields. An empty query matches nothing. Matching entries are returned
  // sorted by descending relevance (see VaultStore.cpp for the scoring
  // model); the ordering is fully deterministic.
  std::vector<Entry> search(const std::string& query) const;

  // --- Serialization -----------------------------------------------------------
  // Serializes every stored entry into a JSON array of objects:
  //   [ { "service":..., "username":..., "password":..., "notes":... }, ... ]
  // The array may appear in any order.
  nlohmann::json serialize() const;

  // Replaces this vault's contents from a JSON array produced by
  // serialize(). Returns false (leaving the vault untouched) when the
  // document is malformed or contains duplicate services.
  bool deserialize(const nlohmann::json& doc);

  // --- File persistence -------------------------------------------------------
  // Writes the vault to `path` as a single binary file with the layout
  //   [Salt 16 bytes][Nonce 24 bytes][XChaCha20-Poly1305 ciphertext]
  // The payload is the JSON from serialize(). Returns false on I/O or
  // cryptographic failure; this vault is never modified.
  bool saveToFile(const std::string& path, const std::string& masterPassword) const;

  // Loads a vault saved by saveToFile(). The master password must match the
  // one used when saving. The current contents are replaced only after the
  // file decrypts and validates successfully, so a wrong password, corrupt
  // file, or missing file fails gracefully and leaves this vault untouched.
  bool loadFromFile(const std::string& path, const std::string& masterPassword);

 private:
  HashTable<std::string, Entry> entries_;
};

}  // namespace passwordmagnets::storage
