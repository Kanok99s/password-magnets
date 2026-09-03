// Architecture: a dark pipeline band showing the four layers of the app and
// the flow between them, ending in the encrypted vault file.

import { ArrowRight } from "lucide-react";
import { SectionHeading, STAGES } from "./content";

export default function ArchitectureSection() {
  return (
    <section id="architecture" className="scroll-mt-20 bg-slate-900 px-6 py-20 sm:py-24">
      <div className="mx-auto max-w-6xl">
        <SectionHeading
          dark
          index="03"
          eyebrow="How it fits together"
          title="Four narrow layers, one encrypted file"
          lede="Data flows down to the disk only after it is sealed; nothing on the drive is ever plaintext."
        />
        <div className="flex flex-col items-stretch gap-3 lg:flex-row lg:items-center">
          {STAGES.map((stage, i) => (
            <div key={stage.name} className="contents">
              <div className="flex-1 rounded-2xl border border-slate-700/80 bg-slate-800/70 p-5">
                <div className="grid h-9 w-9 place-items-center rounded-lg bg-indigo-500/15 text-indigo-300 ring-1 ring-inset ring-indigo-400/20">
                  <stage.icon className="h-[18px] w-[18px]" strokeWidth={1.9} />
                </div>
                <p className="mt-3 font-display text-sm font-semibold text-white">{stage.name}</p>
                <p className="mt-1 text-xs leading-relaxed text-slate-400">{stage.sub}</p>
                <p className="mt-3 font-mono text-[11px] text-indigo-300">{stage.line}</p>
              </div>
              {i < STAGES.length - 1 && (
                <div className="flex items-center justify-center py-1 lg:px-1 lg:py-0">
                  <ArrowRight className="h-4 w-4 rotate-90 text-slate-600 lg:rotate-0" />
                </div>
              )}
            </div>
          ))}
        </div>
        <p className="mt-10 text-center text-xs leading-relaxed text-slate-500">
          Every mutation is written back to vault.bin immediately, and a
          headless --checkpoint round-trip check runs in CI on every commit.
        </p>
      </div>
    </section>
  );
}
