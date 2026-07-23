// main.cpp

#include <iostream>
#include <iomanip>
#include <fstream>   
#include <sstream>

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <chrono>

#include "analizador_lexico.h"
#include "analizador_sintactico.h"
#include "analizador_semantico.h"


using json = nlohmann::json;
using namespace std;

// ─────────────────────────────────────────────
// Colores ANSI
// ─────────────────────────────────────────────
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"

void imprimirSeparador(const string& titulo) {
    cout << "\n";
    cout << CYAN << BOLD;
    cout << "========================================\n";
    cout << " " << titulo << "\n";
    cout << "========================================\n";
    cout << RESET;
}

void mostrarTokens(BufferTokens buffer) {
    cout << GREEN << "TOKENS GENERADOS:\n\n" << RESET;

    while (buffer.hayMas()) {

        Token t = buffer.siguiente();

        // ignorar espacios visualmente
        if (t.tipo == TipoToken::ESPACIO ||
            t.tipo == TipoToken::SALTO_LINEA)
            continue;

        cout << left << setw(28)
             << ("[" + nombreTipo(t.tipo) + "]");

        cout << " original=\""
             << t.valorOriginal << "\"";

        cout << " normal=\""
             << t.valorNormal << "\"";

        cout << " (L"
             << t.linea
             << ":C"
             << t.columna
             << ")";

        if (t.inicioOracion)
            cout << "  [INICIO_ORACION]";

        if (t.inicioLinea)
            cout << "  [INICIO_LINEA]";

        if (t.inicioParrafo)
            cout << "  [INICIO_PARRAFO]";

        cout << "\n";
    }
}

void mostrarErroresLexicos(const AnalizadorLexico& lexer) {

    if (!lexer.tieneErrores())
        return;

    cout << RED << BOLD;
    cout << "\nERRORES LEXICOS:\n";
    cout << RESET;

    for (const auto& e : lexer.obtenerErrores()) {

        cout << RED
             << "  [Linea "
             << e.linea
             << ", Col "
             << e.columna
             << "] "
             << e.descripcion
             << RESET
             << "\n";
    }
}

void mostrarErroresSintacticos(const AnalizadorSintactico& parser) {

    if (!parser.tieneErrores())
        return;

    cout << RED << BOLD;
    cout << "\nERRORES SINTACTICOS:\n";
    cout << RESET;

    for (const auto& e : parser.obtenerErrores()) {

        cout << RED
             << "  [Linea "
             << e.linea
             << ", Col "
             << e.columna
             << "] "
             << e.descripcion
             << RESET
             << "\n";
    }
}

static string leerArchivoJson(const string& ruta) {
    ifstream f(ruta);
    if (!f.is_open())
        throw runtime_error("No se pudo abrir el archivo JSON: " + ruta);
    ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}



// -------------------------------------------------------------------
// 1. Algoritmo de alineamiento con transposiciones (Levenshtein-Damerau)
//    Retorna la secuencia de operaciones para transformar esperada -> ingresada
// -------------------------------------------------------------------
enum OpType { MATCH, SUBSTITUTE, INSERT, DELETE, TRANSPOSE };

struct EditOp {
    OpType type;
    int pos;           // índice en la palabra esperada (para MATCH/SUBSTITUTE/DELETE)
    int pos2;          // índice en la palabra ingresada (para INSERT/TRANSPOSE)
    char from;         // carácter en esperada
    char to;           // carácter en ingresada (si aplica)
};

