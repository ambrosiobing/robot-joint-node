# Chapter 4: the board support package

Mostly not written. What is here is the rule chapter 4 imposes on every chapter
after it, which is the part that has to exist first or it cannot exist at all.

## What runs

```bash
python check_layers.py
```

Nothing above the board layer may include a vendor header. The checker walks the
C in this repository, skips `src/bsp/`, and fails with the file and the include
when anything else names the silicon.

## Why this one check came before the rest of the chapter

The moment a control loop includes a peripheral header, the control loop is tied
to one silicon vendor and cannot be built on a host machine. Chapter 20 has to
build most of this node on a host with no hardware at all, and that is only
possible if the rule was kept from here onward rather than retrofitted at the
end. Retrofitting it means untangling every include in the tree at the point
where the deadline is closest.

So the rule ships before the thing it protects. It currently passes trivially,
because there is very little C to check, and it was confirmed to fail by planting
an include that breaks it.

## What chapter 4 still owes

Its actual deliverable is a board file another engineer can read, and none of it
exists:

- the pin map, as a description rather than as comments scattered through code
- the clock configuration, with the arithmetic that produces it
- the peripheral table: which peripheral, which mode, which clock, which pins,
  which interrupts and transfers

That file is the input to most later chapters, so it is the highest-value thing
missing from this tree. It is not started, and saying it is started would make
every chapter that cites it look further along than it is.
