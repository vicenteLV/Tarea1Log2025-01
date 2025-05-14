# Tarea1Log2025-01

Este proyecto implementa y compara los algoritmos de ordenamiento externo Mergesort y Quicksort para el manejo de grandes volúmenes de datos que no caben en memoria principal. El programa está estructurado en módulos independientes:
- Sistema de Disco (disco.c/.h): Simula memoria secundaria con operaciones de I/O por bloques
- Mergesort Externo (merge.c/.h): Implementa mergesort con búsqueda automática de aridad óptima
- Quicksort Externo (quick.c/.h): Implementa quicksort con selección aleatoria de pivotes
- Generación de Datos (experimento.c/.h): Crea secuencias aleatorias para experimentación
- Control Principal (main.c): Coordina experimentos y recolecta estadísticas

El sistema genera 75 archivos de prueba (15 tamaños × 5 secuencias), encuentra la aridad óptima usando búsqueda binaria, ejecuta ambos algoritmos y registra métricas de tiempo y accesos a disco en formato CSV.

Software requerido:
- Compilador: GCC 7.0+ o compatible con C99
- Sistema operativo: Linux, macOS, o Windows con WSL

Uso del docker:
- Descargar docker desde: https://www.docker.com/
- Simular entorno con memoria limitada (desde terminal antes de ejecutar ./main): docker run --rm -it -m 500m -v "$PWD":/workspace pabloskewes/cc4102-cpp-env bash


El programa ejecutará automáticamente:
-Búsqueda de aridad óptima (~15-30 min)
-Generación de datos por tamaño
-Experimentos de ambos algoritmos
-Guardado de resultados en CSV

Salida:
-Resultados: resultados_experimento.csv
-Datos de prueba: secuencia_N_X.bin (se mantienen para reutilización)
-Archivos temporales: Se eliminan automáticamente



