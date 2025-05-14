#include "disco.h"
#include "merge.h" 
#include "quick.h"
#include "experimento.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//estructura para resultados completos
typedef struct {
    size_t N;
    EstadisticasMerge statsMerge[5];
    EstadisticasQuick statsQuick[5];
    double promedioTiempoMerge;
    double promedioTiempoQuick;  
    double promedioAccesosMerge;
    double promedioAccesosQuick;
} ResultadoCompleto;

//declaraciones de funciones del main
int ejecutarExperimentosCompletos(void);
int encontrarYUsarAridadOptima(void);
int ejecutarComparacionAlgoritmos(int aridad);
int calcularPromedios(ResultadoCompleto *resultado);
int guardarResultadoIndividualCSV(ResultadoCompleto *resultado, int esElPrimero);
char* generarNombreSecuencia(size_t N, int secuencia);
void limpiarTemporales(void);

//funcion auxiliar para verificar archivos ordenados
int verificarArchivoOrdenado(const char *filename, size_t N) {
    ArchivoBin archivo;
    if (abrirArchivo(&archivo, filename, "rb") != 0) {
        return -1;
    }
    
    int64_t anterior, actual;
    
    //leer primer elemento
    if (leerElemento(&archivo, 0, &anterior) != 0) {
        cerrarArchivo(&archivo);
        return -1;
    }
    
    //verificar orden
    for (size_t i = 1; i < N; i++) {
        if (leerElemento(&archivo, i, &actual) != 0) {
            cerrarArchivo(&archivo);
            return -1;
        }
        
        if (actual < anterior) {
            cerrarArchivo(&archivo);
            return 0; //no esta ordenado
        }
        
        anterior = actual;
    }
    
    cerrarArchivo(&archivo);
    return 1; //esta ordenado
}

int main(int argc, char *argv[]) {
    //ejecutar experimentos completos
    if (ejecutarExperimentosCompletos() != 0) {
        return 1;
    }
    
    //limpiar archivos temporales
    limpiarTemporales();
    
    return 0;
}

int ejecutarExperimentosCompletos(void) {
    printf("Encontrando aridad optima...\n");
    int aridad = encontrarYUsarAridadOptima();
    if (aridad < 0) {
        return -1;
    }
    
    printf("Ejecutando comparaciones...\n");
    if (ejecutarComparacionAlgoritmos(aridad) != 0) {
        return -1;
    }
    
    return 0;
}

int encontrarYUsarAridadOptima(void) {
    //usar archivo de 60M para encontrar aridad
    size_t M = MEMORY_LIMIT / ELEMENT_SIZE;
    size_t N_60M = 60 * M;
    
    //crear archivo de prueba si no existe
    char *archivoTest = generarNombreSecuencia(N_60M, 0);
    
    if (obtenerTamañoArchivo(archivoTest) != N_60M) {
        if (generarSecuenciaAleatoria(archivoTest, N_60M) != 0) {
            free(archivoTest);
            return -1;
        }
    }
    
    //encontrar aridad optima
    int aridad = encontrarAridadOptima(archivoTest, N_60M);
    
    free(archivoTest);
    return aridad;
}

int ejecutarComparacionAlgoritmos(int aridad) {
    size_t M = MEMORY_LIMIT / ELEMENT_SIZE;
    
    for (int multiplicador = 4; multiplicador <= 60; multiplicador += 4) {
        size_t N = multiplicador * M;
        ResultadoCompleto resultado;
        resultado.N = N;
        
        printf("Procesando %dM...\n", multiplicador);
        
        //generar secuencias para este N si no existen
        for (int secuencia = 0; secuencia < 5; secuencia++) {
            char *archivo = generarNombreSecuencia(N, secuencia);
            
            if (obtenerTamañoArchivo(archivo) != N) {
                if (generarSecuenciaAleatoria(archivo, N) != 0) {
                    free(archivo);
                    return -1;
                }
            }
            free(archivo);
        }
        
        //ejecutar experimentos para este N
        for (int secuencia = 0; secuencia < 5; secuencia++) {
            char *archivo = generarNombreSecuencia(N, secuencia);
            
            //ejecutar mergesort
            char archivoSalidaMerge[256];
            snprintf(archivoSalidaMerge, sizeof(archivoSalidaMerge), 
                     "resultado_merge_%d_%d.bin", multiplicador, secuencia);
            
            if (mergesortExterno(archivo, archivoSalidaMerge, N, aridad, 
                                &resultado.statsMerge[secuencia]) != 0) {
                free(archivo);
                return -1;
            }
            
            //verificar resultado de mergesort
            if (verificarArchivoOrdenado(archivoSalidaMerge, N) != 1) {
                free(archivo);
                return -1;
            }
            remove(archivoSalidaMerge);
            
            //ejecutar quicksort
            char archivoSalidaQuick[256];
            snprintf(archivoSalidaQuick, sizeof(archivoSalidaQuick),
                     "resultado_quick_%d_%d.bin", multiplicador, secuencia);
            
            if (quicksortExterno(archivo, archivoSalidaQuick, N, aridad,
                                &resultado.statsQuick[secuencia]) != 0) {
                free(archivo);
                return -1;
            }
            
            //verificar resultado de quicksort
            if (verificarArchivoOrdenado(archivoSalidaQuick, N) != 1) {
                free(archivo);
                return -1;
            }
            remove(archivoSalidaQuick);
            
            free(archivo);
        }
        
        //calcular promedios para este N
        calcularPromedios(&resultado);
        
        //guardar resultados inmediatamente para este N
        guardarResultadoIndividualCSV(&resultado, multiplicador == 4);
    }
    
    return 0;
}

