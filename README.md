## Projet IA M1 info
### Puissance 4 algorithme MCTS

* Compilation et execution
~~~
unzip IA_connect4.zip
cd IA_connect4/
mkdir build
cd build
cmake ..
make && ./jeu y max 1.45
~~~
avec premier param ```[y/n]``` : optimisation (toujours choisir un coup gagnant lorsque c'est possible)
<br>avec deuxieme param ```[rob/max]``` : methode du choix du meilleur coup a la fin de l'algo<br> (robuste : rob, ou maximum : max)<br>avec troisieme param : temps d'exécution maximum des itérations de MCTS (nombre décimal ou entier)

* une version exécutable est disponible sous /out si l'outil ```CMake``` n'est pas installé
~~~
cd IA_connect4/out/
./jeu y max
~~~
* commande gcc :
    * ```gcc -O3 jeu.c -o jeu -lm && ./jeu y max```  
