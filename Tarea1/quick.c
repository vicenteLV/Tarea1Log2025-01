#include "quick.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/*
implementa quicksort externo principal
archivoEntrada: archivo binario con datos desordenados
archivoSalida: archivo binario donde guardar resultado ordenado
numElementos: cantidad de elementos en el archivo
aridad: numero de subarchivos (cantidad de pivotes + 1)
stats: estructura para guardar estadisticas
return: 0 si exito, -1 si error
*/
int quicksortExterno(const char *archivoEntrada, const char *archivoSalida,
                     size_t numElementos, int aridad, EstadisticasQuick *stats) {
    
    contadorACero();
    clock_t inicio = clock();
    
    //verificar si archivo cabe en memoria
    size_t elementosEnMemoria = MEMORY_LIMIT / ELEMENT_SIZE;
    
    if (numElementos <= elementosEnMemoria) {
        //caso base: archivo cabe en memoria, usar quicksort clasico
        printf("Archivo cabe en memoria, usando quicksort clasico\n");
        
        //cargar archivo completo en memoria
        int64_t *arreglo = malloc(numElementos * sizeof(int64_t));
        if (!arreglo) {
            printf("Error: no se pudo allocar memoria para ordenar\n");
            return -1;
        }
        
        //leer archivo a memoria
        ArchivoBin archivo;
        if (abrirArchivo(&archivo, archivoEntrada, "rb") != 0) {
            free(arreglo);
            return -1;
        }
        
        for (size_t i = 0; i < numElementos; i++) {
            if (leerElemento(&archivo, i, &arreglo[i]) != 0) {
                free(arreglo);
                cerrarArchivo(&archivo);
                return -1;
            }
        }
        cerrarArchivo(&archivo);
        
        //ordenar en memoria
        printf("  Ordenando %zu elementos en memoria\n", numElementos);
        quicksortClasico(arreglo, numElementos);
        
        //escribir resultado al archivo de salida
        if (abrirArchivo(&archivo, archivoSalida, "wb") != 0) {
            free(arreglo);
            return -1;
        }
        
        for (size_t i = 0; i < numElementos; i++) {
            if (escribirElemento(&archivo, i, arreglo[i]) != 0) {
                free(arreglo);
                cerrarArchivo(&archivo);
                return -1;
            }
        }
        
        cerrarArchivo(&archivo);
        free(arreglo);
    } else {
        //caso recursivo
        printf("Particionando archivo con %d pivotes (aridad %d)\n", aridad - 1, aridad);
        
        //a-1 pivotes
        int numPivotes = aridad - 1;
        int64_t *pivotes = malloc(numPivotes * sizeof(int64_t));
        if (!pivotes) {
            return -1;
        }
        
        if (seleccionarPivotes(archivoEntrada, numElementos, numPivotes, pivotes) != 0) {
            free(pivotes);
            return -1;
        }
        
        //nombres para subarchivos
        char **nombresSubarchivos = malloc(aridad * sizeof(char*));
        size_t *tamañosSubarchivos = malloc(aridad * sizeof(size_t));
        
        for (int i = 0; i < aridad; i++) {
            nombresSubarchivos[i] = malloc(256);
            snprintf(nombresSubarchivos[i], 256, "temp_quick_%d.bin", i);
        }
        
        //particionar archivo pivotes
        if (particionarArchivo(archivoEntrada, numElementos, pivotes, numPivotes,
                              nombresSubarchivos, tamañosSubarchivos) != 0) {
            //limpiar memoria
            for (int i = 0; i < aridad; i++) {
                free(nombresSubarchivos[i]);
            }
            free(nombresSubarchivos);
            free(tamañosSubarchivos);
            free(pivotes);
            return -1;
        }
        
        free(pivotes);
        
        //ordenar cada subarchivo recursivamente
        for (int i = 0; i < aridad; i++) {
            if (tamañosSubarchivos[i] > 0) {
                printf("Ordenando subarchivo %d (%zu elementos)\n", i, tamañosSubarchivos[i]);
                
                //llamada recursiva
                EstadisticasQuick statsTemp;
                if (quicksortExterno(nombresSubarchivos[i], nombresSubarchivos[i], 
                                    tamañosSubarchivos[i], aridad, &statsTemp) != 0) {
                    for (int j = 0; j < aridad; j++) {
                        free(nombresSubarchivos[j]);
                    }
                    free(nombresSubarchivos);
                    free(tamañosSubarchivos);
                    return -1;
                }
            }
        }
        
        //concatenar subarchivos ordenados
        printf("Concatenando %d subarchivos\n", aridad);
        if (concatenarSubarchivos(nombresSubarchivos, aridad, archivoSalida) != 0) {
            //limpiar memoria
            for (int i = 0; i < aridad; i++) {
                free(nombresSubarchivos[i]);
            }
            free(nombresSubarchivos);
            free(tamañosSubarchivos);
            return -1;
        }
        
        //eliminar archivos temporales
        for (int i = 0; i < aridad; i++) {
            remove(nombresSubarchivos[i]);
            free(nombresSubarchivos[i]);
        }
        free(nombresSubarchivos);
        free(tamañosSubarchivos);
    }
    
    //guardar estadisticas
    if (stats) {
        stats->accesosdisco = obtenerAccesos();
        stats->tiempoEjecucion = ((double)(clock() - inicio)) / CLOCKS_PER_SEC;
        stats->aridadUsada = aridad;
    }
    
    return 0;
}

