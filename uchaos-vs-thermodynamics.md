## THE CODE RATIONALISATION ABOUT ENTROPY

- [LinkedIn post #1](https://www.linkedin.com/posts/robertofoglietta_uchaos-v029x-the-code-rationalisation-activity-7434929849980551169-h5pR)

Novel approach behind uChaos influenced coding and commenting. With the v0.2.9 the two are more aligned with the mainstream idea of entropy.

The v0.2.9.2 goes a step further in test automatisation in combination with the Bare Minimal Linux System project developed to provide a near 0°K VM.

Moreover the "coder rationalisation" is not just about aesthetic or commenting in a more conformant way but it also brings few but interesting novels: 1. reschedulings + statistical events are presented with two separated numbers; 2. jitter and not just the latency is took in full consideration increasing the entropy volume injection.

This explains because with 5 stochastics bits of entroy and 2500 statistics bits of entropy, uchaos managed to deliver a 4K with a 3bits per byte of entropy from the perspective of PractRand thus an attacker unpredictibility (which is the essential KPI, after all).

Moreover, using -d7 instead of -d3 doesn't increase the statistics entropy but more than double increases the stochastics entropy: 5 --> 12. The speed on the "-icount" ultra-cold QEMU virtual machine w/ USE_EXP_COMPR is quite slow: 18Kh/s but to fullfil the kernel 52 hashes of these are enough, let say 64 for a good reasonable margin. This requires 0.01s which is 10x time shorter that the shortest boot time observed 0.1s

```sh
root@u-bmls:/# dmesg -r | uchaos -T 4 -d3 -r31 | wc -c

>>> WARNING!! <<< uChaos is compiled with USE_EXP_COMPR

uChaos v0.2.9.2 w/sb !/pr; repetitions: none found, status OK

Tests: 8 w/ duplicates 0 over 0.5 K hashes (0.00 ppm)
Hamming weight, avg is 50.0246 % expected 50 % (+492.2 ppm)
Hamming distance: 12 < 32.01575 > 49 over 16.13 K XORs
Hamming dist/avg: 0.99752 < 1U:32 +492.2 ppm > 1.00253

Times: running: 0.029 s, hashing: 0.003 s, speed: 17.7 Kh/s
Time deltas avg: 663 <727.4> 63030 ns over 22K (w/ 5 + 2496)
Ratios over avg: 0.91 <1U> 86.66, over min: 1U <1.10> 95.07
Parameter settings: s(0), q(0), p(0:3), d(31), r(1), i(0), RTSC(0)

4104
```

---

## A BANANA 🍌 POSSIBLY!

- [LinkedIn post #2](https://www.linkedin.com/posts/robertofoglietta_uchaos-v029x-the-code-rationalisation-activity-7435050841352380416-BKeI)

In the code the assumption is that every event brings in 9 bits of entropy. It seems overstating but despite the assumption is pretty gross, all the jitters bring some contribution even if they aren't related directly by an event and this because events cause a disruption in the next "expected" dynamics, in particular the 5 stochastics are rescheduling at unexpected times.

Considering that by a gross estimation (reschedule yes or not) 2^5 = 64 but the cycle are 1M/64 = 2^(20-6) = 2^14 = 16k. This means that the chance of an expected reschedule is 1/3277 and this makes these events rare enough to make their predictability not easy to forecast (an average would fail because not even the minimum elements for a stats are present). When they are 12, then p=1/1365. You can easily calculate the entropy of such relatively low-probability events.

- `p = 1/3277 ≈ 0.000305: -log₂(p) ≈ 11.68 bits (5x12=60)`
- `p = 1/1365 ≈ 0.000733: -log₂(p) ≈ 10.42 bits (12x10=120)`

These are the neat direct contributions and indicate that 3 and 7 are the "golden" thresholds (2^n - 1) to consider because below the 3 the events are too rare to be impacting enough and above 7 starts to become too frequent and they risk to be "normalised".

When PractRand indicates an "unusual" at 1E-4 or 1E-3 means that such a piece is missing a contribution in stochastics compared the expectancy and thus the "disorder" is not so high thus watermarks might arise. Despite PractRand changing flag-name at 1E-6, almost all the flags from uChaos output are in that range above (a bit past the borderline).

Which makes sense because the distribution of events in the range of 1E-3 and 1-E4 are not stable enough to be predictable by statistics thus a couple or zero were one was expected creates "turbulence" (1E-3)² / 1E-3 = 1E-3 which is spread but like a wave propagates bouncing many times before being absorbed.

When that double-or-zero event happens at near the end of the chunk elaboration not enough elaboration remains to dissipate the "ops!" ripples before delivering the output and that nasty b*tch of PackRand smells it. LOL

Even these events are NOT a sign of a flaw but the "pulse" of an engine that works as expected and in its unpredictability sometimes leaks a vibration. Guess what? This is also the reason because in nature rarely the "white noise" exists as we intend apart "temperature" which is a steady macroscopic characteristic, because seen microscopic volumes have a lot of local and sometimes also macro turbulence.

What? A software scheduler running on a 100% software emulated deterministic (-icount) machine shows the same characteristics of real-world heat noise when using an over-precise thermometer outside lab stable conditions? Exactly, information is energy thus displays the same patterns that affect energy or matter.

This doesn't mean if we think about a banana, it materialises. Unless we think 🍌 all together in a very focused and synchronous manner...

---

### THE STOCHASTICS HITS CREATES RIPPLES 2⁻ⁿ FADING

- [LinkedIn post #3](https://www.linkedin.com/posts/robertofoglietta_uchaos-v029x-the-code-rationalisation-activity-7435061834484748290-qv2C)

In `prpr/uchaos.c`, uncommon event index is a relevant info, therefore v0.2.9.3:

```c
  // Knuth, based on gold section seeded by 1E-3 ~ 1E-4 event idx
  hsh = mm3ns32(hsh, pidx(&chr));
```

Considering the frequencies presented in the previous post, 1/3277 is 12 bits and 1/1365 is 10. In this estimation the numbers change from 5x12=60 to 5x24=120 and 12x10=120 in 12x20=240. Which brings to consider the -d7 more suitable for the ultra-deterministic VMs.

Because 240 bits are nearly what the kernel needs to fill-up its entropy bucket and they are provided by elaborating 1Mb / 64b hashes, just this would fulfill the kernel in 1s (too much, IMHO) but injection in the pool can be progressive thus the first injection of entropy will reach the pool at 256 hashes internally elaborated which is 4M/s internally is 0.004s.

The density of entropy by uncommon stochastics effects (hit ripples) decreases by 2^n thus hashes are XORed before the output in 2^n progression. Fortunately, the disruption caused by these stochastics events is so massive that randomness throughput is 17Kh/s with a minimum estimated entropy density of 3 bits per byte (each hash is 64bits, 8 bytes).

In this way uChaos v0.2.9.3 passed the 4MB test (by many concatenated smaller chuncks, aka periodic internal statistics reset) on a 100% software emulated qemy machine single cpu !SMP almost allnoconfig with -nodefault and -icount as internal time ticker (aka ultra-deterministic VMs) which was not a simple task to achieve by a project that in its preliminary research started one month ago and using non-cryptographic "tricks" but stuff from '90s.

Not because using cryptographic hashes or algorithms like ChaCha20 is wrong, but because introducing that stuff would have "taintened" the goal. Instead when anything else apart from XOR, rotl64 and avalanche multiplication are involved then entropy is real or the result is predictable and thus PractRand would make the tests fail. This is the power of simplicity or said with different words, the challenge for the highs without short-cuts.

This means that in kernel space it takes 1ms to fulfill the entropy bucket.

```sh
root@u-bmls:/# uchaos -r31 -i16 -d7 -K1 <dmesg.txt >/dev/null
uChaos v0.2.9.4 w/sb !/pr; repetitions: none found, status OK

Tests: 2 w/ duplicates 0 over 0.1 K hashes (0.00 ppm)
Hamming weight, avg is 50.1093 % expected 50 % (+2185.6 ppm)
Hamming distance: 18 < 32.06994 > 46 over 4.032 K XORs
Hamming dist/avg: 0.99995 < 1U:32 +2185.6 ppm > 1.00005

Times: running: 0.016 s, hashing: 0.001 s, speed: 8.0 Kh/s
Time deltas avg: 661 <713.7> 3419 ns over 19K (w/ 4 + 2112)
Ratios over avg: 0.93 <1U> 4.79, over min: 1U <1.08> 5.17
Parameter settings: s(0), q(0), p(0:7), d(31), r(16), i(0), RTSC(0)
```

While the hashing speed (on the 0°K VM) is 46.3 Kh/s w/1Mb production

---

(c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, CC BY-ND-NC 4.0
