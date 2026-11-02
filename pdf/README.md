# Comparing "The Tragedy of Complexity": Oehmke & Zawadowski vs. Foglietta

> **Version 5** — updated after structural analysis of the four satellite articles (#350–#355), the two older articles (#302, #090 — TCMO), and the two excluded mid-series articles (#352, #353). Sections 6–8 record how that analysis changed the convergence/divergence ledger. Underlying analyses: `notes-oehmke-zawadowski-tragedy-of-complexity.md`, `notes-foglietta-tragedy-of-complexity.md` (extended theory with primitives T7–T23).

---

## 1. Same subject?

**Partially — and the overlap is deeper than version 1 of this comparison recognized.** The two works share a title and a master intuition: *complexity is not the tragedy; the tragedy is a systematic distortion in how agents handle complexity when attention is scarce.* But they occupy complementary positions in one phenomenon:

- **Oehmke & Zawadowski** model the **supply side**: competing producers choose the complexity of products/messages to capture a consumer's limited attention, generating an equilibrium externality.

- **Foglietta** describes the **demand side and its collapse**: audiences, voters, and professionals who should allocate attention to complex matters instead skip the "versus," because allegiance rewards simplification more than inquiry does.

Each text also *performs* its own diagnosis. The paper's medium is the attention-grabbing academic product it theorizes (punchy title, viral Excel quote, everyday examples); the article's medium is the attention market it critiques — its Premise concedes that "explaining by examples is greatly easier" and selects "the most hot-debated topics" because "they are also attention catching."

> **Version note.** The analyzed paper PDF is the 2019 working paper; the published version (*Management Science*, online June 1, 2026, doi: 10.1287/mnsc.2024.08215) renames producers as **"suppliers"** and extends the model to **"suppliers of messages competing for the attention of a single party": financial firms facing regulatory bodies, and division heads reporting to a CEO.** The published model is thus explicitly a model of media/political/organizational discourse, not only of retail products.

---

## 2. Structural isomorphisms (the deep overlap)

- **I1. Scarce common resource.** Both build on a hard budget: the consumer's time `T` in the paper; the reader's willingness-to-invest-effort in the article ("understanding a topic in a complex world isn't as cheap as having an opinion"). Both treat it as *common* — attention given to one good/message is taken from all others.

- **I2. Strategic distortion of complexity.** In the paper, distortion is an equilibrium best response (`∂t_i/∂c_i` ≠ 0 ⇒ producers deviate from the planner's optimum). In the article, distortion is a social best response: simplification "becomes useful not because it explains reality better, but because it makes allegiance easier to express." Both insist the distortion is *rational for the agent producing it* and harmful in aggregate — the signature of an externality.

- **I3. Direction-dependence: too complex AND too simple.** The paper's central surprise (Proposition 1): the externality can push *either way* — well-understood goods overcomplicated, poorly understood goods dumbed down, distortion toward intermediate complexity. Foglietta's article is built on the identical bipartition: overcomplicated regulation/smartphones vs. dumbed-down Brexit debates and the 3%-rule heuristic.

- **I4. Counterproductive remedies at the margin.** The paper's complexity paradox: more attention capacity `T` can *increase* excess complexity. The article's reception paradox: more sophisticated arguments may be *less* likely to be received. In both, adding resources/information does not cure the distortion because the distortion is incentive-driven, not capacity-driven.

- **I5. Diagnostics over moralizing.** The paper offers observable red flags: the software-update effect (value falls at fixed attention ⇒ overcomplication); `t_i/c_i ≷ 1` as the over/under-complication boundary. The article offers process red flags: the skipped "versus," the absent hypothesis → thesis → confutation → correction sequence, "stadium fanaticism."

---

## 3. Where they genuinely differ

| Dimension | Oehmke & Zawadowski | Foglietta |
|---|---|---|
| **Genre / proof standard** | Formal model: FOCs, propositions, proofs, comparative statics | Divulgative essay (LinkedIn transcription, CC BY-NC-ND); argument by curated examples and self-review |
| **Mechanism** | *Equilibrium externality*: rational, self-interested suppliers ignore the shadow cost of attention they impose on competitors | *Epistemic erosion*: the method of structured inquiry is not merely unused but "not even contemplated"; belonging outbids inquiry |
| **Failure location** | In the incentives of producers (supply) | In the dispositions of audiences and institutions (demand), which then *reward* distorted supply |
| **Benchmark** | Benevolent social planner maximizing total surplus | Ideal deliberative process (Marcus Aurelius: 10,000 uninformed opinions = no value) |
| **Comparative statics axes** | `T`, `N`, `θ`, `w, δ, α` | Scale triad: **domain** (Australia/Italy), **scenario** (Siberia), **time** (diesel as option on time) |
| **Remedy** | Internalize the shadow cost of attention (with an explicit warning that a naive "reduce complexity" mandate is wrong) | Restore method-awareness: stage the versus, run hypothesis→confutation→correction |
| **Self-application** | Media/politicians example inside the model (Brexit dumbing-down); "giving up" region = nobody reads terms and conditions | The article peer-reviews *itself* (the OK/KO block) and embeds its own AI-usage protocol |
| **Fallacy handling** | Agents are rational; inefficiency comes from missing markets, not bad reasoning | Institutionalized fallacies are a first-class object (3% rule: overgeneralization + coincidence≠causation) |

---

## 4. Cross-reading: what each text explains about the other

**What the paper adds to the article.** Foglietta observes that debate is dumbed down and asks why the versus is skipped; the paper provides a candidate micro-foundation: when many message-suppliers compete for one audience's attention, equilibrium complexity is pushed toward intermediate levels, and the planner's optimum is never reached. The "curse of variety" (more goods ⇒ dumbing down) formalizes the complaint about proliferating online outlets; the complexity paradox formalizes the observation that more information-processing capacity has not produced more understanding. Even Foglietta's rhetorical strategy (hot examples, short punchy text) is what a supplier *should* do in the paper's equilibrium.

**What the article adds to the paper.** The paper assumes the consumer mechanically allocates attention optimally; it cannot explain why a whole society would stop valuing the *method* of understanding. Foglietta supplies that demand-side failure. The article also extends the tragedy beyond markets to non-market suppliers (regulators, media, politicians) — which the published version of the paper then formalizes.

**Convergence on the hardest point.** Both texts refuse the naive remedy: the paper ("a regulation that simply aims to reduce complexity is not the right policy… complexity can be too high or too low") and the article (sophistication "may be precisely what prevents [an argument] from being received").

---

## 5. Summary (pre-satellites — superseded by §6–7)

The two works are **not** the same subject, but they are **the two halves of one subject**: the paper proves that when attention is a common resource, competing suppliers systematically distort the complexity of what they offer; the article documents the audience-side rot that lets the distortion go unpunished.

---

## 6. UPDATE — How the four satellite articles changed the ledger

> Analyzed with the same structural method; full results in the extended theory notes (T7–T15). Verdicts: **#350 and #351 are worked examples** (collapsible; grand-strategy "scenario" + "roots/time" dimensions) with two minor innovations; **#354 and #355 are genuine theory extensions**.

### 6.1 Convergences — strengthened

- **C1 (upgraded). No villains required.** The paper's agents are rational and the tragedy is still generic — the mechanism never needed bad intentions. Foglietta originally diagnosed a *psychological* coping device ("we need to believe there is a Great Evil Enemy… because we cannot accept that we are facing the consequences of our own decisions"). **#354 converts this into a mechanism (T10)**: "truffe esistono a prescindere che vi siano truffatori" — practices born for concrete reasons (casino chips, maybe for hygiene) have profitable side effects that get discovered and incentivized, producing aggregate dysfunction with no planner (the 5-step Executive-Summary mechanism). The two theories now meet at the same point from opposite directions: **systemic dysfunction without conspiracy**. This is the single most important convergence upgrade.

- **C2 (upgraded). Terminal states exist on both sides.** The paper has the consumer's "giving up" region (`t_i = 0` for very complex goods — nobody reads the terms and conditions). **#354 adds the supplier-side terminal state (T11)**: agents who can no longer "distinguish the description of reality from reality, the map from the territory, the entry cost from the entry path." The tragedy now has a completion condition on each side of the market, not only on the demand side.

- **C3 (new, direct). AI suppliers are now inside both theories.** The published paper models "suppliers of messages competing for the attention of a single party" — a category that includes chatbots. **#355 (T13) specifies the new supplier's pathology**: internally coherent, edulcorated, single-narrative output (ChatGPT's account of the Putin call needed five explicit (\*) corrections; "coherence ≠ correctness"). The paper's framework covers the *fact* of a new supplier; Foglietta supplies the *properties* that make this supplier dangerous (zero-marginal-cost personalization, smoothing/sycophancy, no experiential contact — T15).

- **C4 (extended). Remedy skepticism.** Already convergent in v2, now deepened: the paper rejects a blanket "reduce complexity" mandate; **#354 rejects one-word institutional fixes wholesale** — "there is no magic word, not even 'democracy'; a mature society conserves the capacity to recognize and correct errors" (Popper; Pikalevo as the constraint-restoration paradox).

### 6.2 Divergences — newly arisen

- **D1 (new). Rationality on the supply side.** The paper's producers optimize perfectly forever. **#354's "idiozia terminale"** names agents whose sense of reality has ceased ("terminal cretins… or cretins manipulated as puppets") — a behavioral decay layer the model excludes by assumption. In the model, giving up happens only on the demand side; in the extended informal theory, suppliers degrade too.

- **D2 (new). The value function itself can lie.** The paper takes `v_i(c_i, t_i/c_i)` as *true* surplus — fraud has no representation. **#354 attacks exactly this**: Theranos was a misrepresented value function (business plan without PoC); fiat/QE and equity-swap-driven "products" decouple *represented* value from *producible* value. This is a question the formal model cannot ask: what if the surplus being divided is fictional?

- **D3 (new). Domain-boundary failures.** **#354's Software Bias (T9)**: practices rational in one domain (IT: MVP, mocked prototypes, "fake it till you make it") become fraud when exported across an undebated domain boundary (atoms, medicine). The model has a single complexity dimension `c_i` within one domain; it has no representation of paradigm export.

- **D4 (new). Tacit, embodied complexity.** **#355 (T15)**: relational complexity (43% of Ukrainians with close relatives in Russia alongside only 6% desiring unification — "the complexity of the relations was ignored") is only partially visible to statistics and inaccessible to intelligence that has not "walked the alleys" — asserted as a limit for humans *and* superintelligences. The model assumes complexity is fully representable in a function; Foglietta now claims part of it is not.

- **D5 (shifted). Remedy level.** The paper's remedy is market design (internalize the shadow cost of attention; permits). **#354 shifts the remedy to education and constraint restoration** (re-scholarization as "defective-product recall" for college education; Pikalevo-style authority re-anchoring systems to reality, with the degeneration risk explicitly acknowledged). The gap is no longer just "economic vs. epistemic" but "mechanism design vs. cultural re-armament."

### 6.3 Items weakened or dissolved

- **Dissolved: the clean supply/demand division of labor (v2's framing).** After #354 (supply-side behavioral decay; consultancy "senior" tricks; incentive-system designers) and #355 (AI suppliers; correcting the machine as civic labor, T14), Foglietta's informal theory now spans **both halves of the attention market** — supply and demand — plus the epistemic infrastructure beneath them. The paper still models only the supply side against a fully rational demand side. The accurate current statement is: *the paper is a formal supply-side mechanism with rational demand; Foglietta is a whole-market account with behavioral and epistemic layers on both sides.*

- **Weakened: "the article assumes the externality is priced in."** v2 credited Foglietta with a demand-side failure the paper must assume away; T8 (budget-theater simplification, #350) and T10 (emergent institutionalization, #354) show Foglietta now also theorizes *supply-side* incentive structures — procurement budgets manufacturing conflict narratives — which sit much closer to the paper's own mechanism than anything in #349 did.

---

## 7. SECOND UPDATE — TCMO (#090) and the political-scale example (#302)

> Triggered by two older articles. **#302 — *Politics and Democracy for Dummies* (2025-03-03)**: worked example at the political-theory scale, part of the *methodological cluster* (it embeds the SoNia/AleX peer-review toolchain, and documents the machine contradicting its own quality rating on re-prompting — another T13 data point). **#090 — *L'importanza del TCMO* (2017; II ed. 2024-12-22)**: not an example at all — the **foundational cost account, written nine years before #349**. TCMO = *Total Cost of Mistake Ownership* = "the total cost of the perseverance of a mistake," cumulative like compound interest on unpaid debt, arising from the divergence between the decision model and the real model, and discharged onto whoever does not own the decision. Chronology matters: the cost account was the **seed**, the tragedy article the **flowering**.

### 7.1 Convergences — deepened again

- **C5 (new, structural). TCMO is the demand-side homologue of the paper's shadow price `λ`.** The paper: suppliers persist in distorted complexity because they do not face the shadow cost of the attention they divert; the planner internalizes it; tradable permits would price it. Foglietta: agents persist in error because they do not face the total cost of the mistakes they externalize; decision–consequence coupling would price it. **The paper's planner ↔ Foglietta's consequence-coupled society; attention permits ↔ TCMO accounting.** Both theories are, at bottom, theories of *unpriced externalities* — one in the attention market, one in the mistake market — and both remedies are re-pricing operations.

- **C1 (deepened). No villains required — now with a mechanism for the villain-belief itself.** #090's scapegoat analysis (T19: dissociation from accumulated losses generates the capro espiatorio) explains *why* the Great Evil Enemy belief is the attractor of unpaid TCMO balances. The paper never needed villains; Foglietta now explains why audiences nonetheless manufacture them.

- **C4 (deepened). Remedy skepticism — now unified.** #302 supplies the coupling argument in its purest historical form (Athens: "those voting for war were going to war" vs. modern representation: "who should f*ck all of us in our names"), and T18 unifies Foglietta's whole remedy layer — Pikalevo (forced re-coupling), re-scholarization (individual TCMO accounting), "partecipazione" (coupling by construction) — as one principle: **restore decision–consequence coupling**. The paper's warning against a naive "reduce complexity" mandate and #354's "no magic word, not even democracy" are the same principle seen from the supply and the institutional side.

### 7.2 Divergences — confirmed and grounded

T16–T19 now give the D1–D4 divergences a documented pedigree:

- **D1 (supply-side irrationality)** ← T16's converse: the paper's rational agents never face TCMO at all (no mistakes exist in the model); Foglietta's agents are *defined* by mistake-ownership avoidance (the skipped versus as risk-transfer).

- **D2 (fictional value)** ← T17's accounting: the paper's `v_i` is always real surplus; TCMO is precisely the ledger of represented-vs-real divergence (#354's Theranos/fiat material), which the model cannot record.

- **D3/D4 (domain boundaries, tacit complexity)** ← unchanged, but now integrated: TCMO dumping works only where latency hides it (open-system illusion), and embodied knowledge (T15) is the part of the ledger no statistical accounting reaches.

- **The paper's statics vs. the theory's dynamics** — the sharpest formal gap: the model has no intertemporal compounding, so it cannot represent TCMO's core phenomenon (the growing ball, the latency-to-state-change, the denial ladder). The giving-up region is the only dynamic the paper has; TCMO supplies the missing ones.

### 7.3 Revised synthesis (final)

> *The paper proves that competing suppliers of complexity/messages systematically distort what they offer when attention is a common resource — a static, rational-agent account of an unpriced externality. Foglietta's extended theory describes the entire market around that mechanism — rational and degraded suppliers (including AI), rational and collapsed audiences — and, beneath it, the cost account that sustains the whole configuration: agents skip the versus because mistake-ownership is individually expensive and collectively dilutable; the skipped versus compounds into a collective TCMO balance that surfaces as sudden catastrophe; and the only durable remedy is re-coupling decisions to their consequences. The paper formalizes the attention externality; the informal theory adds the mistake externality — and the two are homologous: `λ` prices diverted attention, TCMO prices externalized error.*

---

## 8. THIRD UPDATE — The two excluded articles (#352, #353): T20–T23

> Published between #351 and #354 but left out of Foglietta's collection. Analysis verdict: **both deserve inclusion — #352 carries more extension material than included #350; #353 is small but closes a load-bearing gap in the remedy layer** (exclusion was likely editorial: length and register, not insignificance).

### 8.1 What the excluded articles add

- **#352 — *Il problema da risolvere siete voi*** (sprawling; four articles fused into one): (i) **T20 — non-scalability of understanding** ("N × not understanding a damn thing = not understanding a damn thing": 400 → 4K → 40K pages) — the organizational face of emergent dysfunction; the model has no organizations, so no counterpart exists on the paper's side. (ii) **T21 — asymmetric fractal replacement** ("metastasis" strategy: diffusion model ≠ functioning model) — the corpus's only operational remedy for agents too small to confront a system frontally. (iii) **T22 — the covariance argument against fiat privilege** (relativistic consistency forbids privileged frames; seigniorage is a privileged frame) — Foglietta's rare move into formal reasoning, landing exactly on D2 (represented vs. real value); it even speaks TCMO natively ("liability transfer as in all bankruptcies… not to hold the hot potato"). (iv) The corpus's fullest AI-assisted research demonstration (85 sources / 35 selected / two AIs) on nuclear policy — T13/T14 at full scale. Caveat: its demographics polemic is its least method-guarded passage.

- **#353 — *La spudorata menzogna merita l'ardito furore*** (short): **T23 — speech-coupling as an institutional remedy**, with a working precedent: Wales' Senedd Cymru Act 2026 (removal of politicians for lying) — the first *implemented* re-pricing of discourse in the corpus. Plus the shameless lie as the terminal form of simplification (T1 degenerating into T11), and the self-deception substrate (post-hoc whys; continuous self-lying) grounding T16/T19 psychologically.

### 8.2 Ledger effects

- **C4/C5 (deepened, remedy convergence).** T23 is Foglietta's closest analogue to the paper's remedy: the Wales Act is a legal mechanism that re-prices a previously free externality (the brazen lie) — structurally the same operation as the paper's regulation that internalizes the shadow cost of attention, and as TCMO accounting. The remedy convergence is no longer only *doctrinal* (both reject naive mandates) but **instrumental**: both theories end at a pricing/coupling mechanism, and Foglietta's side now has a real-world instance.

- **C6 (new, stylistic).** T22 shows the informal theory can, on demand, reason formally (a coherence argument in the paper's own style) — narrowing the genre divergence from "math vs. rhetoric" to "systematic formalism vs. occasional formal argument."

- **D-series (extended).** T20 adds a clean new entry: organizations as non-understanding multipliers have no representation in a model of individual producers; and T21 (operational strategy) has no counterpart in a model without agency types or coalitions.

- **Collection-composition insight.** The included set is strong on diagnosis and mechanism (#349, #354, #355) but under-weights the remedy and organization layers; the two excluded articles carry exactly those. The rational cut was to merge #350+#351 and split #352 — not to exclude.

### 8.3 Final synthesis (v5)

> *The paper proves that competing suppliers of complexity/messages systematically distort what they offer when attention is a common resource — a static, rational-agent account of an unpriced externality. Foglietta's extended theory describes the entire market around that mechanism — rational and degraded suppliers (including AI), rational and collapsed audiences — and the cost account beneath it: agents skip the versus because mistake-ownership is individually expensive and collectively dilutable; the skipped versus compounds into a collective TCMO balance that surfaces as sudden catastrophe; organizations scale non-understanding faster than comprehension; and the remedies converge on one operation — re-pricing what was free: the paper would price diverted attention (permits), the informal theory prices externalized error (TCMO accounting, decision–consequence coupling, and now the speech-coupling precedent of the Wales Act). The paper formalizes the attention externality; the informal theory adds the mistake externality; and T23 shows both pointing at the same instrument — make the agent face the cost they currently export.*
