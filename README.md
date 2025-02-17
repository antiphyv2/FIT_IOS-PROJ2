# IOS - Projekt 2 (synchronizace procesů)

Zadání je inspirováno knihou Allen B. Downey: The Little Book of Semaphores (The barbershop problem).

Popis Úlohy (Pošta):
V systému máme 3 typy procesů: (0) hlavní proces, (1) poštovní úředník a (2) zákazník. Každý
zákazník jde na poštu vyřídit jeden ze tří typů požadavků: listovní služby, balíky, peněžní služby.
Každý požadavek je jednoznačně identifikován číslem (dopisy:1, balíky:2, peněžní služby:3). Po
příchodu se zařadí do fronty dle činnosti, kterou jde vyřídit. Každý úředník obsluhuje všechny fronty
(vybírá pokaždé náhodně jednu z front). Pokud aktuálně nečeká žádný zákazník, tak si úředník bere
krátkou přestávku. Po uzavření pošty úředníci dokončí obsluhu všech zákazníků ve frontě a po
vyprázdnění všech front odhází domů. Případní zákazníci, kteří přijdou po uzavření pošty, odcházejí
domů (zítra je také den).

Spuštění:
$ ./proj2 NZ NU TZ TU F
• NZ: počet zákazníků
• NU: počet úředníků
• TZ: Maximální čas v milisekundách, po který zákazník po svém vytvoření čeká, než vejde na poštu (eventuálně odchází s nepořízenou). 0<=TZ<=10000
• TU: Maximální délka přestávky úředníka v milisekundách. 0<=TU<=100
• F: Maximální čas v milisekundách, po kterém je uzavřena pošta pro nově příchozí. 0<=F<=10000
