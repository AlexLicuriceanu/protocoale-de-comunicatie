
# Tema 2 Protocoale de Comunicatie - Aplicatie client-server TCP si UDP pentru gestionarea mesajelor

# Autor

- [@AlexLicuriceanu](https://www.github.com/AlexLicuriceanu)

# Organizare

## Structuri de date pentru mesaje
* struct udp_msg: Reprezinta structura de date folosita pentru a incadra mesajele UDP care vin de la clientii UDP.
    - char topic[]: numele topicului.
    - unsigned char type: tipul payload-ului.
    - char data[]: payload-ul mesajului. 

* struct tcp_msg: Reprezinta structura de date folosita pentru a incadra mesajele TCP care sunt transmise de server ori primite de la clienti ca cereri de conectare sau deconectare.
    - char type: tipul mesajului.
    - char data[]: payload-ul mesajului.

* struct subscribe_msg: Reprezinta structura de date folosita pentru a incadra mesajele de subscribe/unsubscribe primite de la clientii TCP.
    - int type: tipul mesajului, 0 pentru unsubscribe, 1 pentru subscribe.
    - char topic[]: numele topicului.
    - int sf: flag pentru optiunea de store-and-forward, 0 pentru abonare fara SF, 1 pentru abonare cu SF.   

## Structuri de date folosite de server
* id_socket: Map care leaga identificatorul clientului cu socketul acestuia.

* socket_id: Map care leaga socketul cu identificatorul clientului.

* topic_clients: Map care leaga un topic cu clientii abonati la acesta. Intrarile din map-ul acesta vor fi modificate doar cand un client trimite comanda de unsubscribe, nu si cand un client se deconecteaza de la server.

* sf_client_topics: Map care leaga identificatorul clientului doar cu topicurile la care acesta este abonat cu optiunea de store-and-forward.

* client_waiting_msgs: Map care leaga identificatorul unui client cu o lista de mesaje care au fost primite de server si care contin topicuri la care clientul este abonat cu store-and-forward.

## Detalii implementare

### Programul pentru server

#### Pasul 1: Setup

* Dezactivez stdout buffering.
* Pornesc socketi pentru TCP si UDP.
* Dezactivez algoritmul lui Nagle.
* Creez o lista cu descriptorii de fisiere.
* Pornesc bucla infinita.

#### Pasul 2: Rularea server-ului

* Monitorizez toti descriptorii de fisiere (mulitplexare).
* Pentru fiecare descriptor din lista, selectez socketul pe care se afla date.
* De aici reies 3 cazuri:

    1. Daca vin date pe socket-ul de UDP:
        - Citesc mesajul intr-o structura de tipul udp_msg.
        - Creez un nou mesaj TCP cu structura tcp_msg.
        - In corpul mesajului TCP, copiez mesajul UDP primit.
        - Gasesc toti clientii care sunt abonati la topicul din mesajul primit.
        - Verific daca exista clienti care nu sunt conectati dar sunt abonati la topic cu optiunea de store-and-forward si pun mesajul in lista de asteptare pentru clientul offline.
        - Altfel, transmit mesajul TCP la clientii care sunt conectati si abonati la topic.

    2. Daca vin date pe socket-ul de TCP:
        - Iau identificatorul primit in mesaj.
        - Verific daca exista deja un client conectat care are acelasi identificator si in caz afirmativ, refuz conexiunea si afisez un mesaj de eroare.
        - Daca nu exista un client conectat cu acelasi nume, folosesc structurile id_socket si socket_id pentru a memora numele si socketul clientului.
        - Verific daca exista mesaje in asteptare in client_waiting_msgs pentru clientul acesta. Daca exista, creez noi pachete TCP si le trimit la client in ordinea in care au fost primit de server cat timp clientul era offline. Dupa ce au fost trimise toate, intrarea pentru acest client este stearsa din lista pentru clienti cu mesaje in asteptare.

    3. Daca vin comenzi de la STDIN:
        - Citesc comanda si verific daca aceasta este "exit", fiind singura la care raspunde serverul.
        - Iterez toti descriptorii de fisiere, inchid toate conexiunile, inchid bucla infinita, socketii de TCP si UDP si programul se termina. 

* La deconectarea unui client ii setez intrarea din id_socket pe -1 pentru a marca faptul ca acesta a mai fost conectat anterior, lucru ce ma ajuta la store-and-forward si ii sterg complet intrarea din socket_id.
* Pentru mesajele de tip subscribe/unsubscribe, verific tipul mesajului:

    1. Daca este 0, clientul vrea sa se dezaboneze de la topic si doar il sterg din topic_subscribers.
    2. Daca este 1, clientul vrea sa se aboneze la un topic. Il adaug la topic_clients pentru topicul respectiv si verific daca vrea sa se aboneze cu store-and-forward. Daca vrea sa se aboneze cu store-and-forward, il adaug si la sf_client_topics.

### Programul pentru client

#### Pasul 1: Setup

* Dezactivez STDOUT buffering.
* Deschid un socket si pornesc conexiunea la server.
* Dezactivez algoritmul lui Nagle.
* Trimit un mesaj de conectare TCP catre server care contine identificator clientului.
* Pornesc bucla infinita. 

#### Pasul 2: Rularea server-ului

* Multiplexez intrarile, fie voi primi comenzi de la STDIN, fie mesaje de la server.
* Daca primesc comanda de subscribe/unsubscribe, creez un nou pachet TCP in care pun tipul mesajului, 0 pentru unsubscribe, 1 pentru subscribe, numele topicului si valoarea flag-ului pentru store-and-forward in cazul mesajului de subscribe.
* Trimit mesajul TCP la server si astept un raspuns.
* Daca serverul raspunde cu un cod de succes, afisez un mesaj pentru executarea cu succes a comenzii.
* Pentru mesajele venite de la server, mapez datele intr-un udp_msg si verific tipul mesajului UDP primit (sau altfel spus, tipul mesajului emis de clientul UDP):

    1. Daca tipul este 1, mesajul contine un int, extrag octetul de semn si numarul, verific octetul de semn si afisez numarul in functie de acesta.
    2. Daca tipul este 2, mesajul contine un short real number, pe care il extrag si il impart la 100 conform cerintei.
    3. Daca tipul este 3, mesajul contine un float, extrag octetul de semn, numarul si puterea. Impart numarul la 10^putere si il inmultesc cu -1 in functie de octetul de semn.
    4. Daca tipul este 4, mesajul contine un string, si doar afisez ce se afla in payload-ul mesajului UDP primit.
