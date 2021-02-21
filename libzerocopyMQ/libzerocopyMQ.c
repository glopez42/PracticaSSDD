#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "zerocopyMQ.h"
#include "comun.h"

#define TAM 1024

int s, leido;
struct sockaddr_in dir;
struct hostent *host_info;
int conexion = 0;

/*funcion que se realiza solo una vez por cliente la primera vez que intenta acceder al servidor*/
int conectar()
{

    if ((s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
    {
        perror("error creando socket");
        return 1;
    }

    host_info = gethostbyname(getenv("BROKER_HOST"));
    dir.sin_addr = *(struct in_addr *)host_info->h_addr;
    dir.sin_port = htons(atoi(getenv("BROKER_PORT")));
    dir.sin_family = PF_INET;
    if (connect(s, (struct sockaddr *)&dir, sizeof(dir)) < 0)
    {
        perror("error en connect");
        close(s);
        return 1;
    }
    /*ponemos conexion a 1 para que no intente volver a conectarse*/
    conexion = 1;
    return 0;
}

int createMQ(const char *cola)
{

    int nameLength, leido;
    char *mensaje;
    char operacion[2];

    operacion[0] = 'C';
    operacion[1] = '\0';
    nameLength = strlen(cola);

    if (conexion == 0)
    {
        if (conectar() != 0)
        {
            perror("error al conectar con el servidor");
            return 1;
        }
    }

    /*mensaje que contiene una letra con el modo de operacion*/
    /*y el nombre de la cola*/
    mensaje = malloc(1 + nameLength);
    strcpy(mensaje, operacion);
    strcat(mensaje, cola);
    leido = strlen(mensaje);

    if (write(s, mensaje, leido) < 0)
    {
        perror("error en write");
        close(s);
        return 1;
    }

    return 0;
}

int destroyMQ(const char *cola)
{

    if (conexion == 0)
    {
        if (conectar() != 0)
        {
            perror("error al conectar con el servidor");
            return 1;
        }
    }
    return 0;
}

int put(const char *cola, const void *mensaje, uint32_t tam)
{

    if (conexion == 0)
    {
        if (conectar() != 0)
        {
            perror("error al conectar con el servidor");
            return 1;
        }
    }

    return 0;
}

int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking)
{

    if (conexion == 0)
    {
        if (conectar() != 0)
        {
            perror("error al conectar con el servidor");
            return 1;
        }
    }

    return 0;
}
