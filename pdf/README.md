## The Tragedy of Complexity

- [The Tragedy of Complexity by Foglietta](https://github.com/robang74/working-in-progress/blob/main/pdf/The-Tragedy-of-Complexity-with-Examples-and-AI-prompts.pdf) &nbsp; (2026)
    - [Structural Synthetic Analysis](https://github.com/robang74/working-in-progress/blob/main/pdf/notes-foglietta-tragedy-of-complexity.md) &nbsp; (notes)

- [The Tragedy of Complexity by Oehmke & Zawadowski](https://github.com/robang74/working-in-progress/blob/main/pdf/The-Tragedy-of-Complexity-in-Economics-by-Zawadowski.pdf) &nbsp; (2019)
    - [Structural Synthetic Analysis](https://github.com/robang74/working-in-progress/blob/main/pdf/notes-zawadowski-tragedy-of-complexity.md) &nbsp; (notes)

### Comparing Oehmke & Zawadowski vs. Foglietta

> **Version 6 (integrated).** Consolidated rewrite of versions 1–5: all update rounds (satellite articles #350–#355; older articles #302, #090 introducing TCMO; excluded mid-series articles #352, #353) are merged into a single final-state document. No superseded material remains. Underlying analyses: `notes-oehmke-zawadowski-tragedy-of-complexity.md` (full structural analysis of the paper) and `notes-foglietta-tragedy-of-complexity.md` (full structural analyses of nine Foglietta articles and the extended informal theory, primitives T1–T23).

#### References

- #349 -- [The Tragedy of Complexity](https://robang74.github.io/roberto-a-foglietta/html/349-the-tragedy-of-complexity.html) &nbsp; (2026-10-11)

- #350 -- [Il Terrore dell'invasione Russa](https://robang74.github.io/roberto-a-foglietta/html/350-il-terrore-dell-invasione-russa.html) &nbsp; (2026-10-16)

- #351 -- [Do not forget the roots](https://robang74.github.io/roberto-a-foglietta/html/351-do-not-forget-the-roots.html) &nbsp; (2026-10-18)

- #352 -- [Il problema da risolvere siete voi!](https://robang74.github.io/roberto-a-foglietta/html/352-il-problema-da-risolvere-siete-voi.html) &nbsp; (2026-10-20)

- #353 -- [La spudorata menzogna merita l'ardito furore](https://robang74.github.io/roberto-a-foglietta/html/353-la-spudorata-menzogna-merita-l-ardito-furore.html) &nbsp; (2026-10-23)

- #354 -- [Novelli Icaro mandati allo sbaraglio](https://robang74.github.io/roberto-a-foglietta/html/354-novelli-icaro-mandati-allo-sbaraglio.html) &nbsp; (2026-10-31)

- #302 -- [Politics and democracy for dummies](https://robang74.github.io/roberto-a-foglietta/html/302-politics-and-democracy-for-dummies.html) &nbsp; (2025-03-03)

- #090 -- [L'importanza del TCMO](https://robang74.github.io/roberto-a-foglietta/html/090-l-importanza-del-tcmo.html) &nbsp; (2017-10-13)

- tools -- [A collection of useful prompts](https://robang74.github.io/chatbots-for-fun/html/collection-of-useful-prompts-by-raf.html) &nbsp; (2025-12-23)

---

### 1. Same subject?

**Partially — and the overlap is deeper than a first reading suggests.** The two works share a title and a master intuition: *complexity is not the tragedy; the tragedy is a systematic distortion in how agents handle complexity when attention is scarce.* Their relationship is best stated at the end of the full analysis (§7), but the starting asymmetry is:

- **Oehmke & Zawadowski** model the **supply side** of the attention economy: competing producers choose the complexity of products/messages to capture a consumer's limited attention, generating an equilibrium externality. Demand is fully rational throughout.

- **Foglietta** begins on the **demand side**: audiences, voters, and professionals who should allocate attention to complex matters instead skip the "versus," because allegiance rewards simplification more than inquiry does.

Each text also *performs* its own diagnosis. The paper's medium is the attention-grabbing academic product it theorizes (punchy title, viral Excel quote, everyday examples); the article's medium is the attention market it critiques — its Premise concedes that "explaining by examples is greatly easier" and selects "the most hot-debated topics" because "they are also attention catching."

> **Version note (paper).** The analyzed PDF is the 2019 working paper; the published version (*Management Science*, online June 1, 2026, doi: 10.1287/mnsc.2024.08215) renames producers as **"suppliers"** and extends the model to **"suppliers of messages competing for the attention of a single party": financial firms facing regulatory bodies, and division heads reporting to a CEO.** The published model is thus explicitly a model of media/political/organizational discourse, not only of retail products — a fact that matters repeatedly below.

---

### 2. Structural isomorphisms (the stable core)

These survive every update round:

- **I1. Scarce common resource.** Both build on a hard budget: the consumer's time `T` in the paper; the reader's willingness-to-invest-effort in the article ("understanding a topic in a complex world isn't as cheap as having an opinion"). Both treat it as *common* — attention given to one good/message is taken from all others.

- **I2. Strategic distortion of complexity.** In the paper, distortion is an equilibrium best response (`∂t_i/∂c_i` ≠ 0 ⇒ producers deviate from the planner's optimum). In the article, distortion is a social best response: simplification "becomes useful not because it explains reality better, but because it makes allegiance easier to express." Both insist the distortion is *rational for the agent producing it* and harmful in aggregate — the signature of an externality.

- **I3. Direction-dependence: too complex AND too simple.** The paper's central surprise (Proposition 1): the externality can push *either way* — well-understood goods overcomplicated, poorly understood goods dumbed down, distortion toward intermediate complexity. Foglietta's article is built on the identical bipartition: overcomplicated regulation/smartphones vs. dumbed-down Brexit debates and the 3%-rule heuristic.

- **I4. Counterproductive remedies at the margin.** The paper's complexity paradox: more attention capacity `T` can *increase* excess complexity. The article's reception paradox: more sophisticated arguments may be *less* likely to be received. In both, adding resources or information does not cure the distortion because the distortion is incentive-driven, not capacity-driven.

- **I5. Diagnostics over moralizing.** The paper offers observable red flags: the software-update effect (value falls at fixed attention ⇒ overcomplication); `t_i/c_i ≷ 1` as the over/under-complication boundary. The article offers process red flags: the skipped "versus," the absent hypothesis → thesis → confutation → correction sequence, "stadium fanaticism."

---

### 3. Convergences — final-state ledger (C1–C6)

- **C1. No villains required.** The paper's agents are rational and the tragedy is still generic — the mechanism never needed bad intentions. Foglietta originally diagnosed a *psychological* coping device ("we need to believe there is a Great Evil Enemy… because we cannot accept that we are facing the consequences of our own decisions"); the corpus then converted this into a mechanism (#354, T10: side effects discovered → incentivized → institutionalized — "truffe esistono a prescindere che vi siano truffatori") and explained the belief itself (#090, T19: the scapegoat is the attractor of unpaid TCMO balances). The two theories meet from opposite directions at **systemic dysfunction without conspiracy**.

- **C2. Terminal states on both sides.** The paper has the consumer's "giving up" region (`t_i = 0` for very complex goods — nobody reads the terms and conditions). #354 adds the supplier-side terminal state (T11): agents who can no longer "distinguish the description of reality from reality, the map from the territory, the entry cost from the entry path." The tragedy has a completion condition on each side of the market.

- **C3. AI suppliers are inside both theories.** The published paper models "suppliers of messages competing for the attention of a single party" — a category that includes chatbots. #355 (T13) specifies the new supplier's pathology: internally coherent, edulcorated, single-narrative output (ChatGPT's account of the Putin call needed five explicit corrections; coherence ≠ correctness), and T14 establishes the counter-duty: correcting the machine as civic method-labor.

- **C4. Remedy skepticism — doctrinal and instrumental.** Doctrinally: the paper rejects a blanket "reduce complexity" mandate (complexity can be too low); the corpus rejects one-word institutional fixes ("no magic word, not even 'democracy'"). Instrumentally: T23 (#353) supplies Foglietta's closest analogue to the paper's remedy — Wales' Senedd Cymru Act 2026, a legal mechanism that re-prices a previously free externality (the brazen lie). Both theories end at a **re-pricing operation**.

- **C5. TCMO is the demand-side homologue of the paper's shadow price `λ`.** Suppliers persist in distorted complexity because they do not face the shadow cost of the attention they divert; agents persist in error because they do not face the total cost of the mistakes they externalize. The paper's planner ↔ Foglietta's consequence-coupled society; attention permits ↔ TCMO accounting. Both theories are, at bottom, theories of **unpriced externalities** — one in the attention market, one in the mistake market.

- **C6. Occasional formalism.** #352's covariance argument (relativistic consistency forbids privileged frames; fiat seigniorage is a privileged frame) shows the informal theory can reason formally on demand — narrowing the genre divergence from "math vs. rhetoric" to "systematic formalism vs. occasional formal argument."

---

### 4. Divergences — final-state ledger (D1–D6)

All divergences are now grounded in dated corpus articles:

- **D1. Rationality on the supply side.** The paper's producers optimize perfectly forever. #354's "idiozia terminale" names agents whose sense of reality has ceased ("terminal cretins… or cretins manipulated as puppets") — a behavioral decay layer the model excludes by assumption. In the model, giving up happens only on the demand side; in the extended theory, suppliers degrade too (T16 grounds the individual driver: mistake-ownership avoidance).

- **D2. The value function itself can lie.** The paper takes `v_i(c_i, t_i/c_i)` as *true* surplus — fraud has no representation. #354 attacks exactly this: Theranos was a misrepresented value function (business plan without PoC); fiat/QE and equity-swap-driven "products" decouple *represented* value from *producible* value. TCMO (#090) is the ledger of that divergence; #352 speaks it natively ("liability transfer as in all bankruptcies… not to hold the hot potato").

- **D3. Domain-boundary failures.** #354's Software Bias (T9): practices rational in one domain (IT: MVP, mocked prototypes, "fake it till you make it") become fraud when exported across an undebated domain boundary (atoms, medicine). The model has a single complexity dimension `c_i` within one domain.

- **D4. Tacit, embodied complexity.** #355 (T15): relational complexity (43% of Ukrainians with close relatives in Russia alongside only 6% desiring unification — "the complexity of the relations was ignored") is only partially visible to statistics and inaccessible to intelligence that has not "walked the alleys" — a limit asserted for humans and superintelligences alike. The model assumes complexity is fully representable in a function.

- **D5. Statics vs. dynamics.** The sharpest formal gap: the model is static and mistake-free (`v` is real, agents optimize correctly). TCMO's core phenomena — compounding error (the growing ball: ping-pong → tennis → billiard → bowling), latency-to-state-change catastrophe, the denial ladder — have no representation. The giving-up region is the paper's only dynamic; the informal theory supplies the missing ones (T17).

- **D6. Organizations and agency types.** #352's T20 ("N × not understanding a damn thing = not understanding a damn thing": 400 → 4K → 40K pages) — organizations multiply non-understanding, never comprehension; and its T21 (asymmetric fractal replacement, the "metastasis" strategy) is an operational remedy for agents too small to confront a system frontally. A model of individual producers with one consumer has no organizations, coalitions, or agency types to represent either.

---

### 5. Cross-reading

**What the paper adds to the corpus.** Foglietta observes that debate is dumbed down and asks why the versus is skipped; the paper provides a candidate micro-foundation: when many message-suppliers compete for one audience's attention, equilibrium complexity is pushed toward intermediate levels, and the planner's optimum is never reached. The "curse of variety" (more goods ⇒ dumbing down) formalizes the complaint about proliferating online outlets; the complexity paradox formalizes the observation that more information-processing capacity has not produced more understanding. Even Foglietta's rhetorical strategy (hot examples, short punchy text) is what a supplier *should* do in the paper's equilibrium.

**What the corpus adds to the paper.** The paper assumes the consumer mechanically allocates attention optimally; it cannot explain why a whole society would stop valuing the *method* of understanding. The corpus supplies that demand-side failure (T1/T2), its incentive micro-foundation (T16: engaging the versus means personally owning mistake risk; conforming dilutes it across the crowd), the cost account of the whole process (TCMO: the skipped versus compounds into a collective balance that surfaces as sudden catastrophe), the organizational scaling law (T20), the remedy space (coupling restoration: Pikalevo, re-scholarization, participation, speech-coupling), and the AI-turn of the whole problem (T13–T15).

---

### 6. The extended informal theory — primitive map

| Tier | Primitives | Content | Source |
|---|---|---|---|
| **Core** (T1–T6) | T1 skipped versus / stadium fanaticism; T2 forgotten method; T3 scale invariance {domain, scenario, time}; T4 institutionalized fallacies; T5 iceberg ("how many aspects ignored?"); T6 self-referential method (OK/KO self-review; AI as 3rd-party PoV) | The tragedy stated | #349 (2026) |
| **Mechanisms** (T7–T15) | T7 self-hosted versus; T8 budget-theater; T9 software bias (domain export); T10 emergence without conspirators; T11 terminal alienation; T12 correction problem (Pikalevo, re-scholarization); T13 machine simplifier; T14 correcting the machine; T15 embodied vs. statistical knowledge | The tragedy mechanized, incl. the AI turn | #350, #354, #355 (2025–26) |
| **Cost account** (T16–T19) | T16 mistake-ownership avoidance (incentive micro-foundation of T1); T17 compounding & catastrophe latency (denial ladder); T18 decision–consequence coupling (unified remedy); T19 scapegoat as attractor of unpaid TCMO; (+ speculative anthropic extension) | Why the tragedy persists and how it ends | #090 (2017, the seed), #302 (2025) |
| **Organization & institution** (T20–T23) | T20 non-scalability of understanding; T21 asymmetric fractal replacement; T22 covariance argument against privilege; T23 speech-coupling (Wales Act precedent) | The tragedy at organizational and legal scale | #352, #353 (2026, excluded from the collection) |

Chronology matters: the cost account (#090, 2017) *pre-dates* the tragedy article (#349, 2026) by nine years — the seed, not the corollary.

---

### 7. Collection assessment

Nine articles analyzed. Significance tiers:

- **Theory core (indispensable):** #349 (statement), #354 (mechanisms T9–T12), #090 (cost account T16–T19), #355 (meta-level T13–T15).

- **Methodological cluster:** the prompts page + #302 (embedded SoNia/AleX toolchain; Athens vs. modern contrast = purest statement of T18).

- **Example tier (collapsible):** #350 + #351 (grand-strategy scenario + historical roots; keepers: T7 self-hosted versus, T8 budget-theater).

- **Wrongly excluded:** #352 (T20–T22 + the fullest AI-research demonstration; more extension material than included #350) and #353 (T23, the only implemented institutional remedy). The rational editorial cut was to *merge* #350+#351 and *split* #352 — not to exclude.

---

### 8. Final synthesis

> *The paper proves that competing suppliers of complexity/messages systematically distort what they offer when attention is a common resource — a static, rational-agent account of an unpriced externality. Foglietta's extended theory describes the entire market around that mechanism — rational and degraded suppliers (including AI), rational and collapsed audiences — and the cost account beneath it: agents skip the versus because mistake-ownership is individually expensive and collectively dilutable; the skipped versus compounds into a collective TCMO balance that surfaces as sudden catastrophe; organizations scale non-understanding faster than comprehension; and the remedies converge on one operation — re-pricing what was free. The paper formalizes the attention externality; the informal theory adds the mistake externality; and the Wales Act shows both pointing at the same instrument: make the agent face the cost they currently export.*
