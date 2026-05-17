/* ================================================================
 * Archivo      : Loteria_Final.c
 * Proyecto     : Loteria Magica - Simulacion de la Loteria Mexicana
 * Equipo       : Los Parametrizados
 * Materia      : Programacion Estructurada, 2do Semestre
 * Facultad     : Matematicas - Ingenieria de Software
 * Compilacion  : gcc -std=c99 -Wall -Wextra -o Loteria_Final Loteria_Final.c
 * Fecha        : Mayo 2026
 *
 * Integrantes y modulos principales:
 * Cardos Pilon Sebastian Joshua     - Modulo 5 entrada_, flujo_
 * Gonzalez Cardena Azul Anneliese   - Modulo 4 util_
 * Morales Bautista Kevin Enrique    - Modulo 6 proceso_habilidad, salida_patron
 * Ojeda Pech Juan Jose              - Modulos 0-2 macros, structs, datos
 * Pacheco Cervantes Felipe de Jesus - Modulo 6 proceso_, Modulo 7 salida_, main
 *
 * ESTANDAR DE CODIFICACION
 * ---------------------------------------------------------------
 *  Macros / constantes : UPPER_SNAKE_CASE  (visibilidad inmediata)
 *  Tipos (structs)     : PascalCase        (distingue tipos de vars)
 *  Funciones           : snake_case con prefijo de modulo
 *                        util_    -> operaciones auxiliares generales
 *                        entrada_ -> captura de datos del usuario
 *                        proceso_ -> logica central del juego
 *                        salida_  -> visualizacion y escritura
 *                        flujo_   -> coordinacion del ciclo principal
 *  Variables locales   : snake_case, nombres descriptivos en espanol
 *  Variables globales  : snake_case con prefijo g_
 *  Longitud de funcion : maximo ~50 lineas (cohesion alta)
 *  Indentacion         : 4 espacios, sin tabuladores
 *  Llaves              : misma linea en if/for/while, linea nueva en func
 * ================================================================ */

// Bibliotecas estandar permitidas por RNF-01 y RNF-10
#include <stdio.h>    // printf, scanf, fgets, FILE, fprintf, fopen
#include <stdlib.h>   // srand, rand, exit, memset
#include <string.h>   // strcpy, strncpy, strcspn, memset
#include <time.h>     // time() para semilla aleatoria

/* ================================================================
 *  MODULO 0 - MACROS Y CONSTANTES
 *  RF-01 (config), RF-03 (tableros), RF-05 (mazo), RF-06 (habilidad)
 * ================================================================ */
#define MAX_JUGADORES      8
#define MIN_JUGADORES      2
#define TAM                4          // lado del tablero 4x4
#define CASILLAS          16          // TAM * TAM
#define TOTAL_CARTAS      54
#define MAX_NOMBRE_JUG    24
#define MAX_NOMBRE_CARTA  32
#define HISTORIAL_CSV     "historial.csv"

// Modos de juego (RF-01)
#define MODO_GRANDE        1  // tabla completa
#define MODO_CHICO         2  // patron especial

// Patrones disponibles en cuadro chico (RF-09)
#define PAT_LIBRE          0
#define PAT_DIAG_P         1  // diagonal principal 
#define PAT_DIAG_S         2  // diagonal secundaria 
#define PAT_FILA           3
#define PAT_COLUMNA        4
#define PAT_ESQUINAS       5
#define PAT_CENTRO         6
#define PAT_MAX            6  // ultimo patron valido

// Habilidades dinamicas (RF-08)
#define PROB_HABILIDAD    25  // % de probabilidad de activar
#define TURNOS_BLOQUEO     2  // turnos sin poder marcar

/* ================================================================
 *  MODULO 1 - ESTRUCTURAS  (PascalCase)
 *  Justificacion: PascalCase comunica que es un tipo definido por
 *  el equipo y no un tipo primitivo del lenguaje (RNF-06).
 * ================================================================ */

/*
 * Carta
 * Representa una carta del mazo oficial de la Loteria Mexicana.
 * habilidad = 1 indica que puede activar el efecto de bloqueo (RF-08).
 */
