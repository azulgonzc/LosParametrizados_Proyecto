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
#define PAT_DIAG_P         1     // Diagonal principal  
#define PAT_DIAG_S         2     // Diagonal secundaria 
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

// Nombres de patrones para seleccion previa
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
 *  PROTOTIPOS
 * ================================================================ */

// util
void        util_limpiar_pantalla(void);
void        util_pausar(void);
int         util_leer_entero(int min, int max);
void        util_leer_cadena(char *dest, int tam);
void        util_fisher_yates(int arr[], int n);

// entrada
void        entrada_configurar_jugadores(void);
void        entrada_configurar_modo(void);
void        entrada_asignar_tableros(void);

// proceso
void        proceso_barajar(void);
void        proceso_marcar(int jug_idx, int carta_idx);
int         proceso_revisar_patron(int (*t)[TAM], int p);
int         proceso_tabla_llena(int jug_idx);
int         proceso_patron_chico(int jug_idx);
int         proceso_verificar_ganador(int jug_idx);
const char *proceso_nombre_patron_logrado(int jug_idx);
void        proceso_aplicar_habilidad(int carta_idx);

// salida
void        salida_tablero(int jug_idx);
void        salida_todos_tableros(void);
void        salida_historial_cartas(void);
void        salida_mostrar_modo(void);
void        salida_patron_objetivo(void);
void        salida_reglas(void);
void        salida_guardar_resultado(const char *ganador, const char *patron);
void        salida_historial_partidas(void);

// flujo
void        flujo_jugar(void);
void        flujo_menu_principal(void);

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

int util_leer_entero(int min, int max) // Lee y valida un entero en [min, max]
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

void util_fisher_yates(int arr[], int n) // Garantiza que cada partida tenga un orden distinto sin repeticiones
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

void entrada_configurar_modo(void) // Elige modo grande/chico y patron deseado
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

void entrada_asignar_tableros(void) // Asigna 16 cartas unicas a cada jugador mediante Fisher-Yates parcial independiente
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
 *  MODULO 6 - PROCESO: LOGICA DE JUEGO
 * ================================================================ */

void proceso_barajar(void) // Mezcla completa del mazo y reset de contadores
{
    for (int i = 0; i < TOTAL_CARTAS; i++)
        g_partida.orden_baraja[i] = i;
    util_fisher_yates(g_partida.orden_baraja, TOTAL_CARTAS);
    g_partida.carta_actual = 0;
    memset(g_partida.ya_cantada, 0, sizeof(g_partida.ya_cantada));
}

void proceso_marcar(int jug_idx, int carta_idx) // Marca la casilla si la carta cantada esta en el tablero; respeta bloqueo activo
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

int proceso_revisar_patron(int (*t)[TAM], int p) // Evalua si el tablero t cumple el patron p; sin efectos secundarios
{
    switch (p) {
        case PAT_DIAG_P:
            return t[0][0] && t[1][1] && t[2][2] && t[3][3];
        case PAT_DIAG_S:
            return t[0][3] && t[1][2] && t[2][1] && t[3][0];
        case PAT_FILA:
            for (int i = 0; i < TAM; i++)
                if (t[i][0] && t[i][1] && t[i][2] && t[i][3]) return 1;
            return 0;
        case PAT_COLUMNA:
            for (int j = 0; j < TAM; j++)
                if (t[0][j] && t[1][j] && t[2][j] && t[3][j]) return 1;
            return 0;
        case PAT_ESQUINAS:
            return t[0][0] && t[0][3] && t[3][0] && t[3][3];
        case PAT_CENTRO:
            return t[1][1] && t[1][2] && t[2][1] && t[2][2];
        default: return 0;
    }
}

int proceso_tabla_llena(int jug_idx) // Devuelve 1 si las 16 casillas estan marcadas
{
    for (int r = 0; r < TAM; r++)
        for (int c = 0; c < TAM; c++)
            if (!g_jugadores[jug_idx].tabla[r][c]) return 0;
    return 1;
}

