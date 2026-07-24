// main.cpp

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <fstream>
#include <sstream>
#include <codecvt>
#include <locale>
#include <algorithm>
#include <chrono>

#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

// ────────────────────────────────────────────────────────────────
//  Carga de archivos JSON de reglas
// ────────────────────────────────────────────────────────────────

static string leerArchivo(const string& ruta) {
    ifstream f(ruta);
    if (!f.is_open()) throw runtime_error("No se pudo abrir: " + ruta);
    ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// Estructura para pares visuales (cargados desde pares_visuales.json)
struct ParVisual {
    vector<wchar_t> grafemas;
    string severidad;
};

// Estructura para homófonos (cargados desde homofonos.json)
struct GrupoHomofono {
    string tipo;
    vector<vector<wstring>> pares;
};

// ────────────────────────────────────────────────────────────────
//  Clase que carga y almacena las reglas
// ────────────────────────────────────────────────────────────────

class ReglasDislexia {
public:
    static ReglasDislexia& instancia() {
        static ReglasDislexia inst;
        return inst;
    }

    // Verifica si dos caracteres forman un par de confusión visual de alta severidad
    bool esParVisualAlto(wchar_t a, wchar_t b) const {
        for (const auto& p : paresVisuales) {
            if (p.severidad != "alta") continue;
            if (p.grafemas.size() != 2) continue;
            if ((p.grafemas[0] == a && p.grafemas[1] == b) ||
                (p.grafemas[0] == b && p.grafemas[1] == a))
                return true;
        }
        return false;
    }

    // Devuelve el tipo de homófono si los caracteres forman un par homófono
    string tipoHomofono(wchar_t a, wchar_t b) const {
        // Convertimos a wstring de un solo carácter para buscar en la tabla
        wstring sa(1, a), sb(1, b);
        // Los pares de homófonos están almacenados como listas de palabras, pero aquí
        // solo necesitamos comparar a nivel de carácter. Para simplificar, usamos un mapa
        // de pares de caracteres conocido.
        // Creamos un mapa manual de pares típicos.
        static const unordered_map<wstring, string> mapa = {
            {L"bv", "b_v"}, {L"vb", "b_v"},
            {L"ll", "ll_y"}, {L"yl", "ll_y"}, {L"ly", "ll_y"}, {L"yy", "ll_y"},
            {L"sc", "s_c_z"}, {L"cs", "s_c_z"}, {L"sz", "s_c_z"}, {L"zs", "s_c_z"},
            {L"cz", "s_c_z"}, {L"zc", "s_c_z"},
            {L"gj", "g_j"}, {L"jg", "g_j"},
            // h muda: comparar con 'h' o sin ella
            {L"h", "h_muda"}, // cuando uno es h y el otro no
        };
        wstring clave = sa + sb;
        auto it = mapa.find(clave);
        if (it != mapa.end()) return it->second;
        // Si uno es 'h' y el otro no, es h_muda
        if (a == L'h' || b == L'h') return "h_muda";
        return "";
    }

private:
    vector<ParVisual> paresVisuales;
    vector<GrupoHomofono> gruposHomofonos;

    ReglasDislexia() {
        cargarParesVisuales();
        cargarHomofonos();
    }

    void cargarParesVisuales() {
        try {
            string contenido = leerArchivo("../data/pares_visuales.json");
            json j = json::parse(contenido);
            for (const auto& item : j["pares"]) {
                ParVisual p;
                p.severidad = item["severidad"].get<string>();
                for (const auto& g : item["grafemas"]) {
                    string utf8 = g.get<string>();
                    // Convertir a wchar_t (asumimos que el JSON contiene un solo carácter)
                    if (utf8.size() == 1) {
                        p.grafemas.push_back((wchar_t)utf8[0]);
                    } else {
                        // Para caracteres multibyte, usamos conversión simple (solo para demo)
                        // En producción usaríamos una librería como utfcpp.
                        // Asumimos que los caracteres son ASCII o están en el rango BMP.
                        // Convertimos manualmente usando wstring_convert.
                        wstring_convert<codecvt_utf8<wchar_t>> conv;
                        wstring w = conv.from_bytes(utf8);
                        if (!w.empty()) p.grafemas.push_back(w[0]);
                    }
                }
                paresVisuales.push_back(p);
            }
        } catch (...) {
            // Si falla la carga, continuamos con reglas por defecto (pares comunes)
            cerr << "Advertencia: No se pudo cargar pares_visuales.json. Usando reglas por defecto.\n";
            // Añadir pares básicos manualmente
            paresVisuales.push_back({{L'b', L'd'}, "alta"});
            paresVisuales.push_back({{L'p', L'q'}, "alta"});
            paresVisuales.push_back({{L'd', L'p'}, "alta"});
            paresVisuales.push_back({{L'b', L'q'}, "alta"});
            paresVisuales.push_back({{L'u', L'n'}, "alta"});
            paresVisuales.push_back({{L'm', L'w'}, "alta"});
        }
    }

    void cargarHomofonos() {
        try {
            string contenido = leerArchivo("../data/homofonos.json");
            json j = json::parse(contenido);
            for (const auto& grupo : j["grupos"]) {
                GrupoHomofono gh;
                gh.tipo = grupo["tipo"].get<string>();
                for (const auto& par : grupo["pares"]) {
                    vector<wstring> parWords;
                    for (const auto& palabra : par) {
                        string utf8 = palabra.get<string>();
                        wstring_convert<codecvt_utf8<wchar_t>> conv;
                        parWords.push_back(conv.from_bytes(utf8));
                    }
                    gh.pares.push_back(parWords);
                }
                gruposHomofonos.push_back(gh);
            }
        } catch (...) {
            cerr << "Advertencia: No se pudo cargar homofonos.json. Usando reglas por defecto.\n";
            // Añadir pares básicos manualmente
        }
    }
};

// ────────────────────────────────────────────────────────────────
//  Algoritmo de alineamiento Damerau‑Levenshtein sobre wstring
// ────────────────────────────────────────────────────────────────

enum OpType { MATCH, SUBSTITUTE, INSERT, DELETE, TRANSPOSE };

struct EditOp {
    OpType type;
    int posEsp;        // índice en referencia (para MATCH/SUBSTITUTE/DELETE)
    int posUsr;        // índice en usuario (para MATCH/SUBSTITUTE/INSERT)
    wchar_t charEsp;   // carácter en referencia
    wchar_t charUsr;   // carácter en usuario (si aplica)
};

vector<EditOp> alignWords(const wstring& ref, const wstring& usr) {
    int n = ref.size(), m = usr.size();
    vector<vector<int>> dp(n+1, vector<int>(m+1, 0));
    vector<vector<EditOp>> ops(n+1, vector<EditOp>(m+1));

    for (int i = 0; i <= n; ++i) {
        dp[i][0] = i;
        ops[i][0] = {DELETE, i-1, -1, ref[i-1], L'\0'};
    }
    for (int j = 0; j <= m; ++j) {
        dp[0][j] = j;
        ops[0][j] = {INSERT, -1, j-1, L'\0', usr[j-1]};
    }

    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            int cost = (ref[i-1] == usr[j-1]) ? 0 : 1;
            int del = dp[i-1][j] + 1;
            int ins = dp[i][j-1] + 1;
            int sub = dp[i-1][j-1] + cost;
            int best = min(del, min(ins, sub));
            EditOp op;

            if (best == sub && cost == 0) {
                op = {MATCH, i-1, j-1, ref[i-1], usr[j-1]};
            } else if (best == sub) {
                op = {SUBSTITUTE, i-1, j-1, ref[i-1], usr[j-1]};
            } else if (best == del) {
                op = {DELETE, i-1, -1, ref[i-1], L'\0'};
            } else { // ins
                op = {INSERT, -1, j-1, L'\0', usr[j-1]};
            }

            // Transposición (Damerau)
            if (i > 1 && j > 1 && ref[i-1] == usr[j-2] && ref[i-2] == usr[j-1]) {
                int transp = dp[i-2][j-2] + 1;
                if (transp < best) {
                    best = transp;
                    op = {TRANSPOSE, i-2, j-2, ref[i-2], usr[j-2]};
                }
            }
            dp[i][j] = best;
            ops[i][j] = op;
        }
    }

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

