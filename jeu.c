#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <float.h>
#include <string.h>

// Paramètres du jeu
#define LARGEUR_MAX 7		// nb max de fils pour un Node (= nb max de coups possibles)
#define TEMPS 3			// temps de calcul pour un coup avec MCTS (en secondes)
#define HAUTEUR 6
#define LARGEUR 7

// Constantes/paramètres Algo MCTS
#define RECOMPENSE_ORDI_GAGNE 1
#define RECOMPENSE_MATCHNUL 0.5
#define RECOMPENSE_HUMAIN_GAGNE 0
#define CONSTANTE_C 1.4142136	// exploration parameter

// macros
#define AUTRE_JOUEUR(i) (1-(i)) // 0 si i=1, 1 si i=0
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))


/**
 * @brief Criteres de fin de partie
 *
 */
typedef enum { NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;


/**
 * @brief Méthode du choix du coup à jouer pour MCTS
 *
 */
typedef enum { MAXIMUM, ROBUSTE } MethodeChoixCoup;


/**
 * @brief Definition du type Etat (état/position du jeu)
 *
 */
typedef struct EtatSt {
	int joueur;
	char grid[HAUTEUR][LARGEUR];
} Etat;


/**
 * @brief Definition du type de coup
 *
 */
typedef struct {
	int col;	//pour Puissance4 juste choisir la col dans laquelle le jeton va tomber
} Coup;


/**
 * @brief Copie de l'état sr dans l'état etat
 *
 * @param src etat source
 * @return Etat* etat
 */
Etat *copieEtat(Etat * src) {
	Etat *etat = (Etat *) malloc(sizeof(Etat));
	etat->joueur = src->joueur;
	int i, j;
	for (i = 0; i < HAUTEUR; i++) {
		for (j = 0; j < LARGEUR; j++) {
			etat->grid[i][j] = src->grid[i][j];
		}
	}
	return etat;
}


/**
 * @brief Création de l'état initial
 *
 * @return Etat* etat initial
 */
Etat *etat_initial(void) {
	Etat *etat = (Etat *) malloc(sizeof(Etat));
	int i, j;
	for (i = 0; i < HAUTEUR; i++)
		for (j = 0; j < LARGEUR; j++)
			etat->grid[i][j] = ' ';
	return etat;
}


/**
 * @brief afficher l'état courant du jeu
 *
 * @param etat etat courant
 */
void afficheJeu(Etat * etat) {
	int j;
	printf("   |");
	for (j = 0; j < LARGEUR; j++)
		printf(" %d |", j);
	printf("\n");
	printf("--------------------------------");
	printf("\n");
	for (int i = 0; i < HAUTEUR; i++) {
		printf(" %d |", i);
		for (j = 0; j < LARGEUR; j++)
			printf(" %c |", etat->grid[i][j]);
		printf("\n");
		printf("--------------------------------");
		printf("\n");
	}
}


/**
 * @brief Création d'un nouveau coup
 *
 * @param i colonne
 * @return Coup* coup
 */
Coup *nouveauCoup(int i) {
	Coup *coup = (Coup *) malloc(sizeof(Coup));
	coup->col = i;
	return coup;
}
/**
 * @brief Demander à l'humain quel coup jouer
 *
 * @return Coup* coup à jouer
 */
Coup *demanderCoup() {
	int i = -1;
	printf(" Quelle colonne [0-6] ? ");
	(void)scanf("%d", &i);
	return nouveauCoup(i);
}


/**
 * @brief  Modifier l'état en jouant un coup
 *
 * @param etat etat courant
 * @param coup coup à jouer
 * @return int  0 si le coup n'est pas possible
 */
int jouerCoup(Etat * etat, Coup * coup) {
	//le coup est-il possible ? la colonne est-elle pleine, la ligne 0 est-elle occupee ?
	if (coup->col < 0 || coup->col >= LARGEUR || etat->grid[0][coup->col] != ' ') {
		return 0;
	} else {
		int playedRow = -1;
		for (int line = HAUTEUR-1;line >= 0 && playedRow == -1;) {
			if (etat->grid[line][coup->col] != ' '){	//emplacement deja pris
				line--;		//on remonte la colonne
			}
			else{		//emplacement inoccupe
				playedRow = line;
			}
		}
		if (playedRow == -1){
			return 0;
		}
		etat->grid[playedRow][coup->col] = etat->joueur ? 'O' : 'X'; //on joue le coup
		etat->joueur = AUTRE_JOUEUR(etat->joueur);// à l'autre joueur de jouer
		return 1;
	}
}


/**
 * @brief coups jouables
 *
 * @param etat etat du jeu
 * @return Coup** liste de coups possibles à partir d'un état
 */
Coup **coups_possibles(Etat * etat) {
	Coup **coups = (Coup **) malloc((1 + LARGEUR_MAX) * sizeof(Coup *));
	int i, k = 0;
	for (i = 0; i < LARGEUR; i++) {

		if (etat->grid[0][i] == ' ') { //si la ligne 0 est vide
			coups[k] = nouveauCoup(i);	// on peut jouer dans cette colonne
			k++;
		}
	}
	coups[k] = NULL; // marqueur de fin de liste
	return coups;
}


/**
 * @brief nombre de coups possibles
 *
 * @param etat etat du jeu
 * @return int nombre de coups possibles
 */
int nb_coups_possibles(Etat * etat) {
	int ret = 0;
	for (int i = 0; i < LARGEUR; i++) {
		if (etat->grid[0][i] == ' '){
			ret++;// on peut jouer dans cette colonne
		}
	}
	return ret;
}


/**
 * @brief definition du type node
 */
typedef struct NoeudSt {
	int joueur;			// joueur qui a joué pour arriver ici
	Coup *coup;			// coup joué par ce joueur pour arriver ici
	Etat *etat;			// etat du jeu
	struct NoeudSt *parent;
	struct NoeudSt *enfants[LARGEUR_MAX];	// liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;		// nb d'enfants présents dans la liste
	// POUR MCTS:
	float nb_victoires; // nb de victoires pour ce noeud, décimal
	int nb_simus;  // nb de simulations pour ce noeud
} Noeud;


/**
 * @brief  creer un nouveau Noeud en jouant un coup à partir d'un parent
 * NB : utiliser nouveauNoeud(NULL, NULL) pour créer la racine
 * @param parent Noeud parent
 * @param coup Coup joué pour arriver à ce noeud
 * @return Noeud*  nouveau Noeud créé
 */
Noeud *nouveauNoeud(Noeud * parent, Coup * coup) {
	Noeud *noeud = (Noeud *) malloc(sizeof(Noeud));
	if (parent != NULL && coup != NULL) {
		noeud->etat = copieEtat(parent->etat);
		jouerCoup(noeud->etat, coup);
		noeud->coup = coup;
		noeud->joueur = AUTRE_JOUEUR(parent->joueur);
	} else {
		noeud->etat = NULL;
		noeud->coup = NULL;
		noeud->joueur = 0;
	}
	noeud->parent = parent;
	noeud->nb_enfants = 0;
	// POUR MCTS:
	noeud->nb_victoires = 0.0f;
	noeud->nb_simus = 0;
	return noeud;
}


/**
 * @brief  ajouter un enfant a un parent en jouant un coup
 *
 * @param parent Noeud parent
 * @param coup coup joue pour arriver a cet enfant
 * @return Node*  le pointeur sur l'enfant ajoute
 */
Noeud *ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud *enfant = nouveauNoeud(parent, coup);
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}


