// Stack section: tech-stack chips plus two "quality assurance" cards.

import { QA_CARDS, SectionHeading, STACK } from "./content";

export default function StackSection() {
  return (
    <section id="stack" className="scroll-mt-20 bg-white px-6 py-20 sm:py-24">
      <div className="mx-auto max-w-6xl">
        <SectionHeading
          index="04"
          eyebrow="Toolchain"
          title="Built to build cleanly"
          lede="One CMake project with presets, dependency management for every platform, and a test suite that runs headless - so CI can verify real builds."
        />
        <div className="flex flex-wrap items-center justify-center gap-3">
          {STACK.map((item) => (
            <span
              key={item}
              className="rounded-full border border-slate-200 bg-slate-50 px-4 py-2 text-sm font-medium text-slate-700 transition-colors hover:border-indigo-200 hover:text-indigo-700"
            >
              {item}
            </span>
          ))}
        </div>
        <div className="mx-auto mt-12 grid max-w-4xl gap-4 sm:grid-cols-2">
          {QA_CARDS.map((card) => (
            <div
              key={card.title}
              className="flex items-start gap-4 rounded-2xl border border-slate-200 p-5"
            >
              <div className="grid h-9 w-9 shrink-0 place-items-center rounded-lg bg-slate-100 text-slate-700">
                <card.icon className="h-4 w-4" strokeWidth={1.9} />
              </div>
              <div>
                <p className="text-sm font-semibold text-slate-900">
                  {card.title}
                </p>
                <p className="mt-1 text-sm leading-relaxed text-slate-600">
                  {card.body}
                </p>
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
