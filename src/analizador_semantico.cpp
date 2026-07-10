#include "analizador_semantico.h"
#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <iomanip>

using json = nlohmann::json;
using namespace std;


#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"
#define MAGENTA "\033[35m"
#define BLUE    "\033[34m"

static string leerArchivo(const string& ruta) {
    ifstream f(ruta);
    if (!f.is_open())
        throw runtime_error("No se pudo abrir: " + ruta);
    ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

string nombreCategoria(CategoriaRiesgo cat) {
    static const unordered_map<int, string> tabla = {
        {(int)CategoriaRiesgo::CONFUSION_VISUAL_B_D_P_Q,  "confusion_visual_b_d_p_q"},
        {(int)CategoriaRiesgo::CONFUSION_VISUAL_OTROS,     "confusion_visual_otros"},
        {(int)CategoriaRiesgo::HOMOFONO_B_V,               "homofono_b_v"},
        {(int)CategoriaRiesgo::HOMOFONO_LL_Y,              "homofono_ll_y"},
        {(int)CategoriaRiesgo::HOMOFONO_S_C_Z,             "homofono_s_c_z"},
        {(int)CategoriaRiesgo::HOMOFONO_G_J,               "homofono_g_j"},
        {(int)CategoriaRiesgo::HOMOFONO_H_MUDA,            "homofono_h_muda"},
        {(int)CategoriaRiesgo::HOMOFONO_OTROS,             "homofono_otros"},
        {(int)CategoriaRiesgo::TILDE_DIACRITICA,           "tilde_diacritica"},
        {(int)CategoriaRiesgo::DIGRAFO,                    "digrafo"},
        {(int)CategoriaRiesgo::SINFON_R,                   "sinfon_r"},
        {(int)CategoriaRiesgo::SINFON_L,                   "sinfon_l"},
        {(int)CategoriaRiesgo::DIPTONGO,                   "diptongo"},
        {(int)CategoriaRiesgo::TRIPTONGO,                  "triptongo"},
        {(int)CategoriaRiesgo::CONTEXTO_DEPENDIENTE,       "contexto_dependiente"},
        {(int)CategoriaRiesgo::SILABA_INVERSA,             "silaba_inversa"},
        {(int)CategoriaRiesgo::INVERSION_SILABICA,         "inversion_silabica"},
        {(int)CategoriaRiesgo::TERMINACION_PROBLEMATICA,   "terminacion_problematica"},
        {(int)CategoriaRiesgo::LONGITUD,                   "longitud"},
        {(int)CategoriaRiesgo::H_MUDA,                     "h_muda"},
        {(int)CategoriaRiesgo::LISTA_CURADA,               "lista_curada"},
        {(int)CategoriaRiesgo::SIGHT_WORD,                 "sight_word"},
        {(int)CategoriaRiesgo::ACENTO_INVALIDO,            "acento_invalido_detectado_por_lexico"},
    };
    auto it = tabla.find((int)cat);
    return (it != tabla.end()) ? it->second : "DESCONOCIDA";
}

AnalizadorSemantico::AnalizadorSemantico(
    const NodoSintactico& ast,
    const string& jsonPonderacion,
    const string& /*jsonReglas*/,       
    const string& jsonPares,
    const string& jsonSilabicos,
    const string& jsonHomofonos,
    const string& jsonPalabrasRiesgo,
    const string& jsonSightWords,
    const string& jsonFalsosPositivos,
    const string& jsonErroresTipicos
)
: ast_(ast)
{
    cargarPonderacion(jsonPonderacion);
    cargarPares(jsonPares);
    cargarSilabicos(jsonSilabicos);
    cargarHomofonos(jsonHomofonos);
    cargarPalabrasRiesgo(jsonPalabrasRiesgo);
    cargarSightWords(jsonSightWords);
    cargarFalsosPositivos(jsonFalsosPositivos);
    cargarErroresTipicos(jsonErroresTipicos);
}

// ─────────────────────────────────────────────────────────────────────────
// Carga de JSON
// ─────────────────────────────────────────────────────────────────────────
void AnalizadorSemantico::cargarPonderacion(const string& contenido) {
    auto j = json::parse(contenido);

    for (const auto& item : j["pesos_por_categoria"]) {
        string cat = item["categoria"].get<string>();
        double peso = item["peso_base"].get<double>();
        pesos_[cat] = peso;
    }

    for (const auto& mod : j["modificadores"]) {
        string nombre = mod["nombre"].get<string>();
        double valor  = mod["valor"].get<double>();
        if (nombre == "sight_word_multiplier") modificador_sight_word_ = valor;
        if (nombre == "multiples_riesgos_bonus") modificador_multi_bonus_ = valor;
        if (nombre == "lista_curada_bonus")  modificador_curada_bonus_ = valor;
    }

    for (const auto& niv : j["niveles_indicador"]) {
        NivelIndicador n;
        n.min   = niv["rango"][0].get<int>();
        n.max   = niv["rango"][1].get<int>();
        n.nivel = niv["nivel"].get<string>();
        n.desc  = niv["descripcion"].get<string>();
        niveles_.push_back(n);
    }
}

void AnalizadorSemantico::cargarPares(const string& contenido) {
    auto j = json::parse(contenido);
    for (const auto& par : j["pares"]) {
        string sev = par["severidad"].get<string>();
        if (sev != "alta") continue;
        auto grafemas = par["grafemas"].get<vector<string>>();
        for (const auto& g : grafemas) {
            if (g.size() == 1) {
                char c = g[0];
                grafemas_alta_.insert(c);
                // b, d, p, q van al subconjunto específico
                if (c=='b'||c=='d'||c=='p'||c=='q')
                    grafemas_alta_bdpq_.insert(c);
            }
        }
    }
}

void AnalizadorSemantico::cargarSilabicos(const string& contenido) {
    auto j = json::parse(contenido);
    auto cats = j["categorias"];

    auto cargar = [&](const string& key, vector<string>& dest) {
        if (cats.contains(key) && cats[key].contains("patrones")) {
            for (const auto& p : cats[key]["patrones"])
                if (p.is_string()) dest.push_back(p.get<string>());
        }
    };

    cargar("sinfon_con_r",        patrones_sinfon_r_);
    cargar("sinfon_con_l",        patrones_sinfon_l_);
    cargar("diptongo",            patrones_diptongo_);
    cargar("triptongo",           patrones_triptongo_);
    cargar("digrafo",             patrones_digrafo_);
    cargar("silaba_inversa",      patrones_silaba_inversa_);
    cargar("terminacion_problematica", patrones_terminacion_);

    if (cats.contains("inversion_silabica") &&
        cats["inversion_silabica"].contains("patrones_pares")) {
        for (const auto& par : cats["inversion_silabica"]["patrones_pares"]) {
            if (par.size() >= 2)
                pares_inversion_.push_back({par[0].get<string>(),
                                             par[1].get<string>()});
        }
    }
}

void AnalizadorSemantico::cargarHomofonos(const string& contenido) {
    auto j = json::parse(contenido);
    for (const auto& grupo : j["grupos"]) {
        string tipo = grupo["tipo"].get<string>();
        for (const auto& par : grupo["pares"]) {
            for (const auto& w : par) {
                string palabra = w.get<string>();
                tabla_homofonos_[palabra] = tipo;
            }
        }
    }
}

void AnalizadorSemantico::cargarPalabrasRiesgo(const string& contenido) {
    auto j = json::parse(contenido);
    for (const auto& item : j["palabras"]) {
        string palabra = item["palabra"].get<string>();
        lista_curada_.insert(palabra);
    }
}

void AnalizadorSemantico::cargarSightWords(const string& contenido) {
    auto j = json::parse(contenido);
    for (const auto& w : j["palabras"])
        sight_words_.insert(w.get<string>());
}

void AnalizadorSemantico::cargarFalsosPositivos(const string& contenido) {
    auto j = json::parse(contenido);

    if (j.contains("palabras")) {
        for (const auto& w : j["palabras"])
            if (w.is_string()) falsos_positivos_.insert(w.get<string>());
    } else if (j.contains("falsos_positivos")) {
        for (const auto& w : j["falsos_positivos"])
            if (w.is_string()) falsos_positivos_.insert(w.get<string>());
    }
}

void AnalizadorSemantico::cargarErroresTipicos(const string& contenido) {
    auto j = json::parse(contenido);
    for (const auto& item : j["errores"]) {
        string inc  = item["incorrecto"].get<string>();
        string corr = item["correcto"].get<string>();
        string tipo = item["tipo"].get<string>();
        errores_tipicos_[inc] = {corr, tipo};
    }
}

ResultadoSemantico AnalizadorSemantico::analizar() {
    ResultadoSemantico res;
    recorrerNodo(ast_, false, res);

    double promedioRiesgo = 0.0;
    if (res.totalPalabrasAnalizables > 0) {
        double promedio = res.sumaPesos / res.totalPalabrasAnalizables;

        double sumaModelo = 0.0;
        for (const auto& par : pesos_)
            sumaModelo += par.second;
        double promedioModelo = pesos_.empty() ? 1.5 : sumaModelo / pesos_.size();

        double referencia = promedioModelo * 4.0;
        promedioRiesgo = min(100.0, (promedio / referencia) * 100.0);
    }

    double componenteErrores = 0.0;
    if (res.totalPalabrasAnalizables > 0) {
        int numErrores = (int)res.erroresOrtograficos.size();
        double proporcion = (double)numErrores / res.totalPalabrasAnalizables;

        // 30% de errores → 100% del componente.
        componenteErrores = min(100.0, (proporcion / 0.20) * 100.0);
    }

    res.indicadorPorcentual = min(100.0, (componenteErrores * 0.70) + (promedioRiesgo * 0.30));

    res.componenteErroresOrtograficos = componenteErrores;
    res.componentePalabrasRiesgo = promedioRiesgo;

    res.nivelIndicador  = calcularNivel(res.indicadorPorcentual, res.descripcionNivel);
    return res;
}

// ─────────────────────────────────────────────────────────────────────────
// Recorrido recursivo del AST
// ─────────────────────────────────────────────────────────────────────────
void AnalizadorSemantico::recorrerNodo(const NodoSintactico& nodo, bool dentroFraseCita, ResultadoSemantico& res)
{
    bool esCita = dentroFraseCita ||
                  (nodo.tipo == TipoNodo::FRASE_CITA);

    if (nodo.tipo == TipoNodo::PALABRA) {
        analizarPalabra(nodo, esCita, res);
        return;
    }

    for (const auto& hijo : nodo.hijos)
        recorrerNodo(hijo, esCita, res);
}

// ─────────────────────────────────────────────────────────────────────────
// Análisis de un nodo PALABRA
// ─────────────────────────────────────────────────────────────────────────
void AnalizadorSemantico::analizarPalabra(const NodoSintactico& nodo, bool dentroFraseCita, ResultadoSemantico& res)
{
    const string& valorNormal   = nodo.valor;
    const string& valorOriginal = nodo.valorOriginal;

    if (esFalsoPositivo(nodo)) return;

    res.totalPalabrasAnalizables++;

    vector<CategoriaRiesgo> cats;
    aplicarReglas(valorNormal, valorOriginal, cats);

    {
        auto itH = tabla_homofonos_.find(valorNormal);
        if (itH != tabla_homofonos_.end() && itH->second == "tilde_diacritica") {
            if (valorOriginal != valorNormal) {
                bool yaEsta = false;
                for (auto& c : cats)
                    if (c == CategoriaRiesgo::TILDE_DIACRITICA) { yaEsta = true; break; }
                if (!yaEsta)
                    cats.push_back(CategoriaRiesgo::TILDE_DIACRITICA);
            }
        }
    }

    {
        auto itErr = errores_tipicos_.find(valorNormal);
        if (itErr != errores_tipicos_.end()) {
            ErrorOrtografico err;
            err.palabraEncontrada = valorOriginal;
            err.palabraCorrecta = itErr->second.first;
            err.tipoError = itErr->second.second;
            err.linea  = nodo.linea;
            err.columna = nodo.columna;
            res.erroresOrtograficos.push_back(err);
        }
    }

    if (cats.empty()) return;

    ResultadoPalabra rp;
    rp.valorOriginal = valorOriginal;
    rp.valorNormal   = valorNormal;
    rp.linea         = nodo.linea;
    rp.columna       = nodo.columna;
    rp.categorias    = cats;
    rp.enFraseCita   = dentroFraseCita;
    rp.esSightWord   = sight_words_.count(valorNormal) > 0;
    rp.enListaCurada = lista_curada_.count(valorNormal) > 0;

    rp.pesoFinal = calcularPeso(rp);

    if (dentroFraseCita) rp.pesoFinal *= 0.5;

    res.palabrasRiesgo.push_back(rp);
    res.sumaPesos += rp.pesoFinal;
}

// ─────────────────────────────────────────────────────────────────────────
// Falsos positivos
// ─────────────────────────────────────────────────────────────────────────
bool AnalizadorSemantico::esFalsoPositivo(const NodoSintactico& nodo) const {
    const string& vNorm = nodo.valor;
    const string& vOrig = nodo.valorOriginal;

    if (vNorm.empty() || vNorm.size() == 1) return true;

    // FP06 - abreviatura: 3 letras o menos
    if (vNorm.size() <= 3) return true;

    // FP07 - sigla: 4 letras o menos todo en mayusculas
    if (vOrig.size() <= 4) {
        bool todaMayus = true;
        for (unsigned char c : vOrig) {
            if (c < 'A' || c > 'Z') { todaMayus = false; break; }
        }
        if (todaMayus) return true;
    }

    // FP01 - nombre propio en medio de frase
    if (!vOrig.empty() && !vNorm.empty()) {
        unsigned char primerOrig = (unsigned char)vOrig[0];
        unsigned char primerNorm = (unsigned char)vNorm[0];
        if (primerOrig >= 'A' && primerOrig <= 'Z' &&
            primerNorm >= 'a' && primerNorm <= 'z') {
            if (!sight_words_.count(vNorm))
                return true;
        }
    }

    if (falsos_positivos_.count(vNorm)) return true;
    return false;
}

// Aplicación de reglas R01-R16
void AnalizadorSemantico::aplicarReglas(const string& valorNormal, const string& valorOriginal, vector<CategoriaRiesgo>& cats) const
{
    reglaR01_confusionVisual(valorNormal, cats);
    reglaR02_sinfonR(valorNormal, cats);
    reglaR03_sinfonL(valorNormal, cats);
    reglaR04_diptongo(valorNormal, valorOriginal, cats);
    reglaR05_digrafo(valorNormal, cats);
    reglaR06_homofono(valorNormal, cats);
    reglaR07_palabraLarga(valorNormal, cats);
    reglaR08_hInicial(valorNormal, cats);
    reglaR09_contextoC(valorNormal, cats);
    reglaR10_contextoG(valorNormal, cats);
    reglaR11_silabaInversa(valorNormal, cats);
    reglaR12_listaCurada(valorNormal, cats);
    reglaR13_inversionSilabica(valorNormal, cats);

    bool tieneRiesgoPrevio = !cats.empty();
    reglaR14_sightWordConRiesgo(valorNormal, tieneRiesgoPrevio, cats);

    reglaR15_triptongo(valorNormal, cats);
    reglaR16_terminacion(valorNormal, cats);

    sort(cats.begin(), cats.end());
    cats.erase(unique(cats.begin(), cats.end()), cats.end());
}

// R01  Confusión visual grafema
void AnalizadorSemantico::reglaR01_confusionVisual(const string& v, vector<CategoriaRiesgo>& cats) const{
    if (v.size() < 3) return;

    int cntB=0, cntD=0, cntP=0, cntQ=0;
    int cntU=0, cntN=0;

    for (char c : v) {
        if      (c == 'b') cntB++;
        else if (c == 'd') cntD++;
        else if (c == 'p') cntP++;
        else if (c == 'q') cntQ++;
        else if (c == 'u') cntU++;
        else if (c == 'n') cntN++;
    }

    int distintosBDPQ = (cntB>0?1:0)+(cntD>0?1:0)+(cntP>0?1:0)+(cntQ>0?1:0);
    if (distintosBDPQ >= 2)
        cats.push_back(CategoriaRiesgo::CONFUSION_VISUAL_B_D_P_Q);

    if (cntU > 0 && cntN > 0)
        cats.push_back(CategoriaRiesgo::CONFUSION_VISUAL_OTROS);
}

// R02  Sinfón con R
void AnalizadorSemantico::reglaR02_sinfonR(const string& v, vector<CategoriaRiesgo>& cats) const{
    for (const auto& p : patrones_sinfon_r_)
        if (contienePatron(v, p)) { cats.push_back(CategoriaRiesgo::SINFON_R); return; }
}

// R03  Sinfón con L
void AnalizadorSemantico::reglaR03_sinfonL(const string& v, vector<CategoriaRiesgo>& cats) const{
    for (const auto& p : patrones_sinfon_l_)
        if (contienePatron(v, p)) { cats.push_back(CategoriaRiesgo::SINFON_L); return; }
}

// R04  Diptongo
void AnalizadorSemantico::reglaR04_diptongo(const string& vNorm,
                                             const string& vOrig,
                                             vector<CategoriaRiesgo>& cats) const
{
    if (tieneVocalDebilConTilde(vOrig)) return;

    for (const auto& p : patrones_diptongo_)
        if (contienePatron(vNorm, p)) { cats.push_back(CategoriaRiesgo::DIPTONGO); return; }
}

// R05  Dígrafo
void AnalizadorSemantico::reglaR05_digrafo(const string& v, vector<CategoriaRiesgo>& cats) const{
    for (const auto& p : patrones_digrafo_)
        if (contienePatron(v, p)) { cats.push_back(CategoriaRiesgo::DIGRAFO); return; }
}

// R06  Homófono
void AnalizadorSemantico::reglaR06_homofono(const string& v, vector<CategoriaRiesgo>& cats) const{
    auto it = tabla_homofonos_.find(v);
    if (it == tabla_homofonos_.end()) return;

    const string& tipo = it->second;
    if      (tipo == "b_v")              cats.push_back(CategoriaRiesgo::HOMOFONO_B_V);
    else if (tipo == "ll_y")             cats.push_back(CategoriaRiesgo::HOMOFONO_LL_Y);
    else if (tipo == "s_c_z")            cats.push_back(CategoriaRiesgo::HOMOFONO_S_C_Z);
    else if (tipo == "g_j")              cats.push_back(CategoriaRiesgo::HOMOFONO_G_J);
    else if (tipo == "h_muda")           cats.push_back(CategoriaRiesgo::HOMOFONO_H_MUDA);
    else if (tipo == "tilde_diacritica") { }
    else                                 cats.push_back(CategoriaRiesgo::HOMOFONO_OTROS);
}

// R07  Palabra larga (≥ 4 sílabas)
void AnalizadorSemantico::reglaR07_palabraLarga(const string& v, vector<CategoriaRiesgo>& cats) const{
    if (contarSilabas(v) >= 4)
        cats.push_back(CategoriaRiesgo::LONGITUD);
}

// R08  H inicial muda
void AnalizadorSemantico::reglaR08_hInicial(const string& v, vector<CategoriaRiesgo>& cats) const {
    if (!v.empty() && v[0] == 'h')
        cats.push_back(CategoriaRiesgo::H_MUDA);
}

// R09  Contexto C (ce, ci)
void AnalizadorSemantico::reglaR09_contextoC(const string& v, vector<CategoriaRiesgo>& cats) const {
    if (contienePatron(v,"ce") || contienePatron(v,"ci"))
        cats.push_back(CategoriaRiesgo::CONTEXTO_DEPENDIENTE);
}

// R10  Contexto G (ge, gi)
void AnalizadorSemantico::reglaR10_contextoG(const string& v, vector<CategoriaRiesgo>& cats) const {
    if (contienePatron(v,"ge") || contienePatron(v,"gi"))
        cats.push_back(CategoriaRiesgo::CONTEXTO_DEPENDIENTE);
}

// R11  Sílaba inversa inicial
void AnalizadorSemantico::reglaR11_silabaInversa(const string& v, vector<CategoriaRiesgo>& cats) const {
    for (const auto& p : patrones_silaba_inversa_) {
        if (v.size() > p.size() + 1 && iniciaCon(v, p)) {
            cats.push_back(CategoriaRiesgo::SILABA_INVERSA);
            return;
        }
    }
}

// R12  Palabra en lista curada
void AnalizadorSemantico::reglaR12_listaCurada( const string& v, vector<CategoriaRiesgo>& cats) const{
    if (lista_curada_.count(v))
        cats.push_back(CategoriaRiesgo::LISTA_CURADA);
}

// R13  Inversión silábica (pardo/prado)
void AnalizadorSemantico::reglaR13_inversionSilabica(const string& v, vector<CategoriaRiesgo>& cats) const{
    for (const auto& par : pares_inversion_) {
        if (v == par.first || v == par.second) {
            cats.push_back(CategoriaRiesgo::INVERSION_SILABICA);
            return;
        }
    }
}

// R14  Sight word + riesgo previo
void AnalizadorSemantico::reglaR14_sightWordConRiesgo(const string& v, bool tieneRiesgo, vector<CategoriaRiesgo>& cats) const{
    if (tieneRiesgo && sight_words_.count(v))
        cats.push_back(CategoriaRiesgo::SIGHT_WORD);
}

// R15  Triptongo
void AnalizadorSemantico::reglaR15_triptongo(const string& v, vector<CategoriaRiesgo>& cats) const{
    for (const auto& p : patrones_triptongo_)
        if (contienePatron(v, p)) { cats.push_back(CategoriaRiesgo::TRIPTONGO); return; }
}

// R16  Terminación problemática
void AnalizadorSemantico::reglaR16_terminacion(const string& v, vector<CategoriaRiesgo>& cats) const{
    for (const auto& p : patrones_terminacion_)
        if (terminaCon(v, p)) { cats.push_back(CategoriaRiesgo::TERMINACION_PROBLEMATICA); return; }
}

// Silabeo (usado por R07)
static bool esVocalFuerte(char c) { return c=='a'||c=='e'||c=='o'; }
static bool esVocalDebil(char c)  { return c=='i'||c=='u'; }
static bool esVocal(char c)       { return esVocalFuerte(c)||esVocalDebil(c); }

int AnalizadorSemantico::contarSilabas(const string& v) const {

    int silabas = 0;
    size_t i = 0;
    bool enVocal = false;
    char prevVocal = '\0';

    while (i < v.size()) {
        unsigned char uc = (unsigned char)v[i];

        if (uc >= 128) {
            int bytes = 1;
            if      ((uc & 0xE0) == 0xC0) bytes = 2;
            else if ((uc & 0xF0) == 0xE0) bytes = 3;
            else if ((uc & 0xF8) == 0xF0) bytes = 4;
            if (enVocal) 
                silabas++;
            silabas++;  
            enVocal   = false;
            prevVocal = '\0';
            i += bytes;
            continue;
        }

        char c = (char)v[i];

        if (esVocal(c)) {
            if (!enVocal) {
                enVocal   = true;
                prevVocal = c;
                silabas++;
            } else {
                bool esDiptongo = false;
                if ((esVocalDebil(prevVocal) && esVocalFuerte(c)) ||
                    (esVocalFuerte(prevVocal) && esVocalDebil(c))) {
                    esDiptongo = true;
                }
                if (!esDiptongo) silabas++;
                prevVocal = c;
            }
        } else {
            enVocal   = false;
            prevVocal = '\0';
        }
        i++;
    }
    return max(silabas, 1);
}

// Cálculo de peso
double AnalizadorSemantico::calcularPeso(const ResultadoPalabra& rp) const {
    double peso = 0.0;

    for (const auto& cat : rp.categorias) {
        string nombre = nombreCategoria(cat);
        string alias  = resolverAlias(nombre);
        if (alias.empty()) continue;
        auto it = pesos_.find(alias);
        if (it != pesos_.end()) peso += it->second;
    }

    if (rp.enListaCurada) peso += modificador_curada_bonus_;

    int numReglas = (int)rp.categorias.size();
    for (const auto& c : rp.categorias)
        if (c == CategoriaRiesgo::SIGHT_WORD || c == CategoriaRiesgo::LISTA_CURADA)
            numReglas--;
    if (numReglas >= 2) peso += modificador_multi_bonus_;

    if (rp.esSightWord) peso *= modificador_sight_word_;

    return peso;
}

// ───────────────────────────────────────────────────────────────────────────
// Alias de categorías (alias_categorias de modelo_ponderacion.json)
// ───────────────────────────────────────────────────────────────────────────
string AnalizadorSemantico::resolverAlias(const string& alias) const {
    static const unordered_map<string,string> mapa = {
        {"sinfon_r",                        "sinfon_r"},
        {"sinfon_l",                        "sinfon_l"},
        {"diptongo",                        "diptongo"},
        {"triptongo",                       "triptongo"},
        {"digrafo",                         "digrafo"},
        {"longitud",                        "longitud"},
        {"h_muda",                          "h_muda"},
        {"homofono_h_muda",                 "h_muda"},
        {"contexto_dependiente",            "contexto_dependiente"},
        {"contexto_c",                      "contexto_dependiente"},
        {"contexto_g",                      "contexto_dependiente"},
        {"silaba_inversa",                  "silaba_inversa"},
        {"inversion_silabica",              "inversion_silabica"},
        {"terminacion_problematica",        "terminacion_problematica"},
        {"terminacion",                     "terminacion_problematica"},
        {"tilde_diacritica",                "tilde_diacritica"},
        {"acento_invalido_detectado_por_lexico", "acento_invalido_detectado_por_lexico"},
        {"ll_y",                            "homofono_ll_y"},
        {"homofono_b_v",                    "homofono_b_v"},
        {"homofono_ll_y",                   "homofono_ll_y"},
        {"homofono_s_c_z",                  "homofono_s_c_z"},
        {"homofono_g_j",                    "homofono_g_j"},
        {"homofono_otros",                  "homofono_otros"},
        {"confusion_visual_b_d_p_q",        "confusion_visual_b_d_p_q"},
        {"confusion_visual_otros",          "confusion_visual_otros"},
        // Sin peso propio:
        {"lista_curada",                    ""},
        {"sight_word",                      ""},
    };
    auto it = mapa.find(alias);
    if (it != mapa.end()) return it->second;
    return alias;
}

// ─────────────────────────────────────────────────────────────────────────
// Nivel del indicador
// ─────────────────────────────────────────────────────────────────────────
string AnalizadorSemantico::calcularNivel(double ind, string& desc) const {
    for (const auto& n : niveles_) {
        if (ind >= n.min && ind <= n.max) {
            desc = n.desc;
            return n.nivel;
        }
    }
    desc = "";
    return "DESCONOCIDO";
}

// ─────────────────────────────────────────────────────────────────────────
// Utilidades de patrón
// ─────────────────────────────────────────────────────────────────────────
bool AnalizadorSemantico::contienePatron(const string& palabra,
                                          const string& patron) const {
    return palabra.find(patron) != string::npos;
}

bool AnalizadorSemantico::iniciaCon(const string& palabra,
                                     const string& patron) const {
    if (patron.size() > palabra.size()) return false;
    return palabra.substr(0, patron.size()) == patron;
}

bool AnalizadorSemantico::terminaCon(const string& palabra,
                                      const string& patron) const {
    if (patron.size() > palabra.size()) return false;
    return palabra.substr(palabra.size() - patron.size()) == patron;
}

bool AnalizadorSemantico::tieneVocalDebilConTilde(const string& orig) const {
    static const vector<string> tildes_debiles = {
        "\xC3\xAD",  // í
        "\xC3\xBA",  // ú
        "\xC3\x8D",  // Í
        "\xC3\x9A",  // Ú
    };
    for (const auto& t : tildes_debiles)
        if (orig.find(t) != string::npos) return true;
    return false;
}

// ─────────────────────────────────────────────────────────────────────────
// Impresión de reporte en consola
// ─────────────────────────────────────────────────────────────────────────
string AnalizadorSemantico::colorCategoria(CategoriaRiesgo cat) const {
    switch (cat) {
        case CategoriaRiesgo::CONFUSION_VISUAL_B_D_P_Q: return RED;
        case CategoriaRiesgo::CONFUSION_VISUAL_OTROS:   return RED;
        case CategoriaRiesgo::HOMOFONO_B_V:
        case CategoriaRiesgo::HOMOFONO_LL_Y:
        case CategoriaRiesgo::HOMOFONO_S_C_Z:
        case CategoriaRiesgo::HOMOFONO_G_J:
        case CategoriaRiesgo::HOMOFONO_H_MUDA:
        case CategoriaRiesgo::HOMOFONO_OTROS:           return MAGENTA;
        case CategoriaRiesgo::SINFON_R:
        case CategoriaRiesgo::SINFON_L:                 return YELLOW;
        case CategoriaRiesgo::DIPTONGO:
        case CategoriaRiesgo::TRIPTONGO:                return CYAN;
        case CategoriaRiesgo::DIGRAFO:                  return BLUE;
        case CategoriaRiesgo::H_MUDA:
        case CategoriaRiesgo::CONTEXTO_DEPENDIENTE:     return GREEN;
        default:                                         return RESET;
    }
}

void AnalizadorSemantico::imprimirReporte(const ResultadoSemantico& res) const {
    

    if (res.palabrasRiesgo.empty()) {
        cout << GREEN << "No se detectaron palabras de riesgo.\n" << RESET;
    } else {
        cout << BOLD << "Palabras identificadas (" << res.palabrasRiesgo.size() << "):\n\n" << RESET;

        for (const auto& rp : res.palabrasRiesgo) {
            cout << BOLD << "  \"" << rp.valorOriginal << "\""
                 << RESET << "  (L" << rp.linea << ":C" << rp.columna << ")"
                 << "  peso=" << fixed << setprecision(2) << rp.pesoFinal;
            if (rp.esSightWord)   cout << "  [SIGHT_WORD x3]";
            if (rp.enListaCurada) cout << "  [CURADA +0.3]";
            if (rp.enFraseCita)   cout << "  [CITA x0.5]";
            cout << "\n";

            for (const auto& cat : rp.categorias) {
                cout << colorCategoria(cat)
                     << "    -> " << nombreCategoria(cat)
                     << RESET << "\n";
            }
            cout << "\n";
        }
    }

    cout << BOLD << "----------------------------------------\n" << RESET;
    cout << "Palabras analizables : " << res.totalPalabrasAnalizables << "\n";
    cout << "Suma de pesos        : " << fixed << setprecision(3) << res.sumaPesos << "\n";
    cout << "Errores ortograficos : " << res.erroresOrtograficos.size() << "\n";
    cout << "Componente errores : " << fixed << setprecision(2) << res.componenteErroresOrtograficos << "% (peso 80%)\n";
    cout << "Componente riesgo  : " << fixed << setprecision(2) << res.componentePalabrasRiesgo << "% (peso 20%)\n";
    cout << "Indicador porcentual : ";

    string color;
    if      (res.indicadorPorcentual <= 15) color = GREEN;
    else if (res.indicadorPorcentual <= 35) color = YELLOW;
    else if (res.indicadorPorcentual <= 60) color = RED;
    else                                    color = BOLD;

    cout << color << fixed << setprecision(2)
         << res.indicadorPorcentual << "%" << RESET << "\n";
    cout << "Nivel                : " << BOLD << res.nivelIndicador << RESET << "\n";
    cout << "                       " << res.descripcionNivel << "\n\n";
    cout << YELLOW << "NOTA: Indicador computacional de riesgo linguistico.\n"
         << "El diagnostico clinico requiere evaluacion por especialista.\n" << RESET;

    if (!res.erroresOrtograficos.empty()) {
        cout << BOLD << RED
             << "\n========================================\n"
             << " ERRORES ORTOGRAFICOS DISLEXICOS\n"
             << "========================================\n"
             << RESET;
        cout << BOLD << "Errores de escritura detectados ("
             << res.erroresOrtograficos.size() << "):\n\n" << RESET;

        for (const auto& err : res.erroresOrtograficos) {
            cout << RED << "  \"" << err.palabraEncontrada << "\""
                 << RESET << "  ->  deberia ser: "
                 << GREEN << "\"" << err.palabraCorrecta << "\""
                 << RESET
                 << "  (L" << err.linea << ":C" << err.columna << ")\n";
            cout << "      tipo: " << err.tipoError << "\n\n";
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Escritura de reporte a archivo .txt
// ─────────────────────────────────────────────────────────────────────────
void AnalizadorSemantico::escribirReporte(const ResultadoSemantico& res,
                                           const string& rutaSalida) const
{
    ofstream f(rutaSalida);
    if (!f.is_open())
        throw runtime_error("No se pudo crear: " + rutaSalida);

    f << " REPORTE DE ANALISIS DISLEXICO\n";
    f << "==========================================\n\n";

    f << "Palabras analizables : " << res.totalPalabrasAnalizables << "\n";
    f << "Palabras de riesgo   : " << res.palabrasRiesgo.size() << "\n";
    f << "Errores ortograficos : " << res.erroresOrtograficos.size() << "\n";
    f << "Componente errores   : " << fixed << setprecision(2) << res.componenteErroresOrtograficos << "% (peso 70%)\n";
    f << "Componente riesgo    : " << fixed << setprecision(2) << res.componentePalabrasRiesgo << "% (peso 30%)\n";
    f << "Indicador            : " << fixed << setprecision(2) << res.indicadorPorcentual << "%\n";
    f << "Nivel                : " << res.nivelIndicador << "\n";
    f << "Descripcion          : " << res.descripcionNivel << "\n\n";
    f << "=======================\n";
    f << "PALABRAS IDENTIFICADAS\n";

    for (const auto& rp : res.palabrasRiesgo) {
        f << "Palabra : " << rp.valorOriginal << "  (L" << rp.linea << ":C" << rp.columna << ")\n";
        f << "Normal  : " << rp.valorNormal << "\n";
        f << "Peso  : " << fixed << setprecision(2) << rp.pesoFinal << "\n";
        if (rp.esSightWord)   f << "Flags : SIGHT_WORD (x3.0)\n";
        if (rp.enListaCurada) f << "Flags : LISTA_CURADA (+0.3)\n";
        if (rp.enFraseCita)   f << "Flags : EN_CITA (x0.5)\n";
        f << "Tipo(s)  : ";
        for (size_t i = 0; i < rp.categorias.size(); i++) {
            if (i) f << ", ";
            f << nombreCategoria(rp.categorias[i]);
        }
        f << "\n\n";
    }

    if (!res.erroresOrtograficos.empty()) {
        f << "\n==========================================\n";
        f << "ERRORES ORTOGRAFICOS DISLEXICOS\n";
        f << "==========================================\n\n";
        for (const auto& err : res.erroresOrtograficos) {
            f << "Encontrado : " << err.palabraEncontrada
              << "  (L" << err.linea << ":C" << err.columna << ")\n";
            f << "Correcto   : " << err.palabraCorrecta << "\n";
            f << "Tipo error : " << err.tipoError << "\n\n";
        }
    }

    f << "NOTA CLINICA: Este es un indicador computacional basado en la estructura linguistica del texto. El diagnostico de dislexia requiere evaluacion por un especialista.\n";
}