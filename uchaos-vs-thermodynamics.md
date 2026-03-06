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

### uCHAOS: WHAT'S NEW IN THE v0.3.3 ?

- [LinkedIn post #4](https://www.linkedin.com/posts/robertofoglietta_uchaos-whats-new-in-the-v033-looking-activity-7435689736264077313-WPQX)

Looking at the source code from v0.2.9.6 to v0.3.3, it seems that the engine function has been completely rewritten. It is not in that way, changes impacted in many lines but the changes are easier to exaplain.

First of all, the command lines options/arguments have been refactored for a more user-friendly approach (UI/UX impovement by clarity in mind) and also the stats presentation has been improved for clarity and shortness.

The option -Z has been added to be the sibiling of -S but with the internal statistics (the engine functional part only) which resets every 512KB. It can be useful to quickly make a comparison instead of restarting the application.

The engine function has several parts enumerated from 0. to 9. and each parts have several steps. Every step of each part has been reviewed for being KISS and theoretically easier to support (with clairty in mind). In particular, reducing as much as possible every arbitrary choice and when necessary made it in such a way that is one of the most reasonable.

Moreover, clarity in mind for being KISS to explain and support, refers also for the internal stats. In such a way the numbers printed out are as simply straighforward referable to the internal "gears" as much as possible. In this case clarity goes with transparency. And all of these changes are supporting a faster or easier adoption.

Now the cycle is very rational: 0. prepare; 1. get nanoseconds; 2. check the latency and jitter; 3. updates stats and deal with events these data, 4. deal with an internal 64-bit pool of entropy; 5.+6. inkect the entropy into the hashing process; 7. the execption manager activation by 3. is the sensitive part for stochastics bi-forkations by jittering; 8. preparation for the next round or finalising the output of the function.

The changes are not just reorganising the code, are also changing how the engine is working and potentially we should be surprised that such changes did not makes such a difference in the output quality. In short, the fundamentals remained the same and how the entropy is mixed into output changed and obviously the output changed but not in its fundamental nature.

Because its fundamental nature is to have not any structure every change is potentially undectable because by the nature of the output only flebile watermarks (if any) are allowed. Because "unpredictability" requires "no structure" while "no structure" implies that is not self-explanatory enough for being leveraged by an __external__ attacker. While insiders might have a little of chance because PractRand can "certify" only the "quality of no-structure" by an external perspective. In absence of an internal certification or QA, we can assume that an internal attacker might have a chance.

Now with the v0.3.3 every single line of each part is "self-explanatory" because also the relationships among parts have been minimised, and those few remenaining are for speeding in execution.

---

### uCHAOS COMPARISON v0.2.9x vs v0.3.x IN THEORY

- [LinkedIn post #5](https://www.linkedin.com/posts/robertofoglietta_uchaos-whats-new-in-the-v033-looking-activity-7435704061435305985-xD9O)

The v0.3.x introduced the -Z option which is the sibling of -S but with an internal stats reset for every 512KB of output. Apparently there is no difference between the -Z and -S outputs from the PractRand point of view and it is hard to see also for a few MB of data in output. Moreover, it seems that -S is a little more stable / flat in terms of white noise than -Z which is nice to see because as we have seen with v2.8.x more sensitive is a "thermometer" more keen to be affected by tiny local "turbulence".

Moreover, the -Z option should do a reset at 16KB instead of 512KB and -d7 might be mandatory instead of -d3. Thus the -Z is not fine-tuned yet because it has been designed for the next level of low-entropy systems like real-time micro-kernels and among them Neutrino. In fact, the Linux kernel shows a latency range that extends within 640ns to 5740 ns, which is a 8.92 range extension.

```text
uChaos v0.3.3 w/sb !/pr; repetitions: 0, status OK

Tests: 256 w/ duplicates 0 over 16.4 K hashes (0.00 ppm)
2 Hamming weight, avg is 50.0001 % 50% by (+2.0 ppm)
Hamming distance: 14 < 32.00006 > 50 over 516.1 K XORS
Hamming dist/avg: 0.99160 < 10:32 +2.0 ppm > 1.00686

Perform: exec 0.380s, 0.34 MB/s; hash 0.122s, 133.9 KH/s
Latency: 643 <785.4> 5736 ns over 183 K w/ e:9, x:73460
Ratios : 0.82 <avg=1U> 7.3theta , min=1U <1.22> 8.92
Setting: s:0, q:0, p:0%:0ns, d:7, г:31, 1:16, 2:0
```

While the average is 22% above the minimum and in a symmetric range (more typical in the real-time system) this means that latency will be around 20% more or less the average, collapsing the range from 9.8 to 1.5 times. Although real-time is not related to low-response time but latency constrained (deterministic predictability) usually als the response time tends to be shorter. There are 140ns between the minimum and the average, which are seven bits span (when absolute values are involved).

Instead, a hard real time system might offer just three bit range. Below, the uchaos might start to accuse the lack of entropy density and its output might fail to pass the tests. This would be the proper scenario for fine tuning the -Z option in combination with -d7.

Comparing the two -S and -Z, the theory and conclusions that can be inferred are not changing with respect to the past claims. Despite the -S and -Z seem providing the same quality output, this happens because in the meantime the ability of uChaos engine to extract entropy from the scheduler have been drastically increased at the point to make the stats reset as meaningless.

Well, it is meaningless in macroscopic terms. When we move our focus from the particles to the billiard balls the fundamental laws of physics remain the same and statistically speaking is the same system but the huge N (or an abundance of entropy) makes some aspects irrelevant. A massive billiard ball has a well-defined temperature and its mass is not able to be influenced by vortexes at microscopic scale.

In conclusion, what was real for uChaos v0.2.8.x and v0.2.9.x, still be relatable to the v0.3.x as well but the code changed and uChaos ability to extract entropy is increased at such point for which we cannot see anymore some borderline phenomenon.

---

(c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, CC BY-ND-NC 4.0
