# Chapter 17: what a real-time fieldbus would change

**There is no code in this directory, and that is the chapter.**

Sixteen chapters built a joint node on a bus that is good enough for a joint and
is not what a serious machine uses between its controller and its axes. This
chapter says what the difference is worth, what it would cost, and why this
volume stops short of it. The deliverable is one document.

## What runs

```bash
python tools/check_decision.py    # the document still has its shape and its sources
```

That is the whole suite. There is nothing to compile and nothing to flash,
because nothing was built and nothing could be: the hardware the chapter is
about is not on this bench and could not be put there.

## Why there is no code

The chapter's own key facts put it plainly: real or modelled, **neither**.
Everything here is read, quoted, priced and decided.

Two findings make it structural rather than a matter of effort. This part has no
Ethernet controller, which chapter 4 established from the **absence** of two
interrupt vector slots in the vendor's own header rather than from a product
page. And this silicon vendor sells no subdevice controller at all. Writing more
firmware does not reach the other side of either.

A chapter that quietly skipped the subject would leave a reader unable to tell a
structural limit from a gap in the work. A chapter that pretended to build it
would be worse.

## What the check can and cannot do

`check_decision.py` checks two things, because they are the two that can be
checked mechanically:

- the four headings the chapter specifies are all present, since a decision that
  has quietly lost "what would change it" is an excuse rather than a decision
- every claim row carries a provenance marker, so a claim whose source went
  missing during an edit fails rather than surviving to be quoted at somebody

It checks shape, not truth. No program can tell whether a subscription is still
annual or whether a licence changed again last month. The licence section says
so itself: those are the most perishable claims in the document and the ones
most likely to matter, and settling them again needs a person and a browser.

Both rules were shown to fail before they were trusted. Removing a heading fails
the first; the three-route table failed the second on its first run, because it
made three claims with no source, and the document was corrected rather than the
check loosened.

## Provenance markers

| Marker | Means |
|---|---|
| `[quoted]` | a sentence copied from a document that was opened |
| `[read]` | read from a document that was opened, not quoted word for word |
| `[measured]` | measured in this volume, on this bench |
| `[unconfirmed]` | looked for and not settled, and written as open rather than guessed |

One claim in the document carries `[unconfirmed]` and stays that way: the
kernel-space stack has a GPLv2 file and an LGPLv2.1 file at its root and its
readme does not say which covers which half. The conventional split is the
obvious assumption and it is not written down as a fact here.

## Acceptance criteria, and which are covered

| Criterion | Covered by |
|---|---|
| The premise settled from documents, not impressions | the two structural findings, both `[read]` |
| What it would buy, in figures | the synchronisation and throughput table |
| What it would cost, in parts, weeks and obligations | the cost table, including the two corrected assumptions |
| Why not here | the absence of an Ethernet controller and of a subdevice controller |
| What would change it | three routes, each costed, none built |
| Every claim says where it came from | `check_decision.py`, 21 claim rows |
| Whether any of it is still true | **not covered, and not coverable here**, see above |
