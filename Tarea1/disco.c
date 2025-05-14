#include "disco.h"
#include <string.h>
#include <assert.h>

long long accesos = 0; //contador de accesos

//vuelve el contador a 0
void contadorACero(void){
    accesos = 0;
}

//entrega accesos registrados
long long obtenerAccesos(void){
    return accesos;
}

int leerBloque