int calcularPromedios(ResultadoCompleto *resultado) {
    //promedios mergesort
    double sumaTiempoMerge = 0.0;
    double sumaAccesosMerge = 0.0;
    
    for (int i = 0; i < 5; i++) {
        sumaTiempoMerge += resultado->statsMerge[i].tiempoEjecucion;
        sumaAccesosMerge += resultado->statsMerge[i].accesosdisco;
    }
    
    resultado->promedioTiempoMerge = sumaTiempoMerge / 5.0;
    resultado->promedioAccesosMerge = sumaAccesosMerge / 5.0;
    
    //promedios quicksort
    double sumaTiempoQuick = 0.0;
    double sumaAccesosQuick = 0.0;
    
    for (int i = 0; i < 5; i++) {
        sumaTiempoQuick += resultado->statsQuick[i].tiempoEjecucion;
        sumaAccesosQuick += resultado->statsQuick[i].accesosdisco;
    }
    
    resultado->promedioTiempoQuick = sumaTiempoQuick / 5.0;
    resultado->promedioAccesosQuick = sumaAccesosQuick / 5.0;
    
    return 0;
}

int guardarResultadoIndividualCSV(ResultadoCompleto *resultado, int esElPrimero) {
    FILE *archivo = fopen("resultados_experimento.csv", esElPrimero ? "w" : "a");
    if (!archivo) {
        return -1;
    }
    
    //escribir header solo la primera vez
    if (esElPrimero) {
        fprintf(archivo, "=== RESUMEN PROMEDIO ===\n");
        fprintf(archivo, "Tamaño,N_Elementos,Merge_Tiempo_Promedio,Quick_Tiempo_Promedio,");
        fprintf(archivo, "Merge_Accesos_Promedio,Quick_Accesos_Promedio,Razon_Tiempo,Razon_Accesos\n");
    }
    
    //escribir datos de este N
    size_t M = MEMORY_LIMIT / ELEMENT_SIZE;
    int multiplicador = resultado->N / M;
    double razonTiempo = resultado->promedioTiempoMerge / resultado->promedioTiempoQuick;
    double razonAccesos = resultado->promedioAccesosMerge / resultado->promedioAccesosQuick;
    
    fprintf(archivo, "%dM,%zu,%.6f,%.6f,%.0f,%.0f,%.6f,%.6f\n",
            multiplicador, resultado->N,
            resultado->promedioTiempoMerge,
            resultado->promedioTiempoQuick,
            resultado->promedioAccesosMerge,
            resultado->promedioAccesosQuick,
            razonTiempo, razonAccesos);
    
    fclose(archivo);
    return 0;
}

char* generarNombreSecuencia(size_t N, int secuencia) {
    char *nombre = malloc(256);
    snprintf(nombre, 256, "secuencia_%zu_%d.bin", N, secuencia);
    return nombre;
}

void limpiarTemporales(void) {
    //limpiar archivos conocidos
    char nombreArchivo[256];
    
    //limpiar archivos de subarchivos temporales (temp_sub_X.bin)
    for (int i = 0; i < 100; i++) {
        snprintf(nombreArchivo, sizeof(nombreArchivo), "temp_sub_%d.bin", i);
        remove(nombreArchivo);
    }
    
    //limpiar archivos de quicksort temporales (temp_quick_X.bin)
    for (int i = 0; i < 100; i++) {
        snprintf(nombreArchivo, sizeof(nombreArchivo), "temp_quick_%d.bin", i);
        remove(nombreArchivo);
    }
    
    //limpiar archivos de prueba de aridad
    for (int i = 2; i <= ELEMENTS_PER_BLOCK; i++) {
        snprintf(nombreArchivo, sizeof(nombreArchivo), "copia_prueba_%d.bin", i);
        remove(nombreArchivo);
        snprintf(nombreArchivo, sizeof(nombreArchivo), "salida_prueba_%d.bin", i);
        remove(nombreArchivo);
    }
    
    //limpiar archivos de test
    remove("test_merge.bin");
    remove("test_quick.bin");
}