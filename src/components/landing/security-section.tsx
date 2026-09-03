// Security section: the four engineering pillars plus the on-disk format line.

import { PILLARS, SectionHeading } from "./content";

export default function SecuritySection() {
  return (
    <section id="security" className="scroll-mt-20 px-6 py-20 sm:py-24">
      <div className="mx-auto max-w-6xl">
        <SectionHeading
          index="02"
          eyebrow="Engineering choices"
          title="Crypto and containers you can read"
          lede="A security tool should not hide behind hand-waving. Every important decision here is explicit, documented, and visible in the source."
        />
        <div className="grid gap-5 md:grid-cols-2">
          {PILLARS.map((pillar) => (
            <div
              key={pillar.title}
              className="rounded-2xl border border-slate-200 bg-white p-6 sm:p-7"
            >
              <div className="flex items-center justify-between gap-3">
                <div className="grid h-10 w-10 place-items-center rounded-xl bg-indigo-50 text-indigo-600 ring-1 ring-inset ring-indigo-100">
                  <pillar.icon className="h-5 w-5" strokeWidth={1.9} />
                </div>
                <span className="rounded-full bg-slate-100 px-3 py-1 font-mono text-[11px] font-medium text-slate-600">
                  {pillar.tag}
                </span>
              </div>
              <h3 className="mt-4 text-base font-semibold text-slate-900">
                {pillar.title}
              </h3>
              <p className="mt-2 text-sm leading-relaxed text-slate-600">
                {pillar.body}
              </p>
            </div>
          ))}
        </div>

        <div className="mt-10 flex flex-col items-center justify-center gap-3 rounded-2xl bg-slate-100/80 px-6 py-6 text-center sm:flex-row sm:gap-5">
          <p className="text-sm font-medium text-slate-700">
            One file format, end to end:
          </p>
          <p className="font-mono text-xs leading-relaxed text-indigo-700">
            {"[ 16-byte salt ][ 24-byte nonce ][ Poly1305 MAC || ciphertext ]"}
          </p>
        </div>
      </div>
    </section>
  );
}
