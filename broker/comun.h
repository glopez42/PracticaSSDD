/*
 * Incluya en este fichero todas las definiciones que pueden
 * necesitar compartir el broker y la biblioteca, si es que las hubiera.
 */

struct paquete
{
    char operación;
    char *nombreCola;
    int tamanyo;
    char *mensaje;
};
