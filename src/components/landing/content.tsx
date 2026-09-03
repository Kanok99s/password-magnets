// Content + shared building blocks for the PasswordMagnets presentation page.
// Kept in one small module so each section below stays a focused component.

import {
  ArchiveRestore,
  ClipboardCheck,
  Cpu,
  Database,
  Dices,
  FileLock2,
  Fingerprint,
  FlaskConical,
  GitBranch,
  KeyRound,
  Layers,
  Lock,
  Search,
  ShieldCheck,
  TimerReset,
  type LucideIcon,
} from "lucide-react";

export const NAV_LINKS = [
  { href: "#features", label: "Features" },
  { href: "#security", label: "Security" },
  { href: "#architecture", label: "Architecture" },
  { href: "#stack", label: "Stack" },
];

export type Feature = { icon: LucideIcon; title: string; body: string };

export const FEATURES: Feature[] = [
  {
    icon: Database,
    title: "One encrypted file",
    body: "Every entry lives in a single self-contained vault file on your machine — no plaintext database, no cloud account, nothing to sync.",
  },
  {
    icon: Search,
    title: "Search that ranks",
    body: "A Knuth–Morris–Pratt substring engine with a deterministic relevance model re-ranks results on every keystroke as you type.",
  },
  {
    icon: Dices,
    title: "Bias-free generator",
    body: "Random 16–24 character passwords sampled with rejection — no modulo bias can quietly skew the character distribution.",
  },
  {
    icon: ClipboardCheck,
    title: "Clipboard auto-clear",
    body: "A copied password is scrubbed from the clipboard after 20 seconds, tracked by digest only — never a second plaintext copy.",
  },
  {
    icon: TimerReset,
    title: "Inactivity auto-lock",
    body: "Five minutes without input locks the vault and wipes passwords from the table, memory, and any watched clipboard secret.",
  },
  {
    icon: ArchiveRestore,
    title: "Encrypted backups",
    body: "Export or import the vault as one encrypted snapshot — merge it into the open vault or replace it, then re-save instantly.",
  },
];

export type Pillar = { icon: LucideIcon; title: string; tag: string; body: string };

export const PILLARS: Pillar[] = [
  {
    icon: Fingerprint,
    title: "Key derivation done right",
    tag: "Argon2id · crypto_pwhash",
    body: "The master password never touches the cipher directly. A random per-file salt feeds Argon2id, a memory-hard KDF, so GPU brute-force is expensive and identical passwords still produce unique keys.",
  },
  {
    icon: Lock,
    title: "Authenticated encryption",
    tag: "XChaCha20-Poly1305",
    body: "The vault is sealed with libsodium's secret-box construction: fresh 24-byte nonces plus a Poly1305 MAC, so tampering or a wrong key is rejected before any plaintext is trusted.",
  },
  {
    icon: Layers,
    title: "A hash table from scratch",
    tag: "chaining · load ≤ 0.7",
    body: "No std::unordered_map. Buckets stay a power of two, indexing uses a mask instead of a modulo, and the table doubles once the load factor passes 0.7 — deterministic and unit-tested.",
  },
  {
    icon: ShieldCheck,
    title: "Secret hygiene",
    tag: "sodium_memzero · RAII",
    body: "Derived keys are non-copyable and wiped on destruction, decrypted buffers are cleared on every exit path, and locking scrubs rows, cells, and the vault copy from memory.",
  },
];

export type Stage = { icon: LucideIcon; name: string; sub: string; line: string };

export const STAGES: Stage[] = [
  {
    icon: Cpu,
    name: "Qt6 Widgets UI",
    sub: "Login · vault · entry dialogs",
    line: "plaintext only in RAM",
  },
  {
    icon: Database,
    name: "VaultStore",
    sub: "CRUD · KMP search · JSON",
    line: "custom HashTable",
  },
  {
    icon: KeyRound,
    name: "CryptoEngine",
    sub: "libsodium seal / open",
    line: "Argon2id → XChaCha20",
  },
  {
    icon: FileLock2,
    name: "vault.bin",
    sub: "one self-contained file",
    line: "salt · nonce · MAC ∥ text",
  },
];

export const STACK = [
  "C++20",
  "Qt 6 Widgets",
  "libsodium",
  "nlohmann/json",
  "CMake + presets",
  "CTest",
  "GitHub Actions",
  "Linux · macOS · Windows",
];

// Rows shown inside the CSS-drawn vault-window preview. The bullet counts are
// purely decorative (the real app mirrors the true password length).
export const MOCK_ROWS: { service: string; username: string; dots: number }[] = [
  { service: "GitHub", username: "octocat", dots: 12 },
  { service: "AWS Console", username: "admin", dots: 16 },
  { service: "Vercel", username: "you@resume.dev", dots: 8 },
  { service: "Spotify", username: "listener_42", dots: 10 },
];

export const QA_CARDS: { icon: LucideIcon; title: string; body: string }[] = [
  {
    icon: FlaskConical,
    title: "Four headless test suites",
    body: "libsodium smoke, hash-table invariants, search ranking, and an end-to-end encrypted checkpoint.",
  },
  {
    icon: GitBranch,
    title: "Verified on Ubuntu in CI",
    body: "A GitHub Actions workflow installs Qt6 + libsodium, then configures, builds, and tests the release preset.",
  },
];

/* ------------------------------------------------------------------------- */
/* Shared building blocks                                                    */
/* ------------------------------------------------------------------------- */

export function Wordmark({ className = "" }: { className?: string }) {
  return (
    <span className={`inline-flex items-center gap-2.5 ${className}`}>
      <span className="grid h-8 w-8 place-items-center rounded-xl bg-indigo-600 text-white shadow-sm shadow-indigo-600/30">
        <Lock className="h-4 w-4" strokeWidth={2.2} />
      </span>
      <span className="font-display text-[15px] font-semibold tracking-tight text-slate-900">
        PasswordMagnets
      </span>
    </span>
  );
}

export function SectionHeading({
  index,
  eyebrow,
  title,
  lede,
  dark = false,
}: {
  index: string;
  eyebrow: string;
  title: string;
  lede: string;
  dark?: boolean;
}) {
  return (
    <div className="mx-auto mb-12 max-w-2xl text-center sm:mb-16">
      <p
        className={`mb-3 font-mono text-xs font-medium uppercase tracking-[0.2em] ${
          dark ? "text-indigo-400" : "text-indigo-600"
        }`}
      >
        {index} · {eyebrow}
      </p>
      <h2
        className={`font-display text-3xl font-semibold tracking-tight sm:text-4xl ${
          dark ? "text-white" : "text-slate-900"
        }`}
      >
        {title}
      </h2>
      <p
        className={`mt-4 text-base leading-relaxed ${
          dark ? "text-slate-400" : "text-slate-600"
        }`}
      >
        {lede}
      </p>
    </div>
  );
}
