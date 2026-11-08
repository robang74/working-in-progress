## Il debito tecnico ha impattato sull'IoT

Articolo scritto come composizione di alcuni miei post pubblicati su LinkedIn relativi all'argomento ossia alle implicazioni sull'industria IT (terminologia usata in maniera sarcastica, non realistica, per definire i limiti del modello industriale quando applicato ad un ambito ad esso incompatibile come la scienza dell'informazione e alla tecnologia informatica) e sull'illusione che una società iper-tecnologica potesse essere sostenibile nonostante un debito tecnico immenso che giorno dopo giorno stiamo retrodatando fino alla causa primaria che fu il burst della bolla delle dot-com nel 2001 e quindi con l'introduzione del paradigma industriale nello sviluppo software.

Una risposta sbagliata e applicata ad un settore che però era estraneo alla radice del problema: l'avidità dei mercati per un settore in forte espansione ma trainato da società che non avevano le capacità manageriali per gestirlo. In estrema sintesi la questione è stata ridotta al solito paradigma top-down: se gli informatici obbedissero al mercato come gli operai sulla linea di produzione, l'avidità dei mercati non avrebbe creato l'evento del 2001. La realtà sta invece dimostrando che anche cercando di sostituire gli informatici con i robot (le AI), il problema si ripropone tale e quale: il mercato NON ha le competenze per giudicare la tecnologia.

Il caso SSHole prova che il debito tecnico è da riportarsi almeno al 2012 e riguarda proprio una scelta di mercato introdotta per favorire il mercato. Il caso della Lenovo FU100 e modelli analoghi mostra chiaramente che il mercato NON è competente in tecnologia e quindi non può essere il mercato a guidarla. Nonostante questo, non abbiamo ancora osservato il fenomeno nella sua interezza, abbiamo solo osservato alcuni timidi segnali di incertezza. Fra i quali, i frighi della GDO che diventano riscaldatori perché un nemico esterno o un qualche mattacchione ha deciso di fare "login" e spostare il valore del freddo da -4°C a +50°C oppure da -20°C a +110°C.

Giustamente le persone comuni hanno cominciato a chiedersi cosa succederebbe ai PoS e agli ATM se questi andassero fuori uso e non avessero contanti. Il fallimento della banca in Paraguay ha mostrato ai più attenti, coloro che non leggono solo le veline e il gossip, cosa effettivamente significa fallimento sistemico strutturale. Il 90% delle persone impattate aveva il 90% delle loro riserve finanziarie depositate presso un solo operatore. Ancora oggi, in Italia, coloro che si appoggiano su almeno due operatori e hanno disponibilità su altri 4 o 5 canali secondari appaiono dei marziani quando è semplicemente diversificazione del portafoglio per mitigazione dei rischi emergenziali. La società stessa non è pronta a tutto questo e nemmeno lontanamente consapevole.   

---

### LA REAZIONE INCONSUETA E PECULIARE A SSHOLE

- [LinkedIn post](https://lnkd.in/p/dqhyKRgn) &nsbp; (2026-11-08)

La questione del post è che si stia osservando una reazione a questo tipo di vulnerabilità piuttosto inconsueto. Probabilmente perché l'attacco ai frigoriferi di mezza europa ha portato alla gente comune il problema della sicurezza informatica. Paradossalmente stanno estendendo questo concetto ai POS e agli ATM. Se anche quelli sono vulnerabili come i frighi della GDO, il concetto di cashless economy è un rischio da evitare.

Il problema a questo punto diventa però di sostenibilità economica delle aziende che forniscono soluzioni "domotiche" perché ora sono costrette a fare un'audit interno qualora dovessero rispondere ai proprio clienti e questo implica di avere l'intero processo di sviluppo (inclusa la toolchain) in qualità e anche aggiornabile, non solo la toolchain ma anche le flotte. 

Il turnover del settore (aumento di stipendio spesso legato a cambio di azienda o uso strutturale di consulenti esterni o di outsourcing) ha già distrutto buona parte della capacità di reagire ad un evento del genere. Quello degli aggiornamenti delle flotte embedded, è un problema che l'AI ha enormi difficoltà ad affrontare (complessità estrinseca).

Insomma, anche per le PMI del terziario, finisce l'epoca delle vacche grasse che per altro è sempre stata un'illusione più che una realtà (economia di scala). Quelle italiane stanno già sparendo come ci conferma l'ISTAT e non solo per decisione della proprietà ma anche in un gran numero di fallimenti. Anche per i grandi gruppi in questo settore, il mal di testa non è cosa di poco conto e qui la situazione scala dalla GDO all'illusione della cashless economy, tanto comoda ma anche così fragile.

Correggere l'errore significa ripristinare l'unico comportamento razionale: la funzione crypt() delle librerie musl devono ritornare NULL non una stringa. Questo però avrebbe un impatto devastante perché coloro i quali devono fornire gli aggiornamenti alle flotte si troverebbero a dover validare tutto lo stack software dal 2012 in poi, pena crash improvvisi e inaspettati.

In pratica significa che il debito tecnico si riporta indietro non al 2016 ma al 2012 e un passo alla volta su questa strada, scopriremo che ci si riporta al 2002, l'anno dopo il burst finanziario delle doc-com e l'introduzione del paradigma IT guys come operai del futuro. 

Quel futuro è oggi, e gli operai di musl stanno ancora discutendo su quale stringa ritornare invece di fare quello che qualsiasi informatico propriamente farebbe per deontologia  professionale: ritornare NULL e infischiarsene di chi non abbia i fondamentali del mesterie.

Perché a tutti può sfuggire un NULL check, ma non per questo le librerie devono strutturalmente esentare i programmatori dal fare il loro mestiere come è corretto farlo. Ci divertiremo un mondo quando lo stesso concetto si estenderà alla gestione dei mutex e dei semafori: ne vederemo davvero delle belle, perché quelli sono i casi tipici del firmware automotive!

---

### ORDINE DI SCUDERIA: RIDIMENSIONARE IL PROBLEMA

- [LinkedIn post](https://lnkd.in/p/dUkD2Rq2) &nsbp; (2026-11-08)

La scoperta della vulnerabilità CVE-2026-123158 documentata anche dal NIST e che richiede di adottare un "defensive coding approach" specifico anche in progetti come BusyBox sembra aver scatenato una reazione piuttosto peculiare. Qualcosa che non avevo mai visto prima: negazione (è uno scherzo), panico (reazione infondata), realizzazione (toolchain e embedded systems da aggiornare), rfiuto (sminuire il pericolo).

Il primo screenshot parte da UNIX ma quello è normale. INel 2012 c'è una decisione da parte del team di sviluppo musl (altenativa alle librerie GNU) piuttosto discutibile perché se i programmatori non sanno gestirsi il NULL come valore lecito di ritorno dovrebbero starsene confinati in un ambiente tipo .NET e non programmare in C. Qui qualche domanda dovremmo cominciarcela a porcela. La scelta di ritornare NULL è documentata nei sorgenti di musl, non è un'ipotesi.

La vulnerabilità compare nel 2016. Però non è una vulnerabilità "assoluta", è un corner case di configurazioni. Occorre usare musl (sistemi embedded, Alpine e poche altre distro fra cui OpenWrt), occorre che non si faccia uso del sistema PAM (sistemi embedded, Alpine e poche altre distro fra cui OpenWrt) almeno un utente in /etc/shadow sia configurato e per ottenere l'accesso come root occorre che root sia configurato con asterisco. Che è esattamente la configurazione scelta da Alpine.

Dove interviene il ridimensionamento del pericolo in questo screenshot? Affermando che il problema emerge nel 2022. No, diventa progressivamente più probabile vederlo "in the wild" ma il problema è precedente. Per altro quella scelta NON è assurda. Anzi è molto più sensata di quella del 2012 ed è la stessa scelta che generalmente si adotta nei sistemi esposti in rete con SSH (porta 22) in listening per accedere come root e configurare l'account con asterisco per usare l'autenticazione passwordless via chiave crittografica è non solo comodo (fleet management) ma piuttosto comune.

Relativamente pochi adottano la pratica di usare admin per poi fare su/do. La ragione è che questa scelta più in linea con i fondamentali dell'amministrazione di sistemi UNIX dove root non è mai login (carattere punto esclamativo) ma basata su paradigmi di amministrazione con personale umano al terminale (console). Oggi questo approccio è superato, tendamo ad usare sistemi di gestione automatica dei container o più in generale relativi al mondo IoT delle flotte.

Qui cominciano i sudori freddi e la necessità di ridimensionare il problema per calmare i clienti in panico. I sistemi embedded fra cui SCADA e IoT, così come i router (OpenWRT) adottano l'approccio Alpine (generalmente o su richiesta) perché usare la chiave crittofrafica per root è decisamente più sicuro, più comodo e in linea con l'amministrazione delle flotte. Cioè la stessa configurazione che Alpine ha scelto!

<br>

## THIS BUG IS LIKE A JOKE!

- [LinkedIn post](https://lnkd.in/p/dqhyKRgn) &nsbp; (2026-11-05)

On Alpine Linux, type 257 to 1024 characters of anything and you get root. 

By now, "they just made a login" sounds in a completely another way... 🤗 🍌 

~> https://lnkd.in/dPsHmwzE (CVE on NIST)

A critical vulnerability in OpenSSH that remained hidden for ten years, since 2016 and also the name is like a joke: *sshole!

### SSHole mitigation and chpasswd scripting

- [LinkedIn post](https://lnkd.in/p/dkBWwaVr) &nsbp; (2026-11-06)

Let me explain something that it is not universally obvious about this vulnerability and these two patchsets:

1 -  mitigation doesn't mean resolving or fixing, but preventing and defensive coding (aka we learn the lesson)

2 - since the problem can cause a problem of password changing for all the users, scriptability is necessary

AFAIK, current busybox (and also the previous versions) do not have any mitigation and chpasswd applet exit() at the first user for which the password fails to update. Hence the script fails to change part of the list. Therefore the second patch aims to fix this shortcoming.

All these two steps are necessary as first emergency response. In particular, patching, recompiling and deploying busybox can be faster than updating the whole toolchain. While installing a separate chpasswd as a standalone binary is a viable temporary option when updating the busybox version isn't possible.

This buys time, this allows companies to face the emergency but it doesn't solve the problem. It solves the collapse of the IT department and at the same time jeopardises the career of their managers. Why? Let them learn by themselves.

All of this whatever my name (copyleft is based on copyright, for those who eventually forgot it) is reported and my contribution is adequately acknowledged or not. And if not reported, then it might cause another issue on top of issues list described above which explicitly isn't exhaustive. Ergo, whatever they take the *sshole vulnerability, it remains a tsunami of sh*t.

- Spoiler: reality never allows free rides, but TCMO applies.

TCMO means Total Cost of an Ownership Mistake, aka cumulative debt generates negative interests by network externalities (complexity unchallenged) and the cost increases exponentially in time. Guess what? Reality never negotiates the bill, also.

---

### LINUX CVE-2026-98432 OR A BACKDOOR?

- [LinkedIn post](https://lnkd.in/p/dr3HsUjm) &nsbp; (2026-11-05)

Every Linux system which includes Android Smartphone has a root remotable backdoor inside since 2016. This is another way to see the same thing from the perspective of who knew the purpose in advance of that change (or got it soon after its introduction).

Isn't an affirmation, it is a plausible scenario, it is a legit question.

Another legit question is about the Open Source paradigm: many eyes watching the source code brings in more security because vulnerabilities are discovered faster and got fixed. It seems rational, so has it failed?

- https://lnkd.in/p/df7GkY_w (post that explain why not)

In short because "The Tragedy of Commons": everybody takes AS-IS and few pay attention, or developers, or reviewers. The gatekeepers are more interested in the bureaucratic aspects, while janitors about code readability and maintainability. And developers are more focused on features and testing.

Insecurity by complexity, never has been taken in account. And this aspect is related to big tech companies and national security agencies. The first are focused on profit, the former are focused on certifications. None of them care about the "what if" and this is where the Tragedy of Complexity kicks into the Tragedy of Commons.

- Two sides of the same coin.

How much artificial intelligence creates this problem? The answer relies on the "unreleased AI model" which inspected public code. Those kinds of models are also inspecting proprietary code. The incentives are asymmetric: a company should pay for inspection and allow the access while inspecting the kernel brings in a proof-of-concept, a study case, something that the marketing needs to push big companies to open their wallet.

- Look, the kernel has 9.8 breach: are your OS also? Pay to check.

 The AI did not change the paradigm nor falsified it. AI added those eyes that companies and agencies never paid for having humans watching the code. Or they paid not as much as needed or without planning wisely. Open Source fails? It isn't a tragedy, after all. They are just hobbyists, not a profitable company. Azure, Amazon, Google, etc. are profitable companies which leverage Open Source.

The Open Source paradigm isn't wrong. The "greedy is good", creates the Tragedy of Commons. The AI revealed the Tragedy of Complexity. The original sin is rotted in having imposed a single business paradigm by the suppression of every other alternative.

In short, the Tragedy of Commons in Open Source did not happen for an accident but was planned: they would find out by themselves, and in the meantime we took. Great, so what is missing here? The TCMO (total cost of a mistake ownership) which in short it means: the Tragedy of Commons isn't a destiny but a warning. It happened once, then the lesson should have been learned not exploited by free riding.

---

### THE PHP PATCH JUST CAME OUT

- [LinkedIn post](https://lnkd.in/p/df7GkY_w) &nsbp; (2026-10-21)

And that's when the real danger begins because everyone able to understand the difference between the old codebase and the new one has received the minimum necessary information to understand the underlying vulnerability.

Is this a failure of Open Source? Wasn't it more secure because many eyes watching the code make the code more secure? The first question's answer is the second question. They take the code without contributing to it, without even inspecting it. Who are they?

The Dutch parliament, the Dutch national cyber agency and Finland's central government have pulled websites offline ahead of the PHP patch. They took AS-IS and then deployed AS-IS.

Politics do it as usual but the national cybersecurity agency? Aren't they paid to inspect code before deploying it? They don't because their HR policy is politics-biased and politics never promotes by merit, never supports participation.

Politics counts votes therefore creates and supports vote collectors which are in charge of winning elections and have no clue about what is necessary to make the system work, the system on which they sit.

In the last 25 years everything was working, apparently. Because the least-effort as IT policy (aka IT people as blue-collars) never hits a real war nor a people insurrection, none of any kind apart from staged crysis.

The 25 years IT debt that accumulates is just the tip of the iceberg, not the event nor the cause. The root cause is having ignored the problem for so long that the problem hits like a huge immense iceberg.

In Italy we have an old saying that recites: "finché la barca va, lasciala andare" which translated is "until the ship goes, let it go" but the correct translation in English is "only dead fishes always follow the flow".

That's it, my dear dead fishes. 🤗 🍌

<br>

## Il mercato non comprende l'IT

- [LinkedIn post](https://lnkd.in/p/db9t93U3) &nsbp; (2026-11-05)

Lenovo ThinkPlus FU100, cifratura AES-256 ma dati in chiaro su microSD.

Un ricercatore ha smontato una chiavetta USB con lettore di impronte digitali prodotta da Lenovo, scoprendo che, nonostante la promessa di cifratura AES-256, i file sono leggibili su una comune scheda SD accessibile a chiunque abbia fisicamente il dispositivo. 

Lenovo ha confermato che la stessa architettura è condivisa dai modelli FU200, TU800 e TSD303. L'azienda ha pubblicato l'avviso LEN-210545 il 13 gennaio 2026.

Sintesi: prezzo premium, impronte digitali, cifratura top-civile, la apri e la leggi con un comunissimo lettore di carte microsd. Il mercato si orienta rispetto ai consumatori, necessariamente. Sarebbe sciocco pensare altrimenti.

