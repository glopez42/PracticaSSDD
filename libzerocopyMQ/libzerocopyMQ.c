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

int s, leido;
struct sockaddr_in dir;
struct hostent *host_info;

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

    return 0;
}

int createMQ(const char *cola)
{
    int nameLength, code;
    char respuesta;
    char operacion;
    struct iovec envio[3];

    if (conectar() != 0)
    {
        perror("Error al conectar con el servidor");
        return 1;
    }

    /*letra C para crear una cola*/
    operacion = 'C';
    nameLength = strlen(cola) + 1;

    envio[0].iov_base = &operacion;
    envio[0].iov_len = sizeof(operacion);
    envio[1].iov_base = &nameLength;
    envio[1].iov_len = sizeof(nameLength);
    envio[2].iov_base = (char *)cola;
    envio[2].iov_len = nameLength;
    
    /*mandamos mensaje*/
    if (writev(s, envio, 3) < 0)
    {
        perror("Error al mandar un mensaje desde el cliente");
        close(s);
        return -1;
    }

    /*recibimos respuesta*/
    if ((recv(s, &respuesta, sizeof(char) + 1, MSG_WAITALL)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    /*TRATAMIENTO DE RESPUESTA*/
    code = (respuesta == 'B') ? 0 : -1;
    
    close(s);
    return code;
}

int destroyMQ(const char *cola)
{
    int nameLength, code;
    char respuesta;
    char operacion;
    struct iovec envio[3];

    if (conectar() != 0)
    {
        perror("Error al conectar con el servidor");
        return 1;
    }

    /*letra D para destruir una cola*/
    operacion = 'D';
    nameLength = strlen(cola) + 1;

    envio[0].iov_base = &operacion;
    envio[0].iov_len = sizeof(operacion);
    envio[1].iov_base = &nameLength;
    envio[1].iov_len = sizeof(nameLength);
    envio[2].iov_base = (char *)cola;
    envio[2].iov_len = nameLength;
    
    /*mandamos mensaje*/
    if (writev(s, envio, 3) < 0)
    {
        perror("Error al mandar un mensaje desde el cliente");
        close(s);
        return -1;
    }

    /*recibimos respuesta*/
    if ((recv(s, &respuesta, sizeof(char) + 1, MSG_WAITALL)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    /*TRATAMIENTO DE RESPUESTA*/
    code = (respuesta == 'B') ? 0 : -1;
    
    close(s);
    return code;
}

int put(const char *cola, const void *mensaje, uint32_t tam)
{
    int nameLength, code;
    char respuesta;
    char operacion;
    struct iovec envio[5];

    if (tam == 0)
    {
        return 0;
    }

    if (conectar() != 0)
    {
        perror("Error al conectar con el servidor");
        return 1;
    }

    /*letra P para poner un mensaje en una cola*/
    operacion = 'P';
    nameLength = strlen(cola) + 1;

    envio[0].iov_base = &operacion;
    envio[0].iov_len = sizeof(operacion);
    envio[1].iov_base = &nameLength;
    envio[1].iov_len = sizeof(nameLength);
    envio[2].iov_base = (char *)cola;
    envio[2].iov_len = nameLength;
    envio[3].iov_base = &tam;
    envio[3].iov_len = sizeof(tam);
    envio[4].iov_base = (char *)mensaje;
    envio[4].iov_len = tam;
    
    /*mandamos mensaje*/
    if (writev(s, envio, 5) < 0)
    {
        perror("Error al mandar un mensaje desde el cliente");
        close(s);
        return -1;
    }

    /*recibimos respuesta*/
    if ((recv(s, &respuesta, sizeof(char) + 1, MSG_WAITALL)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    /*TRATAMIENTO DE RESPUESTA*/
    code = (respuesta == 'B') ? 0 : -1;
    
    close(s);
    return code;   
}

int get(const char *cola, void **mensaje, uint32_t *tam, bool blocking)
{

    int nameLength;
    char operacion;
    struct iovec envio[3];

    if (conectar() != 0)
    {
        perror("Error al conectar con el servidor");
        return 1;
    }

    /*letra G para obtener un mensaje de una cola
     o letra B para una lectura bloqueante*/
    operacion = (blocking)? 'B':'G';
    nameLength = strlen(cola) + 1;

    envio[0].iov_base = &operacion;
    envio[0].iov_len = sizeof(operacion);
    envio[1].iov_base = &nameLength;
    envio[1].iov_len = sizeof(nameLength);
    envio[2].iov_base = (char *)cola;
    envio[2].iov_len = nameLength;

     
    /*mandamos mensaje*/
    if (writev(s, envio, 3) < 0)
    {
        perror("Error al mandar un mensaje desde el cliente");
        close(s);
        return -1;
    }

    /*recibimos el tamaño del mensaje que vamos a recibir*/
    if ((recv(s, tam, sizeof(int), MSG_WAITALL)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    /*si no existe la cola*/
    if (*tam == -1)
    {
        close(s);
        return -1;
    }

    /*si la cola está vacía, no es un error*/
    if (*tam == 0)
    {
        close(s);
        return 0;
    }

    *mensaje = (char *) malloc(*tam + 1);

    /*recibimos el mensaje*/
    if ((recv(s, *mensaje, *tam + 1, MSG_WAITALL)) < 0)
    {
        perror("Error al recibir un mensaje del servidor");
        close(s);
        return -1;
    }

    close(s);
    return 0;   
}
