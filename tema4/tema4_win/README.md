		Copyright @ 2021 Grecu Andrei-George 335CA, All rights reserved
			   
Tema 4	Planificator de threaduri

TEXT : https://ocw.cs.pub.ro/courses/so/teme/tema-4
GITHUB : https://github.com/grecuandrei/THREAD_scheduler_SO

## Structura

	Pentru tema mi-am creeat 2 structuri:
	- params, care contine caracteristicile specifice fiecarui thread (task) in
	parte la un anumit moment (prioritate, timpul de executie ramas, functia pe
	care o executa, statusul);
	- scheduler, care contine informatiile globale ale planificatorului, cum ar fi
	valorile threadului curent (cele de mai sus), cuanta de timp maxima pentru
	toate taskurile, numarul de evenimente dupa care se asteapta (io);

## Tema

	Tema mi-a fost utila in intelegerea modului de planificare si organizare al threadurilor
	pe care un procesor obisnuit le executa. Implementarea cozii de prioritati este una
	triviala, intrucat am folosit o lista inlantuita, care are timpul de inserare liniar,
	spre deosebire de un heap binar, de exemplu, unde ar fi fost logaritmic.

## Implementare

	Nu am implementat functiile so_wait si so_signal. Dificultatile intampinate au fost
	cele obisnuite de sincronizare, lucrul cu mutexuri, in special deadlock-urile.

## Logica

	Fiecare thread este introdus in coada de asteptare, dupa ce este blocat cu ajutorul
	unui mutex (lock) - functia so_fork. Apoi este apelata o functie care decide care
	este threadul care trebuie sa ruleze in momentul curent de timp. Pentru acest thread
	voi debloca mutexul asociat threadului blocat anterior.

	In momentul in care un thread se termina, se apeleaza din nou functia de planificare,
	un alt thread (cel cu prioritatea cea mai mare) este extras din coada (unlock) si executat
	in mod similar (se simuleaza functia asociata lui), iar apoi este introdus in coada
	de threaduri terminate. La final se face join intre toate threadurile ce si-au terminat
	executia (se gasesc in finished_threads) si se elibereaza resursele asociate.

## Teste
    
    Nu am intalnit probleme in cadrul testelor

## Resurse utilizate

	- laboratoarele 8 si 9 (ocw)
	- cursurile despre planificarea si executia threadurilor
	- header-ul din enunt
