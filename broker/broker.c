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

int main(int argc, char *argv[])
{
    int s, s_conec, nameLength, leido;
    unsigned int tam_dir;
    struct sockaddr_in dir, dir_cliente;
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

            switch (operacion)
            {
            case 'C':
                leido = recv(s_conec, nombreCola, nameLength * sizeof(char), MSG_WAITALL);
                printf("%d %d\n",nameLength, leido);
                printf("%s\n",&nombreCola[1]);
                break;
            
            default:
                break;
            }

            operacion='B';
            envio[0].iov_base = (void *) &operacion;
            envio[0].iov_len = sizeof(char);
            writev(s_conec, envio, 1);
            close(s_conec);

        }
        else {
            perror("error en recv");
            close(s_conec);
            close(s);
            return 1;
        }

        printf("bien\n");
        free(nombreCola);
        
    }
    close(s);
    return 0;
}