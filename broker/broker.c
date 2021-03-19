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

struct paquete
{
    int tam;
    char *mensaje;
};

struct diccionario *dic;
struct diccionario *esperando;

/*función auxiliar que comprueba si para una cola*/
/*hay clientes esperando a un mensaje*/
void comprobarBloqueados(char *nombreCola)
{
    struct cola *cola;
    struct iovec envio[2];
    struct paquete *paquete;
    int *s, error, p;
    cola = dic_get(esperando, nombreCola, &error);
    if ( error != -1 && ((p = cola_length(cola)) > 0))
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

int main(int argc, char *argv[])
{
    int s, s_conec, nameLength, error, *socketCliente;
    unsigned int tam_dir;
    struct sockaddr_in dir, dir_cliente;
    struct cola *cola;
    struct paquete *paquete;
    struct iovec resultado[1];
    struct iovec envio[2];
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
    esperando = dic_create();

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
            switch (operacion)
            {
            /*createMQ*/
            case 'C':

                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);
                cola = cola_create();
                /*Se mete en los diccionarios*/
                if (dic_put(dic, nombreCola, cola) < 0)
                {
                    /*devuelve el caracter E si hay un error*/
                    operacion = 'E';
                }
                else
                {
                    operacion = 'B';
                }
                break;

            /*destroyMQ*/
            case 'D':

                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);

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

            /*put*/
            case 'P':

                /*guardamos espacio para el paquete que se meterá en la cola*/
                paquete = malloc(sizeof(struct paquete));
                /*recibimos el nombre de la cola*/
                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);

                /*recibimos el tamaño del mensaje*/
                recv(s_conec, &(paquete->tam), sizeof(int), MSG_WAITALL);
                /*guardamos espacio para recibir el nombre de la cola*/
                paquete->mensaje = malloc(paquete->tam);
                /*recibimos el mensaje*/
                recv(s_conec, paquete->mensaje, paquete->tam, MSG_WAITALL);

                cola = dic_get(dic, nombreCola, &error);
                if (error < 0)
                {
                    /*devuelve el caracter E si no existe la cola*/
                    operacion = 'E';
                    break;
                }

                if (cola_push_back(cola, paquete) < 0)
                {
                    /*devuelve el caracter E si hay un error*/
                    operacion = 'E';
                    break;
                }

                operacion = 'B';

                /*comprobamos si hay algún cliente esperando en esta cola*/
                comprobarBloqueados(nombreCola);
                break;

            /*get No Bloqueante*/
            case 'G':

                /*recibimos el nombre de la cola*/
                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);

                cola = dic_get(dic, nombreCola, &error);
                if (error < 0)
                {
                    /*devuelve -1 si hay un error*/
                    resultado[0].iov_base = &error;
                    resultado[0].iov_len = sizeof(int);
                    writev(s_conec, resultado, 1);
                    close(s_conec);
                    /*vuelve al bucle del servidor*/
                    continue;
                }

                paquete = cola_pop_front(cola, &error);
                if (error < 0)
                {
                    /*devuelve 0 si la cola está vacía*/
                    error = 0;
                    resultado[0].iov_base = &error;
                    resultado[0].iov_len = sizeof(int);
                    writev(s_conec, resultado, 1);
                    close(s_conec);
                    /*vuelve al bucle del servidor*/
                    continue;
                }

                /*hacemos el envío del mensaje*/
                envio[0].iov_base = &(paquete->tam);
                envio[0].iov_len = sizeof(int);
                envio[1].iov_base = paquete->mensaje;
                envio[1].iov_len = paquete->tam + 1;
                writev(s_conec, envio, 2);

                /*liberamos memoria*/
                free(paquete->mensaje);
                free(paquete);

                close(s_conec);
                continue;

            /*get Bloqueante*/
            case 'B':
                /*recibimos el nombre de la cola*/
                recv(s_conec, nombreCola, nameLength, MSG_WAITALL);

                cola = dic_get(dic, nombreCola, &error);
                if (error < 0)
                {
                    /*devuelve -1 si hay un error*/
                    resultado[0].iov_base = &error;
                    resultado[0].iov_len = sizeof(int);
                    writev(s_conec, resultado, 1);
                    close(s_conec);
                    /*vuelve al bucle del servidor*/
                    continue;
                }

                paquete = cola_pop_front(cola, &error);
                if (error < 0)
                {
                    /*si la cola de mensajes está vacía se guarda el socket en la cola de espera*/
                    socketCliente = malloc(sizeof(int));
                    *socketCliente = s_conec;
                    cola = dic_get(esperando, nombreCola, &error);
                    /*si no existe la cola de clientes esperando, se crea*/
                    if (error < 0)
                    {
                        cola = cola_create();
                        cola_push_back(cola, socketCliente);
                        dic_put(esperando, nombreCola, cola);
                    }
                    else
                    {
                        /*si no, se mete directamente*/
                        cola_push_back(cola, socketCliente);
                    }

                    /*vuelve al bucle del servidor*/
                    continue;
                }

                /*si no está vacía hacemos el envío del mensaje*/
                envio[0].iov_base = &(paquete->tam);
                envio[0].iov_len = sizeof(int);
                envio[1].iov_base = paquete->mensaje;
                envio[1].iov_len = paquete->tam + 1;
                writev(s_conec, envio, 2);

                /*liberamos memoria*/
                free(paquete->mensaje);
                free(paquete);

                close(s_conec);
                continue;

            default:
                /*si no es ninguno de los casos anteriores, hay un error*/
                operacion = 'E';
                break;
            }

            /*enviamos respuesta*/
            resultado[0].iov_base = &operacion;
            resultado[0].iov_len = sizeof(char);
            writev(s_conec, resultado, 1);
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