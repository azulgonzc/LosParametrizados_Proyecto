/* ================================================================
* Archivo  : loteria.c
* Proyecto : Loteria Magica - Simulacion de la Loteria Mexicana
* Autores  : Equipo "Los Parametrizados"
* Estandar : C99  |  gcc -std=c99 -Wall -Wextra -o loteria loteria.c
*
* ESTANDAR DE CODIFICACION (justificacion al final del bloque)
* ---------------------------------------------------------------
*  * Macros / constantes : UPPER_SNAKE_CASE   -> visibilidad inmediata
*  * Tipos (structs)     : PascalCase         -> distingue tipos de vars
*  * Funciones           : snake_case con prefijo de modulo
*                          entrada_*  proceso_*  salida_*  util_*
*                          -> trazabilidad directa con diagrama de bloques
*  * Variables locales   : snake_case corto y descriptivo
*  * Variables globales  : snake_case con prefijo g_
*                          -> diferenciacion inmediata de alcance
*  * Arreglos de estado  : snake_case, sin prefijo (son campos de struct)
*  * Comentarios         : bloque para secciones y funciones;
*                          linea // para aclaraciones de una linea
*  * Longitud de funcion : max. ~50 lineas (cohesion funcional alta)
*  Justificacion: el estandar sigue la convencion MISRA-C lite adaptada
*  a proyectos academicos en C99.  El prefijo de modulo permite navegar
*  el archivo de un solo vistazo y facilita la futura separacion en
*  archivos .h / .c sin refactorizar nombres.
* ================================================================ */

// Bibliotecas estandar (unica dependencia externa) ----
#include <stdio.h>    // E/S estandar: printf, scanf, fgets, FILE
#include <stdlib.h>   // srand, rand, exit, memset              
#include <string.h>   // strcpy, strncpy, strcspn, memset        
#include <time.h>     // time() -> semilla aleatoria  

/* ================================================================
 *  MODULO 0 - MACROS Y CONSTANTES
 * ================================================================ */
#define MAX_JUGADORES      8
#define MIN_JUGADORES      2
#define TAM                4     // Lado del tablero (4x4)        
#define CASILLAS          16     // TAM * TAM                     
#define TOTAL_CARTAS      54
#define MAX_NOMBRE_JUG    24
#define MAX_NOMBRE_CARTA  32
#define HISTORIAL_CSV     "historial.csv"

// Modos de juego 
#define MODO_GRANDE        1     // Tabla completa                
#define MODO_CHICO         2     // Patron especial               

// Patrones (cuadro chico)
#define PAT_LIBRE          0
#define PAT_DIAG_P         1     // Diagonal principal  \         
#define PAT_DIAG_S         2     // Diagonal secundaria /         
#define PAT_FILA           3
#define PAT_COLUMNA        4
#define PAT_ESQUINAS       5
#define PAT_CENTRO         6
#define PAT_MAX            6     // Ultimo patron valido          

// Habilidades dinamicas
#define PROB_HABILIDAD    25     // % de probabilidad de activar  
#define TURNOS_BLOQUEO     2     // Turnos sin poder marcar       

/* ================================================================
 *  MODULO 1 - ESTRUCTURAS  (PascalCase)
 * ================================================================ */

// Carta del mazo con flag de habilidad especial
typedef struct {
	char nombre[MAX_NOMBRE_CARTA];
	int  habilidad;    // 1 = puede activar efecto especial     
} Carta;

// Estado de un jugador durante la partida 
typedef struct {
	char nombre[MAX_NOMBRE_JUG];
	int  tabla[TAM][TAM];          // 0 = libre | 1 = marcado  
	int  asignacion[TAM][TAM];     // indice de carta por casilla
	int  bloqueado;                // turnos restantes de bloqueo 
} Jugador;

// Contexto completo de una partida 
typedef struct {
	int  num_jugadores;
	int  modo_juego;
	int  patron_chico;
	int  orden_baraja[TOTAL_CARTAS];  // Fisher-Yates        
	int  carta_actual;                // proxima a cantar        
	int  ya_cantada[TOTAL_CARTAS];    // historial del turno     
} Partida;

/* ================================================================
 *  MODULO 2 - DATOS: LAS 54 CARTAS
 * ================================================================ */