// Función de alineamiento (Damerau-Levenshtein con costos unitarios)
vector<EditOp> alignWords(const string& a, const string& b) {
    int n = a.size(), m = b.size();
    vector<vector<int>> dp(n+1, vector<int>(m+1, 0));
    vector<vector<EditOp>> ops(n+1, vector<EditOp>(m+1));

    // Inicialización
    for (int i = 0; i <= n; ++i) {
        dp[i][0] = i;
        ops[i][0] = {DELETE, i-1, -1, a[i-1], '\0'};
    }
    for (int j = 0; j <= m; ++j) {
        dp[0][j] = j;
        ops[0][j] = {INSERT, -1, j-1, '\0', b[j-1]};
    }

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            int cost = (a[i-1] == b[j-1]) ? 0 : 1;
            int del = dp[i-1][j] + 1;
            int ins = dp[i][j-1] + 1;
            int sub = dp[i-1][j-1] + cost;
            int best = min(del, min(ins, sub));
            EditOp op;

            if (best == sub && cost == 0) {
                op = {MATCH, i-1, j-1, a[i-1], b[j-1]};
            } else if (best == sub) {
                op = {SUBSTITUTE, i-1, j-1, a[i-1], b[j-1]};
            } else if (best == del) {
                op = {DELETE, i-1, -1, a[i-1], '\0'};
            } else { // ins
                op = {INSERT, -1, j-1, '\0', b[j-1]};
            }

            // Transposición (Damerau): verificar si i>1 y j>1 y a[i-1]==b[j-2] y a[i-2]==b[j-1]
            if (i > 1 && j > 1 && a[i-1] == b[j-2] && a[i-2] == b[j-1]) {
                int transp = dp[i-2][j-2] + 1;
                if (transp < best) {
                    best = transp;
                    op = {TRANSPOSE, i-2, j-2, a[i-2], b[j-2]}; // posición del primer carácter intercambiado
                }
            }
            dp[i][j] = best;
            ops[i][j] = op;
        }
    }

    // Reconstruir secuencia de operaciones (recorrido inverso)
    vector<EditOp> result;
    int i = n, j = m;
    while (i > 0 || j > 0) {
        EditOp op = ops[i][j];
        result.push_back(op);
        if (op.type == MATCH || op.type == SUBSTITUTE) { --i; --j; }
        else if (op.type == DELETE) { --i; }
        else if (op.type == INSERT) { --j; }
        else if (op.type == TRANSPOSE) { i -= 2; j -= 2; }
    }
    reverse(result.begin(), result.end());
    return result;
}


// -------------------------------------------------------------------
// 2. Clasificación de errores utilizando las tablas del analizador
// -------------------------------------------------------------------

// Función que carga los JSON una sola vez (singleton)
class RecursosDislexia {
public:
    unordered_set<char> grafemasAlta;
    unordered_set<char> grafemasAltaBDPQ;
    unordered_map<string,string> homofonos; // palabra -> tipo
    // ... otras tablas si se necesitan