/*
quicksort externo recursivo (version simplificada)
archivoEntrada: archivo a ordenar en lugar
numElementos: cantidad de elementos
aridad: numero de subarchivos a usar
return: 0 si exito, -1 si error
*/
int quicksortExternoRecursivo(const char *archivoEntrada, size_t numElementos, 
                              int aridad) {
    EstadisticasQuick stats;
    return quicksortExterno(archivoEntrada, archivoEntrada, numElementos, aridad, &stats);
}

/*
selecciona a-1 pivotes aleatorios de un bloque del archivo
archivo: archivo de donde seleccionar pivotes
numElementos: cantidad total de elementos
numPivotes: cantidad de pivotes a seleccionar (a-1)
pivotes: array donde guardar los pivotes seleccionados
return: 0 si exito, -1 si error
*/
int seleccionarPivotes(const char *archivo, size_t numElementos, 
                       int numPivotes, int64_t *pivotes) {
    
    //leer un bloque aleatorio
    ArchivoBin arch;
    if (abrirArchivo(&arch, archivo, "rb") != 0) {
        return -1;
    }
    
    //calcular numero de bloques disponibles
    size_t totalBloques = (numElementos + ELEMENTS_PER_BLOCK - 1) / ELEMENTS_PER_BLOCK;
    
    //seleccionar bloque aleatorio
    srand(time(NULL));
    size_t bloqueAleatorio = rand() % totalBloques;
    
    //cargar el bloque aleatorio
    if (cargarEnMemoria(&arch, bloqueAleatorio) != 0) {
        cerrarArchivo(&arch);
        return -1;
    }
    
    //calcular cuantos elementos tiene este bloque
    size_t elementosEnBloque = ELEMENTS_PER_BLOCK;
    if (bloqueAleatorio == totalBloques - 1) {
        //ultimo bloque puede tener menos elementos
        elementosEnBloque = numElementos - (bloqueAleatorio * ELEMENTS_PER_BLOCK);
    }
    
    //seleccionar numPivotes elementos aleatorios del bloque
    int64_t *elementosBloque = malloc(elementosEnBloque * sizeof(int64_t));
    if (!elementosBloque) {
        cerrarArchivo(&arch);
        return -1;
    }
    
    //copiar elementos del buffer al array temporal
    for (size_t i = 0; i < elementosEnBloque; i++) {
        elementosBloque[i] = arch.buffer[i];
    }
    
    cerrarArchivo(&arch);
    
    //si el bloque tiene menos elementos que pivotes necesarios
    if (elementosEnBloque < numPivotes) {
        printf("Error: bloque tiene menos elementos que pivotes necesarios\n");
        free(elementosBloque);
        return -1;
    }
    
    //seleccionar numPivotes elementos aleatorios
    for (int i = 0; i < numPivotes; i++) {
        int indiceAleatorio = rand() % elementosEnBloque;
        pivotes[i] = elementosBloque[indiceAleatorio];
        
        //evitar duplicados removiendo el elemento seleccionado
        elementosBloque[indiceAleatorio] = elementosBloque[elementosEnBloque - 1];
        elementosEnBloque--;
    }
    
    free(elementosBloque);
    
    //ordenar los pivotes para facilitar la particion
    qsort(pivotes, numPivotes, sizeof(int64_t), compararInt64);
    
    printf("  Pivotes seleccionados: ");
    for (int i = 0; i < numPivotes; i++) {
        printf("%ld ", pivotes[i]);
    }
    printf("\n");
    
    return 0;
}