// ────────────────────────────────────────────────────────────────
//  Clasificación de errores usando las reglas cargadas
// ────────────────────────────────────────────────────────────────

string clasificarError(const EditOp& op) {
    if (op.type == MATCH) return "MATCH";
    if (op.type == DELETE) return "OMISION";
    if (op.type == INSERT) return "INSERCION";  // aunque el usuario añadió, lo tratamos como omisión en referencia

    if (op.type == TRANSPOSE) return "INVERSION";

    // Sustitución
    wchar_t a = op.charEsp, b = op.charUsr;
    auto& reglas = ReglasDislexia::instancia();

    // 1. Rotación visual (b/d, p/q, etc.)
    if (reglas.esParVisualAlto(a, b)) return "ROTACION_VISUAL";

    // 2. Homófono (b/v, ll/y, s/c/z, etc.)
    string tipoH = reglas.tipoHomofono(a, b);
    if (!tipoH.empty()) return "SUSTITUCION_FONETICA";

    // 3. Contexto dependiente (c->s, g->j) – lo incluimos en homófonos

    // 4. Otros
    return "OTRO";
}

// ────────────────────────────────────────────────────────────────
//  Generación del IR en JSON
// ────────────────────────────────────────────────────────────────

json generarIR(const wstring& ref, const wstring& usr, double tiempo) {

    // Convertidor UTF-8 para las palabras completas
    wstring_convert<codecvt_utf8<wchar_t>> conv;


    vector<EditOp> ops = alignWords(ref, usr);
    json errores = json::array();
    int totalErrores = 0;

    for (const auto& op : ops) {
        if (op.type == MATCH) continue;
        totalErrores++;

        json error;
        error["posicion"] = op.posEsp >= 0 ? op.posEsp : (op.posUsr >= 0 ? op.posUsr : 0);

        // Convertir caracteres wchar_t a UTF-8 para el JSON
        //wstring_convert<codecvt_utf8<wchar_t>> conv;
        string charEsp = (op.charEsp != L'\0') ? conv.to_bytes(op.charEsp) : "";
        string charUsr = (op.charUsr != L'\0') ? conv.to_bytes(op.charUsr) : "";

        error["letraEsperada"] = charEsp;
        error["letraIngresada"] = charUsr;

        string tipo = clasificarError(op);
        error["tipoError"] = tipo;
        error["descripcion"] = "Error de tipo " + tipo + " en posición " + to_string(error["posicion"].get<int>());

        errores.push_back(error);
    }

    double maxLen = max(ref.size(), usr.size());
    double score = 1.0 - (totalErrores / (double)maxLen);
    if (score < 0) score = 0;
    if (score > 1) score = 1;

    string nivel;
    if (score >= 0.9) nivel = "BAJO";
    else if (score >= 0.7) nivel = "MEDIO";
    else nivel = "ALTO";

    
    json result;
    result["palabraEsperada"] = conv.to_bytes(ref);
    result["palabraIngresada"] = conv.to_bytes(usr);
    result["scoreGlobal"] = score;
    result["nivelRiesgo"] = nivel;
    result["errores"] = errores;
    result["tiempoProcesamiento"] = tiempo;

    return result;
}

// ────────────────────────────────────────────────────────────────
//  main: lee dos líneas de stdin y emite JSON
// ────────────────────────────────────────────────────────────────

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    auto start = chrono::high_resolution_clock::now();

    string refUtf8, usrUtf8;
    if (!getline(cin, refUtf8)) {
        cerr << "Error: No se pudo leer el texto de referencia\n";
        return 1;
    }
    if (!getline(cin, usrUtf8)) {
        cerr << "Error: No se pudo leer el texto del usuario\n";
        return 1;
    }

    // Convertir a wstring (UTF-8 -> UTF-32)
    wstring_convert<codecvt_utf8<wchar_t>> conv;
    wstring ref, usr;
    try {
        ref = conv.from_bytes(refUtf8);
        usr = conv.from_bytes(usrUtf8);
    } catch (...) {
        cerr << "Error de conversión UTF-8\n";
        return 1;
    }

    auto end = chrono::high_resolution_clock::now();
    double tiempo = chrono::duration<double>(end - start).count();

    json ir = generarIR(ref, usr, tiempo);
    cout << ir.dump(2) << endl;

    return 0;
}