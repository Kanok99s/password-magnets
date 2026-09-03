// PasswordMagnets - single-page project presentation site.
//
// Intended to be deployed as a lightweight "project page" under a personal /
// resume website. Sections live in src/components/landing/ and are assembled
// here with a sticky nav and a footer.

import { NAV_LINKS, Wordmark } from "@/components/landing/content";
import HeroSection from "@/components/landing/hero-section";
import FeaturesSection from "@/components/landing/features-section";
import SecuritySection from "@/components/landing/security-section";
import ArchitectureSection from "@/components/landing/architecture-section";
import StackSection from "@/components/landing/stack-section";

export default function Index() {
  return (
    <div className="min-h-screen bg-slate-50 font-sans text-slate-900 antialiased">
      <header className="sticky top-0 z-20 border-b border-slate-200/70 bg-slate-50/80 backdrop-blur">
        <div className="mx-auto flex w-full max-w-6xl items-center justify-between px-6 py-3.5">
          <a href="#top" aria-label="Back to top">
            <Wordmark />
          </a>
          <nav className="hidden items-center gap-7 md:flex">
            {NAV_LINKS.map((link) => (
              <a
                key={link.href}
                href={link.href}
                className="text-sm font-medium text-slate-600 transition-colors hover:text-slate-900"
              >
                {link.label}
              </a>
            ))}
          </nav>
          <a
            href="#features"
            className="rounded-full bg-indigo-600 px-4 py-2 text-sm font-semibold text-white shadow-sm shadow-indigo-600/25 transition-colors hover:bg-indigo-500"
          >
            Overview
          </a>
        </div>
      </header>

      <main id="top">
        <HeroSection />
        <FeaturesSection />
        <SecuritySection />
        <ArchitectureSection />
        <StackSection />
      </main>

      <footer className="border-t border-slate-200 bg-slate-50 px-6 py-10">
        <div className="mx-auto flex w-full max-w-6xl flex-col items-center justify-between gap-5 sm:flex-row">
          <Wordmark />
          <nav className="flex flex-wrap items-center justify-center gap-x-6 gap-y-2">
            {NAV_LINKS.map((link) => (
              <a
                key={link.href}
                href={link.href}
                className="text-xs font-medium text-slate-500 transition-colors hover:text-slate-900"
              >
                {link.label}
              </a>
            ))}
            <a
              href="#top"
              className="text-xs font-medium text-indigo-600 transition-colors hover:text-indigo-500"
            >
              Back to top
            </a>
          </nav>
        </div>
        <p className="mt-6 text-center text-[11px] leading-relaxed text-slate-400">
          Built with C++20, Qt6 Widgets, and libsodium - one project page in a
          personal portfolio.
        </p>
      </footer>
    </div>
  );
}
