# IOS-Projekt2
IOS – projekt 2 (synchronizace)
Zadání je inspirováno knihou Allen B. Downey: The Little Book of Semaphores (The barbershop
problem)
Popis Úlohy (Pošta)
V systému máme 3 typy procesů: (0) hlavní proces, (1) poštovní úředník a (2) zákazník. Každý
zákazník jde na poštu vyřídit jeden ze tří typů požadavků: listovní služby, balíky, peněžní služby.
Každý požadavek je jednoznačně identifikován číslem (dopisy:1, balíky:2, peněžní služby:3). Po
příchodu se zařadí do fronty dle činnosti, kterou jde vyřídit. Každý úředník obsluhuje všechny fronty
(vybírá pokaždé náhodně jednu z front). Pokud aktuálně nečeká žádný zákazník, tak si úředník bere
krátkou přestávku. Po uzavření pošty úředníci dokončí obsluhu všech zákazníků ve frontě a po
vyprázdnění všech front odhází domů. Případní zákazníci, kteří přijdou po uzavření pošty, odcházejí
domů (zítra je také den).
