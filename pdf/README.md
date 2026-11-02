# Comparing "The Tragedy of Complexity": Oehmke & Zawadowski vs. Foglietta

> **Version 3** — updated after structural analysis of Foglietta's four satellite articles (#350, #351, #354, #355), which he proposed as explanatory examples. Section 6 records how that analysis changed the convergence/divergence ledger. Underlying analyses: `notes-oehmke-zawadowski-tragedy-of-complexity.md`, `notes-foglietta-tragedy-of-complexity.md` (incl. the extended theory v2, primitives T7–T15).

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

## 5. Summary (pre-satellites)

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

### 6.4 Revised synthesis (one line)

> *The paper proves that competing suppliers of complexity/messages systematically distort what they offer when attention is a common resource; Foglietta's extended theory now describes the entire market around that mechanism — rational and degraded suppliers (including AI), rational and collapsed audiences, the institutional side effects that connect them without any conspiracy, and the educational/authoritative remedies that can re-anchor the system to reality. The paper remains the only formalized mechanism; the article is no longer merely the demand-side lament, but the surrounding account the model assumes away.*
