// Hero: eyebrow chips, headline, pitch, call-to-actions, and the CSS-drawn
// preview of the real vault window underneath.

import { Button } from "@/components/ui/button";
import { ArrowRight } from "lucide-react";
import VaultWindowPreview from "./vault-preview";

export default function HeroSection() {
  return (
    <section className="px-6 pb-20 pt-16 text-center sm:pb-24 sm:pt-24">
      <div className="mx-auto max-w-3xl">
        <p className="mb-6 inline-flex flex-wrap items-center justify-center gap-2 rounded-full border border-slate-200 bg-white px-4 py-1.5 text-xs font-medium text-slate-600">
          <span className="font-mono font-semibold text-indigo-600">C++20</span>
          <span className="text-slate-300">-</span>
          Qt6 Widgets
          <span className="text-slate-300">-</span>
          libsodium
          <span className="text-slate-300">-</span>
          built from scratch
        </p>

        <h1 className="font-display text-4xl font-semibold leading-[1.08] tracking-tight text-slate-900 sm:text-6xl">
          Your passwords,
          <br />
          behind <span className="text-indigo-600">one lock</span>.
        </h1>

        <p className="mx-auto mt-6 max-w-2xl text-base leading-relaxed text-slate-600 sm:text-lg">
          <strong className="font-semibold text-slate-800">
            PasswordMagnets
          </strong>{" "}
          is a cross-platform desktop password manager. Every login is sealed
          in a single file with Argon2id and XChaCha20-Poly1305 - and its hash
          table, search engine, and Qt interface are all written from scratch.
        </p>

        <div className="mt-9 flex flex-wrap items-center justify-center gap-3">
          <Button
            asChild
            className="h-11 rounded-full bg-indigo-600 px-7 shadow-sm shadow-indigo-600/25 hover:bg-indigo-500"
          >
            <a href="#features">
              Explore the features
              <ArrowRight className="h-4 w-4" />
            </a>
          </Button>
          <Button
            asChild
            variant="outline"
            className="h-11 rounded-full border-slate-300 bg-white px-7 text-slate-700 hover:bg-slate-100"
          >
            <a href="#security">Under the hood</a>
          </Button>
        </div>
      </div>

      <div className="mt-16 sm:mt-20">
        <VaultWindowPreview />
      </div>
    </section>
  );
}
