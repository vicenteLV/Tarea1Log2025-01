#ifndef EXPERIMENTO_H
#define EXPERIMENTO_H

#include "disco.h"
#include <stdint.h>

//funciones para generar datos de prueba
int generarSecuenciaAleatoria(const char *filename, size_t numElementos);
int generarSecuenciasParaTamaño(size_t N);
int generarTodasLasSecuencias(void);
int generarUnaSecuencia(size_t N, int numeroSecuencia);

//funciones de verificacion
int verificarArchivoGenerado(const char *filename, size_t elementosEsperados);
int verificarTodasLasSecuencias(void);

#endif 