/**
 * @brief liberer memoire d'un noeud et de ses descendants
 *
 * @param Noeud noeud a liberer
 */
void freeNoeud(Noeud * Noeud) {
	if (Noeud->etat != NULL)
		free(Noeud->etat);
	while (Noeud->nb_enfants > 0) {
		freeNoeud(Noeud->enfants[Noeud->nb_enfants - 1]);
		Noeud->nb_enfants--;
	}
	if (Noeud->coup != NULL){
		free(Noeud->coup);
	}
	free(Noeud);
}


/**
 * @brief teste si l'etat courant est un etat terminal
 *
 * @param etat etat du jeu
 * @return FinDePartie retourne le type de fin de partie
 */
FinDePartie testFin(Etat * etat) {
	// tester si un joueur a gagné
	int i, j, k, n = 0;
	for (i = 0; i < HAUTEUR; i++) {
		for (j = 0; j < LARGEUR; j++) {
			if (etat->grid[i][j] != ' ') {
				n++;		// nb coups joués
				k = 0;// rows
				while (k < 4 && i + k < HAUTEUR && etat->grid[i + k][j] == etat->grid[i][j])
					k++;
				if (k == 4) {
					return etat->grid[i][j] == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;
				}
				k = 0;// colonnes
				while (k < 4 && j + k < LARGEUR && etat->grid[i][j + k] == etat->grid[i][j])
					k++;
				if (k == 4) {
					return etat->grid[i][j] == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;
				}
				k = 0;// diagonales
				while (k < 4 && i + k < HAUTEUR && j + k < LARGEUR && etat->grid[i + k][j + k] == etat->grid[i][j])
					k++;
				if (k == 4) {
					return etat->grid[i][j] == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;
				}
				k = 0;// anti-diagonales
				while (k < 4 && i + k < HAUTEUR && j - k >= 0 && etat->grid[i + k][j - k] == etat->grid[i][j])
					k++;
				if (k == 4) {
					return etat->grid[i][j] == 'O' ? ORDI_GAGNE : HUMAIN_GAGNE;
				}
			}
		}
	}
	if (n == LARGEUR * HAUTEUR)// et sinon tester le match nul
		return MATCHNUL;
	return NON;
}