// Mazo oficial de la Loteria Mexicana.
// Las cartas con habilidad=1 pueden activar bloqueo de marcaje.
static const Carta g_mazo[TOTAL_CARTAS] = {
    {"El Gallo",       0}, {"El Diablito",    1}, {"La Dama",        0},
    {"El Catrin",      0}, {"El Paraguas",    1}, {"La Sirena",      1},
    {"La Escalera",    0}, {"La Botella",     1}, {"El Barril",      0},
    {"El Arbol",       0}, {"El Melon",       0}, {"El Valiente",    1},
    {"El Gordo",       0}, {"La Muerte",      1}, {"El Perico",      0},
    {"La Garza",       0}, {"El Pajaro",      0}, {"La Mano",        1},
    {"La Bota",        0}, {"La Luna",        1}, {"El Cotorro",     0},
    {"El Borracho",    1}, {"El Negrito",     0}, {"El Corazon",     1},
    {"La Sandia",      0}, {"El Tambor",      0}, {"El Camaron",     0},
    {"El Pescado",     0}, {"La Palma",       0}, {"La Maceta",      0},
    {"El Arpa",        0}, {"La Rana",        0}, {"La Chalupa",     0},
    {"El Alacran",     1}, {"La Rosa",        0}, {"La Calavera",    1},
    {"El Nopal",       0}, {"El Cantaro",     0}, {"La Musica",      0},
    {"La Arana",       1}, {"El Soldado",     0}, {"La Estrella",    1},
    {"El Cazo",        0}, {"El Mundo",       0}, {"El Apache",      1},
    {"El Venado",      0}, {"El Sol",         1}, {"La Corona",      0},
    {"La Chalina",     0}, {"El Camino",      0}, {"La Bandera",     0},
    {"El Borrego",     0}, {"El Gorrito",     0}, {"La Paloma",      0},
};

// Nombres de patrones (indexados por PAT_*) 
static const char *g_nom_patron[] = {
    "Libre",
    "Diagonal \\",
    "Diagonal /",
    "Fila completa",
    "Columna completa",
    "4 Esquinas",
    "Centro 4 casillas"
};

// Nombres de modos (indexados por MODO_*) 
static const char *g_nom_modo[] = {
    "",
    "Cuadro grande (tabla completa)",
    "Cuadro chico (patron especial)"
};

/* ================================================================
 *  MODULO 3 - ESTADO GLOBAL
 * ================================================================ */
static Jugador g_jugadores[MAX_JUGADORES];
static Partida g_partida;

/* ================================================================
 *  MODULO 4 - UTILIDADES
 * ================================================================ */

void util_limpiar_pantalla(void) // Portable: cls en Windows, clear en Unix
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

void util_pausar(void) // Vacia el buffer y espera ENTER del usuario
{
    printf("\n  Presiona ENTER para continuar...");
    while (getchar() != '\n');
}

int util_leer_entero(int min, int max) // Lee y valida un entero en [min,max]
{
    char buf[16];
    int  val;
    while (1) {
        if (fgets(buf, sizeof(buf), stdin) == NULL) continue; // fgets para evitar bucles infinitos
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max)
            return val;
        printf("  Opcion invalida. Ingresa entre %d y %d: ", min, max);
    }
}

void util_leer_cadena(char *dest, int tam) // Captura una cadena segura con fgets y limpia el salto de linea
{
	if (fgets(dest, tam, stdin) == NULL) {
		dest[0] = '\0';
		return;
	}
	
	dest[strcspn(dest, "\n")] = '\0';
}

// Garantiza que cada partida tenga un orden distinto sin repeticiones.
void util_fisher_yates(int arr[], int n)
{
    for (int i = n - 1; i > 0; i--) {
        int j   = rand() % (i + 1);
        int tmp = arr[i];
        arr[i]  = arr[j];
        arr[j]  = tmp;
    }
}

/* ===============================================================================================
	MODULO 5 - ENTRADA: ASIGNACIÓN DE TABLEROS Y CONFIGURACION DE JUGADORES
================================================================================================== */

