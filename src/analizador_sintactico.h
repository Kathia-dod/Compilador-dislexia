#ifndef ANALIZADOR_SINTACTICO_H
#define ANALIZADOR_SINTACTICO_H

#include "analizador_lexico.h"
#include <string>
#include <vector>

using namespace std;

// ─────────────────────────────────────────────
//  Tipos de nodo del árbol sintáctico
// ─────────────────────────────────────────────
enum class TipoNodo {
    DOCUMENTO,
    PARRAFO,
    ORACION,
    FRASE_INTERROGATIVA,
    FRASE_EXCLAMATIVA,
    FRASE_CITA,
    FRASE_PARENTETICA,
    DIALOGO,
    ENUMERACION,
    PALABRA,
    NUMERO,
    PUNTUACION,
    ERROR_SINTACTICO
};

struct NodoSintactico {
    TipoNodo tipo;
    string valor;
    string valorOriginal;
    int linea;
    int columna;
    vector<NodoSintactico> hijos;
};

struct ErrorSintactico {
    string descripcion;
    int linea;
    int columna;
};

// ─────────────────────────────────────────────
//  Analizador Sintáctico
// ─────────────────────────────────────────────
class AnalizadorSintactico {
private:
    BufferTokens buf;
    vector<ErrorSintactico> errores;

public:
    explicit AnalizadorSintactico(BufferTokens buffer);

    NodoSintactico analizar();

    void imprimirArbol(const NodoSintactico& nodo,
                       int profundidad = 0) const;

    const vector<ErrorSintactico>& obtenerErrores() const;

    bool tieneErrores() const;

private:

    // ─────────────────────────
    // Helpers del buffer
    // ─────────────────────────
    Token actual() const;
    Token consumir();
    Token ver(int adelanto = 0) const;

    bool esFinLogico() const;

    void saltarEspacios();

    bool esTokenDeCierre(TipoToken esperado) const;

    // ─────────────────────────
    // Reglas gramaticales
    // ─────────────────────────
    NodoSintactico parseDocumento();
    NodoSintactico parseParrafo();
    NodoSintactico parseOracion();

    NodoSintactico parseElemento();

    NodoSintactico parseFraseInterrogativa();
    NodoSintactico parseFraseExclamativa();

    NodoSintactico parseFraseCita(
        TipoToken apertura,
        TipoToken cierre
    );

    NodoSintactico parseFraseParentetica();

    NodoSintactico parseDialogo();

    NodoSintactico parseContenidoGeneral();

    // ─────────────────────────
    // Utilidades
    // ─────────────────────────
    void registrarError(
        const string& desc,
        int linea,
        int col
    );

    string nombreNodo(TipoNodo tipo) const;

    string colorNodo(TipoNodo tipo) const;

    int lineaActual() const;
    int colActual() const;
};

#endif