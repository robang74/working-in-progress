## CAN WE IMPROVE RANDOMNESS? NOT REALLY!

- https://www.linkedin.com/posts/robertofoglietta_can-we-improve-randomness-not-really-this-activity-7432739785695383552-8ImI

This question is very subtle because "true randomness" is the absence of ANY structure even if there are forms of watermarks that we can consider "weak" because they are so "delicate" to disappear with trivial operations which is the same for the steganography.

So, in trying to falsificate that uchaos.c output can be associated with true randomness, the best we can do is to seek for structures. And in fact, a weak watermark has been identified at 4GB but with a trivial operation -- like mixing 4 source 1:2:3:4 or discarding the 8th hash of a 8-serie -- it can easily pass 16GB without triggering even the weakest doubt about its randomness.

This is a VERY good result, especially considering that uchaos provide the output starting with a fixed input and the 16GB has been produced by repetitively running uchaos on the same input. Because some algorithms like Park-Miller do not repeat for a HUGE number of steps but from the same seed they replicate the same sequence and the distribution of the number is not randomly homogeneous.

Guess what? Passing each random number through a P-K hash, the result is a P-K distribution. In essence, what I am expecting is that uchaos with P-K filter applied at the end are going to fail the test before 16GB because P-K is expected to be identified at 2GB. Here, its application is more than a single hashing but two 32bit hashes. The test is ongoing and we will see.

Instead a different algorithm like murmur3 (rewritten in 64bit version) which is an avalanche based bit-mixer may have some better chance to destroy the uchaos single source output production. We will see.

However, I consider a VERY meaningful result that PractRand would be able to catch the P-K hash structure in output even if it is applied in 32+32 way (which is supposedly quicker and easier to catch). And that is the reason because both functions have been written in 32bit in the first place.

~> https://lnkd.in/dGeVGRBE (reference C-language code)

### UPDATE ABOUT TESTING

1. the uchaos + P-K hashing fails as expected, improving randomness isn't an easy trick. Even with good "entropy" sources. It easier to do worse. By stdin32, it fails even quickly.

2. uchaos + P-K + murmur4 64bit: there is no chance that mm3 can recover the output after being crippled into a P-K grid. The randomness is lost and the recovery attemps is worsening the situation by failing 4x time faster at 64bit (2^28-->2^26).

3. uchaos + mm3: it passes smoothly 2^34 (16GB) test, showing that mm3 isn't the troubling one. Instead mm3 can blend away the fragile watermark that was affecting the last 3 or 5 bits. Something a trivial operation was doing but a trusted mixer grants.

4. uchaos + a 32+1 bit avalanche mix: a similar operation from mm3, adding avalanche in the original code but impacting on 64bits instead of the LSByte, is enough to let uchaos pass the 16GB test w/ zero flag.

There is no way to trick randomness, it is a kind of magic!

---

## CAN WE IMPROVE RANDOMNESS? NOT REALLY! #2

- https://www.linkedin.com/posts/robertofoglietta_can-we-improve-randomness-not-really-this-activity-7433168633356283904-MxpG

We can add a grid-bias to white noise but not remove it and this shows that tests about uchaos were right: it is producing chaos in Lorenz' terms (cfr. comments in the C code, lnkd.in/dGeVGRBE).

> We can improve randomness by a lot.
> E.g. use a cryptographic PRNG, such as low bits
> of secure-hash of (seed appended to index).

This answer is technically correct and in different ways we are saying the same thing: once we have good randomness we can multiply it (pseudo-RNG). But ONLY applying "good" algos and the Parker-Miller LCG (Linear congruential generator) isn't.

This is about give "true entropy" in input and expect "better entropy" in output but "true entropy" in this specific context is a sequence of numbers (data, so what is the difference?) that are unpredictable, usually because they have no structure (auto-correlation).