    static RecursosDislexia& instance() {
        static RecursosDislexia inst;
        return inst;
    }

private:
    RecursosDislexia() {
        // Cargar pares_visuales.json para obtener grafemas de alta severidad
        // (simplificado: aquí usamos un subset fijo para demostración)
        // En producción, usar el analizador_semantico para cargar todos los JSON.
        grafemasAlta = {'b','d','p','q','u','n','m','w'};
        grafemasAltaBDPQ = {'b','d','p','q'};
        // homofonos básicos (b/v, ll/y, s/c/z, g/j, h muda)
        homofonos["baca"] = "b_v"; homofonos["vaca"] = "b_v";
        homofonos["bale"] = "b_v"; homofonos["vale"] = "b_v";
        homofonos["barón"] = "b_v"; homofonos["varón"] = "b_v";
        homofonos["basta"] = "b_v"; homofonos["vasta"] = "b_v";
        homofonos["bate"] = "b_v"; homofonos["vate"] = "b_v";
        homofonos["bello"] = "b_v"; homofonos["vello"] = "b_v";
        homofonos["beta"] = "b_v"; homofonos["veta"] = "b_v";
        homofonos["casa"] = "s_c_z"; homofonos["caza"] = "s_c_z";
        homofonos["maza"] = "s_c_z"; homofonos["masa"] = "s_c_z";
        homofonos["cazo"] = "s_c_z"; homofonos["caso"] = "s_c_z";
        homofonos["cien"] = "s_c_z"; homofonos["sien"] = "s_c_z";
        homofonos["cocer"] = "s_c_z"; homofonos["coser"] = "s_c_z";
        homofonos["ceda"] = "s_c_z"; homofonos["seda"] = "s_c_z";
        homofonos["cima"] = "s_c_z"; homofonos["sima"] = "s_c_z";
        homofonos["gira"] = "g_j"; homofonos["jira"] = "g_j";
        homofonos["gerente"] = "g_j"; homofonos["jerente"] = "g_j";
        homofonos["gesto"] = "g_j"; homofonos["jesto"] = "g_j";
        // ll/y
        homofonos["callado"] = "ll_y"; homofonos["cayado"] = "ll_y";
        homofonos["halla"] = "ll_y"; homofonos["haya"] = "ll_y";
        homofonos["malla"] = "ll_y"; homofonos["maya"] = "ll_y";
        homofonos["pollo"] = "ll_y"; homofonos["poyo"] = "ll_y";
        homofonos["pulla"] = "ll_y"; homofonos["puya"] = "ll_y";
        // h muda
        homofonos["a"] = "h_muda"; homofonos["ha"] = "h_muda";
        homofonos["asta"] = "h_muda"; homofonos["hasta"] = "h_muda";
        homofonos["aya"] = "h_muda"; homofonos["haya"] = "h_muda";
        homofonos["echo"] = "h_muda"; homofonos["hecho"] = "h_muda";
        homofonos["ojear"] = "h_muda"; homofonos["hojear"] = "h_muda";
        homofonos["ola"] = "h_muda"; homofonos["hola"] = "h_muda";
        homofonos["ora"] = "h_muda"; homofonos["hora"] = "h_muda";
        // ... se pueden completar con los datos reales
    }
};

string clasificarTipoError(char esperada, char ingresada, const string& palabraEsperada, const string& palabraIngresada) {
    // Si es omisión o inserción
    if (ingresada == '\0') return "OMISION";
    if (esperada == '\0') return "OMISION"; // o "INSERCION"

    // Verificar pares visuales (b/d, p/q, u/n, m/w, etc.)
    auto& vis = RecursosDislexia::instance().grafemasAlta;
    if (vis.count(esperada) && vis.count(ingresada)) {
        // Si ambos son b/d/p/q
        auto& bdpq = RecursosDislexia::instance().grafemasAltaBDPQ;
        if (bdpq.count(esperada) && bdpq.count(ingresada)) {
            return "ROTACION_VISUAL";
        }
        return "ROTACION_VISUAL"; // otros pares de alta confusión
    }

    // Verificar homófonos (usando la tabla de homofonos)
    // Para simplificar, comparamos pares de letras típicos
    string par = string(1, esperada) + ingresada;
    if (par == "bv" || par == "vb") return "SUSTITUCION_FONETICA";
    if (par == "ll" || par == "yl" || par == "ly" || par == "yy") return "SUSTITUCION_FONETICA";
    if (par == "sc" || par == "cs" || par == "sz" || par == "zs" || par == "cz" || par == "zc") return "SUSTITUCION_FONETICA";
    if (par == "gj" || par == "jg") return "SUSTITUCION_FONETICA";
    if (esperada == 'h' || ingresada == 'h') return "SUSTITUCION_FONETICA"; // h muda

    // Si no entra en lo anterior, pero son diferentes
    return "OTRO";
}


