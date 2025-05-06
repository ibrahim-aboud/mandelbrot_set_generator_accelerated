/* TP1 HPC
 * REALISÉ PAR :
 * ABOUD IBRAHIM
 * BOUYAKOUB RAYANE
 * **/

#include <stdio.h>
#include <math.h>
#include <time.h>

/** LES HYPERPARAMETRES -variables global- **/
#define RESX 1000
#define RESY 600
int resolutionX = RESX;
int resolutionY = RESY;
long double rectangleWidth = 0.00000005L; //(l for long)
long double rectangleCenterX = -1.86216646L;
long double rectangleCenterY = 0.0L;
int iterationsMax = 10000;
int result_matrix[RESY][RESX];

/* JEU D'ESSAI : chaque ligne est un example veuillez remplacez RESP. dans "rectangleCenterX, rectangleCenterY, rectangleWidth"
 0.0         ,  0.0         , 4.0
-0.776592847 , -0.136640900 , 0.0000005
-1.768778833 , -0.00173890  , 0.000001
-1.86216646  ,  0.0         , 0.00000005
 */

/* FONCTION DE SAUVGARDE D'IMAGE DANS LA MEMOIRE SECONDAIRE -sequentiel- */
void savePPM(char *filename, int matrix[RESX][RESY])
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
long isInSet(long double x, long double y, long iterations, long double x0, long double y0)
{
    // on etudie la suite : z= z^2 + c | tel que ( c = x + i.y ) et z0 = x0+i.y0 = 0 par defaut
    long counter = 0;
    long double currentX;
    long double currentY;

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

/*____________________________________CODE SEQUENTIEL____________________________________ */
int main()
{
    clock_t start = clock();

    // C = cx + cy.i
    long double cx = 0l;
    long double cy = 0l;
    // step : c'est la distance entre chaque deux pixels adjacents
    long double step = rectangleWidth / ((long double)resolutionX);
    // x et y : les indices de la matrice equivalentess au pixels de l'image
    int x = 0;
    int y = 0;

    int result = 0;
    /*_______________________DEBUT PARTIE PARALELLISABLE________________________*/

    for (y = 0; y < resolutionY; y++)
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
    /*_______________________FIN PARTIE PARALELLISABLE___________________________*/

    // construire une image a partir de la matrices des pixel "donner une couleur a chaque pixel"

    clock_t save = clock();

    savePPM("mandelbrot.ppm", result_matrix);

    clock_t end = clock();

    // calcul des differents temps
    double tempCalcul = (double)(save - start) / CLOCKS_PER_SEC;
    double tempSauvgarde = (double)(end - save) / CLOCKS_PER_SEC;
    double tempTotal = (double)(end - start) / CLOCKS_PER_SEC;

    printf("\n Temps de calcul: %f seconds\n Temps sauvgarde image: %f seconds\n Temps Total: %f seconds\n", tempCalcul, tempSauvgarde, tempTotal);

    return 0;
}