/**
 * @brief calculer la B-value d'un noeud 
 * 
 * @param node noeud dont on veut calculer la B-value
 * @param max  0 si on veut calculer la B-value pour le joueur MAX, 1 si le noeud parent est un noeud MIN
 * @return double B-value du noeud courant
 */
double bValueCompute(Noeud* node, short int max) {
	double mu = (double)node->nb_victoires / (double)node->nb_simus;
	if(max != 0){ // si le noeud parent est un noeud MIN
		mu *= -1.0;
	}
	return mu + CONSTANTE_C * sqrt(log(node->parent->nb_simus) / node->nb_simus);
}


/**
 * @brief selectionner un coup a jouer en utilisant l'algorithm UCT
 * 
 * @param node noeud racine de l'arbre de recherche
 * @return Noeud* noeud selectionne
 */
Noeud * uctSelect(Noeud* node) {
	//if we arrive at a terminal node, we stop the simulation
	//or one node whose all the chldren have not been developed
	while (testFin(node->etat) == NON && node->nb_enfants == nb_coups_possibles(node->etat)) {
		Noeud *noeudMaxBValeur = node->enfants[0];
		double maxBValeur = -DBL_MAX;
		int i=0;
		while(i<node->nb_enfants){
			Noeud* noeud = node->enfants[i];
			double bValeurCourante;
			if (noeud->nb_simus == 0){ // we prioritize nodes that are not explored (not simulated)
				bValeurCourante =  DBL_MAX;
			}
			else{
				bValeurCourante = bValueCompute(noeud, node->joueur);
			}
			// highest UCT value
			if (maxBValeur < bValeurCourante) {
				noeudMaxBValeur = node->enfants[i];
				maxBValeur = bValeurCourante;
			}
			i++;
		}
		node = noeudMaxBValeur;
	}
	return node;
}


/**
 * @brief expand a node
 * 
 * @param node node to expand
 * @return Noeud* node expanded
 */
Noeud* expand(Noeud* node) {
	if (testFin(node->etat) == NON) {
		Coup **coups = coups_possibles(node->etat);
		int k = 0;
		while (coups[k]) {
			int coupDejaDev = 0;
			//for each child
			int i;
			for (i = 0; i < node->nb_enfants && !coupDejaDev; i++) {
				// if it's the same move
				if (coups[k]->col == node->enfants[i]->coup->col) {
					coupDejaDev = 1;	//this moves has already a corresponding child
					//we stop children nodes traversal 
				}
			}
			if (coupDejaDev) { //if this moves has a corresponding child
				//we remove this moves
				free(coups[k]);
				//and the moves' list has to shift
				for (i=k; coups[i]; i++) {
					coups[i] = coups[i + 1];
				}
			} else{
				k++;
			}
		}
		int choix = rand() % k; //we develop randomly a child
		k = 0; //we free the memory of unused play-moves
		while ( coups[k] ) {
			if (k != choix){ // except the one that will be freed	in freeNoeud
				free(coups[k]);
			}
			k+=1;
		}
		node = ajouterEnfant(node, coups[choix]);
		free(coups);
	}
	return node;
}


/**
 * @brief simule une partie à partir d'un état donné
 * 
 * @param etat etat de la partie
 * @param choisirCoupGagnant  true si on veut choisir un coup gagnant, false sinon
 * @return FinDePartie état de la partie à la fin de la simulation
 */