// -------------------------------------------------------------------
// 3. Generación del IR JSON
// -------------------------------------------------------------------
json generarIR(const string& esperada, const string& ingresada, double tiempo) {
    vector<EditOp> ops = alignWords(esperada, ingresada);

    json errores = json::array();
    int posEsperada = 0, posIngresada = 0;
    int totalErrores = 0;
    double pesoTotal = 0.0;

    for (const auto& op : ops) {
        int pos = op.pos; // posición en esperada (para MATCH/SUBSTITUTE/DELETE)
        if (op.type == MATCH) {
            posEsperada++;
            posIngresada++;
            continue;
        }

        totalErrores++;
        json error;
        error["posicion"] = pos;
        error["letraEsperada"] = string(1, op.from);
        string tipo;

        if (op.type == SUBSTITUTE) {
            error["letraIngresada"] = string(1, op.to);
            tipo = clasificarTipoError(op.from, op.to, esperada, ingresada);
            posEsperada++; posIngresada++;
        } else if (op.type == DELETE) {
            error["letraIngresada"] = nullptr;
            tipo = "OMISION";
            posEsperada++;
        } else if (op.type == INSERT) {
            error["letraIngresada"] = string(1, op.to);
            tipo = "OMISION"; // lo tratamos como omisión de la esperada (no hay letra esperada)
            error["letraEsperada"] = nullptr; // pero mejor poner como null
            // ajustamos posición: usamos la posición anterior
            // Mejor lo omitimos de la lista, ya que el usuario añadió una letra extra
            continue; // no lo reportamos como error de la palabra esperada
        } else if (op.type == TRANSPOSE) {
            error["letraIngresada"] = string(1, op.to);
            tipo = "INVERSION";
            posEsperada += 2;
            posIngresada += 2;
        }

        error["tipoError"] = tipo;
        // Audio corrección: ruta al fonema de la letra esperada
        if (op.type != INSERT && op.from != '\0') {
            error["audioCorreccion"] = "/assets/audio/fonemas/" + string(1, op.from) + ".mp3";
        } else {
            error["audioCorreccion"] = "/assets/audio/fonemas/generico.mp3";
        }
        // Descripción
        error["descripcion"] = "Error de tipo " + tipo + " en la posición " + to_string(pos);
        errores.push_back(error);

        // Acumular peso (usar pesos del modelo)
        // Por simplicidad, asignamos peso 1.0 por cada error, pero se puede mejorar
        pesoTotal += 1.0;
    }

    // Score global: entre 0 y 1, donde 1 = sin errores
    double maxPosible = max(esperada.size(), ingresada.size());
    double score = 1.0 - (totalErrores / maxPosible);
    if (score < 0) score = 0;
    if (score > 1) score = 1;

    // Nivel de riesgo (umbrales simplificados)
    string nivel;
    if (score >= 0.9) nivel = "BAJO";
    else if (score >= 0.7) nivel = "MEDIO";
    else nivel = "ALTO";

    json result;
    result["palabraEsperada"] = esperada;
    result["palabraIngresada"] = ingresada;
    result["scoreGlobal"] = score;
    result["nivelRiesgo"] = nivel;
    result["errores"] = errores;
    result["palabrasAnalizadas"] = 1;
    result["tiempoProcesamiento"] = tiempo;

    return result;
}


int main() {
    auto start = chrono::high_resolution_clock::now();

    string esperada, ingresada;
    if (!getline(cin, esperada)) {
        cerr << "Error: no se pudo leer la palabra esperada" << endl;
        return 1;
    }
    if (!getline(cin, ingresada)) {
        cerr << "Error: no se pudo leer la palabra ingresada" << endl;
        return 1;
    }

    // Normalizar (minúsculas y quitar tildes) – usar la función existente del lexer
    // (opcional, pero podemos usar una función propia)
    // Por ahora, trabajamos con las cadenas tal cual.

    auto end = chrono::high_resolution_clock::now();
    double tiempo = chrono::duration<double>(end - start).count();

    json ir = generarIR(esperada, ingresada, tiempo);
    cout << ir.dump(2) << endl;

    return 0;
}

