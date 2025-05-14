#include "disco.h"
#include "experimento.h"
#include <time.h>
#include <string.h>

/*
genera una secuencia aleatoria de numeros de 64 bits
filename: nombre del archivo a crear
numElementos: cantidad de elementos a generar
return: 0 si exito, -1 si error
*/
int generarSecuenciaAleatoria(const char *filename, size_t numElementos) {
    ArchivoBin archivo;
    if (abrirArchivo(&archivo, filename, "wb") != 0) {
        printf("Error: no se pudo crear archivo %s\n", filename);
        return -1;
    }
    
    //configurar semilla 
    srand(time(NULL) + rand());
    
    printf("Generando %s con %zu elementos...\n", filename, numElementos);
    
    for (size_t i = 0; i < numElementos; i++) {
        //generar numero
        int64_t numeroAleatorio = ((int64_t)rand() << 32) | rand();
        
        if (escribirElemento(&archivo, i, numeroAleatorio) != 0) {
            printf("Error escribiendo elemento %zu\n", i);
            cerrarArchivo(&archivo);
            return -1;
        }
        
        //mostrar progreso cada millon de elementos
        if ((i + 1) % 1000000 == 0) {
            printf("  Progreso: %zu/%zu elementos\n", i + 1, numElementos);
        }
    }
    
    cerrarArchivo(&archivo);
    printf("Archivo %s generado exitosamente\n", filename);
    return 0;
}

/*
genera 5 secuencias aleatorias para un tamaño N, 
cada secuencia se guarda en un archivo separado
N: tamaño de cada secuencia (en elementos)
return: 0 si exito, -1 si error
*/
int generarSecuenciasParaTamaño(size_t N) {
    char nombreArchivo[256];
    
    printf("\n=== Generando 5 secuencias para N = %zu ===\n", N);
    
    for (int secuencia = 0; secuencia < 5; secuencia++) {
        //crear nombre de archivo con N incluido: secuencia_N_num.bin
        snprintf(nombreArchivo, sizeof(nombreArchivo), "secuencia_%zu_%d.bin", N, secuencia);
        
        printf("Secuencia %d/5: %s\n", secuencia + 1, nombreArchivo);
        
        //resetear contador para esta secuencia
        contadorACero();
        
        if (generarSecuenciaAleatoria(nombreArchivo, N) != 0) {
            printf("Error generando secuencia %d para N=%zu\n", secuencia, N);
            return -1;
        }
        
        printf("Accesos a disco: %lld\n", obtenerAccesos());
    }
    
    return 0;
}

/*
genera todas las secuencias de prueba
cada archivo contiene una sola secuencia con N en el nombre
return: 0 si exito, -1 si error
*/
int generarTodasLasSecuencias(void) {
    //calcular M (elementos que caben en 50MB)
    size_t M = MEMORY_LIMIT / ELEMENT_SIZE;
    
    printf("=== Generando todas las secuencias de prueba ===\n");
    printf("M (50MB) = %zu elementos\n", M);
    printf("Cada archivo: secuencia_N_num.bin\n\n"); //explicacion formato
    
    //generar secuencias para cada tamaño
    for (int multiplicador = 4; multiplicador <= 60; multiplicador += 4) {
        size_t N = multiplicador * M;
        
        printf("Tamaño actual: %dM (%zu elementos)\n", multiplicador, N);
        
        if (generarSecuenciasParaTamaño(N) != 0) {
            printf("Error generando secuencias para %dM\n", multiplicador);
            return -1;
        }
        
        printf("Completado: %dM\n", multiplicador);
    }
    
    printf("\n=== Todas las secuencias generadas exitosamente ===\n");
    printf("Total de archivos creados: 75 (15 tamaños x 5 secuencias)\n");
    return 0;
}

/*
verifica que un archivo binario existe y tiene el tamaño correcto
filename: nombre del archivo a verificar
elementosEsperados: cantidad de elementos que deberia tener
return: 1 si el archivo es correcto, 0 si no, -1 si error
*/
int verificarArchivoGenerado(const char *filename, size_t elementosEsperados) {
    size_t tamañoReal = obtenerTamañoArchivo(filename);
    
    if (tamañoReal == 0) {
        printf("Archivo %s no existe o esta vacio\n", filename);
        return -1;
    }
    
    if (tamañoReal != elementosEsperados) {
        printf("Archivo %s: tamaño incorrecto (%zu en lugar de %zu)\n", 
               filename, tamañoReal, elementosEsperados);
        return 0;
    }
    
    return 1;
}

/*
genera una sola secuencia para un tamaño N especifico
N: tamaño de la secuencia
numeroSecuencia: numero de la secuencia (0-4)
return: 0 si exito, -1 si error
*/
int generarUnaSecuencia(size_t N, int numeroSecuencia) {
    char nombreArchivo[256];
    snprintf(nombreArchivo, sizeof(nombreArchivo), "secuencia_%zu_%d.bin", N, numeroSecuencia);
    
    printf("Generando secuencia individual: %s\n", nombreArchivo);
    contadorACero();
    
    if (generarSecuenciaAleatoria(nombreArchivo, N) != 0) {
        return -1;
    }
    
    printf("Accesos a disco: %lld\n", obtenerAccesos());
    return 0;
}