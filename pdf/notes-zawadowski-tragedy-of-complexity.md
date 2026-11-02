## Structural Synthetic Analysis — "The Tragedy of Complexity" (Oehmke & Zawadowski, 2019 WP)

> Method: Foglietta's *Structural Synthetic Analysis* prompt (skeleton scan → forward index with `[?→]` tags → backward resolution → synthesis → essay).
> Document analyzed: `/mnt/agents/upload/Zawadowski.pdf` — working paper version, September 6, 2019 (published version: *Management Science*, online June 1, 2026, with revisions: "suppliers", "depth of understanding", extended applications).
> 46 pp.: main text pp. 1–30, References pp. 31–32, Appendix A (proofs) pp. 33–42, Appendix B (microfoundation) pp. 43–45.

---

### Step 1 — Full Skeleton Scan

- **Abstract** — premise, attention externality, three main results, complexity paradox, curse of variety
- **1. Introduction** — motivation; literature positioning (obfuscation vs. value-enhancing complexity; Becker 1965; Sims 1998/2003; Bordalo et al. 2016)
- **2. Model Setup** — `N` producers, one consumer; value `v_i(c_i, t_i/c_i)`; Assumption 1 (`v_i(c_i,0)>0`); time budget `T`; quasi-linear utility
- **3. The Tragedy of Complexity**
  - 3.1 The Consumer's Problem — attention allocation, FOC (4), shadow price `λ`
  - 3.2 Equilibrium Complexity: The Producer's Problem — profit `θ_i·v_i`, FOCs (6)–(8), attention-grabbing effect `∂t_i/∂c_i`
  - 3.3 Optimal Complexity: The Planner's Problem — total surplus, FOCs (10)–(12); `N−1` extra reallocation terms
  - 3.4 The Complexity Externality — **Proposition 1** (tragedy of complexity); **Lemma 1** (equivalence results); **Proposition 2** (software update effect)
  - 3.5 Discussion
    - 3.5.1 Key Modeling Assumptions — surplus share θ_i as bargaining weight / regulator / media attention; hard time constraint; timing (Stackelberg)
    - 3.5.2 Tragedy of Complexity and Tragedy of the Commons — **Proposition 3** (consumer short of time: `λ^e > λ^*`)
    - 3.5.3 Complexity through the lens of demand theory — effective attention as commodity priced at `c_i`; Slutsky decomposition; elasticity condition (fn. 9)
- **4. An Explicit Characterization** — functional form (16); attention demand (17); hump-shaped `t̃_i(c_i, λ)`; giving-up region; **Proposition 4** (well-understood goods overcomplicated iff `t_i/c_i > 1`)
- **5. Equilibrium Complexity and Comparative Statics**
  - 5.1 Ex-Ante Homogeneous Goods — **Lemma 2** (FOCs); separating hyperplane (22): `c^e < T/N ⇔` excess complexity; Figure 2; **Proposition 5** (complexity paradox; curse of variety; asymmetries for high δ)
  - 5.2 Ex-ante Heterogeneous Goods — **Proposition 6**; **Table 1** (complexity matrix: absolute × relative complexity)
- **6. Conclusion**
- **References** (31 items)
- **Appendix A. Proofs** — Prop. 1, Lemma 1, Prop. 3–6, Lemma 2
- **Appendix B. Microfoundation with priced good** — divisible goods, prices, monopoly; recovers FOC (7)

---

### Steps 2–3 — Forward Index with `[?→]` Tags, Resolved by Backward Pass

