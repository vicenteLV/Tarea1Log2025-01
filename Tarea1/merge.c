#include "merge.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

/*
implementa merge externo principal
archivoEntrada: archivo binario con datos desordenados
archivoSalida: archivo binario donde guardar resultado ordenado
numElementos: cantidad de elementos en el archivo
aridad: numero de subarchivos a crear (aridad del merge)
stats: estructura para guardar estadisticas
return: 0 si exito, -1 si error
*/
int mergesortExterno(const char *archivoEntrada, const char *archivoSalida, 
                     size_t numElementos, int aridad, EstadisticasMerge *stats) {
    
    contadorACero();
    clock_t inicio = clock();
    
    //verificar si archivo cabe en memoria
    size_t elementosEnMemoria = MEMORY_LIMIT / ELEMENT_SIZE;
    
    if (numElementos <= elementosEnMemoria) {
        //caso base: archivo cabe en memoria, usar mergesort clasico
        printf("Archivo cabe en memoria, usando mergesort clasico\n");
        
        if (ordenarSubarchivoEnMemoria(archivoEntrada, numElementos) != 0) {
            return -1;
        }
        
        //copiar archivo ordenado al archivo de salida
        ArchivoBin entrada, salida;
        if (abrirArchivo(&entrada, archivoEntrada, "rb") != 0 ||
            abrirArchivo(&salida, archivoSalida, "wb") != 0) {
            return -1;
        }
        
        for (size_t i = 0; i < numElementos; i++) {
            int64_t elemento;
            if (leerElemento(&entrada, i, &elemento) != 0 ||
                escribirElemento(&salida, i, elemento) != 0) {
                cerrarArchivo(&entrada);
                cerrarArchivo(&salida);
                return -1;
            }
        }
        
        cerrarArchivo(&entrada);
        cerrarArchivo(&salida);
    } else {
        //caso recursivo: dividir, ordenar y mezclar
        printf("Dividiendo archivo en %d subarchivos\n", aridad);
        
        //crear nombres para subarchivos
        char **nombresSubarchivos = malloc(aridad * sizeof(char*));
        for (int i = 0; i < aridad; i++) {
            nombresSubarchivos[i] = malloc(256);
            snprintf(nombresSubarchivos[i], 256, "temp_sub_%d.bin", i);
        }
        
        //dividir archivo
        if (dividirArchivo(archivoEntrada, numElementos, aridad, nombresSubarchivos) != 0) {
            //limpiar memoria
            for (int i = 0; i < aridad; i++) {
                free(nombresSubarchivos[i]);
            }
            free(nombresSubarchivos);
            return -1;
        }
        
        //ordenar cada subarchivo recursivamente
        for (int i = 0; i < aridad; i++) {
            size_t tamañoSubarchivo = obtenerTamañoArchivo(nombresSubarchivos[i]);
            printf("Ordenando subarchivo %d (%zu elementos)\n", i, tamañoSubarchivo);
            
            //llamada recursiva
            EstadisticasMerge statsTemp;
            if (mergesortExterno(nombresSubarchivos[i], nombresSubarchivos[i], 
                                tamañoSubarchivo, aridad, &statsTemp) != 0) {
                //limpiar memoria
                for (int j = 0; j < aridad; j++) {
                    free(nombresSubarchivos[j]);
                }
                free(nombresSubarchivos);
                return -1;
            }
        }
        
        //mezclar subarchivos ordenados
        printf("Mezclando %d subarchivos\n", aridad);
        if (mezclarSubarchivos(nombresSubarchivos, aridad, archivoSalida, numElementos) != 0) {
            //limpiar memoria
            for (int i = 0; i < aridad; i++) {
                free(nombresSubarchivos[i]);
            }
            free(nombresSubarchivos);
            return -1;
        }
        
        //eliminar archivos temporales
        for (int i = 0; i < aridad; i++) {
            remove(nombresSubarchivos[i]);
            free(nombresSubarchivos[i]);
        }
        free(nombresSubarchivos);
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
divide un archivo grande en varios subarchivos
archivoEntrada: archivo a dividir
numElementos: total de elementos en el archivo
aridad: numero de subarchivos a crear
nombresSubarchivos: array con nombres de los subarchivos a crear
return: 0 si exito, -1 si error
*/
int dividirArchivo(const char *archivoEntrada, size_t numElementos, 
                   int aridad, char **nombresSubarchivos) {
    
    ArchivoBin entrada;
    if (abrirArchivo(&entrada, archivoEntrada, "rb") != 0) {
        return -1;
    }
    
    //calcular tamaño de cada subarchivo
    size_t elementosPorSubarchivo = numElementos / aridad;
    size_t elementosExtra = numElementos % aridad;
    
    size_t posicionActual = 0;
    
    for (int i = 0; i < aridad; i++) {
        //calcular tamaño de este subarchivo
        size_t tamañoEsteSubarchivo = elementosPorSubarchivo;
        if (i < elementosExtra) {
            tamañoEsteSubarchivo++;
        }
        
        //crear subarchivo
        ArchivoBin subarchivo;
        if (abrirArchivo(&subarchivo, nombresSubarchivos[i], "wb") != 0) {
            cerrarArchivo(&entrada);
            return -1;
        }
        
        //copiar elementos al subarchivo
        for (size_t j = 0; j < tamañoEsteSubarchivo; j++) {
            int64_t elemento;
            if (leerElemento(&entrada, posicionActual + j, &elemento) != 0 ||
                escribirElemento(&subarchivo, j, elemento) != 0) {
                cerrarArchivo(&entrada);
                cerrarArchivo(&subarchivo);
                return -1;
            }
        }
        
        cerrarArchivo(&subarchivo);
        posicionActual += tamañoEsteSubarchivo;
        
        printf("Subarchivo %d: %zu elementos\n", i, tamañoEsteSubarchivo);
    }
    
    cerrarArchivo(&entrada);
    return 0;
}

/*
ordena un subarchivo que cabe en memoria usando mergesort clasico
nombreArchivo: archivo a ordenar en lugar
numElementos: cantidad de elementos en el archivo
return: 0 si exito, -1 si error
*/
int ordenarSubarchivoEnMemoria(const char *nombreArchivo, size_t numElementos) {
    //cargar archivo completo en memoria
    int64_t *arreglo = malloc(numElementos * sizeof(int64_t));
    if (!arreglo) {
        printf("Error\n");
        return -1;
    }
    
    //leer archivo a memoria
    ArchivoBin archivo;
    if (abrirArchivo(&archivo, nombreArchivo, "rb") != 0) {
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
    mergesortClasico(arreglo, numElementos);
    
    //escribir de vuelta al archivo
    if (abrirArchivo(&archivo, nombreArchivo, "wb") != 0) {
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
    return 0;
}

/*
mezcla multiples subarchivos ordenados en uno solo
nombresSubarchivos: array con nombres de subarchivos ordenados
numArchivos: cantidad de subarchivos
archivoSalida: archivo donde guardar el resultado mezclado
numElementosTotal: total de elementos esperados
return: 0 si exito, -1 si error
*/
int mezclarSubarchivos(char **nombresSubarchivos, int numArchivos, 
                       const char *archivoSalida, size_t numElementosTotal) {
    
    //abrir todos los subarchivos para lectura
    ArchivoBin *subarchivos = malloc(numArchivos * sizeof(ArchivoBin));
    size_t *posiciones = malloc(numArchivos * sizeof(size_t));
    size_t *tamaños = malloc(numArchivos * sizeof(size_t));
    int64_t *valoresActuales = malloc(numArchivos * sizeof(int64_t));
    int *activos = malloc(numArchivos * sizeof(int));
    
    for (int i = 0; i < numArchivos; i++) {
        if (abrirArchivo(&subarchivos[i], nombresSubarchivos[i], "rb") != 0) {
            //limpiar en caso de error
            for (int j = 0; j < i; j++) {
                cerrarArchivo(&subarchivos[j]);
            }
            free(subarchivos);
            free(posiciones);
            free(tamaños);
            free(valoresActuales);
            free(activos);
            return -1;
        }
        
        posiciones[i] = 0;
        tamaños[i] = obtenerTamañoArchivo(nombresSubarchivos[i]);
        activos[i] = (tamaños[i] > 0) ? 1 : 0;
        
        //leer primer elemento de cada subarchivo
        if (activos[i]) {
            if (leerElemento(&subarchivos[i], 0, &valoresActuales[i]) != 0) {
                activos[i] = 0;
            }
        }
    }
    
    //abrir archivo de salida
    ArchivoBin salida;
    if (abrirArchivo(&salida, archivoSalida, "wb") != 0) {
        for (int i = 0; i < numArchivos; i++) {
            cerrarArchivo(&subarchivos[i]);
        }
        free(subarchivos);
        free(posiciones);
        free(tamaños);
        free(valoresActuales);
        free(activos);
        return -1;
    }
    
    //merge de k-vias (k-way merge)
    size_t elementosEscritos = 0;
    
    while (elementosEscritos < numElementosTotal) {
        //encontrar el minimo de todos los valores actuales
        int indiceMenor = -1;
        int64_t valorMenor;
        
        for (int i = 0; i < numArchivos; i++) {
            if (activos[i]) {
                if (indiceMenor == -1 || valoresActuales[i] < valorMenor) {
                    indiceMenor = i;
                    valorMenor = valoresActuales[i];
                }
            }
        }
        
        if (indiceMenor == -1) {
            printf("Error: no hay mas elementos para mezclar\n");
            break;
        }
        
        //escribir el menor al archivo de salida
        if (escribirElemento(&salida, elementosEscritos, valorMenor) != 0) {
            printf("Error escribiendo elemento %zu\n", elementosEscritos);
            break;
        }
        
        elementosEscritos++;
        posiciones[indiceMenor]++;
        
        //leer siguiente elemento del subarchivo elegido
        if (posiciones[indiceMenor] < tamaños[indiceMenor]) {
            if (leerElemento(&subarchivos[indiceMenor], posiciones[indiceMenor], 
                            &valoresActuales[indiceMenor]) != 0) {
                activos[indiceMenor] = 0;
            }
        } else {
            activos[indiceMenor] = 0;
        }
    }
    
    //cerrar todos los archivos
    cerrarArchivo(&salida);
    for (int i = 0; i < numArchivos; i++) {
        cerrarArchivo(&subarchivos[i]);
    }
    
    //liberar memoria
    free(subarchivos);
    free(posiciones);
    free(tamaños);
    free(valoresActuales);
    free(activos);
    
    printf("  Elementos mezclados: %zu\n", elementosEscritos);
    return 0;
}

/*
encuentra la aridad optima usando busqueda binaria
archivoPrueba: archivo para probar diferentes valores  
numElementos: cantidad de elementos en el archivo
return: aridad optima encontrada, -1 si error
*/
int encontrarAridadOptima(const char *archivoPrueba, size_t numElementos) {
    int b = ELEMENTS_PER_BLOCK; 
    int aridadMin = 2;
    int aridadMax = b;
    
    printf("=== Buscando aridad optima con busqueda binaria ===\n");
    printf("Rango: [%d, %d] (b = %d)\n", aridadMin, aridadMax, b);
    
    int mejorAridad = aridadMin;
    double mejorTiempo = -1;
    long long mejorAccesos = -1;
    
    //busqueda binaria para encontrar la aridad que genera mejores resultados
    int izq = aridadMin;
    int der = aridadMax;
    
    //evaluar extremos y algunos puntos medios
    int aridadesAProbar[] = {izq, (izq + der) / 4, (izq + der) / 2, 3 * (izq + der) / 4, der};
    int numPruebas = 5;
    
    for (int i = 0; i < numPruebas; i++) {
        int aridadActual = aridadesAProbar[i];
        if (aridadActual < aridadMin || aridadActual > aridadMax) continue;
        
        //hacer copia del archivo original para prueba
        char archivoCopia[256];
        snprintf(archivoCopia, sizeof(archivoCopia), "copia_prueba_%d.bin", aridadActual);
        
        //copiar archivo original
        ArchivoBin original, copia;
        if (abrirArchivo(&original, archivoPrueba, "rb") != 0 ||
            abrirArchivo(&copia, archivoCopia, "wb") != 0) {
            continue;
        }
        
        for (size_t j = 0; j < numElementos; j++) {
            int64_t elemento;
            if (leerElemento(&original, j, &elemento) != 0 ||
                escribirElemento(&copia, j, elemento) != 0) {
                break;
            }
        }
        cerrarArchivo(&original);
        cerrarArchivo(&copia);
        
        //probar esta aridad
        EstadisticasMerge stats;
        if (probarAridad(archivoCopia, numElementos, aridadActual, &stats) == 0) {
            printf("Aridad %d: %.3f seg, %lld accesos\n", 
                   aridadActual, stats.tiempoEjecucion, stats.accesosdisco);
            
            //criterio de optimización: minimizar tiempo, en caso de empate minimizar accesos
            if (mejorTiempo < 0 || stats.tiempoEjecucion < mejorTiempo ||
                (stats.tiempoEjecucion == mejorTiempo && stats.accesosdisco < mejorAccesos)) {
                mejorTiempo = stats.tiempoEjecucion;
                mejorAccesos = stats.accesosdisco;
                mejorAridad = aridadActual;
            }
        }
        
        //limpiar archivo temporal
        remove(archivoCopia);
    }
    
    printf("=== Aridad optima encontrada: %d ===\n", mejorAridad);
    printf("Tiempo: %.3f seg, Accesos: %lld\n", mejorTiempo, mejorAccesos);
    return mejorAridad;
}

/*
prueba un valor de aridad especifico
archivoPrueba: archivo para ordenar
numElementos: cantidad de elementos
aridad: valor de aridad a probar
stats: estructura para guardar estadisticas
return: 0 si exito, -1 si error
*/
int probarAridad(const char *archivoPrueba, size_t numElementos, 
                 int aridad, EstadisticasMerge *stats) {
    
    char archivoSalida[256];
    snprintf(archivoSalida, sizeof(archivoSalida), "salida_prueba_%d.bin", aridad);
    
    //ejecutar mergesort con esta aridad
    int resultado = mergesortExterno(archivoPrueba, archivoSalida, numElementos, aridad, stats);
    
    //limpiar archivo de salida
    remove(archivoSalida);
    
    return resultado;
}

/*
calcula la maxima aridad posible basada en el tamaño de bloque
return: maxima aridad calculada
*/
int calcularMaxAridad(void) {
    //la aridad maxima esta limitada por cuántos elementos caben en un bloque
    return ELEMENTS_PER_BLOCK;
}

/*
implementa mergesort clasico en memoria
arreglo: arreglo a ordenar
n: cantidad de elementos
return: 0 si exito, -1 si error
*/
int mergesortClasico(int64_t *arreglo, size_t n) {
    if (n <= 1) return 0;
    
    size_t medio = n / 2;
    
    //ordenar recursivamente cada mitad
    mergesortClasico(arreglo, medio);
    mergesortClasico(arreglo + medio, n - medio);
    
    //mezclar las dos mitades ordenadas
    mergeClasico(arreglo, 0, medio, n);
    
    return 0;
}

/*
mezcla dos subarreglos ordenados en uno solo
arreglo: arreglo que contiene ambos subarreglos
inicio: indice inicial del primer subarreglo
medio: indice donde empieza el segundo subarreglo
fin: indice final (exclusivo) del segundo subarreglo
return: void
*/
void mergeClasico(int64_t *arreglo, size_t inicio, size_t medio, size_t fin) {
    //crear arreglos temporales para las dos mitades
    size_t n1 = medio - inicio;
    size_t n2 = fin - medio;
    
    int64_t *izquierda = malloc(n1 * sizeof(int64_t));
    int64_t *derecha = malloc(n2 * sizeof(int64_t));
    
    //copiar datos a arreglos temporales
    for (size_t i = 0; i < n1; i++) {
        izquierda[i] = arreglo[inicio + i];
    }
    for (size_t j = 0; j < n2; j++) {
        derecha[j] = arreglo[medio + j];
    }
    
    //mezclar los arreglos temporales de vuelta al arreglo original
    size_t i = 0, j = 0, k = inicio;
    
    while (i < n1 && j < n2) {
        if (izquierda[i] <= derecha[j]) {
            arreglo[k] = izquierda[i];
            i++;
        } else {
            arreglo[k] = derecha[j];
            j++;
        }
        k++;
    }
    
    //copiar elementos restantes
    while (i < n1) {
        arreglo[k] = izquierda[i];
        i++;
        k++;
    }
    
    while (j < n2) {
        arreglo[k] = derecha[j];
        j++;
        k++;
    }
    
    free(izquierda);
    free(derecha);
}