/*

int main(int argc, char* argv[]) {
    auto start = chrono::high_resolution_clock::now();
    
    string esperada, ingresada;
    if (!getline(cin, esperada)) {
        cerr << "Error: no se pudo leer la palabra esperada" << endl;
        return 1;
    }
    if (!getline(cin, ingresada)) {
        cerr << "Error: no se pudo leer la palabra ingresada" << endl;
        return 1;
    }

    try {


        AnalizadorLexico lexer(rutaEntrada);

        BufferTokens bufferTokens = lexer.tokenizar();

        mostrarTokens(bufferTokens);

        mostrarErroresLexicos(lexer);

        lexer.resetear();

        // IMPORTANTE:
        // El buffer ya fue consumido al imprimir tokens,
        // por eso necesitamos volver a tokenizar.
        BufferTokens bufferParser = lexer.tokenizar();

        // ─────────────────────────────────────
        // ANALISIS SINTACTICO
        // ─────────────────────────────────────
        imprimirSeparador("ANALISIS SINTACTICO");

        AnalizadorSintactico parser(move(bufferParser));

        NodoSintactico ast = parser.analizar();

        parser.imprimirArbol(ast);

        mostrarErroresSintacticos(parser);

        // ─────────────────────────────────────
        // ANALISIS SEMANTICO
        // ─────────────────────────────────────
        imprimirSeparador("FASE 3 - ANALISIS SEMANTICO DISLEXICO");
 
        string jsonPonderacion    = leerArchivoJson(dirData + "modelo_ponderacion.json");
        string jsonReglas         = leerArchivoJson(dirData + "reglas_deteccion.json");
        string jsonPares          = leerArchivoJson(dirData + "pares_visuales.json");
        string jsonSilabicos      = leerArchivoJson(dirData + "patrones_silabicos.json");
        string jsonHomofonos      = leerArchivoJson(dirData + "homofonos.json");
        string jsonPalabrasRiesgo = leerArchivoJson(dirData + "palabras_riesgo.json");
        string jsonSightWords     = leerArchivoJson(dirData + "sight_words.json");
        string jsonFalsosPos      = leerArchivoJson(dirData + "falsos_positivos.json");
        string jsonErroresTipicos = leerArchivoJson(dirData + "errores_tipicos.json");
 
        AnalizadorSemantico semantico(
            ast,
            jsonPonderacion,
            jsonReglas,
            jsonPares,
            jsonSilabicos,
            jsonHomofonos,
            jsonPalabrasRiesgo,
            jsonSightWords,
            jsonFalsosPos,
            jsonErroresTipicos
        );
 
        ResultadoSemantico resultado = semantico.analizar();
        semantico.imprimirReporte(resultado);
 
        semantico.escribirReporte(resultado, rutaSalida);
        cout << GREEN << "\nReporte guardado en: " << rutaSalida << RESET << "\n";


        // ─────────────────────────────────────
        // RESUMEN FINAL
        // ─────────────────────────────────────
        imprimirSeparador("RESUMEN");

        cout << "Errores lexicos: " << lexer.obtenerErrores().size() << "\n";
        cout << "Errores sintacticos: " << parser.obtenerErrores().size() << "\n";
        cout << "Palabras analizadas  : " << resultado.totalPalabrasAnalizables << "\n";
        cout << "Palabras de riesgo   : " << resultado.palabrasRiesgo.size() << "\n";
        cout << "Errores ortograficos : " << resultado.erroresOrtograficos.size() << "\n"; 
        cout << "Indicador dislexico  : " << fixed << setprecision(2)
             << resultado.indicadorPorcentual << "%  ("
             << resultado.nivelIndicador << ")\n";
 


        if (!lexer.tieneErrores() && !parser.tieneErrores()) {

            cout << GREEN
                 << "\nAnalisis completado correctamente.\n"
                 << RESET;
        }
        else {

            cout << YELLOW
                 << "\nAnalisis finalizado con errores.\n"
                 << RESET;
        }

    }
    catch (const exception& e) {
        cerr << RED << "\nERROR FATAL: " << e.what() << RESET << "\n";
        return 1;
    }

    return 0;
}
*/