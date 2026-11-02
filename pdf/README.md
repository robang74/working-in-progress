# Comparing "The Tragedy of Complexity": Oehmke & Zawadowski vs. Foglietta

> Version 2 — rewritten after applying Foglietta's *Structural Synthetic Analysis* prompt to both full texts (skeleton scan → indexed forward/backward pass → synthesis → essay → support check). See `notes-oehmke-zawadowski-tragedy-of-complexity.md` and `notes-foglietta-tragedy-of-complexity.md` for the underlying analyses.

---

## 1. Same subject?

**Partially — and the overlap is deeper than the first edition of this comparison recognized.** The two works share a title and a master intuition: *complexity is not the tragedy; the tragedy is a systematic distortion in how agents handle complexity when attention is scarce.* But they occupy complementary positions in one phenomenon:

- **Oehmke & Zawadowski** model the **supply side**: competing producers choose the complexity of products/messages to capture a consumer's limited attention, generating an equilibrium externality.
- **Foglietta** describes the **demand side and its collapse**: audiences, voters, and professionals who should allocate attention to complex matters instead skip the "versus," because allegiance rewards simplification more than inquiry does.

Each text also *performs* its own diagnosis. The paper's medium is exactly the attention-grabbing academic product it theorizes (a punchy title, a viral Excel quote, everyday examples); the article's medium is exactly the attention market it critiques — its own Premise concedes that "explaining by examples is greatly easier" and selects "the most hot-debated topics" because "they are also attention catching." Two texts, one economy of attention, opposite ends of it.

> **Version note.** The analyzed paper PDF is the 2019 working paper; the published version (*Management Science*, online June 1, 2026, doi: 10.1287/mnsc.2024.08215) renames producers as **"suppliers"** and effective attention as **"depth of understanding,"** and — critically for this comparison — **extends the model to "suppliers of messages competing for the attention of a single party": financial firms facing regulatory bodies, and division heads reporting to a CEO.** The published model is thus *explicitly* a model of media/political/organizational discourse, not only of retail products. This dissolves much of the apparent distance between the two works.

---

## 2. Structural isomorphisms (the deep overlap)

Applying structural analysis to both texts reveals five genuine isomorphisms — not loose analogies:

**I1. Scarce common resource.** Both build on a hard budget: the consumer's time `T` in the paper; the reader's willingness-to-invest-effort in the article ("understanding a topic in a complex world isn't as cheap as having an opinion"). Both treat it as *common* — attention given to one good/message is taken from all others.