typedef struct {
    char nombre[MAX_NOMBRE_CARTA];
    int  habilidad;
} Carta;

/*
 * Jugador
 * Almacena el estado completo de un participante durante la partida.
 * tabla[r][c] = 0 casilla libre, 1 casilla marcada (RF-07).
 * asignacion[r][c] = indice en g_mazo de la carta en esa casilla (RF-03).
 * bloqueado = turnos restantes sin poder marcar (RF-08).
 */
typedef struct {
    char nombre[MAX_NOMBRE_JUG];
    int  tabla[TAM][TAM];
    int  asignacion[TAM][TAM];
    int  bloqueado;
} Jugador;

/*
 * Partida
 * Contexto completo de una sesion de juego.
 * orden_baraja contiene los indices barajados con Fisher-Yates (RF-05).
 * ya_cantada[i] = 1 si la carta i ya fue mostrada en este turno (RF-14).
 */
typedef struct {
    int  num_jugadores;
    int  modo_juego;
    int  patron_chico;
    int  orden_baraja[TOTAL_CARTAS];
    int  carta_actual;
    int  ya_cantada[TOTAL_CARTAS];
} Partida;

/* ================================================================
 *  MODULO 2 - DATOS: LAS 54 CARTAS  (RF-03, RF-08)
 *  Las cartas con habilidad=1 activan bloqueo de marcaje.
 *  Para agregar una nueva carta especial basta con cambiar su flag.
 * ================================================================ */
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

// Nombres de patrones para mostrar en pantalla (RF-09, RF-15)
static const char *g_nom_patron[] = {
    "Libre",
    "Diagonal \\",
    "Diagonal /",
    "Fila completa",
    "Columna completa",
    "4 Esquinas",
    "Centro 4 casillas"
};

// Nombres de modos indexados por MODO_* (RF-01)
static const char *g_nom_modo[] = {
    "",
    "Cuadro grande (tabla completa)",
    "Cuadro chico (patron especial)"
};

/* ================================================================
 *  MODULO 3 - ESTADO GLOBAL
 *  Prefijo g_ segun estandar para diferenciacion inmediata de alcance.
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
 *  Funciones de soporte general sin logica de negocio.
 *  Atiende: RNF-01 (portabilidad), RNF-09 (robustez), RNF-05 (usabilidad)
 * ================================================================ */

/*
 * util_limpiar_pantalla
 * Proposito : Limpia la consola de forma portable.
 * Entradas  : ninguna.
 * Efecto    : ejecuta cls en Windows o clear en Unix/Mac (RNF-01).
 */
void util_limpiar_pantalla(void)
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

/*
 * util_pausar
 * Proposito : Vacia el buffer de entrada y espera que el usuario presione ENTER.
 * Entradas  : ninguna.
 * Efecto    : muestra el mensaje y detiene la ejecucion hasta recibir '\n'.
 */
void util_pausar(void)
{
    printf("\n  Presiona ENTER para continuar...");
    while (getchar() != '\n');
}

/*
 * util_leer_entero
 * Proposito : Lee y valida un entero en el rango [min, max].
 * Entradas  : min, max (limites inclusivos aceptados).
 * Efecto    : repite la solicitud hasta recibir un valor valido (RNF-09).
 *             Usa fgets + sscanf para evitar problemas de buffer con scanf.
 */
int util_leer_entero(int min, int max)
{
    char buf[16];
    int  val;
    while (1) {
        // Entrada
        if (fgets(buf, sizeof(buf), stdin) == NULL) continue;
        // Proceso: validar que el valor este en el rango permitido
        if (sscanf(buf, "%d", &val) == 1 && val >= min && val <= max)
            return val;
        // Salida
        printf("  Opcion invalida. Ingresa entre %d y %d: ", min, max);
    }
}

/*
 * util_leer_cadena
 * Proposito : Captura una cadena de texto de forma segura con fgets.
 * Entradas  : dest (buffer de destino), tam (tamano del buffer).
 * Efecto    : limpia el salto de linea final; deja dest vacio si falla (RF-13).
 */
