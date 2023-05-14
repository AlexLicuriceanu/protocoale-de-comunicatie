
# Tema 4 Protocoale de Comunicatie - Client Web. Comunicatie cu REST API.

# Autor

- [@AlexLicuriceanu](https://www.github.com/AlexLicuriceanu)
- [@nlohmann](https://www.github.com/nlohmann) (Biblioteca de parsare pentru JSON)

# Organizare

* Am pornit cu implementarea de la codul din Laboratorul 9 - Protocolul HTTP, modificand usor functiile `compute_get_request()` si `compute_post_request()` pentru a functiona cu JSON-uri.

* De asemenea, am ales sa folosesc lohmann[^1] pentru parsarea JSON pentru ca a fost usor de inclus si utilizat in tema.

* In fisierul `client.cpp` se afla toata logica principala pentru client.

* Fisierele `requests.cpp` si `requests.h` contin implementarile pentru functiile care construiesc cereri HTTP de tip POST, GET si DELETE.

* Fisierele `buffer.cpp`, `buffer.h`, `helpers.cpp` si `helpers.h` contin diverse functii auxiliare pentru conectarea la server, trimiterea si primirea pachetelor, etc.

# Detalii implementare

Pe langa logica programului care urmeaza sa fie detaliata, am inclus si multe comentarii relevante in cod.

* Cookie-ul de sesiune si token-ul sunt salvate in variabile si vor fi actualizate pe parcursul unui flux de comenzi. 

* Se porneste bucla infinita si se citeste de la utilizator comanda. De aici rezulta cate un caz pentru fiecare comanda:

* Comanda `register` \
Citesc de la utilizator numele si parola. Validez datele primite verificand daca exista un spatiu in username sau parola. Creez un nou JSON care contine username-ul si parola, apelez `compute_post_request()`, trimit la server POST-ul, primesc raspunsul, il afisez si inchid conexiunea la server.

* Comanda `login` \
Asemanator cu comanda de register, doar ca este schimbat URL-ul iar in caz de logare cu success am scris functia `get_cookie()` care extrage cookie-ul trimis de server. Functia `get_cookie()` doar extrage subsirul care contine "Set-Cookie" si foloseste `strtok()` pentru a gasi doar cookie-ul efectiv.

* Comanda `enter_library` \
Deschid conexiunea, setez URL-ul, apelez `compute_get_request()` impreuna cu cookie-ul de sesiune si trimit mesajul la server. Din raspunsul server-ului extrag token-ul intr-un mod identic cu cel pentru cookie.

* Comanda `get_books` \
Deschide conexinuea la server, creeaza un GET request cu URL-ul corespunzator si afiseaza raspunsul server-ului.

* Comanda `get_book` \
Citesc de la utilizator ID-ul cartii, verific ca acesta sa contina doar numere. Deschid conexiunea la server, compun URL-ul pentru cartea cu ID-ul specificat de utilizator, trimit un GET request la server si afisez raspunsul.

* Comanda `add_book` \
Citesc datele cartii de la utilizator si verific daca pentru `page_count` a fost introdus un numar. Pentru celelalte campuri nu am considerat ca este necesara vreo validare deoarece ar trebui sa fie permis ca utilizatorul sa introduca spatii, numere, litere si alte caractere alfanumerice. Creez un JSON cu campurile cartii, setez URL-ul corespunzator si apelez `compute_post_request()`. Trimit mesajul la server, afisez raspunsul si inchid conexiunea.

* Comanda `delete_book` \
Citesc ID-ul de la utilizator, verific sa fie un numar, deschid conexiunea, compun URL-ul cartii si apelez `compute_delete_request()` pentru a crea mesajul. Trimit mesajul la server, afisez rapsunsul si inchid conexiunea.

* Comanda `logout` \
Creez un GET request pentru URL-ul de logout, il trimit, afisez raspunsul si sterg cookie-ul si tokenul salvat.

* Comanda `exit` \
Break.


[^1]: https://github.com/nlohmann/json
