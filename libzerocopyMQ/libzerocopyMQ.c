#include <stdint.h>
#include <sys/socket.h>
#include <sys/uio.h>
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

/*funcion que se realiza solo una vez por cliente 
* la primera vez que intenta acceder al servidor */
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
    struct iovec envio[1];

    /*letra C para crear una cola*/
    operacion[0] = 'C';
    operacion[1] = '\0';
    nameLength = strlen(cola);

    if (conexion == 0)
    {
        if (conectar() != 0)
        {
            perror("Error al conectar con el servidor");
            return -1;
        }
    }

    /*mensaje que contiene una letra con el modo de operacion*/
    /*y el nombre de la cola*/
    mensaje = malloc(1 + nameLength);
    strcpy(mensaje, operacion);
    strcat(mensaje, cola);
    leido = strlen(mensaje) + 1;

    envio[0].iov_base = mensaje;
    envio[0].iov_len = leido;

    /*mandamos mensaje*/
    if (writev(s, envio, 1) < 0)
    {
        perror("Error al mandar un mensaje desde el cliente");
        close(s);
        return -1;
    }
    /*recibimos respuesta*/
    if ((read(s, mensaje, TAM)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    /*TRATAMIENTO DE RESPUESTA*/

    free(mensaje);
    return 0;
}

int destroyMQ(const char *cola)
{

    int nameLength, leido;
    char *mensaje;
    char operacion[2];
    struct iovec envio[1];

    /*letra D para destruir una cola*/
    operacion[0] = 'D';
    operacion[1] = '\0';
    nameLength = strlen(cola) + 1;

    if (conexion == 0)
    {
        if (conectar() != 0)
        {
            perror("Error al conectar con el servidor");
            return 1;
        }
    }

    /*mensaje que contiene una letra con el modo de operacion*/
    /*y el nombre de la cola*/
    mensaje = malloc(1 + nameLength);
    strcpy(mensaje, operacion);
    strcat(mensaje, cola);
    leido = strlen(mensaje);

    envio[0].iov_base = mensaje;
    envio[0].iov_len = leido;

    /*mandamos mensaje*/
    if (writev(s, envio, 1) < 0)
    {
        perror("Error al mandar un mensaje desde el cliente");
        close(s);
        return -1;
    }
    /*recibimos respuesta*/
    if ((read(s, mensaje, TAM)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    /*TRATAMIENTO DE RESPUESTA*/

    free(mensaje);
    return 0;
}

int put(const char *cola, const void *mensaje, uint32_t tam)
{
    int nameLength, tamLength;
    char * mensajeFinal;
    char operacion[2], tamanyo[8];
    struct iovec envio[1];

    if (conexion == 0)
    {
        if (conectar() != 0)
        {
            perror("error al conectar con el servidor");
            return 1;
        }
    }
    
    /*Letra P para poner en una cola*/
    operacion[0] = 'P';
    operacion[1] = '\0';

    nameLength = strlen(cola);
    /*pasamos tam a texto*/
    sprintf(tamanyo, "%d", tam);
    tamLength = strlen(tamanyo);
    
    /*en el mensaje, primero irá la letra asociada a la operación
    después el tamaño del mensaje seguido de un guion, el mensaje
    y finalmente el nombre de la cola*/
    mensajeFinal = malloc(1 + tamLength + 1 + tam + nameLength);
    strcpy(mensajeFinal, operacion);
    strcat(mensajeFinal, tamanyo);
    strcat(mensajeFinal, "-");
    strcat(mensajeFinal, mensaje);
    strcat(mensajeFinal, cola);

    envio[0].iov_base = mensajeFinal;
    envio[0].iov_len = strlen(mensajeFinal) + 1;

    /*mandamos mensaje*/
    if (writev(s, envio, 1) < 0)
    {
        perror("Error al mandar un mensaje desde el cliente");
        close(s);
        return -1;
    }

    /*recibimos respuesta*/
    if ((read(s, mensajeFinal, TAM)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    /*TRATAMIENTO DE RESPUESTA*/
    
    free(mensajeFinal);
    return 0;
}

int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking)
{

    int nameLength, leido;
    char *msj;
    char operacion[2];
    struct iovec envio[1];

    /*letra G para obtener un mensaje*/
    operacion[0] = 'D';
    operacion[1] = '\0';
    nameLength = strlen(cola);

    if (conexion == 0)
    {
        if (conectar() != 0)
        {
            perror("Error al conectar con el servidor");
            return 1;
        }
    }

    /*mensaje que contiene una letra con el modo de operacion*/
    /*y el nombre de la cola*/
    msj = malloc(1 + nameLength);
    strcpy(msj, operacion);
    strcat(msj, cola);
    leido = strlen(msj) + 1;

    envio[0].iov_base = msj;
    envio[0].iov_len = leido;

    /*mandamos mensaje*/
    if (writev(s, envio, 1) < 0)
    {
        perror("Error al mandar un mensaje desde el cliente");
        close(s);
        return -1;
    }
    /*recibimos respuesta*/
    if ((read(s, msj, TAM)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    free(msj);
    return 0;
}
