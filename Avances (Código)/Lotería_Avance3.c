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
    "Libre", "Diagonal \\", "Diagonal /",
    "Fila completa", "Columna completa", "4 Esquinas", "Centro 4 casillas"
};

// Nombres de modos (indexados por MODO_*) 
static const char *g_nom_modo[] = {
    "", "Cuadro grande (tabla completa)", "Cuadro chico (patron especial)"
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

/* ================================================================
 *  MODULO 5 - ENTRADA: TABLEROS, JUGADORES Y CONFIGURACION
 * ================================================================ */

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

void entrada_configurar_modo(void) // Elige modo grande/chico y patron deseado.
{
    printf("\n  MODO DE JUEGO\n");
    printf("  1. Cuadro grande (llenar toda la tabla)\n"); // El sistema juega de forma automática turno a turno
    printf("  2. Cuadro chico  (patron especial)\n"); // Simula el modo clasico con patron especifico
    printf("  Modo: ");
    g_partida.modo_juego   = util_leer_entero(1, 2);
    g_partida.patron_chico = PAT_LIBRE;

    if (g_partida.modo_juego == MODO_CHICO) {
        printf("\n  PATRON DE CUADRO CHICO\n");
        printf("  0. Libre (gana el que complete cualquier patron primero)\n");
        printf("  1. Diagonal principal (\\)\n");
        printf("  2. Diagonal secundaria (/)\n");
        printf("  3. Fila completa (cualquiera)\n");
        printf("  4. Columna completa (cualquiera)\n");
        printf("  5. Las 4 esquinas\n");
        printf("  6. Las 4 casillas del centro\n");
        printf("  Patron: ");
        g_partida.patron_chico = util_leer_entero(0, PAT_MAX);
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
 *  MODULO 6 - PROCESO: BARAJAR Y MARCAR
 * ================================================================ */

void proceso_barajar(void) // Mezcla completa del mazo y reset de contadores
{
    for (int i = 0; i < TOTAL_CARTAS; i++)
        g_partida.orden_baraja[i] = i;
    util_fisher_yates(g_partida.orden_baraja, TOTAL_CARTAS);
    g_partida.carta_actual = 0;
    memset(g_partida.ya_cantada, 0, sizeof(g_partida.ya_cantada));
}

void proceso_marcar(int jug_idx, int carta_idx) // Marca la casilla si la carta cantada esta en el tablero
{
    if (g_jugadores[jug_idx].bloqueado > 0) {
        g_jugadores[jug_idx].bloqueado--;
        printf("  [Bloqueo] %s no puede marcar. Turnos restantes: %d\n",
               g_jugadores[jug_idx].nombre,
               g_jugadores[jug_idx].bloqueado);
        return;
    }
    for (int r = 0; r < TAM; r++)
        for (int c = 0; c < TAM; c++)
            if (g_jugadores[jug_idx].asignacion[r][c] == carta_idx)
                g_jugadores[jug_idx].tabla[r][c] = 1;
}

/* ================================================================
 *  MODULO 7 - SALIDA: VISUALIZACION BASICA
 * ================================================================ */

// Muestra el tablero de un jugador con cartas marcadas 
void salida_tablero(int jug_idx)
{
    Jugador *j = &g_jugadores[jug_idx];
    printf("  Tablero de %s%s:\n",
           j->nombre, j->bloqueado ? " [BLOQUEADO]" : "");
    printf("  +--------------------+--------------------+"
           "--------------------+--------------------+\n");
    for (int r = 0; r < TAM; r++) {
        printf("  |");
        for (int c = 0; c < TAM; c++) {
            int idx  = j->asignacion[r][c];
            int marc = j->tabla[r][c];
            printf(marc ? "**%-18s|" : "  %-18s|", g_mazo[idx].nombre);
        }
        printf("\n  +--------------------+--------------------+"
               "--------------------+--------------------+\n");
    }
}

void salida_todos_tableros(void)
{
    printf("\n  ========== TABLEROS ==========\n");
    for (int k = 0; k < g_partida.num_jugadores; k++) {
        salida_tablero(k);
        printf("\n");
    }
}

void salida_historial_cartas(void)
{
    printf("\n  --- Cartas cantadas (%d / %d) ---\n",
           g_partida.carta_actual, TOTAL_CARTAS);
    int n = 0;
    for (int i = 0; i < TOTAL_CARTAS; i++) {
        if (g_partida.ya_cantada[i]) {
            printf("  %-22s", g_mazo[i].nombre);
            if (++n % 3 == 0) printf("\n");
        }
    }
    if (n % 3 != 0) printf("\n");
}

// Muestra el modo activo al inicio de cada turno
void salida_mostrar_modo(void)
{
    printf("  Modo: %s", g_nom_modo[g_partida.modo_juego]);
    if (g_partida.modo_juego == MODO_CHICO)
        printf(" | Patron: %s", g_nom_patron[g_partida.patron_chico]);
    printf("\n");
}

/* ================================================================
 *  MODULO 8 - FLUJO: BUCLE DE JUEGO (parcial - sin habilidades aun)
 * ================================================================ */

// Menu de inicio, punto de entrada de la aplicacion.
void flujo_menu_principal(void)
{
    int op = 0;
    while (op != 5) {
        util_limpiar_pantalla();
        printf("  ============================================================\n");
        printf("           LOTERIA MAGICA  -  Los Parametrizados\n");
        printf("  ============================================================\n");
        printf("  [1] Nueva partida\n");
        printf("  [2] Ver reglas\n");
        printf("  [3] Historial de partidas\n");
        printf("  [4] Creditos\n");
        printf("  [5] Salir\n");
        printf("  ------------------------------------------------------------\n");
        printf("  Opcion: ");

        op = util_leer_entero(1, 5);

        switch (op) {
            case 1:
                util_limpiar_pantalla();
                entrada_configurar_jugadores();
                entrada_configurar_modo();
                // flujo_jugar() se completara en siguiente iteracion
                break;
            case 4:
                util_limpiar_pantalla();
                printf("\n  Autores : Cardos, Gonzalez, Morales, Ojeda, Pacheco\n");
                printf("  Equipo  : Los Parametrizados\n");
                printf("  Materia : Programacion Estructurada, 2do Semestre\n");
                util_pausar();
                break;
            case 5:
                printf("\n  !Hasta luego!\n\n");
                break;
            default: break;
        }
    }
}

/* ================================================================
 *  MAIN - Pruebas del avance
 *
 *  Funciones probadas:
 *    - entrada_configurar_modo  : seleccion de modo grande/chico
 *    - salida_mostrar_modo      : imprime el modo activo en pantalla
 *    - salida_tablero           : muestra el tablero de un jugador
 *    - salida_todos_tableros    : muestra todos los tableros a la vez
 *    - salida_historial_cartas  : lista las cartas cantadas hasta ahora
 *    - proceso_marcar           : marca casilla si la carta esta en el tablero
 *    - flujo_menu_principal     : menu principal con opciones de navegacion
 * ================================================================ */
int main(void)
{
    srand((unsigned int)time(NULL));

    printf("  PRUEBA 1 ===================================================\n");
    printf("  Verifica seleccion de modo y que se muestre correctamente.\n");
    printf("  ============================================================\n");

    g_partida.num_jugadores = 2;
    strncpy(g_jugadores[0].nombre, "Jugador 1", MAX_NOMBRE_JUG - 1);
    strncpy(g_jugadores[1].nombre, "Jugador 2", MAX_NOMBRE_JUG - 1);
    g_jugadores[0].bloqueado = 0;
    g_jugadores[1].bloqueado = 0;

    entrada_configurar_modo();
    printf("  Modo configurado: ");
    salida_mostrar_modo();

    printf("\n  PRUEBA 2 =================================================\n");
    printf("  Asigna tableros, saca 5 cartas y verifica que se marcan.\n");
    printf("  ============================================================\n");

    proceso_barajar();
    entrada_asignar_tableros();

    printf("  Tableros iniciales (ninguna casilla marcada):\n");
    salida_todos_tableros();

    printf("  Sacando y marcando 5 cartas...\n\n");
    for (int t = 0; t < 5; t++) {
        int idx = g_partida.orden_baraja[g_partida.carta_actual];
        g_partida.ya_cantada[idx] = 1;
        g_partida.carta_actual++;
        printf("  Carta %d: %s\n", t + 1, g_mazo[idx].nombre);
        for (int k = 0; k < g_partida.num_jugadores; k++)
            proceso_marcar(k, idx);
    }

    printf("\n  Tableros despues de marcar 5 cartas:\n");
    salida_todos_tableros();

    printf("\n  PRUEBA 3 =================================================\n");
    printf("  Lista las cartas cantadas hasta este momento.\n");
    printf("  ============================================================\n");

    salida_historial_cartas();

    printf("\n  PRUEBA 4 =================================================\n");
    printf("  Lanza el menu principal interactivo completo.\n");
    printf("  ============================================================\n");

    util_pausar();
    flujo_menu_principal();
    return 0;
}