/*
particiona el archivo en a subarchivos usando a-1 pivotes
archivoEntrada: archivo a particionar
numElementos: cantidad de elementos
pivotes: array con a-1 pivotes ordenados
numPivotes: cantidad de pivotes (a-1)
nombresSubarchivos: array con nombres de a subarchivos
tamañosSubarchivos: array donde guardar el tamaño de cada subarchivo
return: 0 si exito, -1 si error
*/
int particionarArchivo(const char *archivoEntrada, size_t numElementos,
                       int64_t *pivotes, int numPivotes, 
                       char **nombresSubarchivos, size_t *tamañosSubarchivos) {
    
    //abrir archivo de entrada
    ArchivoBin entrada;
    if (abrirArchivo(&entrada, archivoEntrada, "rb") != 0) {
        return -1;
    }
    
    //crear y abrir todos los subarchivos
    int numSubarchivos = numPivotes + 1;
    ArchivoBin *subarchivos = malloc(numSubarchivos * sizeof(ArchivoBin));
    
    for (int i = 0; i < numSubarchivos; i++) {
        if (abrirArchivo(&subarchivos[i], nombresSubarchivos[i], "wb") != 0) {
            //limpiar en caso de error
            for (int j = 0; j < i; j++) {
                cerrarArchivo(&subarchivos[j]);
            }
            free(subarchivos);
            cerrarArchivo(&entrada);
            return -1;
        }
        tamañosSubarchivos[i] = 0;
    }
    
    //leer todos los elementos y particionarlos
    for (size_t i = 0; i < numElementos; i++) {
        int64_t elemento;
        if (leerElemento(&entrada, i, &elemento) != 0) {
            for (int j = 0; j < numSubarchivos; j++) {
                cerrarArchivo(&subarchivos[j]);
            }
            free(subarchivos);
            cerrarArchivo(&entrada);
            return -1;
        }
        
        //determinar a que subarchivo va el elemento
        int subarchivo = 0;
        for (int j = 0; j < numPivotes; j++) {
            if (elemento <= pivotes[j]) {
                break;
            }
            subarchivo++;
        }
        
        //escribir elemento al subarchivo correspondiente
        if (escribirElemento(&subarchivos[subarchivo], tamañosSubarchivos[subarchivo], elemento) != 0) {
            for (int j = 0; j < numSubarchivos; j++) {
                cerrarArchivo(&subarchivos[j]);
            }
            free(subarchivos);
            cerrarArchivo(&entrada);
            return -1;
        }
        
        tamañosSubarchivos[subarchivo]++;
    }
    
    //cerrar todos los archivos
    cerrarArchivo(&entrada);
    for (int i = 0; i < numSubarchivos; i++) {
        cerrarArchivo(&subarchivos[i]);
        printf("  Subarchivo %d: %zu elementos\n", i, tamañosSubarchivos[i]);
    }
    
    free(subarchivos);
    return 0;
}

/*
concatena multiples subarchivos ordenados en un archivo de salida
nombresSubarchivos: array con nombres de subarchivos
numArchivos: cantidad de subarchivos
archivoSalida: archivo donde guardar el resultado concatenado
return: 0 si exito, -1 si error
*/
int concatenarSubarchivos(char **nombresSubarchivos, int numArchivos,
                          const char *archivoSalida) {
    
    //abrir archivo de salida
    ArchivoBin salida;
    if (abrirArchivo(&salida, archivoSalida, "wb") != 0) {
        return -1;
    }
    
    size_t posicionSalida = 0;
    
    //concatenar cada subarchivo en orden
    for (int i = 0; i < numArchivos; i++) {
        size_t tamañoSubarchivo = obtenerTamañoArchivo(nombresSubarchivos[i]);
        
        if (tamañoSubarchivo > 0) {
            //abrir subarchivo
            ArchivoBin subarchivo;
            if (abrirArchivo(&subarchivo, nombresSubarchivos[i], "rb") != 0) {
                cerrarArchivo(&salida);
                return -1;
            }
            
            //copiar todos los elementos del subarchivo
            for (size_t j = 0; j < tamañoSubarchivo; j++) {
                int64_t elemento;
                if (leerElemento(&subarchivo, j, &elemento) != 0 ||
                    escribirElemento(&salida, posicionSalida, elemento) != 0) {
                    cerrarArchivo(&subarchivo);
                    cerrarArchivo(&salida);
                    return -1;
                }
                posicionSalida++;
            }
            
            cerrarArchivo(&subarchivo);
            printf("  Concatenado subarchivo %d: %zu elementos\n", i, tamañoSubarchivo);
        }
    }
    
    cerrarArchivo(&salida);
    printf("  Total concatenado: %zu elementos\n", posicionSalida);
    return 0;
}

/*
implementa quicksort clasico en memoria
arreglo: arreglo a ordenar
n: cantidad de elementos
return: 0 si exito, -1 si error
*/
int quicksortClasico(int64_t *arreglo, size_t n) {
    if (n <= 1) return 0;
    
    //usar qsort de la biblioteca estandar para simplicidad
    qsort(arreglo, n, sizeof(int64_t), compararInt64);
    
    return 0;
}

/*
particiona un arreglo usando el ultimo elemento como pivote
arreglo: arreglo a particionar
inicio: indice inicial
fin: indice final
return: indice de la posicion final del pivote
*/
int partitionClasico(int64_t *arreglo, int inicio, int fin) {
    int64_t pivote = arreglo[fin];
    int i = inicio - 1;
    
    for (int j = inicio; j < fin; j++) {
        if (arreglo[j] <= pivote) {
            i++;
            intercambiar(&arreglo[i], &arreglo[j]);
        }
    }
    intercambiar(&arreglo[i + 1], &arreglo[fin]);
    
    return i + 1;
}

/*
intercambia dos elementos
a: puntero al primer elemento
b: puntero al segundo elemento
return: void
*/
void intercambiar(int64_t *a, int64_t *b) {
    int64_t temp = *a;
    *a = *b;
    *b = temp;
}

/*
funcion de comparacion para qsort
a: puntero al primer elemento
b: puntero al segundo elemento
return: -1 si a<b, 0 si a==b, 1 si a>b
*/
int compararInt64(const void *a, const void *b) {
    int64_t va = *(const int64_t*)a;
    int64_t vb = *(const int64_t*)b;
    
    if (va < vb) return -1;
    if (va > vb) return 1;
    return 0;
}