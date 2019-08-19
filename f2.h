#define NB_TOUR 15
#define NB_SECT 3

#define TMP_MAX_E1 90 //*60 pour 90min
#define TMP_MAX_E2 90
#define TMP_MAX_E3 60

#define TMP_MAX_Q1 18
#define TMP_MAX_Q2 15
#define TMP_MAX_Q3 12

#define NB_VOIT 18
#define NB_VOIT2 14
#define NB_VOIT3 10

#define MIN_tmp 30.0
#define MAX_tmp 40.0


const NUM_VOIT[18] = {44,6,3,26,19,77,5,7,14,22,11,27,33,55,8,13,9,12};


typedef struct voiture{
    int numVoit; //numero de voiture
    float bestS1; // meilleur tmp premier secteur personnel pour la voiture
    float bestS2; //..
    float bestS3; //..
    float bestTour; //s1+s2+s3
    float total; //tmp de course total pour cette voiture
    char etat; //C = en course, A = abandon
    int standStop; //commence à 0, peut aller jusqu'a 3-4 arrets pour la course
    float secteur[3];  //tmp pour chaque secteur recu, sert à calculer le tour
    float tmp_tour; //tmp pour le tour
    float tmp_total; // pour les essais
    float tmp_retard; //tmp retard par rapport au premier
    int nb_tours;    //nb de laps que la Tvoiture a fait
    float gaptotal; // ????????????????
    int lapDelay; // ????????????????
    int position;
}Tvoiture;


//conserver les meilleurs tmp secteurs
typedef struct secteur{
    int numVoit; //numero de Tvoiture
    float tmp; //meilleur tmp secteur
}Tsecteur;


typedef struct chrono{
    float tmp; //tmp généré aléatoirement
    int numVoit; //numéro de Tvoiture
}CHRONO;

key_t semkey, shmkey;
CHRONO * shmPt ;
Tvoiture* shmGP;
bool Flag_essai = false;
bool Flag_qualifs = false;

int E = 0;     // pour savoir Essai, qualifs et final sont finis
int Q = 0;
int F = 0;
int tabStart[NB_VOIT];

// tabE1/2/3 correspond au tableau du classement final pour les essais 1/2/3
// idem tabQ1 correspond qualif 1/2/3
Tvoiture tabEssai[NB_VOIT], tabE1[NB_VOIT] , tabE2[NB_VOIT] , tabE3[NB_VOIT];
Tvoiture tabQ1[NB_VOIT], tabQ2[NB_VOIT],tabQ3[NB_VOIT], tabF[NB_VOIT];
Tsecteur tabS[3];
CHRONO tabCh[NB_VOIT];





/* ------------------------------------------FONCTIONS GLOBALE ------------------------------------------*/


void getBestSecteur(Tvoiture tabV[] , Tsecteur tabS[],int nbv ){
    float t1,t2,t3;

    int i;

    for (i = 0; i < nbv; i++){
        tabV[i].position = i+1;
    }

    for (i=0;i<nbv;i++){
        if (tabV[i].etat != 'N'){
            t1 = tabV[i].bestS1;
            t2 = tabV[i].bestS2;
            t3 = tabV[i].bestS3;

            if (tabS[0].tmp > t1 ){
                tabS[0].tmp = t1;
                tabS[0].numVoit = tabV[i].numVoit;
            }

            if (tabS[1].tmp > t2 ){

                tabS[1].tmp = t2;
                tabS[1].numVoit = tabV[i].numVoit;
            }

            if (tabS[2].tmp > t3 ){
                tabS[2].tmp = t3;
                tabS[2].numVoit = tabV[i].numVoit;
            }
        }
    }
}


void EInitSemId(){

    // utilisé pour semctl
    union semun  {
        int val;               /* used for SETVAL only */
        struct semid_ds *buf;  /* used for IPC_STAT and IPC_SET */
        ushort *array;         /* used for GETALL and SETALL */
    };

    union semun semun1;
    //initialisé pour correspondre au nombre de Tvoitures
    semun1.val = 0;

    int semid;

    //crée 1 semaphore
   if ((semid = semget(19, 1, 0666 | IPC_CREAT) < 0))  {
      perror("creating semaphore");
      exit(EXIT_FAILURE);
   }



    //initialise le semaphore numero 0 à 24
   if (semctl(semid, 0, SETVAL, semun1 ) < 0){
        perror("initialisation semaphore ");
        exit(EXIT_FAILURE);
   }
    //else printf("semaphore de %d cree\n",valeur);
}