int proceso_patron_chico(int jug_idx) // Delega en revisar_patron segun configuracion; PAT_LIBRE comprueba todos
{
    int (*t)[TAM] = g_jugadores[jug_idx].tabla;
    if (g_partida.patron_chico == PAT_LIBRE) {
        for (int p = PAT_DIAG_P; p <= PAT_MAX; p++)
            if (proceso_revisar_patron(t, p)) return 1;
        return 0;
    }
    return proceso_revisar_patron(t, g_partida.patron_chico);
}

int proceso_verificar_ganador(int jug_idx) // Unifica la verificacion segun el modo activo
{
    return (g_partida.modo_juego == MODO_GRANDE)
               ? proceso_tabla_llena(jug_idx)
               : proceso_patron_chico(jug_idx);
}

const char *proceso_nombre_patron_logrado(int jug_idx) // Devuelve que patron completo el jugador; util en modo LIBRE
{
    int (*t)[TAM] = g_jugadores[jug_idx].tabla;
    for (int p = PAT_DIAG_P; p <= PAT_MAX; p++)
        if (proceso_revisar_patron(t, p)) return g_nom_patron[p];
    return "?";
}

void proceso_aplicar_habilidad(int carta_idx) // Activa el efecto de bloqueo con PROB_HABILIDAD%.
{
    if (!g_mazo[carta_idx].habilidad) return;
    if ((rand() % 100) >= PROB_HABILIDAD) return;

    // Buscar quien tiene la carta y bloquea al siguiente jugador
    for (int k = 0; k < g_partida.num_jugadores; k++) {
        for (int r = 0; r < TAM; r++) {
            for (int c = 0; c < TAM; c++) {
                if (g_jugadores[k].asignacion[r][c] != carta_idx) continue;
                int obj = (k + 1) % g_partida.num_jugadores;
                g_jugadores[obj].bloqueado = TURNOS_BLOQUEO;
                printf("\n  *** HABILIDAD: '%s' activa Bloqueo de Marcaje! "
                       "%s no marca por %d turnos. ***\n\n",
                       g_mazo[carta_idx].nombre,
                       g_jugadores[obj].nombre,
                       TURNOS_BLOQUEO);
                return;
            }
        }
    }
}

/* ================================================================
 *  MODULO 7 - SALIDA: VISUALIZACION
 * ================================================================ */

void salida_tablero(int jug_idx) // Muestra el tablero de un jugador con cartas marcadas
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

void salida_todos_tableros(void) // Imprime los tableros de todos los jugadores activos
{
    printf("\n  ========== TABLEROS ==========\n");
    for (int k = 0; k < g_partida.num_jugadores; k++) {
        salida_tablero(k);
        printf("\n");
    }
}

void salida_historial_cartas(void) // Lista las cartas cantadas en la partida actual
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

void salida_mostrar_modo(void) // Muestra el modo activo al inicio de cada turno
{
    printf("  Modo: %s", g_nom_modo[g_partida.modo_juego]);
    if (g_partida.modo_juego == MODO_CHICO)
        printf(" | Patron: %s", g_nom_patron[g_partida.patron_chico]);
    printf("\n");
}

