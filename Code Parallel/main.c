/* TP1 HPC
 * REALISÉ PAR :
 * ABOUD IBRAHIM
 * BOUYAKOUB RAYANE
 * **/

#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <time.h>

/** LES HYPERPARAMETRES -variables global- **/
#define RESX 1024
#define RESY 1024
int resolutionX = RESX;
int resolutionY = RESY;
 double rectangleWidth = 0.00000005L; //(l for long)
 double rectangleCenterX = -1.86216646L;
 double rectangleCenterY = 0.0L;
int iterationsMax = 1000;
int result_matrix[RESY][RESX];
/****************/
#define NBTHREADS 32
const int NBthreads = NBTHREADS;
pthread_t threads[NBTHREADS];
int threadIndexes[NBTHREADS];
/****************/

/* JEU D'ESSAI : chaque ligne est un example veuillez remplacez RESP. dans "rectangleCenterX, rectangleCenterY, rectangleWidth"
 0.0         ,  0.0         , 4.0
-0.776592847 , -0.136640900 , 0.0000005
-1.768778833 , -0.00173890  , 0.000001
-1.86216646  ,  0.0         , 0.00000005
 */

/* FONCTION DE SAUVGARDE D'IMAGE DANS LA MEMOIRE SECONDAIRE -sequentiel- */
void savePPM(char *filename, int matrix[resolutionY][resolutionX])
{
    // ouvrir une image
    FILE *image = fopen(filename, "w");
    if (image == NULL)
    {
        perror("Error opening file");
        return;
    }

    // ecrire le type, la resolution, et la valeur max de chaque couleur R G B
    fprintf(image, "P3\n");
    fprintf(image, "%d %d\n", resolutionX, resolutionY);
    fprintf(image, "255\n");

    // pour chaque pixel dans l'image
    for (int y = 0; y < resolutionY; y++)
    {
        for (int x = 0; x < resolutionX; x++)
        {
            // acceder a la case correspondant au pixel en question et ecrire sa couleur "3 entiers R G B" du pixel sur le fichier
            // ps : la couleur d'un pixel correspond au nombre d'iteration d'echappement "ou c'est noir si le point appartient a l'ensemble"
            fprintf(image, "%d %d %d\n", (int)(matrix[y][x]) % 255, (int)(matrix[y][x]) % 255, (int)(matrix[y][x] * 10) % 255);
        }
    }

    fclose(image);
}

/* determiner pour un nombre complex donnée "x+i.y" si il appartient a l'ensemble ou pas*/
long isInSet( double x,  double y, long iterations, double x0,  double y0)
{
    // on etudie la suite : z= z^2 + c | tel que ( c = x + i.y ) et z0 = x0+i.y0 = 0 par defaut
    long counter = 0;
     double currentX;
     double currentY;
    do
    {
        // Zn+1 = Zn ^ 2 + c
        currentX = x0 * x0 - y0 * y0 + x;
        currentY = 2 * x0 * y0 + y;
        x0 = currentX;
        y0 = currentY;

        // decrementer le nombre d'iteration max
        iterations--;
        // incrementer le compteur d'iteration courante
        counter++;

        // la suite diverges vers l'infini si |y| ou |x| depasse 2,
        // si le compteur depasse le nombre d'iteration max, on suppose alors
        // que la suite est borné "ne diverge pas vers l'infini"
        // donc un nombre plus grands des iterations augmente la precision du calcul
    } while (iterations != 0 && (currentX * currentX + currentY * currentY) < 4);

    // first result, the point C turns out to be indeed from the julia set
    if (iterations == 0)
    {
        // si la suite est borné alors le nombre complex C appartient a l'ensemble
        return 0;
    }
    // si la suite diverge vers l'infini, alors C n'appartient pas a l'ensemble,
    // on retourn le nombre d'iterations necessaire pour que la suite echappe le
    // cercle de centre (0,0) et de rayon 2
    else
    {
        return counter;
    }
}

/***LA TACHE DE CHAQUE THREAD***/
void *task(void *dataPtr)
{
    // chaque ajout/modification du code sequentiel est souligné par des /*****/
    /**********************************/
    int threadIndex = *((int *)dataPtr);
    /**********************************/
    // C = cx + cy.i
    double cx = 0l;
    double cy = 0l;
    // step : c'est la distance entre chaque deux pixels adjacents
    double step = rectangleWidth / ((double)resolutionX);
    // x et y : les indices de la matrice equivalentess au pixels de l'image
    int x = 0;
    int y = 0;

    int result = 0;
    /*********************************************************/
    for (y = threadIndex; y < resolutionY; y = y + NBthreads)
    /*********************************************************/
    {
        for (x = 0; x < resolutionX; x++)
        {
            // calculer les coordonnée du pixel (cx et cy)
            cx = rectangleCenterX - (rectangleWidth / 2.0L) + x * step;
            cy = rectangleCenterY + (rectangleWidth * resolutionY / resolutionX / 2.0L) - y * step;
            // trouver si la suite complex : Zn+1 = (Zn)²+C est borné
            // avec Z₀ = (0, 0) et C = (cx,cy)
            // result = 0 si la suite est borné
            result = isInSet(cx, cy, iterationsMax, 0.0L, 0.0L);
            result_matrix[y][x] = result;
        }
    }
    return NULL;
}

/*____________________________________CODE PARALLEL____________________________________ */
int main()
{

    clock_t start = clock();

    /*_______________________DEBUT PARTIE PARALELLISÉ________________________*/

    /*_______________CREATION DES THREADS____________________*/
    for (int i = 0; i < NBthreads; i++)
    {
        threadIndexes[i] = i;
        pthread_create(&threads[i], NULL, task, (void *)&threadIndexes[i]);
    }

    printf("\n %d threads crees \n", NBthreads);

    clock_t calculate = clock();
    /*__________ATTENDRE LA FIN DE TOUT LES THREADS_________*/
    for (int i = 0; i < NBthreads; i++)
    {
        pthread_join(threads[i], NULL);
    }

    /*_______________________FIN PARTIE PARALELLISÉ________________________*/

    printf("\n calcul termine, generation de l'image...\n");

    clock_t save = clock();

    // construire une image a partir de la matrices des pixel "donner une couleur a chaque pixel"
    // tache faite par le thread main
    savePPM("mandelbrot.ppm", result_matrix);

    clock_t end = clock();

    // calcul des differents temps
    double tempCreation = (double)(calculate - start) / CLOCKS_PER_SEC;
    double tempCalcul = (double)(save - calculate) / CLOCKS_PER_SEC;
    double tempSauvgarde = (double)(end - save) / CLOCKS_PER_SEC;
    double tempTotal = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n Temps de creation threads: %f seconds\n Temps de calcul: %f seconds\n Temps sauvgarde image: %f seconds\n Temps Total: %f seconds\n", tempCreation, tempCalcul, tempSauvgarde, tempTotal);

    return 0;
}