void shuffle(int array[], int n){
    if (n > 1)
    {
        int i;
        for (i = 0; i < n - 1; i++)
        {
          int j = i + rand() / (RAND_MAX / (n - i) + 1);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}


void openSM() {

    if ((shmkey = ftok("/home", 's')) == (key_t) -1) {
        perror("IPC error: ftok"); exit(1);
    }

    int taille = sizeof(CHRONO);
    taille = taille * 18;

    int shmid1 = shmget(shmkey, (taille),  IPC_CREAT | 0666 ); // ouvre ou le creee le segment


    shmPt = (CHRONO*)shmat(shmid1, NULL, 0);           // on obtient l'address, relapn un pointer, relapne -1 si erreur

    if (shmPt == (CHRONO*)(-1))   // check de shmPt
            perror("shmat");

}


/* ------------------------------------------ FIN FONCTIONS GLOBALE  ------------------------------------------ */





/* ------------------------------------------ FONCTIONS ESSAI ------------------------------------------ */


//copie le contenu du tabCh dans tabEssai
void tryMajTvoiture(int comptSect, Tvoiture tabEssai[], CHRONO tabCh[], int nbV){

    int i;
    printf("\n\n\t\t => Secteur numero %d\n",comptSect+1);

    for (i=0;i<nbV;i++){
        switch (comptSect){

        case 0 :{
            //premier secteur
            tabEssai[i].etat = 'C';         // la Tvoiture est entree en course
            tabEssai[i].secteur[0] = tabCh[i].tmp;

            if (tabEssai[i].bestS1 > tabCh[i].tmp){
                tabEssai[i].bestS1 = tabCh[i].tmp;

            }

        }break;

        case 1 :{
            //deuxieme secteur
            tabEssai[i].secteur[1] = tabCh[i].tmp;
            if (tabEssai[i].bestS2 > tabCh[i].tmp){
                tabEssai[i].bestS2 = tabCh[i].tmp;
            }


        }break;
        case 2 :{
            //dernier secteur
            tabEssai[i].secteur[2] = tabCh[i].tmp;
            if (tabEssai[i].bestS3 > tabCh[i].tmp){
                tabEssai[i].bestS3 = tabCh[i].tmp;
            }

        }break;


        }



        tabEssai[i].tmp_retard = 0.0;
        tabEssai[i].nb_tours = 0;

        tabEssai[i].position = i+1;

        //tmp de course total est ajouté
        tabEssai[i].total += tabCh[i].tmp;
        tabEssai[i].numVoit = tabCh[i].numVoit;
        }
}


void tryWriteSM(int numVoitProcessus){

    int semid,numVoit;
    CHRONO voiture;
    float chrono;
    chrono = 0.0;


    srand(time(NULL) ^ (getpid()<<16));

    if (tabEssai[numVoitProcessus].position > 0) {


        // TEMPS QUI SE CHEVAUCHEMENT POUR GERER LES DEPASSEMENTS
        if ((tabEssai[numVoitProcessus].position <= 5)) chrono = 35.0 + (float)rand() / ((float) RAND_MAX / (35.6 - 35.0));
        else if ((tabEssai[numVoitProcessus].position > 5) && (tabEssai[numVoitProcessus].position <= 10)) chrono = 35.5  + (float)rand() / ((float) RAND_MAX / (36.1 - 35.5 ));
        else if ((tabEssai[numVoitProcessus].position > 10) && (tabEssai[numVoitProcessus].position <= 15)) chrono = 36.0 + (float)rand() / ((float) RAND_MAX / (36.7 - 36.0));
        else if ((tabEssai[numVoitProcessus].position > 15) && (tabEssai[numVoitProcessus].position <= 18)) chrono = 36.5 + (float)rand() / ((float) RAND_MAX / (37.5 - 36.5));
      

        numVoit = tabEssai[numVoitProcessus].numVoit;


        if(tabEssai[numVoitProcessus].etat == 'A') chrono = 9999.9;

    }
    else {
        numVoit = tabEssai[numVoitProcessus].numVoit;
        chrono = MIN_tmp + (float)rand() / ((float) RAND_MAX / (MAX_tmp - MIN_tmp));
    }

    voiture.numVoit = numVoit;       // normalement le No13 n'apparait jamais
    voiture.tmp = chrono;

    shmPt[numVoitProcessus] = voiture;

    semid = semget(19, 1, 0666);
    if (semid < 0){
        printf("semaphores introuvables");
        exit(0);
    }

  struct sembuf v_buf;
    v_buf.sem_num = 0;v_buf.sem_op = 1;v_buf.sem_flg = IPC_NOWAIT;

   if (semop(semid, &v_buf,1) == -1)  {
      perror("Operation V echoué");
   }

}

//tri le tableau chrono
void trySortChrono(CHRONO t[], int nbV){
int i,j;
CHRONO temp;


for (j = 0; j < nbV ; j++){
    for (i = 0; i < nbV -1; i++){

    if (i < nbV){
        if (t[i].tmp > t[i+1].tmp){
            temp = t[i];
            t[i].tmp = t[i+1].tmp;
            t[i].numVoit = t[i+1].numVoit;
            t[i+1] = temp;
        }
        }
    }
}



}

void tryShowsecteur(Tsecteur tabSect[],int comptSect){
    int i;

        printf("\n \t\t Secteur \t\t numero \t\t Temps \n");
        printf("\t\t ------------------------------------------------------------------------------ \n");
        for (i=0;i<3;i++)
            if (tabSect[i].tmp < 99999.0) printf("\t\t %2d \t\t\t %2d \t\t %0.3lf  \n",(i+1), tabSect[i].numVoit ,tabSect[i].tmp);
}

int EGetNbr(){

    srand(time(NULL) ^ (getpid()<<16));
    int Tvoitures = 12 + (int)rand() / ((int) RAND_MAX / (6));
    //printf(" NB VOITURE %d  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!",Tvoitures );
    return(Tvoitures);

}



void essaiOpenSM() {

    int taille = sizeof(CHRONO);
    taille = taille * NB_VOIT;

    int shmid1 = shmget(91, (taille),  IPC_CREAT | 0666 ); // ouvre ou le creee le segment


    shmPt = (CHRONO*)shmat(shmid1, NULL, 0);           // on obtient l'address, relapn un pointer, relapne -1 si erreur

    if (shmPt == (CHRONO*)(-1))   // check de shmPt
            perror("shmat");

}







void essaiInitTab(Tvoiture tabEssai[],Tsecteur tabS[],CHRONO tabCh[],int tabStart[]){

    int i;

    for (i=0;i<NB_VOIT;i++){
        //init tab Tvoiture
        tabEssai[i].bestS1 = 99999.0;
        tabEssai[i].bestS2 = 99999.0;
        tabEssai[i].bestS3 = 99999.0;
        tabEssai[i].bestTour = 999999.0;
        tabEssai[i].total = 0.0;
        tabEssai[i].etat = 'P'; //toutes les Tvoitures commencent en course
        tabEssai[i].standStop = 0; //aucun arret aux stands

        //init tab classement
        tabEssai[i].tmp_retard = 0.0;
        tabEssai[i].nb_tours = 0;
        tabEssai[i].secteur[0] = 99999.0;
        tabEssai[i].secteur[1] = 99999.0;
        tabEssai[i].secteur[2] = 99999.0;
        tabEssai[i].tmp_tour = 9999.0;
        tabEssai[i].tmp_total = 0.0;
        tabEssai[i].position = 0;
        tabEssai[i].numVoit = 0;



        //init tab Chrono
        tabCh[i].tmp = 0.0;
        tabStart[i] = NUM_VOIT[i];


    }
        //init tab secteur
        for (i=0;i<3;i++){
            tabS[i].numVoit = 0;
            tabS[i].tmp = 9999999.0;

        }
        shuffle(tabStart,NB_VOIT);
        for (i=0;i<NB_VOIT;i++)
            tabEssai[i].numVoit = tabStart[i];
}

void essaiInitTab2(Tvoiture tabEssai[],int tabStart[]){

    int i;

    for (i=0;i<NB_VOIT;i++){

        tabEssai[i].etat = 'P'; //toutes les Tvoitures commencent en course
        tabEssai[i].secteur[0] = 99999.0;
        tabEssai[i].secteur[1] = 99999.0;
        tabEssai[i].secteur[2] = 99999.0;
        tabEssai[i].tmp_tour = 99999.0;

    }

    shuffle(tabStart,NB_VOIT);

}



void essaiReadSM(int nbSect, int nbV){
    int semid,j;
    semid = semget(19, 1, 0666);

    if (semid < 0){
        printf("semaphores introuvables processus parent");
        exit(0);
    }

    struct sembuf p_buf;

    p_buf.sem_num = 0; p_buf.sem_op = -nbV; p_buf.sem_flg = IPC_NOWAIT;

    int val = semctl(semid,0,GETVAL);

    if (val == nbV){

        if (semop(semid,&p_buf,1) == -1)  perror("Operation P échoué");
        for (j =0;j<nbV;j++){
            tabCh[j] = shmPt[j];

        }
        trySortChrono(tabCh, nbV);
        tryMajTvoiture(nbSect,tabEssai, tabCh, nbV);
        getBestSecteur(tabEssai,tabS,nbV);
    }
}


void tryCreateChildren(int nbEnfants,int nbSect){

   //tableau de pid enfants
    pid_t * tabPidEnfants;
    pid_t p;
    int enAttente,ii,i;

    tabPidEnfants = malloc(nbEnfants * sizeof(pid_t));

    //creer les enfants
    for ( ii = 0; ii < nbEnfants; ++ii) {
        if ((p = fork()) == 0) {

            /**  PROCESSUS ENFANT VIENT ICI **/

            tryWriteSM(ii);
            exit(0);
        }
        else {

            /**  PROCESSUS PARENT VIENT ICI **/

            tabPidEnfants[ii] = p;
            usleep(50000);
            essaiReadSM(nbSect, nbEnfants);

        }
    }

    // il faut attendre que les enfants exitent pour eviter les zombies
     do {
        enAttente = 0;
        for (i = 0; i < nbEnfants; ++i) {
            if (tabPidEnfants[i] > 0) {
                /** REMPLACER WNOHANG PAR 0 **/

                if (waitpid(tabPidEnfants[i], NULL, 0) == tabPidEnfants[i]) {
                    //l'enfant a fini
                    tabPidEnfants[i] = 0;
                     enAttente = 0;


                }
                else {
                    // l'enfant a pas fini
                    enAttente = 1;
                }
            }

            sleep(0);
        }
    } while (enAttente);

    //nettoyage
    free(tabPidEnfants);



}

//trie le tableau de classement par nb_tours pour savoir qui est premier
void trySortRanking(Tvoiture tabEssai[]){

Tvoiture temp;
int i,j;
    for (j = 0; j < NB_VOIT ; j++){
        for (i = 0; i < NB_VOIT -1 ; i++){
            if (tabEssai[i].tmp_tour > tabEssai[i+1].tmp_tour){
                temp = tabEssai[i];
                tabEssai[i] = tabEssai[i+1];
                tabEssai[i+1] = temp;
            }
        }
    }
}

void trySortFinalRanking(Tvoiture tabv[]){   // en fct de bestTour

Tvoiture temp;
int i,j;
    for (j = 0; j < NB_VOIT ; j++){
        for (i = 0; i < NB_VOIT -1 ; i++){
            if (tabv[i].bestTour > tabv[i+1].bestTour){
                temp = tabv[i];
                tabv[i] = tabv[i+1];
                tabv[i+1] = temp;
            }
        }
    }
}

void tryShowFinalRanking(Tvoiture tabTvoiture[]){

    printf("\n\nAffichage du classement final\n");
    printf("=============================\n\n");
    printf("\n\nPosition\tNumero  \t\t\t Etat \t\t Meilleur Temps S1 \t\t Meilleur Temps S2 \t\t Meilleur Temps S3 \t\t Meilleur Temps Tour\n\n\n");

    int i;


    for (i=0;i < NB_VOIT ;i++){
        if (tabTvoiture[i].total > 0){


            printf("\t %d \t  %d \t \t \t %c \t \t %lf \t\t\t %lf \t\t\t %lf \t\t\t %lf \n",(i+1),tabTvoiture[i].numVoit ,
            tabTvoiture[i].etat,tabTvoiture[i].bestS1 ,tabTvoiture[i].bestS2,
            tabTvoiture[i].bestS3 ,tabTvoiture[i].bestTour);
        }
        else if ((tabTvoiture[i].etat == 'A') || (tabTvoiture[i].etat == 'P')){
            printf("\t%d\t%d\t\t\t%c\t\tN/A\t\t\t\tN/A\t\t\t\tN/A\t\t\t\tN/A\n",(i+1),tabTvoiture[i].numVoit ,
            tabTvoiture[i].etat);
    }
}
}

void tryMajRanking(int nbV){
    int i;

    for (i=0;i<nbV;i++){
        //si la Tvoiture n'a pas abandonnée
        if (tabEssai[i].etat != 'A'){

            tabEssai[i].tmp_tour = 0;
           //enregistre le time d'un lap actuel,
            tabEssai[i].tmp_tour  += tabEssai[i].secteur[0];
            tabEssai[i].tmp_tour  += tabEssai[i].secteur[1];
            tabEssai[i].tmp_tour  += tabEssai[i].secteur[2];
            tabEssai[i].tmp_total  += tabEssai[i].tmp_tour;
            if (tabEssai[i].bestTour > tabEssai[i].tmp_tour) tabEssai[i].bestTour = tabEssai[i].tmp_tour;
        }
    }
    trySortRanking(tabEssai);
    for (i=0;i<nbV;i++){
        //gap par rapport au premier, gap = 0 pour premiere Tvoiture
        tabEssai[i].tmp_retard = tabEssai[i].tmp_tour - tabEssai[0].tmp_tour;
        tabEssai[i].position = i+1;
    }
}

//afficher le classement des Tvoitures en course
void tryShowRanking(int nbV){
    int i;

    printf("\n\nAffichage du classement\n");
    printf("=======================\n\n");
    printf("\n\n\t\tPosition\tNumero\t\t Etat \t\t Temps S1 \t\t Temps S2 \t\t Temps S3 \t\t Temps (tour)\n\n");
    for (i=0;i<nbV;i++){

        


        if (tabEssai[i].etat == 'C'){
            printf("\t\t  %d  \t\t %d \t\t  %c \t\t %.03lf \t\t %.03lf \t\t %.03lf \t\t %.03lf (%.03lf) \n",(i+1),tabEssai[i].numVoit ,tabEssai[i].etat,tabEssai[i].secteur[0] ,
                   tabEssai[i].secteur[1],tabEssai[i].secteur[2] ,tabEssai[i].tmp_tour, tabEssai[i].tmp_retard);
        }
        else if (tabEssai[i].etat == 'A')
            printf("--- %d  --------%d -------ABANDON---ABANDON---ABANDON\n",(i+1),tabEssai[i].numVoit);
    }
}


void essaiAncien(int duree ){

    int i,j;

    essaiOpenSM();
    essaiInitTab(tabEssai,tabS,tabCh,tabStart);
    EInitSemId();


    //for (i=0;i<1;i++){
        printf("\n\n Essai numero %d\n\n",duree);
        srand(time(NULL) ^ (getpid()<<16));
        int nbV = EGetNbr();
        essaiInitTab2(tabEssai,tabStart);
        sleep(1);

        for (j=0;j<NB_SECT;j++){
            tryCreateChildren(nbV,j);
            tryShowsecteur(tabS,j);
        }

        tryMajRanking(nbV);
        tryShowRanking( nbV);

   //}

    trySortFinalRanking(tabEssai);
    tryShowFinalRanking(tabEssai);


}

void essai(int duree, int nb_E ){

    int i,j;

    essaiOpenSM();
    essaiInitTab(tabEssai,tabS,tabCh,tabStart);
    EInitSemId();
    bool fini=false;

    
        printf("\n\n Essai numero %d\n\n",nb_E);
        srand(time(NULL) ^ (getpid()<<16));
        int nbV = EGetNbr();
        essaiInitTab2(tabEssai,tabStart);
        sleep(1);

    while(!fini){ 
        for (j=0;j<NB_SECT;j++){
            tryCreateChildren(nbV,j);
            tryShowsecteur(tabS,j);
        }

        tryMajRanking(nbV);
        tryShowRanking( nbV);
        for (i = 0; i < NB_VOIT; ++i)
        {
            if (tabEssai[i].tmp_total>=duree)
            {
                fini=true;
            }
        }
    }

    trySortFinalRanking(tabEssai);
    tryShowFinalRanking(tabEssai);

}




void essais(){
int i;
    essai(TMP_MAX_E1,1);
    for (i = 0; i<NB_VOIT;i++) tabE1[i] = tabEssai[i];
    essai(TMP_MAX_E2,2);
    for (i = 0; i<NB_VOIT;i++) tabE2[i] = tabEssai[i];
    essai(TMP_MAX_E3,3);
    for (i = 0; i<NB_VOIT;i++) tabE3[i] = tabEssai[i];
    Flag_essai = true;  // condition pour lancer Qualif et GP
    E = 1;              // essais fini
    shmdt(shmPt);

}

/* ------------------------------------------ FIN FOCNTIONS ESSAIS ------------------------------------------ */


/* ------------------------------------------  FONCTION POUR LES QUALIFS ------------------------------------------ */



//trie le tableau de classement par lap pour savoir qui est premier
void sortRanking(Tvoiture tabV[]){

Tvoiture temp;
int i,j;

    for (j = 0; j < NB_VOIT ; j++){
        for (i = 0; i < NB_VOIT -1 ; i++){
            if (tabV[i].bestTour > tabV[i+1].bestTour){

                temp = tabV[i];
                tabV[i] = tabV[i+1];
                tabV[i+1] = temp;

            }
        }
    }
}


/*
//environ 3 arrets aux stands en 44 laps sauf exceptions
void manageStandStop(Tvoiture tabCl[]) {
    //time passé aux stands pour pneu/essence
    float stopMIN_TIME = 15.0 ;
    float stopMAX_TIME = 20.0 ;
    int i;
    float timeStands;
    //time passé aux stands pour reparation leger
    float MIN_TIMEReparation = 35.0;
    float MAX_TIMEReparation = 100.0;
    srand((unsigned)time(0));
    int temp;
    //choisir 3 cars au hasard mais entre des bornes pour éviter doublons
    //-> ceci garantit cette condition => if (tabCl[i].standStop < n)
    temp= 0 + (int)rand() / ((int) RAND_MAX / (7 - 0));
    int numV1 = POSSIBLE_NUMS[temp];
    temp= 8 + (int)rand() / ((int) RAND_MAX / (12 - 8));
    int numV2 = POSSIBLE_NUMS[temp];
    temp= 16 + (int)rand() / ((int) RAND_MAX / (18 - 12));
    int numV3 = POSSIBLE_NUMS[temp];
    for (i=0;i<NB_VOIT;i++){
       if (tabCl[i].etat =='C'){
            //on tombe sur trois cars au hasard
            if ((tabCl[i].numVoit == numV1) || (tabCl[i].numVoit == numV2)|| (tabCl[i].numVoit == numV3)){
                //premier arret aux stands se fait entre 10 et 18 laps
                if ( (tabCl[i].nb_tours >= 10 ) && (tabCl[i].nb_tours < 18) ){
                    if (tabCl[i].standStop == 0){
                        tabCl[i].standStop++;
                        tabCl[i].etat = 'S';
                        timeStands = stopMIN_TIME + (float)rand() / ((float) RAND_MAX / (stopMAX_TIME- stopMIN_TIME));
                        tabCl[i].tmp_tour += timeStands;
                    }
                } else if ( (tabCl[i].laps >= 18 ) && (tabCl[i].laps < 26) ){
                    if (tabCl[i].standStop == 1){
                        tabCl[i].standStop++;
                        tabCl[i].etat = 'S';
                        timeStands = stopMIN_TIME + (float)rand() / ((float) RAND_MAX / (stopMAX_TIME - stopMIN_TIME));
                        tabCl[i].lap += timeStands;
                    }
                    //dernier arret à 34 laps max, on ne fait pas d'arrets aux stands avant la ligne d'arrivé?
                } else if ( (tabCl[i].laps >= 26 ) && (tabCl[i].laps < 34) ){
                    if (tabCl[i].standStop == 2){
                        tabCl[i].standStop++;
                        tabCl[i].etat = 'S';
                        timeStands = stopMIN_TIME + (float)rand() / ((float) RAND_MAX / (stopMAX_TIME - stopMIN_TIME));
                        tabCl[i].lap += timeStands;
                    }
                }
            }
            /** CONDITIONS D'ARRET AUX STANDS EXCEPTIONNELLES !
                PAR EX : SI ACCIDENT SANS ABANDON               **/
            /*if ((tabCl[i].laps < 10 ) || (tabCl[i].laps > 34) ){
                //choisir une car sur les 24
                int temp;
                temp= 0 + (int)rand() / ((int) RAND_MAX / (21 - 0));
                int numcarAccident = POSSIBLE_NUMS[temp];
                //probabilite qu'une car ait un accident
                int probabiliteAccident = 0 + (int)rand() / ((int) RAND_MAX / (1500 - 0));
                if (probabiliteAccident == numcarAccident){
                    //si un accident leger se produit il doit s'arreter aux stands
                    tabCl[i].standStop++;
                    tabCl[i].etat = 'S';
                    printf("%sACCIDENT EXCEPTION -> %s%s\n",RED,getPilote(tabCl[i].num),WHITE);
                    //time qu'il faut pour reparer la car?
                    timeStands = MIN_TIMEReparation + (float)rand() / ((float) RAND_MAX / (MAX_TIMEReparation - MIN_TIMEReparation));
                    tabCl[i].lap += timeStands;
                }
            }
        }
    }
}*/


//trie le tableau de classement par lap pour savoir qui est premier
void sortRankingGP(Tvoiture tabV[]){

Tvoiture temp;
int i,j;

    for (j = 0; j < NB_VOIT ; j++){
        for (i = 0; i < NB_VOIT -1 ; i++){
            if (tabV[i].total > tabV[i+1].total){

                temp = tabV[i];
                tabV[i] = tabV[i+1];
                tabV[i+1] = temp;

            }
        }
    }
}


//affichage du classement à la fin des qualifications
void showFinalRanking(Tvoiture tabClassementDepart[]){
    int i;
    printf("\n\tPosition\t\tNumero\\t\t\t\tMe. 1\t\t\t\tMe. 2\t\t\t\tMe. 3\t\t\tMe. lap\n" );
    printf("\n-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    for (i=0;i<NB_VOIT;i++){


        if (tabClassementDepart[i].bestTour == 2997.0000) tabClassementDepart[i].bestTour = 0.0;
        printf("\t%2d \t\t\t%2d \t\t \t\t\t %0.3lf \t\t\t %0.3lf\t\t\t %0.3lf \t\t %0.3lf \n",(i+1), tabClassementDepart[i].numVoit, tabClassementDepart[i].bestS1, tabClassementDepart[i].bestS2, tabClassementDepart[i].bestS3, tabClassementDepart[i].bestTour );
    }


}



//ecrire l'ordre de départ du classemenet pour le GP à la fin des qualifs
void sharedMemoryGPWrite(Tvoiture tabClassementDepart[]) {

    int shmkeyGP,i;

    if ((shmkeyGP = ftok("/tmp", 'g')) == (key_t) -1) {
        perror("IPC error: ftok"); exit(1);
    }

    int taille = sizeof(Tvoiture);
    taille = taille * NB_VOIT;

    int shmid = shmget(shmkeyGP, (taille),  IPC_CREAT | 0666 ); // ouvre ou le creee le segment


    shmGP = (Tvoiture*)shmat(shmid, NULL, 0);           // on obtient l'address, relapn un pointer, relapne -1 si erreur

    if (shmGP == (Tvoiture*)(-1))   // check de shmGP
            perror("shmat");


    else for (i=0;i<NB_VOIT;i++) shmGP[i] = tabClassementDepart[i];


}



void majRanking(Tvoiture tabV[],int nbV, int GP){
    int i;


    for (i=0;i<nbV;i++){
        //si la Tvoiture n'a pas abandonnée
        if (tabV[i].etat != 'N'){

            tabV[i].tmp_tour = 0;
           //enregistre le time d'un lap actuel,
            tabV[i].tmp_tour  += tabV[i].secteur[0];
            tabV[i].tmp_tour  += tabV[i].secteur[1];
            tabV[i].tmp_tour  += tabV[i].secteur[2];

        }
    }


        //if (GP == 1) manageStandStop(tabV);

        for (i=0;i<nbV;i++){
        //si la car n'a pas abandonnée
        if (tabV[i].etat != 'N'){
            if (tabV[i].bestTour > tabV[i].tmp_tour) tabV[i].bestTour = tabV[i].tmp_tour;

            //tabV[i].total += tabV[i].tmp_tour;

        }
    }


    if (GP ==0) sortRanking(tabV);
    else sortRankingGP(tabV);

    for (i=0;i<nbV;i++){
        if (tabV[i].etat != 'N'){

            tabV[i].tmp_retard = tabV[i].secteur - tabV[0].secteur;
            tabV[i].position = i+1;


            tabV[i].gaptotal = tabV[i].total - tabV[0].total;

           if (tabV[i].gaptotal < (tabV[i].tmp_tour*tabV[i].lapDelay) ){
                if (tabV[i].etat != 'A'){

                     tabV[i].nb_tours ++;
                }
            }
            else{
                tabV[i].gaptotal = 0.0;
                tabV[i].lapDelay++;
            }
        }
    }
}



//random pour crash et abandons
void gererAbandon(Tvoiture tabV[]){
    int min = 0;
    int max = 250;
    int probabilite,i;


    //génère un nombre random entre min et max
    srand((unsigned)time(0));
    probabilite = min + (int)rand() / ((int) RAND_MAX / (max - min));

    //parcours le tableau à la recherche des cars en courses
    for (i=0;i<NB_VOIT;i++){
        if (tabV[i].numVoit == probabilite){
            //si le random corréspond à un numéro de car, la car abandonne
            if (tabV[i].etat == 'C'){


                tabV[i].total = 999999.0;
                tabV[i].etat = 'A';
                tabV[i].standStop = 999;
                tabV[i].tmp_retard = 999.0;
                tabV[i].secteur[0] = 999.0;;
                tabV[i].secteur[1] = 999.0;;
                tabV[i].secteur[2] = 999.0;;
                tabV[i].tmp_tour = 9999.0;
                tabV[i].position = 30;
            }
        }
    }

}

//creation initialisation sémaphore
void InitSemId(){

    if ((semkey = ftok("/tmp", 'o')) == (key_t) -1) {
        perror("IPC error: ftok"); exit(1);
    }

    // utilisé pour semctl
    union semun  {
        int val;               /* used for SETVAL only */
        struct semid_ds *buf;  /* used for IPC_STAT and IPC_SET */
        ushort *array;         /* used for GETALL and SETALL */
    };

    union semun semun1;

    semun1.val = 0;

    int semid;

    //crée 1 semaphore
   if ((semid = semget(semkey, 1, 0666 | IPC_CREAT) < 0))  {
      perror("creating semaphore");
      exit(EXIT_FAILURE);
   }



    //initialise le semaphore numero 0 à 24
   if (semctl(semid, 0, SETVAL, semun1 ) < 0){
        perror("initialisation semaphore ");
        exit(EXIT_FAILURE);
   }

}


//un processus enfant qui va generer son chrono et écrire dans la MP
void writeSM(int numProcessus,Tvoiture tabV[],int tabStart[]){
    int semid,num;

    CHRONO voiture;
    float chrono;

    chrono = 0.0;

    //initialiser le random de maniere differente pour chaque processus
    srand(time(NULL) ^ (getpid()<<16));

    if (tabV[numProcessus].position != 0) {


        // TEMPS QUI SE CHEVAUCHEMENT POUR GERER LES DEPASSEMENTS
        if ((tabV[numProcessus].position > 0) && (tabV[numProcessus].position <= 5)) chrono = 35.0 + (float)rand() / ((float) RAND_MAX / (35.6 - 35.0));
        else if ((tabV[numProcessus].position > 5) && (tabV[numProcessus].position <= 10)) chrono = 35.5  + (float)rand() / ((float) RAND_MAX / (36.1 - 35.5 ));
        else if ((tabV[numProcessus].position > 10) && (tabV[numProcessus].position <= 15)) chrono = 36.0 + (float)rand() / ((float) RAND_MAX / (36.7 - 36.0));
        else if ((tabV[numProcessus].position > 15) && (tabV[numProcessus].position <= 18)) chrono = 36.5 + (float)rand() / ((float) RAND_MAX / (37.5 - 36.5));

        num = tabV[numProcessus].numVoit;

        if(tabV[numProcessus].etat == 'A') chrono = 9999.9;

    }

    else {
        chrono = MIN_tmp + (float)rand() / ((float) RAND_MAX / (MAX_tmp - MIN_tmp));
        num = tabStart[numProcessus];
    }

    voiture.tmp = chrono;
    voiture.numVoit = num;
    shmPt[numProcessus] = voiture;

   //ouvrir les sémaphores qui sont crées dans le main
    semid = semget(semkey, 1, 0666);
    if (semid < 0){
        printf("semaphores introuvables");
        exit(0);
    }

  struct sembuf v_buf;
    v_buf.sem_num = 0;v_buf.sem_op = 1;v_buf.sem_flg = IPC_NOWAIT;

   if (semop(semid, &v_buf,1) == -1)  {
      perror("Operation V echoué");

   }
}


//copie le contenu du tabCh dans tabV
void majTvoiture(int comptSect, Tvoiture tabV[], CHRONO tabCh[],int nbV){

    int i;


    for (i=0;i<nbV;i++){
        switch (comptSect){

        case 0 :{
            //premier secteur
            if (tabV[i].etat != 'N'){
                if (tabV[i].etat == 'S') tabV[i].etat ='C';

                tabV[i].secteur[0] = tabCh[i].tmp;
                if (tabV[i].bestS1 > tabCh[i].tmp){
                    tabV[i].bestS1 = tabCh[i].tmp;

                }
            }
        }break;

        case 1 :{
            //deuxieme secteur
            if (tabV[i].etat != 'N'){
                if (tabV[i].etat == 'S') tabV[i].etat ='C';

                tabV[i].secteur[1] = tabCh[i].tmp;
                if (tabV[i].bestS2 > tabCh[i].tmp){
                    tabV[i].bestS2 = tabCh[i].tmp;
                }
            }

        }break;
        case 2 :{
            //dernier secteur
            if (tabV[i].etat != 'N'){
                if (tabV[i].etat == 'S') tabV[i].etat ='C';

                tabV[i].secteur[2] = tabCh[i].tmp;
                if (tabV[i].bestS3 > tabCh[i].tmp){
                    tabV[i].bestS3 = tabCh[i].tmp;
                }
            }

        }break;


        }

        tabV[i].tmp_retard = 0.0;




        //time de course total est ajouté
        // printf("total before = %f \n",tabV[i].total );
        tabV[i].total += tabCh[i].tmp;
       // printf("total after = %f \n",tabV[i].total );
        //NECESSAIRE sinon position = numero Tvoiture
        tabV[i].numVoit = tabCh[i].numVoit;


    }

}


//serveur qui lit et traite les infos
void readSM(int nbSect,int nbV,CHRONO tabCh[],Tvoiture tabV[], Tsecteur tabS[]){
    int semid,j;


    //ouvrir les sémaphores qui sont crées dans le main
    semid = semget(semkey, 1, 0666);
    if (semid < 0){
        printf("semaphores introuvables processus parent");
        exit(0);
    }



    struct sembuf p_buf;

    p_buf.sem_num = 0;p_buf.sem_op = - nbV;p_buf.sem_flg = IPC_NOWAIT;

    int val = semctl(semid,0,GETVAL);

    if (val == nbV){
        if (semop(semid,&p_buf,1) == -1)  perror("Operation P échoué");

        for (j =0;j<nbV;j++){
            tabCh[j] = shmPt[j];

        }
        majTvoiture(nbSect,tabV, tabCh, nbV);
        getBestSecteur(tabV,tabS, nbV);

    }


}

//initialiser les tableaux
void initTab(Tvoiture tabV[],Tsecteur tabS[],int tabStart[],Tvoiture tabClassementDepart[]){

    int i;

    for (i=0;i<NB_VOIT;i++){

        tabClassementDepart[i].numVoit= 0;
        //init tab Tvoiture
        tabV[i].bestS1 = 99999.0;
        tabV[i].bestS2 = 99999.0;
        tabV[i].bestS3 = 99999.0;
        tabV[i].bestTour = 999999.0;
        tabV[i].total = 0.0;
        tabV[i].etat = 'C'; //toutes les Tvoitures commencent en course
        tabV[i].standStop = 0; //aucun arret aux stands
        tabV[i].tmp_retard = 0.0;
        tabV[i].nb_tours = 0;
        tabV[i].secteur[0] = 0.0;
        tabV[i].secteur[1] = 0.0;
        tabV[i].secteur[2] = 0.0;
        tabV[i].tmp_tour = 0.0;
        tabV[i].position = 0;
        //tabV[i].gaptotal = 0.0;
        //tabV[i].lapDelay =1;

        tabStart[i] = NUM_VOIT[i];
        tabV[i].numVoit = NUM_VOIT[i];


    }
        //init tab sector
        for (i=0;i<3;i++){
            tabS[i].numVoit = 0;
            tabS[i].tmp = 9999999.0;

        }

        shuffle(tabStart,NB_VOIT);


}


//creer x processus correspondant au nombre de Tvoitures nécéssaires
void createChildren(int nbEnfants,int nbSect,CHRONO tabCh[],Tvoiture tabV[],int tabStart[], Tsecteur tabS[]){

   //tableau de pid enfants
    pid_t * tabPidEnfants;
    pid_t p;
    int enAttente,ii,i;


    tabPidEnfants = malloc(nbEnfants * sizeof(pid_t));

    //creer les enfants
    for ( ii = 0; ii < nbEnfants; ++ii) {
        if ((p = fork()) == 0) {

            /**  PROCESSUS ENFANT VIENT ICI **/

            writeSM(ii, tabV, tabStart);

            exit(0);
        }
        else {
            /**  PROCESSUS PARENT VIENT ICI **/

            tabPidEnfants[ii] = p;

            usleep(55000);
            readSM(nbSect,nbEnfants,tabCh, tabV,  tabS);


        }
    }


    // il faut attendre que les enfants exitent pour eviter les zombies
     do {
        enAttente = 0;
        for (i = 0; i < nbEnfants; ++i) {
            if (tabPidEnfants[i] > 0) {


                if (waitpid(tabPidEnfants[i], NULL, 0) == tabPidEnfants[i]) {
                    //l'enfant a fini
                    tabPidEnfants[i] = 0;
                     enAttente = 0;


                }
                else {
                    // l'enfant a pas fini
                    enAttente = 1;
                }
            }

            sleep(0);
        }
    } while (enAttente);

    //nettoyage
    free(tabPidEnfants);



}

//faire une pause apres chaque séance de qualification
int pause(){


    struct timeval t;

    printf("\n\nInformation : La prochaine seance de qualification va commencer prochainement !\n\n\n");

    t.tv_sec = 5;
    t.tv_usec = 0;

    select(0, NULL, NULL, NULL, &t);
    printf("\n\nInformation : La prochaine seance de qualification va commencer !\n\n");

return 0;
}

//affiche les meilleurs time par sector
void showSecteur(Tsecteur tabSect[],int comptSect){
    int i;

        printf("\n\t\t  => Secteur %d\n",comptSect+1);

        printf("\n \t\t Secteur \t\t Numero  \t\t Temps \n");
        printf("\t\t ------------------------------------------------------------------------------ \n");
        for (i=0;i<3;i++)
            if (tabSect[i].tmp < 99999.0) printf("\t\t %2d \t\t\t %2d  \t\t %0.3lf  \n",(i+1), tabSect[i].numVoit ,tabSect[i].tmp);
            //if (tabSect[i].tmp < 99999.0) printf("\t\t %2d \t\t\t %2d %s \t\t %0.3lf  \n",(i+1), tabSect[i].numVoit ,tabSect[i].tmp);
}

//afficher le classement des Tvoitures en course
void showRanking(Tvoiture tabV[],int nbV){
    int i;

    printf("\n\n\n\tPosition \t\t Numero  \t\t\t Temps \t\t Total \n");
    printf("\n-------------------------------------------------------------------------------------------------------------------------------------------------------\n");
    for (i=0;i<nbV;i++){

        if (tabV[i].etat == 'C'){

            printf("\t%2d \t\t\t %2d  \t\t\t %.03lf   \t\t %.03lf \n",(i+1) ,tabV[i].numVoit , tabV[i].tmp_tour, tabV[i].total);}
        else if (tabV[i].etat == 'A')
            printf("\t%2d \t\t\t %2d \t\t ABANDON \t\t\t ABANDON \n",(i+1) ,tabV[i].numVoit);
        else if (tabV[i].etat == 'N')
            printf("\t%2d \t\t\t %2d  \t\t\t NON QUALIFIE \t\t\t NON QUALIFIE  \n",(i+1) ,tabV[i].numVoit);
        else if (tabV[i].etat == 'S')
            printf("\t%2d \t\t\t %2d  \t\t\t %.03lf  +PIT \t\t %.03lf \n",(i+1) ,tabV[i].numVoit ,tabV[i].tmp_tour, tabV[i].total);

    }
}


void qualifs(int tmpQualif,int nbVoitQualif, Tvoiture tabV[],int tabStart[],Tsecteur tabS[] ){

    int i,j,numQualif;
    int u=0;
    CHRONO tabCh[18];
    bool fini=false;

    openSM();

    InitSemId();

    switch (nbVoitQualif){

        case 18: numQualif = 1;break;
        case 14: numQualif = 2;break;
        case 10: numQualif = 3;break;

    }

    printf("\n\nSeance de qualification %d\n",numQualif);
    printf("=========================\n\n");
    while(!fini){    
    u++;                                              
        printf("\n\t\tTOUR NUMERO %d\n",u);
        for (j=0;j<NB_SECT;j++){

            createChildren(nbVoitQualif,j,tabCh, tabV,tabStart,tabS);

             showSecteur(tabS,j);


        }
        gererAbandon( tabV);
        majRanking(tabV,NB_VOIT,0);

        showRanking(tabV, NB_VOIT);

        for (i = 0; i < nbVoitQualif; ++i)
        {
            if (tabV[i].total>=tmpQualif)
            {
                fini=true;
            }
        }

    }



}

//ranger le tabV dans tabClassementDepart apres chaque séance de qualif
void sortStart(int lap, Tvoiture tabV[], Tvoiture tabClassementDepart[]){
    int k,i;

        switch (lap){
            case 1:{

                //on part de la derniere place du tab = 18 et on remonte jusqu'au 15
                for (k=(NB_VOIT-1);k>=NB_VOIT2;k--){

                    tabClassementDepart[k] = tabV[k];
                    //on vide l'emplacement, en conservant son num

                    tabV[k].bestS1 = 999999.0;
                    tabV[k].bestS2 = 999999.0;
                    tabV[k].bestS3 = 999999.0;
                    tabV[k].bestTour = 999999.0;
                    tabV[k].total = 9999999999.0;
                    tabV[k].etat = 'N';
                    tabV[k].standStop = 999999;
                    tabV[k].tmp_retard = 999999.0;//
                    tabV[k].nb_tours = 0;//
                    tabV[k].secteur[0] = 999999.0;;
                    tabV[k].secteur[1] = 999999.0;;
                    tabV[k].secteur[2] = 999999.0;;
                    tabV[k].tmp_tour = 99999.0;
                    tabV[k].position = 30;
                    tabV[k].gaptotal = 0.0;
                    tabV[k].lapDelay =1;

                    tabV[k].numVoit= tabClassementDepart[k].numVoit ;


                }
                //reinitialiser les Tvoitures qualifiées
                for(i=0;i<NB_VOIT2;i++){
                    if (tabV[i].etat != 'A'){
                        tabV[i].bestS1 = 99999.0;
                        tabV[i].bestS2 = 99999.0;
                        tabV[i].bestS3 = 99999.0;
                        tabV[i].bestTour = 999999.0;
                        tabV[i].total = 0.0;
                        tabV[i].etat = 'C'; //toutes les Tvoitures commencent en course
                        tabV[i].standStop = 0; //aucun arret aux stands
                        tabV[i].tmp_retard = 0.0;
                        tabV[i].nb_tours = 0;
                        tabV[i].secteur[0] = 0.0;
                        tabV[i].secteur[1] = 0.0;
                        tabV[i].secteur[2] = 0.0;
                        tabV[i].tmp_tour = 0.0;
                        tabV[k].gaptotal = 0.0;
                        tabV[k].lapDelay =1;


                    }
                }



            }break;
            case 2:{

                //on part de la 15 place du tab et on remonte jusqu'au 10ème
                for (k=(NB_VOIT2-1);k>=NB_VOIT3;k--){

                    tabClassementDepart[k] = tabV[k];
                    //on vide l'emplacement, en conservant son num
                    tabV[k].bestS1 = 999999.0;
                    tabV[k].bestS2 = 999999.0;
                    tabV[k].bestS3 = 999999.0;
                    tabV[k].bestTour = 999999.0;
                    tabV[k].total = 9999999999.0;
                    tabV[k].etat = 'N';
                    tabV[k].standStop = 999999;
                    tabV[k].tmp_retard = 999999.0;
                    tabV[k].nb_tours = 0;
                    tabV[k].secteur[0] = 999999.0;;
                    tabV[k].secteur[1] = 999999.0;;
                    tabV[k].secteur[2] = 999999.0;;
                    tabV[k].tmp_tour = 99999.0;
                    tabV[k].position = 30;
                    tabV[k].gaptotal = 0.0;
                    tabV[k].lapDelay =1;
                    tabV[k].numVoit= tabClassementDepart[k].numVoit ;
                }

                //reinitialiser les Tvoitures qualifiées
                for(i=0;i<NB_VOIT3;i++){
                    if (tabV[i].etat != 'A'){
                        tabV[i].bestS1 = 99999.0;
                        tabV[i].bestS2 = 99999.0;
                        tabV[i].bestS3 = 99999.0;
                        tabV[i].bestTour = 999999.0;
                        tabV[i].total = 0.0;
                        tabV[i].etat = 'C'; //toutes les Tvoitures commencent en course
                        tabV[i].standStop = 0; //aucun arret aux stands
                        tabV[i].tmp_retard = 0.0;
                        tabV[i].nb_tours = 0;
                        tabV[i].secteur[0] = 0.0;
                        tabV[i].secteur[1] = 0.0;
                        tabV[i].secteur[2] = 0.0;
                        tabV[i].tmp_tour = 0.0;
                        tabV[k].gaptotal = 0.0;
                        tabV[k].lapDelay =1;

                    }

                }

            }break;

            case 3:{

                //on part de la 10eme place du tab et on remonte jusqu'au 0
                for (k=(NB_VOIT3-1);k>=0;k--){

                    tabClassementDepart[k] = tabV[k];
                    //on vide l'emplacement, en conservant son num
                    tabV[k].bestS1 = 999999.0;
                    tabV[k].bestS2 = 999999.0;
                    tabV[k].bestS3 = 999999.0;
                    tabV[k].bestTour = 999999.0;
                    tabV[k].total = 9999999999.0;
                    tabV[k].etat = 'N';
                    tabV[k].standStop = 999999;
                    tabV[k].tmp_retard = 999999.0;
                    tabV[k].nb_tours = 0;
                    tabV[k].secteur[0] = 999999.0;;
                    tabV[k].secteur[1] = 999999.0;;
                    tabV[k].secteur[2] = 999999.0;;
                    tabV[k].tmp_tour = 99999.0;
                    tabV[k].position = 30;
                    tabV[k].gaptotal = 0.0;
                    tabV[k].lapDelay =1;

                    tabV[k].numVoit= tabClassementDepart[k].numVoit ;


                }


            }break;



        }


    }

void qualifications(){
    int i;
    int tabStart[NB_VOIT];
    Tvoiture tabV[NB_VOIT],tabClassementDepart[NB_VOIT];
    Tsecteur tabS[3];


    // Qualifs se déroulent en trois parties 
    initTab(tabV, tabS,tabStart,tabClassementDepart);


//Q1
    qualifs(TMP_MAX_Q1,NB_VOIT,tabV, tabStart,tabS);
    for (i = 0; i<NB_VOIT;i++) tabQ1[i] = tabV[i];
    sortStart(1,tabV,tabClassementDepart);
    pause();


//Q2
    qualifs(TMP_MAX_Q2,NB_VOIT2,tabV, tabStart,tabS);
    for (i = 0; i<NB_VOIT;i++) tabQ2[i] = tabV[i];
    sortStart(2,tabV,tabClassementDepart);
    pause();



//Q3
    qualifs(TMP_MAX_Q3,NB_VOIT3,tabV, tabStart,tabS);
    for (i = 0; i<NB_VOIT;i++) tabQ3[i] = tabV[i];
    sortStart(3,tabV,tabClassementDepart);

    showFinalRanking( tabClassementDepart);

    shmdt(shmPt);
    /** FIN QUALIFS **/
    //ecrire le classement pour le départ GP en MP
    sharedMemoryGPWrite(tabClassementDepart);


}

/* ------------------------------------------ FIN FONCTIONS QUALIF ------------------------------------------ */


/* ------------------------------------------ MENUS  ------------------------------------------ */


void menu3(){
    printf("FIN QUALIF");
}


void MenuQualif(){

    int choix;



    srand(time(NULL) ^ (getpid()<<16));

    essaiInitTab(tabEssai, tabS, tabCh,tabStart);

            printf("\n\n\n"
                   "Seances de qualification\n"
                   "========================\n"
          "\t\t1 : Lancer les Qualifications \n"
          //"\t\t2 : Voir Resultats \n"
          "\t\t2 : Quitter\n"
          "\t\t\t Choix :  ");

          scanf("%d", &choix);
    do {
        switch(choix){

            case 1:{
                system("clear");
                qualifications();
                Q = 1;
                sleep(5); // 10sec
                system("clear");

                menu3();
                choix =3;
            }break;


            case 2:{
                    exit(0);
            }break;
            default:{
                printf("\n\n ! Invalide ! \n\n");
                MenuQualif();
            }break;
        }
    }while (choix != 3);

}

void MenuEssai(){

int choix;
    srand(time(NULL) ^ (getpid()<<16));
    essaiInitTab(tabEssai, tabS, tabCh,tabStart);

    printf("\nSeance d'essais\n"
    "===============\n\n"
    "\t\t1 : Lancer les essais \n"
    //"\t\t2 : Voir Resultats \n"
    "\t\t2 : Quitter\n"
    "\t\t\t Choix :  ");

          scanf("%d", &choix);
    do {
        switch(choix){

            case 1:{
                system("clear");
                essais();
                sleep(3); // 3sec
                system("clear");

                MenuQualif();
                choix=3;
                break;
            }break;


            case 2:{
                    exit(0);
            }break;
            default:{
                printf("\n\n ! Invalide ! \n\n");
                MenuEssai();
            }break;
        }
    }while (choix != 3);

}
