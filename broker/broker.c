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

void imprimeDic(char *c, void *v)
{
    printf("nombre %s\n", c);
}

int main(int argc, char *argv[])
{
    struct diccionario *dicColas;
    struct cola *cola;
    struct iovec respuesta[1];
    int s, s_conec, leido;
    unsigned int tam_dir;
    struct sockaddr_in dir, dir_cliente;
    char *buf, operacion, *colaName, *mensajeRespuesta;
    int opcion = 1;

    if (argc != 2)
    {
        fprintf(stderr, "Uso: %s puerto\n", argv[0]);
        return 1;
    }

    /* Para evitar que los hijos se queden zombis */
    signal(SIGCLD, SIG_IGN);

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

    /*Creamos el diccionario que guardará las colas*/
    dicColas = dic_create();

    while (1)
    {
        tam_dir = sizeof(dir_cliente);
        if ((s_conec = accept(s, (struct sockaddr *)&dir_cliente, &tam_dir)) < 0)
        {
            perror("error en accept");
            close(s);
            return 1;
        }
        if (fork() == 0)
        {
            close(s);
            buf = malloc(TAM);
            if ((leido = read(s_conec, buf, TAM)) > 0)
            {

                /*obtenemos la primera letra, que tiene la operacion a realizar*/
                operacion = buf[0];

                switch (operacion)
                {
                /*Crear cola*/
                case 'C':

                    /*el mensaje contiene una letra y el nombre de la cola*/
                    colaName = &buf[1];

                    printf("leido %ld %s\n", strlen(buf), colaName);
                    /*Si la cola existe, manda una respuesta con una sola E (error)*/
                    if (dic_get(dicColas, colaName, NULL) == NULL)
                    {
                        mensajeRespuesta = malloc(sizeof(char) + 1);
                        mensajeRespuesta = "E";

                    }
                    else
                    {
                        /*Si no, la crea y manda una B (bien)*/
                        cola = cola_create();
                        dic_put(dicColas, colaName, NULL);
                        mensajeRespuesta = malloc(sizeof(char) + 1);
                        mensajeRespuesta = "B";
                    }

                    break;

                /*Destruir cola*/
                case 'D':
                    break;

                /*Poner un mensaje*/
                case 'P':
                    break;

                /*Obtener un mensaje*/
                case 'G':
                    break;

                default:
                    break;
                }

                respuesta[0].iov_base = mensajeRespuesta;
                respuesta[0].iov_len = strlen(mensajeRespuesta) + 1;

                if (writev(s_conec, respuesta, 1) < 0)
                {
                    perror("error en write");
                    close(s_conec);
                    exit(1);
                }
                
            }
            printf("hago free\n");
            free(buf);
            printf("hago free\n");
            buf = NULL;

            if (leido < 0)
            {
                perror("error en read");
                close(s_conec);
                exit(1);
            }

            printf("hago free\n");
            free(mensajeRespuesta);
            printf("hago free\n");
            mensajeRespuesta = NULL;

            close(s_conec);
            exit(0);
        }

        close(s_conec);
    }
    close(s);

    return 0;
}