**I2. Strategic distortion of complexity.** In the paper, distortion is an equilibrium best response (`∂t_i/∂c_i` ≠ 0 ⇒ producers deviate from the planner's optimum). In the article, distortion is a social best response: simplification "becomes useful not because it explains reality better, but because it makes allegiance easier to express." Both insist the distortion is *rational for the agent producing it* and harmful in aggregate — that is precisely the signature of an externality.

**I3. Direction-dependence: too complex AND too simple.** The paper's central surprise (Proposition 1) is that the externality can push *either way*: well-understood goods are overcomplicated, poorly understood goods are dumbed down, with distortion toward intermediate complexity. Foglietta's article is built on the identical bipartition: overcomplicated regulation/smartphones on one side, dumbed-down Brexit debates and 3%-rule heuristics on the other. Neither tragedy is "too much complexity" alone.

**I4. Counterproductive remedies at the margin.** The paper's complexity paradox: more attention capacity `T` can *increase* excess complexity. The article's reception paradox: more sophisticated arguments may be *less* likely to be received, "because understanding it requires suspending the ready-made positions through which the audience has learned to interpret the world." In both, adding resources/information does not cure the distortion because the distortion is incentive-driven, not capacity-driven.

**I5. Diagnostics over moralizing.** The paper offers observable red flags: the software-update effect (value falls at fixed attention ⇒ overcomplication), and `t_i/c_i ≷ 1` as the over/under-complication boundary. The article offers process red flags: the skipped "versus," the absent sequence hypothesis → thesis → confutation → correction, "stadium fanaticism" (conclusion first, arguments recruited second). Both supply *tests you can apply to a product or a debate*, rather than a blanket condemnation of complexity or simplicity.

---

## 3. Where they genuinely differ

| Dimension | Oehmke & Zawadowski | Foglietta |
|---|---|---|
| **Genre / proof standard** | Formal model: FOCs, propositions, proofs, comparative statics | Divulgative essay (LinkedIn transcription, CC BY-NC-ND); argument by curated examples and self-review |
| **Mechanism** | *Equilibrium externality*: rational, self-interested suppliers ignore the shadow cost of attention they impose on competitors | *Epistemic erosion*: the method of structured inquiry is not merely unused but "not even contemplated"; belonging outbids inquiry |
| **Failure location** | In the incentives of producers (supply) | In the dispositions of audiences and institutions (demand), which then *reward* distorted supply |
| **Benchmark** | Benevolent social planner maximizing total surplus | Ideal deliberative process (Marcus Aurelius: 10,000 uninformed opinions = no value) |
| **Comparative statics axes** | `T` (attention), `N` (number of goods), `θ` (surplus share), `w, δ, α` (good characteristics) | Scale triad: **domain** (Australia/Italy geography), **scenario** (Siberia readings), **time** (diesel as option on time) |
| **Remedy** | Internalize the shadow cost of attention (the paper itself warns a naive "reduce complexity" mandate is wrong, since complexity can also be too low) | Restore method-awareness: stage the versus, run hypothesis→confutation→correction |
| **Self-application** | Media/politicians example inside the model (Brexit dumbing-down); "giving up" region = nobody reads terms and conditions | The article peer-reviews *itself* (the OK/KO block converts its own EU overclaim into a defensible core) and embeds its own AI-usage protocol |
| **Fallacy handling** | Agents are rational; inefficiency comes from missing markets, not bad reasoning | Institutionalized fallacies are a first-class object (3% rule: overgeneralization + coincidence≠causation) |

Note the complementarity in the last two rows: the paper assumes away reasoning errors to isolate the externality; the article assumes the externality is already priced in and focuses on the reasoning/method collapse that the model cannot see. Read together they form a supply-and-demand account of the same tragedy.

---

## 4. Cross-reading: what each text explains about the other

**What the paper adds to the article.** Foglietta observes that debate is dumbed down and asks why the versus is skipped; the paper provides a candidate micro-foundation: when many message-suppliers compete for one audience's attention, equilibrium complexity is pushed toward intermediate levels — complex enough to grab attention, simple enough to process — and the planner's optimum is never reached. The "curse of variety" (Proposition 5, more goods ⇒ dumbing down) is a formal version of the article's complaint about proliferating online outlets; the complexity paradox is a formal version of Foglietta's observation that more information processing capacity has not produced more understanding. Even Foglietta's rhetorical strategy (hot examples, short punchy text) is what a supplier *should* do in the paper's equilibrium.

**What the article adds to the paper.** The paper assumes the consumer mechanically allocates attention optimally given her budget; it cannot explain why a whole society would stop valuing the *method* of understanding. Foglietta supplies that missing demand-side failure: when allegiance pays more than inquiry, the shadow price of attention stops disciplining suppliers — audiences no longer punish dumbing down. The article also extends the tragedy beyond markets to non-market suppliers (regulators, media, politicians) — which the published version of the paper then formalizes. And the article's 3%-rule case shows a failure mode the model lacks entirely: *institutionalized fallacies*, where the simplified heuristic survives not because it grabs attention but because it became a rule.

**Convergence on the hardest point.** Both texts refuse the naive remedy. The paper: "a regulation that simply aims to reduce complexity is not the right policy... complexity can be too high or too low." The article: presenting more sophisticated arguments "does not necessarily solve the problem. Its sophistication may be precisely what prevents it from being received." Two independent routes to the same conclusion: you cannot fix an incentive/method failure by adding content.

---

## 5. Summary

The two works are **not** the same subject, but they are **the two halves of one subject**. Oehmke & Zawadowski prove that when attention is a common resource, competing suppliers will systematically distort the complexity of what they offer — in either direction. Foglietta shows that the audience's side of that market can rot as well: the versus gets skipped, the method of inquiry is forgotten, and societies reward allegiance over understanding. The published version of the paper closes most of the remaining gap by modeling "suppliers of messages" (regulators, executives) — the exact agents Foglietta discusses. The cleanest one-line synthesis:

> *The paper formalizes why the supply of complexity is distorted; the article documents why the demand for understanding collapses. The tragedy needs both halves — and each text, fittingly, fights for attention with the other's tools.*