The words "usually" and "auto" here are fundamental. Because no structure means no auto-correlation means unpredictability (and this explains the dogma of "true entropy") but unpredictability doesn't imply no structure or no auto-correlation. The structure can unknownable (e.g. Lorenz strange attractor) and auto-correlation can have a period (or a brute-force attack cost) too long to be discoverable in practice.

The word "in practice" is a lot relative. Tomorrow's (or yesterday's undisclosed) quantum computer  would be able to make "in practice" discoverable. Which is the reason for post-quantum cryptography. In both cases, the chaotic behaviour of the Lorenz strange attractor isn't "in practice" reversible (thus discoverable) by quantum computing.

Unless someone invents a macroscopic Maxwell's devil that can defeat the law of thermodynamics. However, Maxwell proposed the analogy of a microscopic devil that allows hot particles to reach one side and cold the others but no viceversa. The absurd is not Maxwell's devil "di per se" but the idea of having such a mechanism that does it systematically on a large scale because it would be the same to charge a battery (aka creating potential energy) from nothing.

There are some processes that are inherently one-way only which implies a not-predictability (to some degree) and in practice we observe totally random low-significative bits. We __observe__ (which is common in quantistics), because otherwise once recorded totally precise conditions we might invert (in theory) the process.

This is also the reason why people started to talk about the "holographic" universe. At the same moment we realise that there is an equivalent Einsenberg's Indetermination Theorem about "observability in terms of precision", we have to consider that it is something tricky like a hologram.

Finally, uchaos proves (aim to) that scheduling jitter is TRUE entropy, breaking the dogma for which "true entropy" can be found only in "material stuff" like a CPU while entropy (whatever is the definition) is more related to information than heat or mass.

## CAN WE IMPROVE RANDOMNESS? NOT REALLY! #3

- https://www.linkedin.com/posts/robertofoglietta_can-we-improve-randomness-not-really-this-activity-7433181877059907584-xSr3

About this: "The danger is only when the adversary has side-channel access to the original jitter or can influence the scheduler." -- yes and no. Yes in the most general case, no in specific cases where:

1. proper jittering is taken into consideration (2nd derivative of time, because it is too sensible to micro-variation any side-attack can cope with unless in god-mode but again, get that mode isn't feasible);

2. even leveraging the 1st degree derivate to check the limit of an attack (or check if a very predictable VM can be too flat for providing entropy) there are some specific if/then/else condition that are inherently unpredictable because they still rely on very fine-grained limits and those limits are inheterly dynamics. 

Functionally, it is like having two 1st degree derivatives which are not totally independent but correlated by 60 bits over 64. Which is exactly the idea behind Lorenz irreversibility.

Technically speaking a VM can be tampered in such a way to provide always predictable time -- it is absurd but for emulation we might use the CPU instruction counter instead of nanoseconds.

When applications ask for time in nanoseconds, they get the number of instructions the CPU executed since boot. Nice, and uchaos has options on the command line like -d7 (or -p1) that set the line on what can be accepted and rejected.

Therefore, a very crimped VM would let uchaos long to run or even completely fail but this isn't worse than accepting "garbage injection" instead of entropy, pretending everything is fine.

The main point here is that the entropy engine in the kernel is based on CPU entropy; this doesn't work with VM unless they allow a transparent CPU access (but a man-in-the-middle attack can happen even if it is extremely hard to fake in a plausible way). 

- Anyway, CPU is matter thus can provide entropy. Remains of the dogma. 

Jitterentropy broke that dogma but in a way that can be acceptable in userland and at my first sight, jitterentropy cannot be ported in kernel space (anyway, earlier versions 2013-2014 were designed for kernel space and got adopted by Fuchsia OS, for example).

While uchaos can work at kernel or userland levels, both and almost identical apart the system calls replaced with kernel functions because it has been designed for it in mind and multi-architecture low-level coding porting as well.

This doesn't mean that uchaos is better compared something else or full mature by design, it is still a functional PoC improving by development. It means that has been designed by scratch to be suitable or reasonably easy to port. It is born ready, not being a full-flagged product, yet.