FinDePartie simulate(Etat * etat, short int choisirCoupGagnant) {
	Etat *simuEtat = copieEtat(etat);
	FinDePartie result = NON;
	while (result == NON) {
		Coup **possibleMoves = coups_possibles(simuEtat);
		int numMoves = 0,k = 0;
		// count number of possible moves
		while (possibleMoves[numMoves]) {
			numMoves++;
		}
		// choose a random move or winning move
		Coup *playedMove = NULL;
		if (choisirCoupGagnant) {
			while (possibleMoves[k] && !playedMove) {
				Etat *moveEval = copieEtat(simuEtat);
				jouerCoup(moveEval, possibleMoves[k]);

				if (testFin(moveEval) == ORDI_GAGNE) {
					playedMove = possibleMoves[k];
				}

				free(moveEval);
				k++;
			}
			if (!playedMove) {
				playedMove = possibleMoves[rand() % numMoves];
			}
		} else {
			playedMove = possibleMoves[rand() % numMoves];
		}
		jouerCoup(simuEtat, playedMove);
		result = testFin(simuEtat);
		k = 0;
		while (possibleMoves[k]) {// free memory
			free(possibleMoves[k]);
			k++;
		}
		free(possibleMoves);
	}
	free(simuEtat);
	return result;
}


/**
 * @brief backpropagation
 * 
 * @param node noeud à partir duquel on remonte
 * @param result résultat de la simulation
 */
void backpropagation(Noeud* node, int result) {
	while (node != NULL) { // node traversal
		node->nb_simus += 1;	// we increment the number of simulations
		if (result == HUMAIN_GAGNE) {
			node->nb_victoires += RECOMPENSE_HUMAIN_GAGNE;	//number of victories updated
		}
		else if(result == ORDI_GAGNE) {
			node->nb_victoires += RECOMPENSE_ORDI_GAGNE;	//number of victories updated
		}
		else {
			node->nb_victoires += RECOMPENSE_MATCHNUL;	//number of victories updated
		}
		node = node->parent;
	}
}

/**
 * @brief find the best node
 * 
 * @param root root of the tree
 * @param methodeChoix ROBUSTE or MAX
 * @return Noeud* 
 */
Noeud *findNodeBest(Noeud * root, MethodeChoixCoup methodeChoix){
	Noeud *nodeBestMove = root->enfants[0];
	int i ,child=0, maxSimus; float maxRatio;
	if(methodeChoix == ROBUSTE){
		maxSimus = nodeBestMove->nb_simus;
		for (i = 1; i < root->nb_enfants; i++) {
			if (maxSimus < root->enfants[i]->nb_simus) {
				nodeBestMove = root->enfants[i];
				maxSimus = nodeBestMove->nb_simus;
				child = i;
			}
		}
		fprintf(stdout,"nb de simu choisi : %d de %d\n", maxSimus, child);
	}
	else if(methodeChoix == MAXIMUM){
		maxRatio = (float)(nodeBestMove->nb_victoires)/ nodeBestMove->nb_simus;
		for(i = 1; i < root->nb_enfants; i++){
			float currentValue = (float)root->enfants[i]->nb_victoires/(float)root->enfants[i]->nb_simus;
			if(currentValue > maxRatio){
				nodeBestMove = root->enfants[i];
				maxRatio = currentValue;
				child = i;
			}
		}
		fprintf(stdout,"Ratio recomp / simulations choisi : %f de %d\n", maxRatio, child);
	}
	return nodeBestMove;
}


/**
 * @brief calcule et joue un coup de l'IA avec MCTS
 * en tempsmax secondes
 * @param etat etat du jeu
 * @param tempsmax temps maximum d'execution de l'IA
 */
