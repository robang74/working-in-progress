## PRPR VIBE CODING TEST 

Good time to test the vibe coding but in C a language that I was used to master.

1. description of what I want.

The result compile and apparently did what is supposed to do but...

2. questioning the AI about what produced

3. deciding how to proceed further and let it go

The result compile and apparently did what is supposed to do but...

4. TODO

Is that more efficient that writing the code from scratch? I have my serious doubts but it is more enjoyable and also more interesting because it resemble the extreme programming (programming in alternance with a peer). I like the exchange of the opinions because in the rush of writing code, sometimes we implicitely give something for granted which is not so much in the most general case.

5. TODO

Once get in touch with the AI, the real work is starting: adding functionality on the basic schema built. Two main fundanting pillar have been set by  now:

- I credited myself to the AI as a software developer and since now, it will tend to relate with me in the proper manner to avoid the risk of seeing its work rejected. However, at the same time, I shown that I am open to accept ideas and reframe pro-vs-cons in a third way. Thus, the AI mirroring will be pro and open as per default policy.

- Starting with a simple scheleton on which add functionalities helps the AI to work on as short as possible CoT and avoid mistakes that will waste time and disrupt confidence of both parties in being able to work as Extreme Programming peers. In the meantime unit test suite is growing.

- Genini chat sharing [link](https://lnkd.in/dPdfrvDj)

In order to have an idea of the overall process, this is the commits history

- rprp.c commits [history](https://lnkd.in/dXt2ij2F)

### CONCLUSION

The AI continues changes the file in a way that it should be tested everytime from zero again, potentially. So, it does not code in a git-patch compliant. Moreover, like Copilot also Gemini has the tendecy to change the file in the whole (and this is because a LLM is made for generating language not to perform text maintenance, including code).

The vibe coding in C-language makes sense only if the code to write is a totally boring. The AI is for the 1st draft canvas (templating) or to work on a specific piece of code (routines or functions). I prefer do not make any comment about working on a project or a git repo because it is NOT that place a LLM/AI belong to. Unsurprisingly for Gemini, can be said the same said about Copilot but Gemini has no any access to git repository.

The whole developement covered a time frame of 5h15m but it includes writing this documentation, the test suite, the code final revision and related testing, plus a full hour of pause and few task in the middle. In total, about three hours which is time in which everything could have been done without the AI.

What is the main difference? For my needs the 1-byte R/W is priviledged when not also mandatory, thus I would write the `prpr` based on that logic instead of using `stdbuf` to enforce the same behaviour. Asking the Gemini to provide me a template and write few additional routines, the result is more general.

