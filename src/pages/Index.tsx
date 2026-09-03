// PasswordMagnets - a single-page project summary, small enough to host under
// a personal/resume website. Intro line, one combined "what it does / how
// it's built" section, a product preview, and a short footer.

import VaultWindowPreview from "@/components/landing/vault-preview";

const DID = [
  "Every login lives in one locally encrypted vault file - no cloud, no plaintext.",
  "Search ranks matches as you type, and secrets stay masked until you copy them.",
  "One-click copy auto-clears the clipboard after 20 seconds.",
  "The vault locks itself after inactivity or when the file changes externally.",
];

const HOW = [
  "C++20 and Qt6 Widgets, built with CMake presets for Linux, macOS, and Windows.",
  "Argon2id key derivation and XChaCha20-Poly1305 encryption via libsodium.",
  "The hash table, search matcher, and JSON save pipeline are written from scratch.",
  "Four headless test suites run in GitHub Actions CI on every commit.",
];

const STACK = [
  "C++20",
  "Qt 6 Widgets",
  "libsodium",
  "nlohmann/json",
  "CMake + CTest",
  "GitHub Actions",
];

export default function Index() {
  return (
    <div className="min-h-screen bg-slate-50 font-sans text-slate-900 antialiased">
      <main className="mx-auto w-full max-w-4xl px-6 pb-16 pt-14 sm:pt-20">
        {/* Intro */}
        <header className="text-center">
          <p className="font-mono text-xs font-medium uppercase tracking-[0.2em] text-indigo-600">
            Portfolio project · C++ / Qt
          </p>
          <h1 className="mt-4 font-display text-3xl font-semibold tracking-tight text-slate-900 sm:text-4xl">
            PasswordMagnets
          </h1>
          <p className="mx-auto mt-5 max-w-2xl text-base leading-relaxed text-slate-600">
            A cross-platform desktop password manager written from scratch. Every
            login is sealed inside a single file on your machine - the crypto,
            search engine, and interface are all my own code.
          </p>
        </header>

        <div className="mt-12">
          <VaultWindowPreview />
        </div>

        {/* One combined "what it does / how it's built" section */}
        <section className="mt-16 grid items-start gap-5 sm:grid-cols-2">
          <div className="rounded-2xl border border-slate-200 bg-white p-6">
            <h2 className="flex items-center gap-2 text-base font-semibold text-slate-900">
              <span className="h-2 w-2 rounded-full bg-indigo-600" />
              What it does
            </h2>
            <ul className="mt-4 space-y-3">
              {DID.map((item) => (
                <li
                  key={item}
                  className="flex items-start gap-3 text-sm leading-relaxed text-slate-600"
                >
                  <span
                    aria-hidden="true"
                    className="mt-[7px] h-1.5 w-1.5 shrink-0 rounded-full bg-indigo-300"
                  />
                  {item}
                </li>
              ))}
            </ul>
          </div>

          <div className="rounded-2xl border border-slate-200 bg-white p-6">
            <h2 className="flex items-center gap-2 text-base font-semibold text-slate-900">
              <span className="h-2 w-2 rounded-full bg-indigo-600" />
              How it's built
            </h2>
            <ul className="mt-4 space-y-3">
              {HOW.map((item) => (
                <li
                  key={item}
                  className="flex items-start gap-3 text-sm leading-relaxed text-slate-600"
                >
                  <span
                    aria-hidden="true"
                    className="mt-[7px] h-1.5 w-1.5 shrink-0 rounded-full bg-indigo-300"
                  />
                  {item}
                </li>
              ))}
            </ul>
          </div>
        </section>

        {/* Tech stack chips */}
        <div className="mt-10 flex flex-wrap items-center justify-center gap-2">
          {STACK.map((chip) => (
            <span
              key={chip}
              className="rounded-full border border-slate-200 bg-white px-3 py-1.5 font-mono text-xs text-slate-600"
            >
              {chip}
            </span>
          ))}
        </div>

        <footer className="mt-14 border-t border-slate-200 pt-6 text-center text-xs text-slate-400">
          PasswordMagnets · a personal portfolio project
        </footer>
      </main>
    </div>
  );
}