| # | Concept (first appearance) | Tag | Resolution |
|---|---|---|---|
| 1 | "attention externality" (Abstract, Intro) | `[?→]` | §3.4, Prop. 1: sign of distortion = sign of `∂t_i/∂c_i`; Lemma 1: four equivalent statistics |
| 2 | "software update effect" (Intro; §3.4 heading) | `[?→]` | Prop. 2: producer overcomplicates iff `∂ṽ_i/∂c_i < 0` at fixed attention (Excel anecdote) |
| 3 | "complexity paradox" (Abstract/Intro) | `[?→]` | Prop. 5(ii): abundant `T` ⇒ `c^e > c*`; more processing capacity can *cause* excess complexity |
| 4 | "curse of variety" (Abstract/Intro) | `[?→]` | Prop. 5(iv): larger `N` ⇒ inefficient simplification; MBA vs. PhD course example |
| 5 | "categorization of goods" (Abstract) | `[?→]` | Table 1 + Prop. 6: four cells (absolute {simple, complex} × relative {too simple, too complex}) |
| 6 | Does the surplus share `θ_i` drive the distortion? (§3.4, equal-θ case) | `[?→]` | Prop. 5: ranking independent of `θ`; Prop. 6(iii): heterogeneity in `θ` *does* shift absolute complexity |
| 7 | Why no externality if attention were purchasable? (§3.5.1) | `[?→]` | Lemma 1(ii): externality works through `∂λ/∂c_i`; fixed `λ` ⇒ no externality |
| 8 | Policy remedy (fn. 8) | `[?→]` | "Reduce complexity" mandates are wrong (complexity can be too low); ideal = internalize shadow cost, e.g. tradable attention permits (deemed impractical) |
| 9 | Very complex goods nobody understands (Intro) | `[?→]` | §4 giving-up region: consumer sets `t_i = 0`; no externality; equilibrium = planner there (fn. 12–13) |
| 10 | Threshold `t_i/c_i = 1` (Prop. 4) | `[?→]` | fn. 14: `t_i/c_i = 1` means the consumer realizes half of the potential benefit `δ_i` of understanding |
| 11 | Asymmetric complexity for identical goods (§5.1 end) | `[?→]` | High `δ`, small `T`: one good simple/unattended, one complex/attended; equilibrium does not pin down which |
| 12 | Spence-quality intuition vs. here (fn. on (14); App. B) | `[?→]` | Appendix B multiplicative value `v_1(x)·v_2(c, t/c)` kills the Spence term; extra monopoly distortions are orthogonal |

---

### Step 4 — Synthesis (section titles + keyword summaries)

- **Abstract** — complexity ⇄ value vs. attention; common-resource externality; intermediate-complexity distortion; paradox + curse of variety.
- **1. Introduction** — puzzle (overcomplicated phones/regulation vs. dumbed-down politics/MBA); novelty: complexity as value-enhancing, cost = attention; three results; literature split (obfuscation vs. attention).
- **2. Model Setup** — `v_i(c_i, t_i/c_i)`, diminishing returns in both arguments; effective attention `e_i = t_i/c_i`; Assumption 1 ⇒ all goods consumed; budget `Σt_i ≤ T`.
- **3.1 Consumer** — equate marginal value per unit time to `λ`; allocates more time to goods where she keeps a larger share `1−θ_i`.
- **3.2 Producer** — chooses `c_i` anticipating `t_i(c)`; FOC balances direct complexity benefit vs. diluted understanding net of attention reallocation; attention-grabbing effect `∂t_i/∂c_i`.
- **3.3 Planner** — same trade-off plus the shadow value of attention reallocated from *all* other goods `j≠i` (the `N−1` neglected terms).
- **3.4 Externality** — Prop. 1 (tragedy of complexity); Lemma 1 (`∂t_i/∂c_i`, `∂λ/∂c_i`, fixed-λ response, cross-partial `∂²ṽ/∂c∂t` share a sign); Prop. 2 (software update effect = red flag for excess complexity).
- **3.5.1 Assumptions** — θ_i as Nash weight/regulator benevolence/media attention sales; hard vs. costly-relaxable attention budget; timing = Stackelberg; complexity non-contractible by consumer.
- **3.5.2 Commons** — attention always overused (`λ^e > λ^*`, Prop. 3) even when complexity distortion direction varies; consumer "constantly feels short of time".
- **3.5.3 Demand theory** — `c_i` = price of effective attention, `T` = income; excess complexity iff income effect > substitution effect; iff own-price elasticity of effective-attention demand < 1.
- **4. Explicit characterization** — quadratic `f_i`, concave understanding term; `t̃_i = max(0, √(c_i δ_i(1−θ_i)w_i/λ) − c_i)`; hump shape; giving-up cutoff; Prop. 4 (`t_i/c_i ≷ 1` ⇔ over/under-complication; phones vs. Brexit example).
- **5.1 Homogeneous goods** — Lemma 2 FOCs; separation at `c = T/N`; Prop. 5: excess complexity when δ high, T abundant, α low, N small; complexity paradox + curse of variety; convergence to first-best `α/2` as `T→∞`.
- **5.2 Heterogeneous goods** — Prop. 6 derivatives at symmetry point; Table 1 complexity matrix: e.g. high-`w_i`/low-`θ_i` goods are complex *and* too complex; high-`α_i`/low-`δ_i` goods complex but too simple.
- **6. Conclusion** — attention externality as generic source of inefficient complexity; direction depends on consumer reallocation; paradox and categorization as applied insights.
- **Appendix A** — proofs via inverse-function theorem on `∂t̃/∂λ < 0`, total differentiation of the binding budget, comparative statics at the critical hyperplane `T^crit = N/4·(α+√(α²−2δ))`.
- **Appendix B** — priced, divisible goods; consumer FOCs for `x_i, t_i`; monopoly pricing; producer complexity FOC identical to (7); planner FOC identical to (12); Spence term neutralized by multiplicative structure.

