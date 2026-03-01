## EXTREME DETERMINISTIC VIRTUAL MACHINE UCHAOS TESTING

- [LinkedIn post #1](https://www.linkedin.com/posts/robertofoglietta_extreme-deterministic-virtual-machine-uchaos-activity-7433603537630052352-JIj_)

uCHAOS is going to be tested against a strongly deterministic virtual machine which runs with a single virtualized CPU and the kernel is compiled with nearly "allnoconfig" without pre-emption (server) in single CPU mode only, and no support for random even for memory addressing.

- `gcc -D_USE_STOCHASTIC_BRANCHES`
- `dmesg | uchaos -i 16 -d 3 -qG 256 -r 32 | RNG_test-musl-static stdin64`
-  [bare minimal linux system](https://github.com/robang74/bare-minimal-linux-system/blob/main/README.md)

Testing previews are quite encouraging and after a few adjustments uChaos compiled with the support for stochastics branching and running with very reasonable optionals parameters passed flawlessly the 4GB practrand test.

Parametric settings explanation: -i16 is for reading the first 8Kb from dmesg; -d3 consider an exception to reschedule every return time within +3 and -3 ns around the previous minimum; -r32 do few dry-run to stabilise statistics but less dry-runs more stochastics branching and -qG256 is to quietly provide 256GB in output without calculating statistics to print)

The main objection could be -r1 (default) with stochastics branching that might create a sort of initial transient (it doesn't) and testing on 5.15.201 which is the LTS version with the .config available (test repeatability). Guess what? Testing "preview", here __preview__ is the keyword.

- [uChaos v0.2.4](https://github.com/robang74/working-in-progress/releases/tag/uchaos-v0.2.4)

In the meantime the v0.2.4 (tagged) managed to pass flawlessly the 256GB practrand test on a i5-8635 bare-metal CPU. That's why testing with strongly deterministic virtual machine commenced.

---

## QUI ABBIAMO UNA COSA DAVVERO INTERESSANTE!

- [LinkedIn post #2](https://www.linkedin.com/posts/robertofoglietta_extreme-deterministic-virtual-machine-uchaos-activity-7433621676963098624-sIha)

Guardiamo il log qui sotto, fino a 1GB tutto bene, poi comincia a franare e a 4GB ha proprio fallito strutturalmente e con un "birthday" cioè una tragedia, significa che questo "coso" che dovrebbe creare numeri casuali si ripete con un ciclo di 4GB. 

Aspe' un momento. Questo "coso" è gira su una VM che è un metronomono (poi la peggioro pure, state tranquilli, la porto a lavorare in stato di oscillazione precisa, me per ora come antipasto va bene). Ha un solo CPU thread fisico associato, ha un solo processore virtuale, il kernel supporta un singolo core (!SMP) non ha il pre-emption (server mode), non ha gestione casuale delle allocazioni di memoria neanche per il kernel e neanche per il boot e non ha rete, niente è prossimo a un allnoconfig a parte che mi serve la seriale per la console altrimenti diventa "isolato" per davvero.

Quindi uchaos è l'unico processo attivo, perché non ha servizi, e non ha accesso I/O disk perché è tutto in RAM anche la initramfs. In termini di temperatura questo è un frezeer.  Eppure uchaos regge per 1GB, e i più cattivi (come le AI) diranno che però ad ogni boot vomita sempre lo stesso identico 1GB (se anche fosse, ho compresso un GB di dati incompressibili in 50Kb e quindi baciate il suolo dove cammino!) ma anyway, siamo seri... LOL

La parte "succosa" è che le versioni 0.25.x ha i stocastics branching, e quelli sono imprevedibili finché le statistiche non si stabilizzano e quando si stabilizzano è gelo a 4GB (con -d7 ma con -d63 ? 😊 ).

```text
Run /init as init process
MemTotal:    1025432 kB
MemAvailable:  1008044 kB
Temporary /dev/shm size is 251570 kB of RAM
Linux u-bmls 5.15.201 #2 Thu Feb 26 22:58:17 CET 2026 x86_64 x86_64 x86_64 GNU/Linux
Active console: ttyS0, entropy: 1 bits

   ===============================================
   Welcome to Minimal Linux System for QEMU x86_64
    System dmsg time was 0.25s at the 1st console
   ===============================================

/ # dmesg | uchaos -i 16 -d 3 -qT 4 -r 7 -k /dev/random >/dev/null

uChaos: v0.2.5.2 w/sb; s(0), d(3ns), p(0), r(7), RTSC(0)

/ # dmesg | uchaos -i 16 -d 3 -qG 1024 -r 7 | RNG_test-musl-static stdin64

uChaos: v0.2.5.2 w/sb; s(0), d(3ns), p(0), r(7), RTSC(0)

RNG_test-musl-static using PractRand version 0.96
RNG = RNG_stdin64, seed = unknown
test set = core, folding = standard (64 bit)

...

length= 1 gigabyte (2^30 bytes), time= 270 seconds
 no anomalies in 227 test result(s)

length= 2 gigabytes (2^31 bytes), time= 520 seconds
 Test Name             Raw    Processed   Evaluation    
 BDayS2(4,24)[60]         R= +6.3 p~= 4.9e-8  very suspicious  
 [Low1/64]BCFN(2+0,13-4U)     R= +9.5 p = 4.4e-4  unusual      
 ...and 240 test result(s) without anomalies

length= 4 gigabytes (2^32 bytes), time= 1031 seconds
 Test Name             Raw    Processed   Evaluation    
 BDayS2(4,24)[63]         R= +25.1 p = 1.6e-95  FAIL !!!!!   
 ...and 255 test result(s) without anomalies
```
---

## USARE RAMI STOCASTICI SULLA CPU REALE

- [LinkedIn post #3](https://www.linkedin.com/posts/robertofoglietta_extreme-deterministic-virtual-machine-uchaos-activity-7433638270531424256-5LOw)

La cosa interessante è che testando la configurazione ottimale per una VM a metronomo su una CPU reale, PractRand talora si lamenta di deboli anomalie "unusual" nel primo GB di dati ma non fallisce. Il che indica che tale configurazione è "eccessiva" dove lo jitter è reale.

Ad intuito direi che -d7 -r31 è la scelta azzeccata, 31 cicli di preriscaldamento prima di create output e +/- 7ns di range intorno al minimo. 

Gemini la spiega così e non è distante dal vero: -d63 è troppo ampio smette di essere sensibile alle microvariazioni e per creare chaos servono almeno 7 sponde da biliardo ma in statistica "matematica" il numerp che viene considerato d'inizio è 30

In effetti anche qui servono almeno 30 passaggi per avere un minimo di "tatto" con il sistema sottostante. Lo scopo è fare leva su almeno 7 biforcazioni concentrate, di cui le prime 1:2:4:8:16 nella parte iniziaòe. Sommando si arriva 31 appunto, e quindi sono 4 biforcazioni stocastiche nei primi 31 dry-run silenziosi.

### Perché questa configurazione è strategicamente corretta:

L'effetto "Trigger": Con un range così stretto (7ns), l'algoritmo non diventa un generatore puramente algoritmico (PRNG), ma resta costantemente "appeso" alla micro-stocasticità dell'host.

I primi 31 cicli di dry-run: se anche due VM partissero con lo stesso timestamp, le piccole differenze nei primi sched_yield() verrebbero accumulate e amplificate prima che il primo bit di output venga generato.

### Le tre ragioni di biforcazione stocastica

Avere tre sorgenti diverse di caos è fondamentale per coprire scenari differenti:

1. - MIN (dlt < dmn): Indica che hai trovato un nuovo "pavimento" di latenza. È la ricerca della massima velocità della CPU. All'inizio è frequentissima, poi diventa rara (1/2^n).

2. - MAX (dlt > dmx): Cattura i "singhiozzi" del sistema. È l'entropia più pura perché deriva da interrupt esterni o pre-emption dell'host che ritardano l'esecuzione della VM.

3. - RNG (excp / -d 7): Questa è la tua "sonda". Invece di aspettare un record (MIN/MAX), c'è una biforcazione nella "zona grigia" di 7ns. È quella che mantiene il sistema non-deterministico nel lungo periodo.

il kernel Linux soffre della "fame d'entropia" proprio nei primi secondi di boot (quando deve generare chiavi SSH, UUID o seed crittografici), mentre uChaos è una "supernova" che dà il massimo proprio in quel brevissimo istante per poi spegnersi in poche decine di secondi approcciando il muro del primo 1GB e poi cede se non viene agitato dall'esterno.

Ma questo è normale, sarebbe magia altrimenti e la magia non convincerebbe nessun esperto: i sistemi reali falliscono in certe condizioni e ci si aspetta che lo facciano altrimenti non è una buona notiza perché allora sarebbe la teoria ad essre sbagliata (oppure il dogma dell'hardware, vero ma non assoluto). Però uchaos fallisce a 5 o 6 ordini di grandezza oltre il limite del suo utilizzo come seeder al boot, e non è poca cosa...

---

## NON È POCA COSA, E NON È TANTO PER DIRE

- [LinkedIn post #4](https://www.linkedin.com/posts/robertofoglietta_extreme-deterministic-virtual-machine-uchaos-activity-7433659456749690881-nwTV)

Un'altra cosa: è interessante notare la questione della sicurezza. Un attacante che controlla la VM dovrebbe gestire i suoi jitter a risoluzione sub-nanosecond, e visto che uchaos fallisce a 5 o 6 ordini di grandezza non sarebbe strano che dovesse poterla controllare con una risoluzione di almeno la metà degli ordini di grandezza, il picosecondo.

Ma a quel punto anche tutte le altre sorgenti di entropia del kernel che sono molto meno sensibili collassrebbero. Infatti è così, il kernel 5.15.201 strilla in printk dell'assenza di entropia. 

Per "ghiacciarlo" ulteriormente ho dovuto dargliela con uchaos. Altrimenti a forza di printk continuava a generare entropia e uchaos non si fermava andava avanti oltre i 4GB e questo significava che c'era qualcosa di "strano" e infatti erano le printk del kernel affamato di entropia.

Zittite le printk, uchaos "finalmente" collassa oltre il primo milairdo di byte 2^30 bene, 2^31 cede, 2^32 fallisce. Giusto così, se il sistema si è congelato in un cristallo temporale (metronomo di alta precisione), non c'è più entropia da estrarre.

Quindi si è anche trovata la soluzione per andare avanti all'infinito (in teoria) nel senso che sapendo qual'è il limite (1GB) quando lo si raggiunge si resettano le statistiche (anche prima) e quindi il branching stocastico riparte di nuovo (dopo NN cicli di dryrun che però sono circa 8-10ms di pausa, non certo un dramma) e BOOM altra supernova di entropia!

### Con il multi-tasking si potrebbe andare oltre

Se poi si usa il comando mtrd che ho scritto in tempi non sospetti e permette di lanciare N thread il cui output viene miscelato in un solo output

- [mtdr.c, il N-mixer](https://raw.githubusercontent.com/robang74/working-in-progress/refs/heads/main/prpr/mtrd.c)

Allora posso far girare in concorrenza 4 istanze di uchaos che dovendo spartirsi una sola CPU saranno costrette ad interferire anche con mtrd che nel frattempo fa il mixing dei flussi. Insomma, c'è una buona possibilità di trasformare la più deterministica delle VM isolate in un potenziale micro-server di numeri casuali: hai bisogno di random? eccoti servito! LOL

Ecco fatto anche il reset periodico delle statistiche:

```sh
for i in $(seq 1 32); do dmesg | uchaos -i 16 -d 3 -qM 128 -r 31;
    echo $i; done | RNG_test-musl-static stdin64`
```

anche senza multi-threading (che poi dicono che baro) e se funziona in sequenza allora non c'è trucco e non c'è inganno!

```text
length= 4 gigabytes (2^32 bytes), time= 1012 seconds
  no anomalies in 256 test result(s)
```

In fact, the 4GB test passed flawlessy and at nearly 4MB/s because the quiet option -q avoids all those doings that aren't strictly related with the production of the output. Così anche il reseeding è garantito. Sotto questo punto di vista, uchaos si pone come la sorgente di entropia quando tutte le altre non riescono ad essere fertili o non abbastanza.

---

## O' TU LETTORE CHE GIUNGESTI SIN QUA

Valutate quello che è stato scritto in questa catena di post e in particolare da questa prospettiva: l'entropia ha molto più a che fare con l'informazione che con la realtà materiale.

Infatti quando uchaos è sull'orlo del collasso zero-entropia perché incapacitato dall'informazione stessa che ha raccolto sul sistema (statistiche) le dimentica (informazione distrutta) e ritorna ad estrarre entropia con l'ignoranza base di chi vede il sistema per la prima volta. 

O' tu lettore che giungesti fin qua, dimmi tu cosa ne pensi...

### Nel frattempo QEMU con -accel tcg -nodefault

Con le opzioni -accel tcg -nodefault, l'emulazione è ridotta ai mini termini ma poiché "less is more" ho deciso di metterci pure -M microvm e -icount con le opzioni più fortemente deterministiche e minimaliste che ho trovato.

In particolare -icount serve per intercettare la syscall del tempo in nanosecondi e iniettare in uchaos il numero di istruzioni eseguite dal tempo t=0 di inizio boot. Quindi determinismo assoluto anche sul clock, motivo per il quale si è dovuto rinunciare all'accelarazione KVM perché ora non solo lo scheduler gira in un ambiente virtuale ma pure la CPU è software!

Il nulla in termini di dispositivi emulati a parte la console seriale, comunque anch'essa virtualizzata, senza la quale non si potrebbero nemmeno digitare ne leggere gli output. Eppur, si muove ed sebbene lo faccia tipo a 4 volte più lentamente, uchaos ha comunque avuto il tosto ardire

```text
length= 256 megabytes (2^28 bytes), time= 859 seconds
  no anomalies in 199 test result(s)
```

di cavare 256 MB di numeri casuali da una macchina virtuale estrema in termini di essenzialità e determinismo. Sono pochi, ma sono senza alcuna macchia e alimentando /dev/random con questo seed anche considerando per prudenza che contenga un bit di entropia per ogni byte sono un milione di volte oltre al necessario per il boot seed.

Un'immensa quantità di "boh non strutturato" specialmente se si considerano le performance di una macchina viratuale interamente sw e prodotta in un millesimo del tempo necessario ad una macchina dotata di un minimo di scopo pratico di fare un milione di boot.

- [0°K qemu linux, il commit](https://github.com/robang74/working-in-progress/commit/d37c7548d01d3dc0800f0e41120fedc9e7789193)

Se penso che con un hash non crittografico inventato prima della teoria stessa e specializzato per il solo testo, operazioni di base come moltiplicazioni di avalanche, xor e rotazioni di registri, whitening finale 1/3 di murmur3, ho estratto 1GB di randomness ad 1MB/s da una macchina virtuale 100% software e meccanicistica anche sul clock di sistema, mi viene da ridere. Letteralmente, però resto umile e mi limito a sorridere.

---

(c) 2026, Roberto A. Foglietta <roberto.foglietta@gmail.com>, CC BY-ND-NC 4.0
