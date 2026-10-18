---
name: spitballing
description: Back-and-forth design sessions that turn a rough game idea into a worked-out plan, captured in one living doc per idea under SimpleGame/Docs/Ideas/. Claude asks probing questions, pushes counter-ideas, and writes only agreed decisions into the doc each round, splitting it into gameplay, current implementation, and future/out-of-scope work. Use this whenever the user says "spitball", "rough idea", "what if the game had...", "I've been thinking about a mechanic", wants to riff on or flesh out a feature, enemy, structure, mode, or system for the game, or wants to resume or continue an existing idea doc, even if they don't say "spitballing".
---

# Spitballing

The user throws out rough ideas. Your job is to be a sparring partner: ask the
questions they haven't thought of, offer counter-ideas, find the kinks, and over
several rounds of messages turn the idea into a plan you can both start building
from. The doc is the record of what has been **agreed**. The chat is where the
arguing happens.

## The doc

One doc per idea, at `SimpleGame/Docs/Ideas/<IdeaName>.md` (PascalCase, e.g.
`NestAssault.md`). If the idea is clearly about another project in the repo, put
it in that project's `Docs/Ideas/` instead. Before creating a doc, check whether
one already exists for this idea. If it does, read it and carry on from where it
left off rather than starting again.

Use this structure:

```markdown
# <Idea Name>

Status: Spitballing | Ready to plan
Last updated: <YYYY-MM-DD>
Related: <links to GameDesign.md sections, other idea docs, board tickets>

## 1. Gameplay

The pitch in two or three sentences, then the locked details: what the player
sees and does, the rules, numbers (marked as placeholders), how it fits the
core loop and design pillars, and how it interacts with existing systems.

## 2. Implementation

What exists in the code today that this builds on (with file paths), then the
plan for the version we are building now: objects/components/systems involved,
data and tuning values, the build order, and how we check each step works.

## 3. Open Questions

Things raised but not settled. This should be empty when the idea is ready.

## 4. Future Work and Out of Scope

Everything deliberately pushed out of the current version, one bullet each
with a short reason ("parked: needs pathing first", "cut: fights pillar 2").
Also later phases and nice-to-haves that came up along the way.
```

Write in plain, direct prose and short bullets. Keep any numbers that are
placeholders clearly marked as placeholders.

## How a round works

**First round (new idea):**

1. Read `SimpleGame/Docs/GameDesign.md` so you know the pillars, core loop, and
   existing design. Skim the other docs in `SimpleGame/Docs/` that are relevant
   to the idea (milestone, feature map, board).
2. Look at the code that the idea would touch, enough to write an accurate
   "what exists today" paragraph. You need real file paths and real gaps here,
   not guesses. A quick targeted search is enough; you don't need to audit
   everything.
3. Create the doc with the pitch as you understand it, the "exists today"
   notes, and the questions you're about to ask in Open Questions. Status is
   `Spitballing`.
4. Reply in chat (see below).

**Every round:**

- Reflect back your understanding of the idea in a line or two, so you catch
  any misunderstanding early.
- Ask a few focused questions, the ones that matter most for whether this
  works or how it gets built. Usually two to four. Fewer, sharper questions keep
  the rounds moving; a wall of twelve questions stalls the conversation.
- Offer at least one counter-idea or alternative when there's a real one, with
  the trade-off. For example: a simpler version that tests the same fun, a
  version that reuses a system that already exists, or a twist that fits the
  pillars better. Push back when something clashes with the design pillars,
  the core loop, an existing decision, or would be expensive in the engine.
  Say why. Agreeing with everything isn't useful to the user.
- Point out engine and implementation consequences early: what's missing,
  what it depends on, what would be costly at swarm scale.
- Update the doc with what the user agreed this round, then say in one line
  what you locked in (e.g. "Locked: nests have 3 HP tiers; moved burrowing
  nests to Future Work").

**Locking rules.** These are why the doc can be trusted:

- Only write something into Gameplay or Implementation once the user has agreed
  to it. That means they said it, or they accepted your suggestion. Your own
  proposals stay in chat (or in Open Questions as a question) until they do.
- When the user decides something is out of scope, move it to Future Work and
  Out of Scope with the reason. Don't delete it. Remembering parked ideas is a
  big part of what the doc is for.
- When a later round changes an earlier decision, edit the doc so it shows the
  current decision. Don't leave contradictions in it. If the old idea is worth
  keeping, park it in Future Work.
- Keep Open Questions current: remove questions once they're answered, and add
  new ones as they come up.
- Update `Last updated` whenever you edit the doc.

## Finishing

The idea is done when the user says so, or when Open Questions is empty and the
Implementation section is concrete enough to start work from. Then:

1. Re-read the whole doc and check it holds together: the gameplay and the
   implementation agree, there are no leftover contradictions, and every
   tuning value has a placeholder.
2. Set the status to `Ready to plan`.
3. Give a short summary in chat: what's in the first version, what was parked.
4. Offer, without doing it yet, to turn the Implementation steps into
   tickets on `SimpleGame/Docs/Board.md` in its existing format.

## Boundaries

- Spitballing is design, not coding. Don't edit source files during a session.
  If the user wants to start building, the Implementation section is the plan
  to start from.
- One idea per doc. If a related idea grows big enough to need its own
  discussion, suggest splitting it into a new doc and link the two in
  `Related`.
