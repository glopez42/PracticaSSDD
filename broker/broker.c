#include <stdio.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include "comun.h"
#include "diccionario.h"
#include "cola.h"

#define TAM 1024

void libera_persona(char *c, void *v)
{
    // libera la clave reservada
    free(c);
    // libera la estructura cola reservada mediante malloc
    free(v);
}

int main(int argc, char *argv[])
{
    int s, s_conec, nameLength, error;
    unsigned int tam_dir;
    struct sockaddr_in dir, dir_cliente;
    struct diccionario *dic;
    struct cola *cola;
    struct iovec envio[1];
    char operacion;
    char *nombreCola;
    int opcion = 1;

    operacion = '\0';
    nameLength = 0;

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s puerto\n", argv[0]);
        return 1;
    }
    if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("error creando socket");
        return 1;
    }
    /* Para reutilizar puerto inmediatamente */
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opcion, sizeof(opcion)) < 0)
    {
        perror("error en setsockopt");
        return 1;
    }
    dir.sin_addr.s_addr = INADDR_ANY;
    dir.sin_port = htons(atoi(argv[1]));
    dir.sin_family = PF_INET;
    if (bind(s, (struct sockaddr *)&dir, sizeof(dir)) < 0)
    {
        perror("error en bind");
        close(s);
        return 1;
    }

    if (listen(s, 5) < 0)
    {
        perror("error en listen");
        close(s);
        return 1;
    }

    /*creamos diccionario de colas*/
    dic = dic_create();

    while (1)
    {
        tam_dir = sizeof(dir_cliente);
        if ((s_conec = accept(s, (struct sockaddr *)&dir_cliente, &tam_dir)) < 0)
        {
            perror("error en accept");
            close(s);
            return 1;
        }

        /*recibimos el paquete*/
        if (recv(s_conec, &operacion, sizeof(char), MSG_WAITALL) > 0)
        {
            recv(s_conec, &nameLength, sizeof(int), MSG_WAITALL);
            /*transformamos el entero (network byte order to host byte order)*/
            nameLength = ntohs(nameLength);
            /*guardamos espacio para recibir el nombre de la cola*/
            nombreCola = malloc(nameLength * sizeof(char));
            printf("%d\n", nameLength);
            switch (operacion)
            {
            case 'C':

                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);
                cola = cola_create();
                nombreCola = nombreCola + sizeof(char);
                printf("%s\n", nombreCola);
                /*Se mete en el diccionario*/
                if (dic_put(dic, nombreCola, cola) < 0)
                {
                    /*devuelve el caracter E si hay un error*/
                    operacion = 'M';
                }
                else
                {
                    operacion = 'B';
                }
                nombreCola = nombreCola - sizeof(char);
                free(nombreCola);
                break;

            case 'D':

                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);
                nombreCola = nombreCola + sizeof(char);
                printf("%s\n", nombreCola);

                /*Se mete en el diccionario*/
                cola = dic_get(dic, nombreCola, &error); 
                if (error < 0)
                {
                    /*devuelve el caracter E si hay un error*/
                    operacion = 'E';
                    break;
                }

                /*se destruye la cola*/
                if (cola_destroy(cola, NULL) < 0)
                {
                    /*devuelve el caracter E si hay un error*/
                    operacion = 'E';
                    break;
                }

                if (dic_remove_entry(dic, nombreCola, NULL) < 0)
                {
                    /*devuelve el caracter E si hay un error*/
                    operacion = 'E';
                    break;
                }

                operacion = 'B';
                nombreCola = nombreCola - sizeof(char);
                free(nombreCola);
                break;

            case 'P':

            case 'G':

            default:
                break;
            }

            envio[0].iov_base = (void *)&operacion;
            envio[0].iov_len = sizeof(char);
            writev(s_conec, envio, 1);
            close(s_conec);
        }
        else
        {
            perror("error en recv");
            close(s_conec);
            close(s);
            return 1;
        }
    }
    free(nombreCola);
    close(s);
    return 0;
}