void util_leer_cadena(char *dest, int tam)
{
    // Entrada
    if (fgets(dest, tam, stdin) == NULL) {
        dest[0] = '\0';
        return;
    }
    // Proceso: limpiar el salto de linea final
    dest[strcspn(dest, "\n")] = '\0';
}

/*
 * util_fisher_yates
 * Proposito : Baraja un arreglo de enteros con permutacion uniforme sin sesgo.
 * Entradas  : arr (arreglo a barajar), n (tamano del arreglo).
 * Efecto    : modifica arr en sitio; cada llamada produce un orden distinto (RF-05, RNF-04).
 */
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
 *  MODULO 5 - ENTRADA: JUGADORES, MODO Y TABLEROS
 *  Captura y valida todos los datos de configuracion antes de jugar.
 *  Atiende: RF-01, RF-02, RF-04, RF-13
 * ================================================================ */

/*
 * entrada_configurar_jugadores
 * Proposito : Solicita la cantidad de jugadores y el nombre de cada uno.
 * Entradas  : teclado (numero de jugadores, nombres).
 * Efecto    : inicializa g_jugadores[].nombre y .bloqueado; asigna nombre
 *             por defecto si el campo queda vacio (RF-02, RF-13).
 *             Resetea tabla y asignacion para evitar datos de partidas previas.
 */
void entrada_configurar_jugadores(void)
{
    // Entrada
    printf("\n  Cuantos jugadores? (%d-%d): ", MIN_JUGADORES, MAX_JUGADORES);
    g_partida.num_jugadores = util_leer_entero(MIN_JUGADORES, MAX_JUGADORES);

    for (int k = 0; k < g_partida.num_jugadores; k++) {
        // Proceso: resetear estado del jugador para evitar datos de partidas anteriores
        memset(g_jugadores[k].tabla,      0, sizeof(g_jugadores[k].tabla));
        memset(g_jugadores[k].asignacion, 0, sizeof(g_jugadores[k].asignacion));
        g_jugadores[k].bloqueado = 0;

        // Entrada
        printf("  Nombre del jugador %d: ", k + 1);
        util_leer_cadena(g_jugadores[k].nombre, MAX_NOMBRE_JUG);

        // Proceso: si el nombre queda vacio, asignar identificador por defecto (RF-02)
        if (g_jugadores[k].nombre[0] == '\0')
            snprintf(g_jugadores[k].nombre, MAX_NOMBRE_JUG, "Jugador %d", k + 1);
    }
}

/*
 * entrada_configurar_modo
 * Proposito : Permite elegir entre cuadro grande y cuadro chico con su patron.
 * Entradas  : teclado (modo de juego, patron de victoria si aplica).
 * Efecto    : asigna g_partida.modo_juego y g_partida.patron_chico (RF-01).
 */
void entrada_configurar_modo(void)
{
    // Salida: mostrar opciones de modo
    printf("\n  MODO DE JUEGO\n");
    printf("  1. Cuadro grande (llenar toda la tabla)\n");
    printf("  2. Cuadro chico  (patron especial)\n");
    printf("  Modo: ");
    // Entrada
    g_partida.modo_juego   = util_leer_entero(1, 2);
    g_partida.patron_chico = PAT_LIBRE;

    if (g_partida.modo_juego == MODO_CHICO) {
        // Salida: mostrar opciones de patron
        printf("\n  PATRON DE CUADRO CHICO\n");
        printf("  0. Libre (gana el primero en completar cualquier patron)\n");
        printf("  1. Diagonal principal (\\)\n");
        printf("  2. Diagonal secundaria (/)\n");
        printf("  3. Fila completa (cualquiera)\n");
        printf("  4. Columna completa (cualquiera)\n");
        printf("  5. Las 4 esquinas\n");
        printf("  6. Las 4 casillas del centro\n");
        printf("  Patron: ");
        // Entrada
        g_partida.patron_chico = util_leer_entero(0, PAT_MAX);
    }
}

