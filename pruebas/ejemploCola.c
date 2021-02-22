#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cola.h"


struct persona {
    int edad;
    int dni;
    char * nombre;
};


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