void salida_patron_objetivo(void) // Dibuja la mascara ASCII del patron elegido
{
    if (g_partida.patron_chico == PAT_LIBRE) {
        printf("  Objetivo: cualquier patron de los disponibles.\n");
        return;
    }

    int obj[TAM][TAM] = {{0}};
    switch (g_partida.patron_chico) {
        case PAT_DIAG_P:
            for (int i = 0; i < TAM; i++) { obj[i][i] = 1; } break;
        case PAT_DIAG_S:
            for (int i = 0; i < TAM; i++) { obj[i][TAM-1-i] = 1; } break;
        case PAT_FILA:
            for (int j = 0; j < TAM; j++) { obj[0][j] = 1; } break;
        case PAT_COLUMNA:
            for (int i = 0; i < TAM; i++) { obj[i][0] = 1; } break;
        case PAT_ESQUINAS:
            obj[0][0]=obj[0][3]=obj[3][0]=obj[3][3]=1; break;
        case PAT_CENTRO:
            obj[1][1]=obj[1][2]=obj[2][1]=obj[2][2]=1; break;
        default: break;
    }

    printf("  Patron objetivo:\n");
    for (int r = 0; r < TAM; r++) {
        printf("  ");
        for (int c = 0; c < TAM; c++)
            printf(obj[r][c] ? "[X]" : "[ ]");
        printf("\n");
    }
    if (g_partida.patron_chico == PAT_FILA)
        printf("  (cualquier fila completa)\n");
    if (g_partida.patron_chico == PAT_COLUMNA)
        printf("  (cualquier columna completa)\n");
}

void salida_reglas(void) // Texto estatico con las reglas del juego.
{
    util_limpiar_pantalla();
    printf("\n  REGLAS DE LOTERIA MAGICA\n");
    printf("  -----------------------------------------------------\n");
    printf("  * Cada jugador recibe una tabla 4x4 con 16 cartas.\n");
    printf("  * Se sacan cartas del mazo (%d en total).\n", TOTAL_CARTAS);
    printf("  * Si la carta esta en tu tabla se marca automaticamente.\n");
    printf("  * Jugadores: de %d a %d.\n\n", MIN_JUGADORES, MAX_JUGADORES);
    printf("  MODOS:\n");
    printf("  * Cuadro grande : llenar toda la tabla (16 casillas).\n");
    printf("  * Cuadro chico  : completar un patron de 4 casillas.\n");
    printf("    Patrones: Diagonal \\  Diagonal /  Fila  Columna\n");
    printf("              4 Esquinas  4 del Centro  Libre\n\n");
    printf("  CARTAS ESPECIALES (modo dinamico):\n");
    printf("  * %d%% de probabilidad de activar Bloqueo de Marcaje:\n",
           PROB_HABILIDAD);
    printf("    el siguiente jugador no puede marcar por %d turnos.\n",
           TURNOS_BLOQUEO);
    printf("  -----------------------------------------------------\n");
}

void salida_guardar_resultado(const char *ganador, const char *patron) // Agrega una linea CSV al historial de partidas
{
    FILE *f = fopen(HISTORIAL_CSV, "a");
    if (!f) {
        printf("  [!] No se pudo guardar el historial.\n");
        return;
    }
    fprintf(f, "%s,%s,%s,%d cartas\n",
            ganador, patron,
            g_nom_modo[g_partida.modo_juego],
            g_partida.carta_actual);
    fclose(f);
}

void salida_historial_partidas(void) // Lee e imprime el archivo CSV de resultados
{
    util_limpiar_pantalla();
    printf("\n  HISTORIAL DE PARTIDAS\n");
    printf("  %-18s %-18s %-22s %s\n",
           "Ganador", "Patron", "Modo", "Cartas");
    printf("  -----------------------------------------------------\n");

    FILE *f = fopen(HISTORIAL_CSV, "r");
    if (!f) {
        printf("  No hay partidas registradas aun.\n");
        printf("  -----------------------------------------------------\n");
        return;
    }

    char linea[128];
    int  n = 0;
    while (fgets(linea, sizeof(linea), f)) {
        char gan[32], pat[32], mod[36], car[16];
        if (sscanf(linea, "%31[^,],%31[^,],%35[^,],%15[^\n]",
                   gan, pat, mod, car) == 4) {
            printf("  %-18s %-18s %-22s %s\n", gan, pat, mod, car);
            n++;
        }
    }
    fclose(f);
    if (n == 0) printf("  No hay partidas registradas aun.\n");
    printf("  -----------------------------------------------------\n");
}