/*
 * entrada_asignar_tableros
 * Proposito : Asigna 16 cartas unicas a cada jugador con Fisher-Yates parcial.
 * Entradas  : g_partida.num_jugadores (ya configurado).
 * Efecto    : llena g_jugadores[k].asignacion y pone tabla en 0 (RF-03, RF-04).
 *             Cada jugador recibe un pool independiente; no se repiten cartas
 *             dentro del mismo tablero (aunque distintos jugadores pueden
 *             compartir cartas, como en la loteria real).
 */
void entrada_asignar_tableros(void)
{
    for (int k = 0; k < g_partida.num_jugadores; k++) {
        int pool[TOTAL_CARTAS];
        for (int i = 0; i < TOTAL_CARTAS; i++) pool[i] = i;

        // Fisher-Yates parcial: solo se necesitan las primeras CASILLAS posiciones
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
 *  Contiene toda la logica central sin ningun printf de interfaz.
 *  Atiende: RF-05, RF-06, RF-07, RF-08, RF-09, RF-10
 * ================================================================ */

/*
 * proceso_barajar
 * Proposito : Mezcla completamente el mazo y reinicia los contadores de turno.
 * Entradas  : ninguna (usa g_partida).
 * Efecto    : ordena g_partida.orden_baraja con Fisher-Yates, pone carta_actual
 *             a 0 y limpia ya_cantada (RF-05, RNF-04).
 */
void proceso_barajar(void)
{
    for (int i = 0; i < TOTAL_CARTAS; i++)
        g_partida.orden_baraja[i] = i;
    util_fisher_yates(g_partida.orden_baraja, TOTAL_CARTAS);
    g_partida.carta_actual = 0;
    memset(g_partida.ya_cantada, 0, sizeof(g_partida.ya_cantada));
}

/*
 * proceso_marcar
 * Proposito : Marca la casilla del jugador si la carta cantada aparece en su tablero.
 * Entradas  : jug_idx (indice del jugador), carta_idx (indice en g_mazo).
 * Efecto    : si el jugador esta bloqueado decrementa el contador y omite el
 *             marcado; de lo contrario marca la casilla correspondiente (RF-07, RF-08).
 */
void proceso_marcar(int jug_idx, int carta_idx)
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

/*
 * proceso_revisar_patron
 * Proposito : Evalua si el tablero t cumple el patron p indicado.
 * Entradas  : t (puntero al tablero 2D del jugador), p (constante PAT_*).
 * Efecto    : devuelve 1 si se cumple, 0 si no; sin efectos secundarios (RF-09).
 *             Para agregar un nuevo patron basta con anadir un case aqui (RNF-07).
 */
int proceso_revisar_patron(int (*t)[TAM], int p)
{
    int i, j;
    switch (p) {
        case PAT_DIAG_P:
            return t[0][0] && t[1][1] && t[2][2] && t[3][3];
        case PAT_DIAG_S:
            return t[0][3] && t[1][2] && t[2][1] && t[3][0];
        case PAT_FILA:
            for (i = 0; i < TAM; i++)
                if (t[i][0] && t[i][1] && t[i][2] && t[i][3]) return 1;
            return 0;
        case PAT_COLUMNA:
            for (j = 0; j < TAM; j++)
                if (t[0][j] && t[1][j] && t[2][j] && t[3][j]) return 1;
            return 0;
        case PAT_ESQUINAS:
            return t[0][0] && t[0][3] && t[3][0] && t[3][3];
        case PAT_CENTRO:
            return t[1][1] && t[1][2] && t[2][1] && t[2][2];
        default: return 0;
    }
}

/*
 * proceso_tabla_llena
 * Proposito : Indica si las 16 casillas del jugador estan marcadas.
 * Entradas  : jug_idx (indice del jugador en g_jugadores).
 * Efecto    : devuelve 1 si gano en modo cuadro grande, 0 si no (RF-09).
 */
int proceso_tabla_llena(int jug_idx)
{
    for (int r = 0; r < TAM; r++)
        for (int c = 0; c < TAM; c++)
            if (!g_jugadores[jug_idx].tabla[r][c]) return 0;
    return 1;
}

/*
 * proceso_patron_chico
 * Proposito : Verifica el patron de cuadro chico configurado para este jugador.
 * Entradas  : jug_idx (indice del jugador).
 * Efecto    : en modo PAT_LIBRE prueba todos los patrones disponibles;
 *             de lo contrario delega en proceso_revisar_patron (RF-09).
 */
int proceso_patron_chico(int jug_idx)
{
    int (*t)[TAM] = g_jugadores[jug_idx].tabla;
    if (g_partida.patron_chico == PAT_LIBRE) {
        for (int p = PAT_DIAG_P; p <= PAT_MAX; p++)
            if (proceso_revisar_patron(t, p)) return 1;
        return 0;
    }
    return proceso_revisar_patron(t, g_partida.patron_chico);
}

/*
 * proceso_verificar_ganador
 * Proposito : Unifica la verificacion de victoria segun el modo activo.
 * Entradas  : jug_idx (indice del jugador).
 * Efecto    : devuelve 1 si el jugador gano, 0 si no (RF-09, RF-10).
 */
int proceso_verificar_ganador(int jug_idx)
{
    return (g_partida.modo_juego == MODO_GRANDE)
               ? proceso_tabla_llena(jug_idx)
               : proceso_patron_chico(jug_idx);
}

/*
 * proceso_nombre_patron_logrado
 * Proposito : Devuelve el nombre del primer patron completado por el jugador.
 * Entradas  : jug_idx (indice del jugador).
 * Efecto    : util en modo PAT_LIBRE para saber que patron logro primero (RF-10).
 */
const char *proceso_nombre_patron_logrado(int jug_idx)
{
    int (*t)[TAM] = g_jugadores[jug_idx].tabla;
    for (int p = PAT_DIAG_P; p <= PAT_MAX; p++)
        if (proceso_revisar_patron(t, p)) return g_nom_patron[p];
    return "?";
}

/*
 * proceso_aplicar_habilidad
 * Proposito : Activa el efecto de bloqueo de marcaje con PROB_HABILIDAD%.
 * Entradas  : carta_idx (indice de la carta recien cantada en g_mazo).
 * Efecto    : si la carta tiene habilidad y la probabilidad se cumple, bloquea
 *             al siguiente jugador en turno durante TURNOS_BLOQUEO (RF-08).
 */
void proceso_aplicar_habilidad(int carta_idx)
{
    if (!g_mazo[carta_idx].habilidad) return;
    if ((rand() % 100) >= PROB_HABILIDAD) return;

    // Identificar quien tiene la carta y bloquear al jugador siguiente
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
 *  MODULO 7 - SALIDA: VISUALIZACION Y ARCHIVOS
 *  Toda presentacion en pantalla y escritura en disco pasa por aqui.
 *  Atiende: RF-06, RF-10, RF-12, RF-14, RF-15, RNF-05, RNF-08
 * ================================================================ */

/*
 * salida_tablero
 * Proposito : Muestra el tablero de un jugador con cartas marcadas resaltadas.
 * Entradas  : jug_idx (indice del jugador).
 * Efecto    : imprime una cuadricula 4x4 con ** en casillas marcadas (RNF-05).
 */
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
            int marc = j->tabla[r][c];
            int idx  = j->asignacion[r][c];
            printf(marc ? "**%-18s|" : "  %-18s|", g_mazo[idx].nombre);
        }
        printf("\n  +--------------------+--------------------+"
               "--------------------+--------------------+\n");
    }
}

