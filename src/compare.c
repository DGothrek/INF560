/** 
 * Compare two image files byte by byte.
 * */

#include <stdio.h>
#include <stdlib.h>

int compare(char *filename1, char *filename2) {
    FILE *f1, *f2;
    char carac1, carac2;
    long size1, size2;

    f1 = fopen(filename1, "rb");
    f2 = fopen(filename2, "rb");
    if (f1 == NULL || f2 == NULL)
    {
        fprintf(stderr, "Error opening file");
        return 0;
    }

    /* Compute size of files */
    fseek(f1, 0, SEEK_END);
    fseek(f2, 0, SEEK_END);
    size1 = ftell(f1);
    size2 = ftell(f2);
    fseek(f1, 0, SEEK_SET);
    fseek(f2, 0, SEEK_SET);

    printf("Size of file1 = %ld\nSize of file2 = %ld\n", size1, size2);

    if (size1 != size2) {
        printf("Les fichiers n'ont pas la même taille\n");
        return 0;
    }

    do
    {
        carac1 = fgetc(f1); // On lit le caractère
        carac2 = fgetc(f2); // On lit le caractère

        if (carac1 != carac2) {
            printf("Le caractère %ld diffère\n", ftell(f1));
            return 0;
        }

    } while (carac1 != EOF);    
    
    printf("Fichiers égaux :) !\n");

    fclose(f1);
    fclose(f2);
    return 1;
}

int main(int argc, char **argv)
{
    char *filename1, *filename2;

    if (argc < 2)
    {
        fprintf(stderr, "Usage: filename1 filename2 \n");
        return 1;
    }
    filename1 = argv[1];
    filename2 = argv[2];

    return compare(filename1, filename2);
}