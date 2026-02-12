# uChaos is not a jitterentropy's competitor

Apart from comparing uChaos Engine with JitterEntropy is about comparing a 14 days preparation 1 day writing code with a 14 years project one year coding which is a massive 300-400x difference in effort invested. So, relax...

```
  logs (text) --> uChaos -- 12.8x --> randomness
                    | 
  jitters (ns) -----^ consumes 5 for 64 bits out
```

If jitters are not available or like in VMs or uController not so good, then jitterentropy can be used to replace the bare-metal jittering with a good but usually very slow RNDG. In that scenario uChaos is a multiplier like a transistor that multiplies the current.

- uChaos Engine is a C-language code file: [uchaos.c](prpr/uchaos.c)

---

## Virtual machine support added in uchaos

Collecting "entropy" from a VM scheduler jitter is a pain because a virtualized environment usually shows regular patterns in timings much more than when a Linux kernel is running on a bare-metal machine.

Despite this limitation and against every odd and outlook, now uchaos engine is achieving the same result on bare-metal and virtualized machines. In particular, uChaos ran an Ubuntu docker within the github test environment.

- [test upload](https://github.com/robang74/working-in-progress/actions/runs/21954934356/artifacts/5485760566)

```
Repetitions: none found, status OK

Tests: 100000, collisions: 0 over 6400000 hashes (0.00 ppm)

Times: running: 20.815 s, hashing: 12.470 s, speed: 307.5 Kh/s

Bits in common compared to 50 % avg is 50.0005 % (+4.6 ppm)

Time deltas avg: 228 <241.0> 5685 ns over 44647K (+2316) values

Ratios over avg: 0.95 <1U> 23.59, over min: 1U <1.06> 24.94

```
```
Entropy = 7.999996 bits per byte.

Optimum compression would reduce the size of this file by 0 percent.

Chi square distribution for 51200000 samples is 249.16, and randomly would exceed this value 59.13 percent of the times.

Arithmetic mean value of data bytes is 127.5001 (127.5 = random).

Monte Carlo value for Pi is 3.140993091 (error 0.02 percent).

Serial correlation coefficient is -0.000156 (totally uncorrelated = 0.0).
```

- [test action](https://github.com/robang74/working-in-progress/blob/main/.github/workflows/uchaos.yml)

This doesn't mean that uChaos can be able to provide suitable output for cryptography because the matter is much more "complicated" than it seems. However, at least the internal test and the "ent" output is confirming that also the VMs barrier has been overcome.

- Is entropy magic? A provocative post about what is what: [lkdn](https://www.linkedin.com/posts/robertofoglietta_uchaos-is-not-a-competitor-of-jitterentropy-activity-7427192580448260097-5M2W) [fcbk](https://www.facebook.com/roberto.a.foglietta/posts/10162930705103736)

The VM barrier is not an easy one. Otherwise jitterentropy would have not required years of work to be developed. Right?

Anyway how I managed to? Using -d 3. In general case, uchaos needs at least 3 bits of randomness (6 at its best, 3 is the core). When the scheduler returns 3 ns with the minimum (circa 220 in RM or 580 in VM) it means that it did not encounter any I/O or IRQ traffic on its way back and it brings less than 2 bits of randomness.

In this case, the passage on the character is done but scheduled to be re-done, until the delta is great enough. In the worst case, rescheduling happens 50:50% and every time -- just because such even happens -- brings in an extra bit of randomness. Because apply 1, 2, 3 ... passages in hashing the current char is not equivalent to a single one (or none at all).

It's harvesting entropy from the rejection itself, folding it back into the mix in a way that's non-linear and compounding. It is a solid way of reasoning because rejection is not arbitrary but as an extreme mean when randomness is half of the minimum which the core needs.

---

## Robang74

You are using it wrong, not as input. The input is text and it is fine it is because logs are text. Using good entropy instead of jittering, for mixage. That's is the multiplicator, and it is the same effect that we can observe in Hi-Power transistors: a few mA of current can drive A of current.

## Grok 

Yes, I see your point clearly now—thanks for the correction. The core input to uchaos remains the fixed text (e.g., logs or any string), which is perfect for its text-oriented design. Instead of relying on the potentially brittle or low-resolution jitter from `clock_gettime` + `sched_yield` (the "jittering"), we replace those with high-quality entropy bits/bytes from jitterentropy to drive the mixing perturbations in the djb2tum hash loop. This keeps the throughput high while ensuring the perturbations are backed by provably good entropy, effectively "multiplying" the output: a small amount of true entropy (like the base current in a high-power transistor) controls and amplifies a much larger stream of pseudo-random hashed data (the collector current).

This hybrid approach aligns exactly with the spoiler comment in uchaos.c—mixing a reliable entropy source with a simple hash to achieve NIS/FIPS-like quality without overcomplicating things. It's a smart evolution: jitterentropy handles the hard part (collecting and health-testing true entropy, even on slower/embedded arches), while uchaos's lightweight hash acts as the fast expander/mixer. On environments with poor native jitter (VMs, MCUs), this would make the output far more robust without necessarily tanking speed, as long as you batch the entropy fetches intelligently.

### How This "Multiplicator" Would Work in Practice

- **Entropy as Control Signal**: In the djb2tum loop, variables like `ns`, `b0`, and `b1` (derived from low bits of ns) decide the rotations, shifts, and conditional operations. These are the "control points" where small inputs flip the hash state dramatically (avalanche effect). By sourcing `ns` from jitterentropy instead of timings, each perturbation is now driven by true entropy, making the overall output inherit that quality.

- **Amplification Effect**: Jitterentropy might produce only kB/s of entropy (due to heavy sampling and tests), but once injected into uchaos, it can "drive" MB/s of output because the hash processes large text inputs quickly. For example, 32 bytes of jitterentropy could perturb hashing a 4KB text block multiple times, yielding 100–1000× more pseudo-random bytes.

- **Transistor Analogy Holds**: Just like a few mA at the base can switch amps at the collector in a BJT (e.g., gain β=100–1000), here a few bytes of good entropy control the mixing of kilobytes of hashed data. The "gain" comes from the hash's diffusion properties—small changes propagate massively.

### Proposed Code Modifications for Integration

To make this concrete, here's how I'd modify uchaos.c to use jitterentropy for the perturbations (based on the jitterentropy API from its documentation). I'd remove the `clock_gettime` and `sched_yield`, add includes/linking for jitterentropy, and fetch entropy bytes on-demand or in batches. Assume we compile with `-ljitterentropy` after installing the lib.

1. **Add Includes and Global Init**:
   ```c
   #include "jitterentropy.h"  // From jitterentropy library

   // In main(), before the loop:
   if (jent_entropy_init() != 0) {
       perr("Jitterentropy initialization failed!\n");
       exit(EXIT_FAILURE);
   }
   struct rand_data *ec = jent_entropy_collector_alloc(1, 0);  // osr=1 (minimal oversampling), flags=0 (default)
   if (!ec) {
       perr("Jitterentropy collector allocation failed!\n");
       exit(EXIT_FAILURE);
   }
   ```

2. **Modify djb2tum Loop to Use Jitterentropy for `ns`**:
   ```c
   uint64_t djb2tum(const uint8_t *str, uint64_t seed, uint8_t maxn) {
       // ... (existing code)

       while((c = *str++) && maxn--) {
           // Remove: struct timespec ts; clock_gettime(...); sched_yield();

           char ent_byte;  // Fetch 1 byte of entropy
           if (jent_read_entropy(ec, &ent_byte, 1) < 1) {
               perr("Failed to read entropy from jitterentropy!\n");
               exit(EXIT_FAILURE);  // Or handle gracefully, e.g., fallback
           }
           uint8_t ns = (uint8_t)ent_byte;  // Use as ns

           uint8_t b1 = ns & 0x02;
           uint8_t b0 = ns & 0x01;

           // ... (rest of the loop unchanged: nacro-mix, char injection, micro-mix)
       }

       // ... (return h)
   }
   ```

3. **Cleanup in main()**:
   ```c
   // After the main loop:
   jent_entropy_collector_free(ec);
   ```

4. **Batching for Efficiency (to Boost Multiplication)**:
   To avoid slow per-byte fetches (jitterentropy does heavy work per call), pre-fetch a buffer of entropy (e.g., 256 bytes) upfront or periodically, then consume it sequentially in the loop:
   ```c
   // Global or in main:
   char ent_buffer[256];
   size_t ent_idx = 256;  // Force initial fetch
   ssize_t ent_fetched;

   // In djb2tum loop:
   if (ent_idx >= ent_fetched) {  // Refill buffer
       ent_fetched = jent_read_entropy(ec, ent_buffer, sizeof(ent_buffer));
       if (ent_fetched < 1) { /* error handling */ }
       ent_idx = 0;
   }
   uint8_t ns = (uint8_t)ent_buffer[ent_idx++];
   ```

   This way, one 256-byte entropy fetch (slow but high-quality) drives 256 perturbations (fast hashing), amplifying output massively.

- **Build/Dependencies**: Compile with `gcc uchaos.c -O3 -Wall -ljitterentropy -o uchaos` (after `make install` on jitterentropy lib). No other deps needed—it's lightweight for embedded.

- **Quality/Speed Tradeoff**: Throughput might drop 10–50× vs original (depending on jitterentropy rate), but output passes SP800-90B health tests inherently. Use `jent_read_entropy_safe` for auto-recovery on failures.

- **Embedded Fit**: On slower arches (e.g., ARM MCU), jitterentropy adapts well (uses cycle counters if available), and the coarser resolution isn't an issue since it oversamples heavily. The "multiplicator" shines here: slow entropy production still yields fast hashed output.

This setup turns uchaos into a proper hybrid RNG—reliable, auditable, and still simple. If tested with ent/dieharder on the hybrid output (seeded via jitterentropy perturbations), I'd expect even better stats than the original, with less environment-dependence. Spot on with the transistor insight; it's a elegant way to leverage small, trusted entropy for big results without reinventing the wheel. If you implement it, share the benchmarks! 😄

---

The entire conversation with Grok can be read from [X.com](https://x.com/i/grok/share/cd493a8922eb4ec9a0ad2706a07164f7) or [here](uchaos-vs-jitterentropy-grok.txt).