/*
 * salida_todos_tableros
 * Proposito : Imprime los tableros de todos los jugadores activos en pantalla.
 * Entradas  : ninguna (usa g_jugadores y g_partida.num_jugadores).
 * Efecto    : llama a salida_tablero por cada jugador (RF-14).
 */
void salida_todos_tableros(void)
{
    printf("\n  ========== TABLEROS ==========\n");
    for (int k = 0; k < g_partida.num_jugadores; k++) {
        salida_tablero(k);
        printf("\n");
    }
}

/*
 * salida_historial_cartas
 * Proposito : Lista las cartas cantadas en la partida actual.
 * Entradas  : ninguna (usa g_partida.ya_cantada).
 * Efecto    : imprime nombres de cartas cantadas en tres columnas (RF-14).
 */
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

/*
 * salida_mostrar_modo
 * Proposito : Muestra el modo de juego y patron activos al inicio de cada turno.
 * Entradas  : ninguna (usa g_partida).
 * Efecto    : imprime una linea descriptiva del estado de la partida (RF-01).
 */
void salida_mostrar_modo(void)
{
    printf("  Modo: %s", g_nom_modo[g_partida.modo_juego]);
    if (g_partida.modo_juego == MODO_CHICO)
        printf(" | Patron: %s", g_nom_patron[g_partida.patron_chico]);
    printf("\n");
}