---

### Step 5 — Analytical Essay

The paper's engine is a single wedge between two first-order conditions. A consumer with a hard time budget `T` equates the marginal value of attention across goods; producers, each extracting a share `θ_i` of their good's value, choose complexity while internalizing the consumer's response `t_i(c)`. The planner differs only by also counting how that response reallocates attention across *all* goods. Because total attention is fixed, the producer's privately optimal wedge `∂t_i/∂c_i` — the attention-grabbing effect — is an externality by construction: it is positive when the consumer digs in (complexity and attention are complements), negative when she gives up.

The paper's central move is to sign this externality with observable sufficient statistics (Lemma 1): the direction of distortion equals the sign of the response of the shadow price of attention `∂λ/∂c_i`, of the fixed-λ attention response, and of the cross-partial of value with respect to complexity and attention. This yields two diagnostics with immediate real-world texture. Proposition 2 — the "software update effect" — says a producer overcomplicates exactly when a user holding her time constant would feel the product got worse (the Excel complaint). Proposition 4 says goods the consumer understands well (`t_i/c_i > 1`) are precisely the ones pushed into excess complexity, while poorly understood goods are dumbed down: distortion toward intermediate complexity, because intermediate goods are where attention is contested. Well-understood smartphones and checking accounts bloat; intricate policy debates are flattened by media and politicians.

Retrospectively, everything in the introduction's shopping list of puzzles is resolved by the equal-`θ` simplification of §3.4 and then refined: the "complexity paradox" (Proposition 5(ii): abundant attention makes excess complexity *more* likely, since equilibrium complexity crosses the separating hyperplane `c = T/N`) and the "curse of variety" (Proposition 5(iv): more goods per unit attention force simplification, echoing online media dumbing-down). The commons analogy is sharpened rather than merely invoked (Proposition 3): whatever the direction of the complexity distortion, the common resource — attention — is always overused, `λ^e > λ^*`. The demand-theory reading (§3.5.3) reveals the deeper structure: complexity is the *price* producers set for the good of understanding, a price the consumer cannot shop around for, which is why the market analogue of a Slutsky decomposition misfires.

The closing sections convert the local externality into global statements. With the explicit functional form, attention demand is hump-shaped with a "giving up" region where the externality vanishes and the market is efficient — a boundary that itself is an empirical signature (nobody reads the terms and conditions). Comparative statics then sort goods into the complexity matrix (Table 1), separating absolute from relative complexity and tying each cell to deep parameters (`w_i, δ_i, α_i, θ_i`). The paper's own candor about limits — a "reduce complexity" mandate would be as wrong as the disease it treats; tradable attention permits are a thought experiment; the quadratic form is expositional — marks the boundary between the robust claim (an attention externality generically distorts complexity) and the model-specific ones (closed forms, thresholds at `t_i/c_i = 1`). Appendix B closes the loop by showing the reduced-form sharing rule is dispensable: with priced divisible goods, a monopolist's complexity FOC reproduces (7), so the tragedy survives once surplus extraction is microfounded.

---

### Supplement — Novelty / Consistency / Support Check (per prompt #4)

- **Novelty claim**: complexity as *potentially value-enhancing* with the cost borne as an attention externality, rather than obfuscation (Carlin 2009; Piccione–Spiegler 2012; Spiegler 2016) — supported by the literature split drawn in §1.
- **Supported**: hump-shaped attention and the giving-up region require separability of `v` in complexity and understanding plus bounded value at infinite depth (fn. 13) — conditions stated explicitly.
- **Supported with simplification**: the clean sign result (Prop. 1) is shown first for equal `θ`; heterogeneous `θ` is handled (weighted sums) but the equal-θ planner FOC collapses to `t_i/c_i` only because reallocation sums to zero — a symmetry artifact worth remembering before exporting the threshold to empirical work.
- **Unverified within the document**: all real-world mappings (MBA/PhD courses, smartphones, Brexit) are illustrative, not evidenced; the policy remedy (attention permits) is flagged by the authors themselves as impractical.
- **Version note**: the 2019 WP's "producers/effective attention" language and two-good applications are superseded in the published *Management Science* version ("suppliers", "depth of understanding", plus regulatory and intra-firm communication applications) — citations should use the published version.
