#ifndef ANALIZADOR_SEMANTICO_H
#define ANALIZADOR_SEMANTICO_H

#include "analizador_sintactico.h"   // NodoSintactico, TipoNodo
#include "analizador_lexico.h"       // Token, TipoToken

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

using namespace std;

// Categorías de reporte
enum class CategoriaRiesgo {
    CONFUSION_VISUAL_B_D_P_Q,
    CONFUSION_VISUAL_OTROS,
    HOMOFONO_B_V,
    HOMOFONO_LL_Y,
    HOMOFONO_S_C_Z,
    HOMOFONO_G_J,
    HOMOFONO_H_MUDA,
    HOMOFONO_OTROS,
    TILDE_DIACRITICA,
    DIGRAFO,
    SINFON_R,
    SINFON_L,
    DIPTONGO,
    TRIPTONGO,
    CONTEXTO_DEPENDIENTE,
    SILABA_INVERSA,
    INVERSION_SILABICA,
    TERMINACION_PROBLEMATICA,
    LONGITUD,
    H_MUDA,
    LISTA_CURADA,
    SIGHT_WORD,
    ACENTO_INVALIDO
};

string nombreCategoria(CategoriaRiesgo cat);

struct ResultadoPalabra {
    string valorOriginal;
    string valorNormal;        // minúsculas + sin tildes de mayúscula
    int linea;
    int columna;

    vector<CategoriaRiesgo> categorias;

    double pesoFinal = 0.0;

    // Flags
    bool esSightWord    = false;
    bool enListaCurada  = false;
    bool enFraseCita    = false;  
};

struct ResultadoSemantico {
    vector<ResultadoPalabra> palabrasRiesgo;

    int totalPalabrasAnalizables = 0;
    double sumaPesos             = 0.0;

    double indicadorPorcentual   = 0.0;

    string nivelIndicador;        // BAJO / MODERADO / ALTO / MUY_ALTO
    string descripcionNivel;
};

class AnalizadorSemantico {
public:
    // El constructor recibe el AST raíz y los JSON ya cargados como strings.
    explicit AnalizadorSemantico(
        const NodoSintactico& ast,
        const string& jsonPonderacion,      // modelo_ponderacion.json
        const string& jsonReglas,           // reglas_deteccion.json
        const string& jsonPares,            // pares_visuales.json
        const string& jsonSilabicos,        // patrones_silabicos.json
        const string& jsonHomofonos,        // homofonos.json
        const string& jsonPalabrasRiesgo,   // palabras_riesgo.json
        const string& jsonSightWords,       // sight_words.json
        const string& jsonFalsosPositivos   // falsos_positivos.json
    );

    ResultadoSemantico analizar();

    void imprimirReporte(const ResultadoSemantico& res) const;

    void escribirReporte(const ResultadoSemantico& res,
                         const string& rutaSalida) const;

private:
    // ── AST ────────────────────────────────────────────────────────────────
    const NodoSintactico& ast_;

    // ── Tablas cargadas desde JSON ─────────────────────────────────────────
    unordered_set<char>   grafemas_alta_;      
    unordered_set<char>   grafemas_alta_bdpq_;

    vector<string> patrones_sinfon_r_;
    vector<string> patrones_sinfon_l_;
    vector<string> patrones_diptongo_;
    vector<string> patrones_triptongo_;
    vector<string> patrones_digrafo_;
    vector<string> patrones_silaba_inversa_;
    vector<string> patrones_terminacion_;

    vector<pair<string,string>> pares_inversion_;

    unordered_map<string,string> tabla_homofonos_; 

    unordered_set<string> lista_curada_;

    unordered_set<string> sight_words_;

    unordered_set<string> falsos_positivos_;

    // modelo_ponderacion
    unordered_map<string,double> pesos_; 
    double modificador_sight_word_ = 3.0;
    double modificador_multi_bonus_ = 0.5;
    double modificador_curada_bonus_= 0.3;

    struct NivelIndicador { int min; int max; string nivel; string desc; };
    vector<NivelIndicador> niveles_;

    // ── Métodos privados ───────────────────────────────────────────────────
    void cargarPonderacion(const string& json);
    void cargarPares(const string& json);
    void cargarSilabicos(const string& json);
    void cargarHomofonos(const string& json);
    void cargarPalabrasRiesgo(const string& json);
    void cargarSightWords(const string& json);
    void cargarFalsosPositivos(const string& json);

    void recorrerNodo(const NodoSintactico& nodo, bool dentroFraseCita, ResultadoSemantico& res);

    void analizarPalabra(const NodoSintactico& nodo, bool dentroFraseCita, ResultadoSemantico& res);

    bool esFalsoPositivo(const NodoSintactico& nodo) const;

    void aplicarReglas(const string& valorNormal, const string& valorOriginal, vector<CategoriaRiesgo>& cats) const;

    void reglaR01_confusionVisual(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR02_sinfonR(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR03_sinfonL(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR04_diptongo(const string& vNorm, const string& vOrig, vector<CategoriaRiesgo>& c) const;
    void reglaR05_digrafo(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR06_homofono(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR07_palabraLarga(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR08_hInicial(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR09_contextoC(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR10_contextoG(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR11_silabaInversa(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR12_listaCurada(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR13_inversionSilabica(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR14_sightWordConRiesgo(const string& v, bool tieneRiesgo, vector<CategoriaRiesgo>& c) const;
    void reglaR15_triptongo(const string& v, vector<CategoriaRiesgo>& c) const;
    void reglaR16_terminacion(const string& v, vector<CategoriaRiesgo>& c) const;

    bool contienePatron(const string& palabra, const string& patron) const;
    bool iniciaCon(const string& palabra, const string& patron) const;
    bool terminaCon(const string& palabra, const string& patron) const;
    bool tieneVocalDebilConTilde(const string& valorOriginal) const;

    int contarSilabas(const string& valorNormal) const;

    double calcularPeso(const ResultadoPalabra& rp) const;

    string resolverAlias(const string& alias) const;

    string calcularNivel(double indicador, string& descripcion) const;

    string colorCategoria(CategoriaRiesgo cat) const;
};

#endif 