/*
 * salida_patron_objetivo
 * Proposito : Dibuja en ASCII la mascara del patron elegido antes de jugar.
 * Entradas  : ninguna (usa g_partida.patron_chico).
 * Efecto    : imprime una cuadricula 4x4 con [X] en casillas objetivo (RF-15).
 *             En modo LIBRE indica que cualquier patron es valido.
 */
void salida_patron_objetivo(void)
{
    if (g_partida.patron_chico == PAT_LIBRE) {
        printf("  Objetivo: cualquier patron de los disponibles.\n");
        return;
    }

    int obj[TAM][TAM] = {{0}};
    switch (g_partida.patron_chico) {
        case PAT_DIAG_P:
            for (int i = 0; i < TAM; i++) obj[i][i] = 1;
            break;
        case PAT_DIAG_S:
            for (int i = 0; i < TAM; i++) obj[i][TAM - 1 - i] = 1;
            break;
        case PAT_FILA:
            for (int j = 0; j < TAM; j++) obj[0][j] = 1;
            break;
        case PAT_COLUMNA:
            for (int i = 0; i < TAM; i++) obj[i][0] = 1;
            break;
        case PAT_ESQUINAS:
            obj[0][0] = obj[0][3] = obj[3][0] = obj[3][3] = 1;
            break;
        case PAT_CENTRO:
            obj[1][1] = obj[1][2] = obj[2][1] = obj[2][2] = 1;
            break;
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

/*
 * salida_reglas
 * Proposito : Muestra el texto estatico con las reglas del juego.
 * Entradas  : ninguna.
 * Efecto    : imprime en consola los modos, patrones y cartas especiales (RNF-05).
 */
void salida_reglas(void)
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

/*
 * salida_guardar_resultado
 * Proposito : Agrega una linea CSV al historial de partidas en disco.
 * Entradas  : ganador (nombre o "Empate: A y B"), patron (nombre del patron logrado).
 * Efecto    : abre historial.csv en modo append y escribe ganador,patron,modo,cartas.
 *             Crea el archivo con encabezado si no existe (RF-12, RNF-08).
 *             Un fallo de escritura muestra aviso pero no interrumpe el juego.
 */
void salida_guardar_resultado(const char *ganador, const char *patron)
{
    // Verificar si el archivo ya existe para escribir el encabezado solo una vez
    int archivo_nuevo = 0;
    FILE *prueba = fopen(HISTORIAL_CSV, "r");
    if (!prueba) {
        archivo_nuevo = 1;
    } else {
        fclose(prueba);
    }

    FILE *f = fopen(HISTORIAL_CSV, "a");
    if (!f) {
        printf("  [!] No se pudo guardar el historial.\n");
        return;
    }

    if (archivo_nuevo)
        fprintf(f, "Ganador,Patron,Modo,Cartas\n");

    fprintf(f, "%s,%s,%s,%d cartas\n",
            ganador, patron,
            g_nom_modo[g_partida.modo_juego],
            g_partida.carta_actual);
    fclose(f);
}

/*
 * salida_historial_partidas
 * Proposito : Lee e imprime el archivo CSV con el historial de partidas.
 * Entradas  : ninguna.
 * Efecto    : muestra cada registro en consola con formato de columnas (RF-12).
 *             Si el archivo no existe o esta vacio, informa al usuario.
 */
void salida_historial_partidas(void)
{
    util_limpiar_pantalla();
    printf("\n  HISTORIAL DE PARTIDAS\n");
    printf("  %-20s %-20s %-28s %s\n",
           "Ganador", "Patron", "Modo", "Cartas");
    printf("  -------------------------------------------------------------------\n");

    FILE *f = fopen(HISTORIAL_CSV, "r");
    if (!f) {
        printf("  No hay partidas registradas aun.\n");
        printf("  -------------------------------------------------------------------\n");
        return;
    }

    char linea[128];
    int  n = 0;
    while (fgets(linea, sizeof(linea), f)) {
        char gan[32], pat[32], mod[36], car[16];
        // Saltar la linea de encabezado del CSV
        if (n == 0 && linea[0] == 'G') { n = -1; continue; }
        if (sscanf(linea, "%31[^,],%31[^,],%35[^,],%15[^\n]",
                   gan, pat, mod, car) == 4) {
            printf("  %-20s %-20s %-28s %s\n", gan, pat, mod, car);
            n++;
        }
    }
    fclose(f);
    if (n <= 0) printf("  No hay partidas registradas aun.\n");
    printf("  -------------------------------------------------------------------\n");
}

/* ================================================================
 *  MODULO 8 - FLUJO: CICLO DE JUEGO
 *  Coordina todas las etapas de una partida: configuracion,
 *  cantado, marcado, verificacion y declaracion de resultado.
 *  Atiende: RF-06, RF-10, RF-11, RF-14
 * ================================================================ */

/*
 * flujo_jugar
 * Proposito : Ciclo principal de una partida; coordina entrada/proceso/salida por turno.
 * Entradas  : g_partida y g_jugadores ya configurados por entrada_configurar_*.
 * Efecto    : ejecuta el juego completo hasta que hay ganador, empate o se agota el mazo.
 *             Guarda el resultado en historial al terminar (RF-10, RF-12).
 */
void flujo_jugar(void)
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

        // Salida: mostrar menu de turno
        printf("\n  [1] Sacar siguiente carta\n");
        printf("  [2] Ver todos los tableros\n");
        printf("  [3] Ver cartas cantadas\n");
        printf("  [4] Salir de la partida\n");
        printf("  ------------------------------------------------------------\n");
        printf("  Opcion: ");

        // Entrada
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

        // op == 1: Sacar carta (RF-06)
        // Proceso: obtener la siguiente carta del mazo barajado
        int idx = g_partida.orden_baraja[g_partida.carta_actual];
        g_partida.ya_cantada[idx] = 1;
        g_partida.carta_actual++;

        // Salida: mostrar la carta cantada
        printf("\n  +================================+\n");
        printf("  |  #%-3d  %-24s|\n", idx + 1, g_mazo[idx].nombre);
        printf("  +================================+\n\n");

        // Proceso: aplicar habilidad antes del marcado (RF-08)
        proceso_aplicar_habilidad(idx);

        // Proceso: marcar tableros de todos los jugadores (RF-07)
        for (int k = 0; k < g_partida.num_jugadores; k++)
            proceso_marcar(k, idx);

        // Salida: mostrar estado actualizado de los tableros
        salida_todos_tableros();

        // Proceso: verificar ganadores y detectar empate (RF-09, RF-10)
        int hay_ganador = 0;
        int ganadores[MAX_JUGADORES];
        int num_ganadores = 0;

        for (int k = 0; k < g_partida.num_jugadores; k++) {
            if (proceso_verificar_ganador(k))
                ganadores[num_ganadores++] = k;
        }

        if (num_ganadores == 1) {
            // Victoria individual
            int k = ganadores[0];
            const char *pat_nom;
            if (g_partida.modo_juego == MODO_GRANDE)
                pat_nom = "Tabla completa";
            else if (g_partida.patron_chico == PAT_LIBRE)
                pat_nom = proceso_nombre_patron_logrado(k);
            else
                pat_nom = g_nom_patron[g_partida.patron_chico];

            // Salida: anunciar ganador individual
            printf("\n  !!! LOTERIA !!! - GANO: %s", g_jugadores[k].nombre);
            if (g_partida.modo_juego == MODO_CHICO)
                printf("  (patron: %s)", pat_nom);
            printf(" !!!\n\n");

            salida_guardar_resultado(g_jugadores[k].nombre, pat_nom);
            hay_ganador = 1;

        } else if (num_ganadores > 1) {
            // Empate: construir cadena con los nombres de los ganadores
            char nombre_empate[64] = "Empate: ";
            char patron_empate[32] = "";
            for (int e = 0; e < num_ganadores; e++) {
                int k = ganadores[e];
                if (e > 0) strncat(nombre_empate, " y ",
                                   sizeof(nombre_empate) - strlen(nombre_empate) - 1);
                strncat(nombre_empate, g_jugadores[k].nombre,
                        sizeof(nombre_empate) - strlen(nombre_empate) - 1);
            }
            // Obtener nombre del patron del primer ganador
            if (g_partida.modo_juego == MODO_GRANDE) {
                strncpy(patron_empate, "Tabla completa", sizeof(patron_empate) - 1);
            } else if (g_partida.patron_chico == PAT_LIBRE) {
                strncpy(patron_empate,
                        proceso_nombre_patron_logrado(ganadores[0]),
                        sizeof(patron_empate) - 1);
            } else {
                strncpy(patron_empate,
                        g_nom_patron[g_partida.patron_chico],
                        sizeof(patron_empate) - 1);
            }

            // Salida: anunciar empate
            printf("\n  !!! EMPATE !!! - %s completaron el patron en el mismo turno !!!\n\n",
                   nombre_empate);
            salida_guardar_resultado(nombre_empate, patron_empate);
            hay_ganador = 1;
        }

        if (!hay_ganador)
            printf("  Ningun ganador todavia. Siguen jugando...\n");

        util_pausar();
        if (hay_ganador) return;
    }

    // Mazo agotado sin ganador: registrar sin ganador con modo correcto (RF-10)
    printf("\n  Se acabaron las cartas. Nadie gano esta ronda.\n");
    const char *pat_sin_ganador = (g_partida.modo_juego == MODO_GRANDE)
                                  ? "Tabla completa"
                                  : g_nom_patron[g_partida.patron_chico];
    salida_guardar_resultado("Sin ganador", pat_sin_ganador);
    util_pausar();
}

