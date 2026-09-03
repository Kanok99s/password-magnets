// Feature grid: six capability cards driven by the FEATURES dataset.

import { FEATURES, SectionHeading } from "./content";

export default function FeaturesSection() {
  return (
    <section id="features" className="scroll-mt-20 bg-white px-6 py-20 sm:py-24">
      <div className="mx-auto max-w-6xl">
        <SectionHeading
          index="01"
          eyebrow="What it does"
          title="A vault that feels like a vault"
          lede="The essentials of a password manager - none of the fluff. Everything happens locally, with feedback on every action."
        />
        <div className="grid gap-5 sm:grid-cols-2 lg:grid-cols-3">
          {FEATURES.map((feature) => (
            <div
              key={feature.title}
              className="group rounded-2xl border border-slate-200 bg-slate-50/50 p-6 transition-colors hover:border-indigo-200 hover:bg-white"
            >
              <div className="mb-4 grid h-11 w-11 place-items-center rounded-xl bg-indigo-100 text-indigo-600 ring-1 ring-inset ring-indigo-100 transition-colors group-hover:bg-indigo-600 group-hover:text-white">
                <feature.icon className="h-5 w-5" strokeWidth={1.9} />
              </div>
              <h3 className="text-base font-semibold text-slate-900">
                {feature.title}
              </h3>
              <p className="mt-2 text-sm leading-relaxed text-slate-600">
                {feature.body}
              </p>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
}
