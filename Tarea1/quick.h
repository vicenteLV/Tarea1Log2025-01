#ifndef QUICK_H
#define QUICK_H

#include "disco.h"
#include <stdint.h>

//estructura para estadisticas de quicksort externo
typedef struct {
    long long accesosdisco;
    double tiempoEjecucion;
    int aridadUsada;
} EstadisticasQuick;

//funciones principales quicksort externo
int quicksortExterno(const char *archivoEntrada, const char *archivoSalida,
                     size_t numElementos, int aridad, EstadisticasQuick *stats);

//auxiliares
int quicksortExternoRecursivo(const char *archivoEntrada, size_t numElementos, 
                              int aridad);

int seleccionarPivotes(const char *archivo, size_t numElementos, 
                       int numPivotes, int64_t *pivotes);

int particionarArchivo(const char *archivoEntrada, size_t numElementos,
                       int64_t *pivotes, int numPivotes, 
                       char **nombresSubarchivos, size_t *tamañosSubarchivos);

int concatenarSubarchivos(char **nombresSubarchivos, int numArchivos,
                          const char *archivoSalida);

//mas auxiliares
int quicksortClasico(int64_t *arreglo, size_t n);
int partitionClasico(int64_t *arreglo, int inicio, int fin);
void intercambiar(int64_t *a, int64_t *b);
int compararInt64(const void *a, const void *b);

#endif 