/* ================================================================
 *  MODULO 8 - FLUJO: BUCLE DE JUEGO
 * ================================================================ */

void flujo_jugar(void) // Ciclo principal de una partida; coordina entrada/proceso/salida por turno
{
    proceso_barajar();
    entrada_asignar_tableros();

    printf("\n  !Que empiece la Loteria!\n");
    salida_mostrar_modo();
    if (g_partida.modo_juego == MODO_CHICO)
        salida_patron_objetivo();
    util_pausar();

    while (g_partida.carta_actual < TOTAL_CARTAS) {
        util_limpiar_pantalla();

        printf("  ============================================================\n");
        printf("  LOTERIA MAGICA                    Carta %d / %d\n",
               g_partida.carta_actual, TOTAL_CARTAS);
        printf("  ============================================================\n");
        salida_mostrar_modo();

        printf("\n  [1] Sacar siguiente carta\n");
        printf("  [2] Ver todos los tableros\n");
        printf("  [3] Ver cartas cantadas\n");
        printf("  [4] Salir de la partida\n");
        printf("  ------------------------------------------------------------\n");
        printf("  Opcion: ");

        int op = util_leer_entero(1, 4);

        if (op == 2) {
            util_limpiar_pantalla();
            salida_todos_tableros();
            util_pausar();
            continue;
        }
        if (op == 3) {
            salida_historial_cartas();
            util_pausar();
            continue;
        }
        if (op == 4) {
            printf("  Saliendo de la partida...\n");
            util_pausar();
            return;
        }

        // op == 1: Sacar carta
        int idx = g_partida.orden_baraja[g_partida.carta_actual];
        g_partida.ya_cantada[idx] = 1;
        g_partida.carta_actual++;

        printf("\n  +================================+\n");
        printf("  |  #%-3d  %-24s|\n", idx + 1, g_mazo[idx].nombre);
        printf("  +================================+\n\n");

        proceso_aplicar_habilidad(idx); // Habilidad dinamica (antes del marcado)

        // Marcar tableros de todos los jugadores
        for (int k = 0; k < g_partida.num_jugadores; k++)
            proceso_marcar(k, idx);

        salida_todos_tableros();

        // Verificar ganadores
        int hay_ganador = 0;
        for (int k = 0; k < g_partida.num_jugadores; k++) {
            if (!proceso_verificar_ganador(k)) continue;

            const char *pat_nom;
            if (g_partida.modo_juego == MODO_GRANDE)
                pat_nom = "Tabla completa";
            else if (g_partida.patron_chico == PAT_LIBRE)
                pat_nom = proceso_nombre_patron_logrado(k);
            else
                pat_nom = g_nom_patron[g_partida.patron_chico];

            printf("\n  !!! LOTERIA !!! - GANO: %s", g_jugadores[k].nombre);
            if (g_partida.modo_juego == MODO_CHICO)
                printf("  (patron: %s)", pat_nom);
            printf(" !!!\n\n");

            salida_guardar_resultado(g_jugadores[k].nombre, pat_nom);
            hay_ganador = 1;
        }

        if (!hay_ganador)
            printf("  Ningun ganador todavia. Siguen jugando...\n");

        util_pausar();
        if (hay_ganador) return;
    }

    // Mazo agotado sin ganador
    printf("\n  Se acabaron las cartas. Nadie gano esta ronda.\n");
    salida_guardar_resultado("Sin ganador",
                             g_nom_patron[g_partida.patron_chico]);
    util_pausar();
}

void flujo_menu_principal(void) // Menu de inicio, punto de entrada de la aplicacion
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
                flujo_jugar();
                break;
            case 2:
                salida_reglas();
                util_pausar();
                break;
            case 3:
                salida_historial_partidas();
                util_pausar();
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
 *  MODULO 9 - MAIN
 * ================================================================ */

int main(void) // Inicializa y lanza el menu principal
{
    srand((unsigned int)time(NULL));
    flujo_menu_principal();
    return 0;
}
