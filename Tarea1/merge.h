#ifndef MERGE_H
#define MERGE_H

#include "disco.h"
#include <stdint.h>

//estructura para estadisticas de mergesort externo
typedef struct {
    long long accesosdisco;
    double tiempoEjecucion;
    int aridadUsada;
} EstadisticasMerge;

//funciones principales de merge externo
int mergesortExterno(const char *archivoEntrada, const char *archivoSalida, 
                     size_t numElementos, int aridad, EstadisticasMerge *stats);

//auxiliares
int dividirArchivo(const char *archivoEntrada, size_t numElementos, 
                   int aridad, char **nombresSubarchivos);

int ordenarSubarchivoEnMemoria(const char *nombreArchivo, size_t numElementos);

int mezclarSubarchivos(char **nombresSubarchivos, int numArchivos, 
                       const char *archivoSalida, size_t numElementosTotal);

//funciones aridad optima
int encontrarAridadOptima(const char *archivoPrueba, size_t numElementos);

int probarAridad(const char *archivoPrueba, size_t numElementos, 
                 int aridad, EstadisticasMerge *stats);

//funciones de utilidad
int calcularMaxAridad(void);
int mergesortClasico(int64_t *arreglo, size_t n);
void mergeClasico(int64_t *arreglo, size_t inicio, size_t medio, size_t fin);

#endif 