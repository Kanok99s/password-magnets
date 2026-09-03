// CSS-drawn preview of the PasswordMagnets desktop vault window. No real
// screenshots exist yet, so this markup stands in for one: a title bar, a
// live-search row, the entry table with masked secrets, and the action bar.

import { MOCK_ROWS } from "./content";

function SearchIcon() {
  return (
    <svg
      className="h-3.5 w-3.5 text-slate-400"
      viewBox="0 0 24 24"
      fill="none"
      stroke="currentColor"
      strokeWidth="2"
      strokeLinecap="round"
      strokeLinejoin="round"
      aria-hidden="true"
    >
      <circle cx="11" cy="11" r="8" />
      <path d="m21 21-4.3-4.3" />
    </svg>
  );
}

export default function VaultWindowPreview() {
  return (
    <div className="mx-auto w-full max-w-3xl">
      <div className="overflow-hidden rounded-2xl border border-slate-200 bg-white text-left shadow-xl shadow-slate-900/5 ring-1 ring-slate-900/5">
        <div className="flex items-center gap-3 border-b border-slate-100 bg-slate-50 px-4 py-2.5">
          <div className="flex gap-1.5">
            <span className="h-2.5 w-2.5 rounded-full bg-rose-300" />
            <span className="h-2.5 w-2.5 rounded-full bg-amber-300" />
            <span className="h-2.5 w-2.5 rounded-full bg-emerald-300" />
          </div>
          <p className="flex-1 truncate text-center font-mono text-[11px] font-medium text-slate-500">
            PasswordMagnets - Vault
          </p>
          <span className="rounded-md bg-indigo-100 px-2 py-0.5 font-mono text-[10px] font-medium text-indigo-700">
            Backup menu
          </span>
        </div>
        <div className="flex items-center gap-3 px-4 pt-4">
          <div className="flex flex-1 items-center gap-2 rounded-lg border border-slate-200 bg-slate-50 px-3 py-1.5">
            <SearchIcon />
            <span className="text-xs text-slate-400">
              Search service or username...
            </span>
          </div>
          <span className="text-xs text-slate-400">4 of 4 entries</span>
        </div>
        <div className="px-2 pb-2 pt-3">
          <div className="grid grid-cols-[2fr_2fr_2fr_4.5rem] items-center gap-2 border-b border-slate-100 px-3 py-2">
            {["Service", "Username", "Password", "Actions"].map((h) => (
              <span
                key={h}
                className="text-[10px] font-semibold uppercase tracking-wider text-slate-400"
              >
                {h}
              </span>
            ))}
          </div>
          {MOCK_ROWS.map((row, i) => (
            <div
              key={row.service}
              className={`grid grid-cols-[2fr_2fr_2fr_4.5rem] items-center gap-2 px-3 py-2.5 text-sm ${
                i % 2 === 1 ? "bg-slate-50/70" : ""
              }`}
            >
              <span className="truncate font-medium text-slate-800">
                {row.service}
              </span>
              <span className="truncate text-slate-500">{row.username}</span>
              <span className="truncate font-mono text-xs tracking-widest text-slate-600">
                {"\u2022".repeat(row.dots)}
              </span>
              <span className="justify-self-start text-xs font-semibold text-indigo-600">
                Copy
              </span>
            </div>
          ))}
        </div>
        <div className="flex items-center gap-2 border-t border-slate-100 bg-slate-50/60 px-4 py-3">
          <span className="rounded-lg bg-indigo-600 px-3.5 py-1.5 text-xs font-semibold text-white">
            Add Entry
          </span>
          <span className="rounded-lg border border-slate-300 bg-white px-3 py-1.5 text-xs font-medium text-slate-700">
            Edit Entry
          </span>
          <span className="rounded-lg border border-red-300 bg-white px-3 py-1.5 text-xs font-medium text-red-600">
            Delete Entry
          </span>
          <span className="ml-auto rounded-lg border border-slate-300 bg-white px-3 py-1.5 text-xs font-medium text-slate-700">
            Lock Vault
          </span>
        </div>
      </div>
      <p className="mt-4 text-center text-xs leading-relaxed text-slate-500">
        The Qt Widgets vault window: live ranking search, masked secrets,
        one-click copy with a 20-second clipboard sweep, and encrypted backups.
      </p>
    </div>
  );
}
