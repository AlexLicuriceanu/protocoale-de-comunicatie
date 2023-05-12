Protocoale de Comunicatie - Tema 1 - Implementare dataplane router
Alexandru LICURICEANU - 325CD

0) Main:
	- Citesc tabela de rutare si o sortez in ordine descrescatoare atat dupa prefix,
	cat si dupa masca.
	- Primesc un pachet, verific ce protocol este incapsulat.
	
	- Daca este un pachet IP, verific checksum-ul si daca checksum-ul calculat nu se
	potriveste cu cel primit, ignor pachetul.
		- Checksum-urile se potrivesc, verific daca in pachetul IP primit se afla un
		mesaj ICMP destinat router-ului. 
		- Daca este incapsulat un mesaj ICMP si are TTL mai mare ca 1, trimit un ICMP
		echo reply. Daca TTL-ul nu este mai mare ca 1, trimit un ICMP time exceeded.
		- Daca nu este incapsulat un mesaj ICMP, dirijez pachetul mai departe in retea
		cu ajutorul functiei forward_packet().
		- De asemenea, daca exista un pachet destinat router-ului care nu are un mesaj
		ICMP incapsulat, acesta va fi ignorat.
		
	- Daca este un pachet ARP, verific tipul acestuia.
		- In cazul in care este un ARP request, verific destinatia acestuia.
		- Daca ii este destinat router-ului, completez header-ul de ethernet si trimit
		un ARP reply celui care a trimis request-ul.
		- Daca nu ii este destinat router-ului, il dirijez mai departe.
		
		- Daca pachetul primit contine un ARP reply, actualizez tabela ARP, scot din
		coada pachetul care astepta raspunsul si il trimit in retea.
		
1) Dirijarea pachetelor:
	- Functia forward_packet() se ocupa cu dirijarea unui pachet generic in retea.
	- Mi-am creat o structura separata numita packet pentru a stoca informatiile unui
	pachet de date.
	
	- Functia verifica integritatea pachetului prin checksum, daca nu se potriveste
	checksum-ul cu cel calculat pe datele primite, se ignora pachetul.
	- Verific TTL > 1, in caz contrar se ignora pachetul si se trimite un nou pachet
	ce contine un mesaj ICMP time exceeded.
	
	- Daca pachetul trece de aceste verificari, se calculeaza ruta cea mai buna cu un
	algoritm de longest prefix match implementat cu cautare binara. Daca nu se gaseste
	o ruta, trimit un mesaj ICMP destination unreachable.
	- Altfel, se decrementeaza TTL-ul si caut in tabela ARP, adresa MAC a urmatorului
	hop. 
		- Daca nu se gaseste o asemenea intrare in tabela ARP, pachetul este pus intr-o
		coada si trimit un ARP broadcast in retea.
		- Daca este gasit o adresa MAC pentru urmatorul hop, trimit pachetul pe cea mai
		buna ruta gasita.
		
2) Protocolul ARP:
	- Functia send_arp() se ocupa cu trimiterea pachetelor ARP in retea. Aceasta doar
	copiaza niste date primite ca parametru intr-un nou header ARP si il trimite in 
	retea.
	
3) Protocolul ICMP:
	- Functiile send_icmp() si send_icmp_error() se ocupa cu trimiterea de pachete ce
	contin ICMP.
	
	- send_icmp() doar copiaza datele, inverseaza adresele transmitator si receptor,
	inlocuieste tipul si codul mesajului ICMP si trimite noul pachet.
	- send_icmp_error() ia headerul IP vechi si 8 bytes din payload-ul vechi si le
	pune in payload-ul nou si trimite un nou pachet cu datele acestea.
	
*) LPM eficient.
	- Am implementat algoritmul de longest prefix match eficient in functia
	get_best_route() folosind o cautare binara.
	
	- Tabela de rutare este ordonata la inceputul programului in ordine descrescatoare
	atat dupa masca, cat si dupa prefix. Pentru aceasta sortare am folosit quicksort.
	
	- Algoritmul functioneaza la fel ca unul obisnuit, doar ca atunci cand gaseste o 
	potrivire, porneste inca o cautare binara de la indicele 0 la cel unde a fost gasita
	anterior potrivirea. Complexitatea algoritmului este O(logn)