void ordijoue_mcts(Etat * etat, float tempsmax, MethodeChoixCoup methodeChoix, int opti){
	clock_t tic, toc;
	tic = clock();
	double temps;
	Coup **coups;
	Coup *meilleur_coup;
	Noeud *nodeBestMove = NULL;
	// Créer l'arbre de recherche
	Noeud *racine = nouveauNoeud(NULL, NULL);
	racine->etat = copieEtat(etat);
	// créer les premiers Nodes:
	coups = coups_possibles(racine->etat);
	int k = 0;
	Noeud *enfant;
	while (coups[k] != NULL) {
		enfant = ajouterEnfant(racine, coups[k]);
		k++;
		//niveau d'optimisation est suffisant (et 1 coup gagnant est possible)
		if(opti>=2 && testFin(enfant->etat) == ORDI_GAGNE){
			nodeBestMove = enfant;
		}
	}
	int iter = 0;
	if(!nodeBestMove){
		do {
			Noeud *selected_node = uctSelect( racine ); //select the node to simulate
			enfant = expand( selected_node );//expand
			Etat *etatCopie = copieEtat( enfant->etat ); //copy the state of the expanded node
			FinDePartie result = simulate( etatCopie, opti>=1 ); // simulate a game from the expanded node
			free( etatCopie );//free memory
			backpropagation( enfant, result ); // back propagate the result onto its parent nodes
			toc = clock();
			temps = ((double)(toc - tic)) / CLOCKS_PER_SEC;
			iter++;
		} while (temps < tempsmax);	 //until time_limit is reached
		nodeBestMove = findNodeBest( racine, methodeChoix );
	}
	// Jouer le meilleur premier coup
	meilleur_coup = nodeBestMove->coup;
	fprintf(stdout,"\nMCTS a choisi la colonne %d", nodeBestMove->coup->col);
	fprintf(stdout,"\n%d iterations UCT en %0.3f s", nodeBestMove->nb_simus, temps);
	fprintf(stdout,"\nEstimated Win-Rate pour MCTS : ");
	if (nodeBestMove->nb_simus > 0){
		fprintf(stdout,"%0.2f %%", (double)nodeBestMove->nb_victoires / nodeBestMove->nb_simus * 100);
	}
	else{
		fprintf(stdout,"aucune");
	}
	fprintf(stdout,"\n");
	jouerCoup(etat, meilleur_coup);
	// Penser à libérer la mémoire :
	freeNoeud(racine);
	free(coups);
}


int main(int argc, const char* argv[]){
	//niveau d'optimisation :
	//	- 0 : fonctionnement normal de l'algo MCTS uct (les simulations sont aléatoires)
	//	- 1 : (Q°3 :) amelioration des simulations consistant à choisir le coup gagnant si possible (by default)
	//	- 2 : lorsqu'un coup gagnant est possible, l'algorithme n'est pas utilisé et le coup est joué directement.
    int optimisationLevel = 1;
	MethodeChoixCoup methodeChoix = MAXIMUM;
	Coup *coup = NULL;
	FinDePartie fin = 0; float tempsmax = 0.0f;
	if(argc < 3){
		fprintf(stderr, "usage :\n\t./jeu <y/n> <max/rob> [time]\n\targv[1] = strategie ameliore\n\targv[2] = maximum/robuste\n\targv[3] = temps limite MCTS(optionnel), e.g. 1.5\n");
		exit(EXIT_FAILURE);
	}
	if(strcmp(argv[1], "y") == 0){
		fprintf(stdout, "\tOpti des simus\n");
		optimisationLevel = 2;
	}
	else{
		fprintf(stdout, "\tPas d'optimisation\n");
	}
	if (strcmp(argv[2],"rob") == 0){
		fprintf(stdout, "\tCritere sur la robustesse\n");
		methodeChoix = ROBUSTE;
	}  else if (strcmp(argv[2],"max") == 0) {
		fprintf(stdout, "\tCritere sur la maximisation\n");
	} else {
		fprintf(stderr, "usage :\n\t./jeu <y/n> <max/rob> [time]\n\targv[1] = strategie ameliore\n\targv[2] = maximum/robuste\n\targv[3] = temps limite MCTS(optionnel), e.g. 1.5\n");
		fprintf(stderr, "\tMethode choix du coup non comprise\n");
		exit(EXIT_FAILURE);
	}
	if(argv[3]){
		fprintf(stdout, "\tTemps alloué pour MCTS = %s\n", argv[3]);
		tempsmax = strtod(argv[3], NULL);
		if( tempsmax <= 0.0f ){ // si le temps est negatif ou nul
			fprintf(stderr, "The value provided was out of range (<=0)\n");
			exit(EXIT_FAILURE);
		}
	}else{
		tempsmax = TEMPS;
	}
	// initialisation
	Etat *etat = etat_initial();
	// Choisir qui commence :
	printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
	(void)scanf("%d", &(etat->joueur));
	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);
		if (etat->joueur == 0) {
			// tour de l'humain
			int possibleMove = 0;
			do {
				coup = demanderCoup();
				if (coup) {
					possibleMove = jouerCoup(etat, coup);
					free(coup);
				}
			}while(possibleMove == 0);
		} else {
			// tour de MCTS
			ordijoue_mcts(etat, tempsmax, methodeChoix, optimisationLevel);
		}
		fin = testFin(etat);
	} while (fin == NON);
	printf("\n");
	afficheJeu(etat);
	if (fin == ORDI_GAGNE)
		printf("** L'ordinateur a gagné **\n");
	else if (fin == MATCHNUL)
		printf(" Match nul !  \n");
	else
		printf("** BRAVO, l'ordinateur a perdu  **\n");
	free(etat);
	return 0;
}
