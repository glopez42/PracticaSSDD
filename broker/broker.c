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
    int s, s_conec, nameLength, messageLength, error;
    unsigned int tam_dir;
    struct sockaddr_in dir, dir_cliente;
    struct diccionario *dic;
    struct cola *cola;
    struct iovec envio[1];
    char operacion;
    char *nombreCola, *mensaje;
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
            /*guardamos espacio para recibir el nombre de la cola*/
            nombreCola = malloc(nameLength * sizeof(char));
            printf("%d\n", nameLength);
            switch (operacion)
            {
            case 'C':

                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);
                cola = cola_create();
                printf("%s\n", nombreCola);
                /*Se mete en el diccionario*/
                if (dic_put(dic, nombreCola, cola) < 0)
                {
                    /*devuelve el caracter E si hay un error*/
                    operacion = 'E';
                    printf("entro\n");
                }
                else
                {
                    operacion = 'B';
                }
                break;

            case 'D':

                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);
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

                free(nombreCola);
                operacion = 'B';
                break;

            case 'P':
                /*recibimos el nombre de la cola*/
                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);
                printf("%s\n", nombreCola);
                /*recibimos el tamaño del mensaje*/
                recv(s_conec, &messageLength, sizeof(int), MSG_WAITALL);
                /*guardamos espacio para recibir el nombre de la cola*/
                mensaje = malloc(messageLength);
                /*recibimos el mensaje*/
                recv(s_conec, mensaje, messageLength, MSG_WAITALL);
                printf("%s\n", mensaje);

                cola = dic_get(dic, nombreCola, &error);
                if (error < 0)
                {
                    /*devuelve el caracter E si no existe la cola*/
                    operacion = 'E';
                    break;
                }

                if (cola_push_back(cola, mensaje) < 0)
                {
                    /*devuelve el caracter E si hay un error*/
                    operacion = 'E';
                    break;
                }
                
                operacion ='B';
                break;

            case 'G':

            default:
                break;
            }

            /*enviamos respuesta*/
            envio[0].iov_base = &operacion;
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

    close(s);
    return 0;
}