void entrada_configurar_jugadores(void) // Solicita cantidad y nombre de cada jugador
{
    printf("\n  Cuantos jugadores? (%d-%d): ", MIN_JUGADORES, MAX_JUGADORES);
    g_partida.num_jugadores = util_leer_entero(MIN_JUGADORES, MAX_JUGADORES);

    for (int k = 0; k < g_partida.num_jugadores; k++) {
        printf("  Nombre del jugador %d: ", k + 1);
        util_leer_cadena(g_jugadores[k].nombre, MAX_NOMBRE_JUG);
        if (g_jugadores[k].nombre[0] == '\0')
            snprintf(g_jugadores[k].nombre, MAX_NOMBRE_JUG, "Jugador %d", k + 1); // Si el nombre queda vacio, asigna jugador k+1
        g_jugadores[k].bloqueado = 0;
    }
}

void entrada_asignar_tableros(void) // Asigna 16 cartas unicas a cada jugador mediante Fisher-Yates parcial independiente.
{
    for (int k = 0; k < g_partida.num_jugadores; k++) {
        int pool[TOTAL_CARTAS];
        for (int i = 0; i < TOTAL_CARTAS; i++) pool[i] = i;

        // Fisher-Yates parcial: barajar las primeras CASILLAS posiciones 
        for (int i = 0; i < CASILLAS; i++) {
            int j   = i + rand() % (TOTAL_CARTAS - i);
            int tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
        }

        int idx = 0;
        for (int r = 0; r < TAM; r++)
            for (int c = 0; c < TAM; c++) {
                g_jugadores[k].asignacion[r][c] = pool[idx++];
                g_jugadores[k].tabla[r][c]      = 0;
            }
    }
}

/* ================================================================
 *  MODULO 6 - PROCESO: BARAJAR
 * ================================================================ */

void proceso_barajar(void) // Mezcla el mazo completo con Fisher-Yates y reinicia los contadores de turno.
{
    for (int i = 0; i < TOTAL_CARTAS; i++)
        g_partida.orden_baraja[i] = i;
    util_fisher_yates(g_partida.orden_baraja, TOTAL_CARTAS);
    g_partida.carta_actual = 0;
    memset(g_partida.ya_cantada, 0, sizeof(g_partida.ya_cantada));
}

/* ================================================================
 *  MAIN - Pruebas del avance
 *
 *  Funciones probadas:
 *    - util_leer_cadena             : nombre por defecto si hay entrada vacia
 *    - util_leer_entero             : solo acepta enteros en rango valido
 *    - util_limpiar_pantalla        : limpia consola de forma portable
 *    - util_pausar                  : espera ENTER antes de continuar
 *    - entrada_configurar_jugadores : solicita y registra cada jugador
 *    - entrada_asignar_tableros     : reparte tableros unicos por jugador
 * ================================================================ */
int main(void)
{
    srand((unsigned int)time(NULL));

    printf("\n  ======================= PRUEBA 1 =========================\n");
    printf("  Solicita jugadores; valida rango (%d-%d) y nombre.\n", MIN_JUGADORES, MAX_JUGADORES);
	printf("  De quedar vacío verifica que la entrada asigne 'Jugador'.\n");
    printf("  ============================================================\n");

    entrada_configurar_jugadores();
	
    printf("\n   Jugadores registrados:\n");
    for (int k = 0; k < g_partida.num_jugadores; k++)
        printf("    %d. '%s'\n", k + 1, g_jugadores[k].nombre);

    printf("\n  ======================= PRUEBA 2 =========================\n");
    printf("  Verifica mezcla del mazo y distribucion unica por jugador.\n");
    printf("  ============================================================\n");

    proceso_barajar();
    entrada_asignar_tableros();

    for (int k = 0; k < g_partida.num_jugadores; k++) {
        printf("  Tablero de %s:\n", g_jugadores[k].nombre);
        printf("  +------------------+------------------+------------------+------------------+\n");
        for (int r = 0; r < TAM; r++) {
            printf("  |");
            for (int c = 0; c < TAM; c++)
                printf("  %-16s|", g_mazo[g_jugadores[k].asignacion[r][c]].nombre);
            printf("\n  +------------------+------------------+------------------+------------------+\n");
        }
        printf("\n");
    }

    printf("  Pruebas completadas.\n");
    util_pausar();
    return 0;
}