/*
 * flujo_menu_principal
 * Proposito : Menu de inicio; punto de entrada de la aplicacion.
 * Entradas  : ninguna.
 * Efecto    : muestra las opciones principales y despacha al caso seleccionado
 *             en bucle hasta que el usuario elige Salir (RF-01, RF-09).
 */
void flujo_menu_principal(void)
{
    int op = 0;
    while (op != 5) {
        util_limpiar_pantalla();
        // Salida: mostrar menu principal
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

        // Entrada
        op = util_leer_entero(1, 5);

        // Proceso: despachar segun la opcion elegida
        switch (op) {
            case 1:
                util_limpiar_pantalla();
                entrada_configurar_jugadores();
                entrada_configurar_modo();
                flujo_jugar();
                break;
            case 2:
                // Salida
                salida_reglas();
                util_pausar();
                break;
            case 3:
                // Salida
                salida_historial_partidas();
                util_pausar();
                break;
            case 4:
                util_limpiar_pantalla();
                // Salida
                printf("\n  Autores  : Cardos, Gonzalez, Morales, Ojeda, Pacheco\n");
                printf("  Equipo   : Los Parametrizados\n");
                printf("  Materia  : Programacion Estructurada, 2do Semestre\n");
                printf("  Repo     : github.com/azulgonzc/LISPE_LosParametrizados\n");
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

/*
 * main
 * Proposito : Punto de entrada del programa; inicializa la semilla aleatoria
 *             y lanza el menu principal.
 * Entradas  : ninguna.
 * Efecto    : srand con time(NULL) garantiza distinto orden en cada ejecucion
 *             (RNF-04); devuelve 0 al sistema operativo al terminar.
 */
int main(void)
{
    srand((unsigned int)time(NULL));
    flujo_menu_principal();
    return 0;
}
