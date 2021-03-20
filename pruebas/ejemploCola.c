#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cola.h"


struct persona {
    int edad;
    int dni;
    char * nombre;
};

/*función auxiliar que comprueba si para una cola*/
/*hay clientes esperando a un mensaje*/
void comprobarBloqueados(const char *nombreCola)
{
    struct cola *cola;
    struct iovec envio[2];
    struct paquete *paquete;
    int *s, error, p;
    cola = dic_get(esperando, nombreCola, &error);
    if (error != -1 && ((p = cola_length(cola)) > 0))
    {
        /*se extrae el socket del primer cliente bloqueado*/
        s = cola_pop_front(cola, &error);
        /*en caso contrario se manda por el socket y se cierra la conexión*/

        cola = dic_get(dic, nombreCola, &error);
        paquete = cola_pop_front(cola, &error);
        envio[0].iov_base = &(paquete->tam);
        envio[0].iov_len = sizeof(int);
        envio[1].iov_base = paquete->mensaje;
        envio[1].iov_len = paquete->tam + 1;
        writev(*s, envio, 2);

        /*liberamos memoria*/
        free(paquete->mensaje);
        free(paquete);

        close(*s);
        free(s);
    }
}


void inserta_persona(struct cola *c) {
    int edad,dni;
    char *nombre;
    struct persona *p;
    printf("Introduzca nombre de la persona");
    scanf("%ms", &nombre); 
    printf("Introduzca edad y dni de la persona");
    scanf("%d%d", &edad, &dni); 
    p = malloc(sizeof(struct persona));
    p->edad = edad; p->dni = dni; p->nombre=nombre;
    cola_push_back(c, p);
}

void imprime_persona(void *v) {
    struct persona *p = v;
    printf("punto: (%s,%d)\n", p->nombre, p->edad);
}


int main(){
    
    int iter;
    struct cola *cola;
    char hola[5];
    cola = cola_create();
    printf("Introduce nº de personas: ");
    scanf("%d", &iter);
    for (int i = 0; i < iter; i++)
    {
        inserta_persona(cola);
    }

    cola_visit(cola, imprime_persona);

    hola[0]='c';
    hola[1]='\0';

    printf("%ld\n", strlen(hola));

}


