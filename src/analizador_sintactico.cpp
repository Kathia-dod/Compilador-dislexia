#include "analizador_sintactico.h"

#include <iostream>

using namespace std;

// ─────────────────────────────────────────────
//  Colores ANSI
// ─────────────────────────────────────────────
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define DIM     "\033[2m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

// ─────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────
AnalizadorSintactico::AnalizadorSintactico(BufferTokens buffer)
    : buf(move(buffer)) {}

// ─────────────────────────────────────────────
// Helpers del buffer
// ─────────────────────────────────────────────
Token AnalizadorSintactico::actual() const {
    return buf.ver(0);
}

Token AnalizadorSintactico::consumir() {
    return buf.siguiente();
}

Token AnalizadorSintactico::ver(int adelanto) const {
    return buf.ver(adelanto);
}

bool AnalizadorSintactico::esFinLogico() const {
    return actual().tipo == TipoToken::FIN_ARCHIVO;
}

int AnalizadorSintactico::lineaActual() const {
    return actual().linea;
}

int AnalizadorSintactico::colActual() const {
    return actual().columna;
}

// ─────────────────────────────────────────────
// Saltar espacios
// ─────────────────────────────────────────────
void AnalizadorSintactico::saltarEspacios() {

    while (buf.hayMas()) {

        TipoToken t = actual().tipo;

        if (t == TipoToken::ESPACIO)
            consumir();
        else
            break;
    }
}

// ─────────────────────────────────────────────
// Errores
// ─────────────────────────────────────────────
void AnalizadorSintactico::registrarError(
    const string& desc,
    int linea,
    int col
) {
    errores.push_back({desc, linea, col});
}

const vector<ErrorSintactico>&
AnalizadorSintactico::obtenerErrores() const {
    return errores;
}

bool AnalizadorSintactico::tieneErrores() const {
    return !errores.empty();
}

// ─────────────────────────────────────────────
// Nombre de nodo
// ─────────────────────────────────────────────
string AnalizadorSintactico::nombreNodo(
    TipoNodo tipo
) const {

    switch (tipo) {

        case TipoNodo::DOCUMENTO:
            return "DOCUMENTO";

        case TipoNodo::PARRAFO:
            return "PARRAFO";

        case TipoNodo::ORACION:
            return "ORACION";

        case TipoNodo::FRASE_INTERROGATIVA:
            return "FRASE_INTERROGATIVA";

        case TipoNodo::FRASE_EXCLAMATIVA:
            return "FRASE_EXCLAMATIVA";

        case TipoNodo::FRASE_CITA:
            return "FRASE_CITA";

        case TipoNodo::FRASE_PARENTETICA:
            return "FRASE_PARENTETICA";

        case TipoNodo::DIALOGO:
            return "DIALOGO";

        case TipoNodo::ENUMERACION:
            return "ENUMERACION";

        case TipoNodo::PALABRA:
            return "PALABRA";

        case TipoNodo::NUMERO:
            return "NUMERO";

        case TipoNodo::PUNTUACION:
            return "PUNTUACION";

        case TipoNodo::ERROR_SINTACTICO:
            return "ERROR_SINTACTICO";

        default:
            return "???";
    }
}

// ─────────────────────────────────────────────
// Color nodo
// ─────────────────────────────────────────────
string AnalizadorSintactico::colorNodo(
    TipoNodo tipo
) const {

    switch (tipo) {

        case TipoNodo::DOCUMENTO:
            return BOLD WHITE;

        case TipoNodo::PARRAFO:
            return BOLD BLUE;

        case TipoNodo::ORACION:
            return CYAN;

        case TipoNodo::FRASE_INTERROGATIVA:
            return YELLOW;

        case TipoNodo::FRASE_EXCLAMATIVA:
            return MAGENTA;

        case TipoNodo::FRASE_CITA:
            return GREEN;

        case TipoNodo::FRASE_PARENTETICA:
            return GREEN;

        case TipoNodo::DIALOGO:
            return MAGENTA;

        case TipoNodo::ENUMERACION:
            return CYAN;

        case TipoNodo::PALABRA:
            return WHITE;

        case TipoNodo::NUMERO:
            return YELLOW;

        case TipoNodo::PUNTUACION:
            return DIM WHITE;

        case TipoNodo::ERROR_SINTACTICO:
            return BOLD RED;

        default:
            return RESET;
    }
}

// ─────────────────────────────────────────────
// Analizar
// ─────────────────────────────────────────────
NodoSintactico AnalizadorSintactico::analizar() {
    return parseDocumento();
}

// ─────────────────────────────────────────────
// Documento
// ─────────────────────────────────────────────
NodoSintactico AnalizadorSintactico::parseDocumento() {

    NodoSintactico doc;

    doc.tipo = TipoNodo::DOCUMENTO;
    doc.linea = 1;
    doc.columna = 1;

    while (!esFinLogico()) {

        saltarEspacios();

        if (esFinLogico())
            break;

        NodoSintactico p = parseParrafo();

        if (!p.hijos.empty())
            doc.hijos.push_back(p);
    }

    return doc;
}

// ─────────────────────────────────────────────
// Párrafo
// ─────────────────────────────────────────────
NodoSintactico AnalizadorSintactico::parseParrafo() {

    NodoSintactico nodo;

    nodo.tipo = TipoNodo::PARRAFO;
    nodo.linea = lineaActual();
    nodo.columna = colActual();

    while (!esFinLogico()) {

        if (actual().tipo == TipoToken::SALTO_LINEA) {
            consumir();

            if (actual().tipo == TipoToken::SALTO_LINEA) {
                consumir();
                break;
            }

            continue;
        }

        NodoSintactico o = parseOracion();

        if (!o.hijos.empty())
            nodo.hijos.push_back(o);
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Oración
// ─────────────────────────────────────────────
NodoSintactico AnalizadorSintactico::parseOracion() {

    NodoSintactico nodo;

    nodo.tipo = TipoNodo::ORACION;
    nodo.linea = lineaActual();
    nodo.columna = colActual();

    while (!esFinLogico()) {

        saltarEspacios();

        Token tok = actual();

        if (tok.tipo == TipoToken::PUNTO) {

            NodoSintactico p;

            p.tipo = TipoNodo::PUNTUACION;
            p.valor = tok.valorOriginal;
            p.linea = tok.linea;
            p.columna = tok.columna;

            nodo.hijos.push_back(p);

            consumir();

            break;
        }

        if (tok.tipo == TipoToken::SALTO_LINEA)
            break;

        NodoSintactico e = parseContenidoGeneral();

        if (!e.valor.empty())
            nodo.hijos.push_back(e);
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Contenido general
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseContenidoGeneral() {

    Token tok = actual();

    consumir();

    NodoSintactico nodo;

    nodo.linea = tok.linea;
    nodo.columna = tok.columna;

    if (tok.tipo == TipoToken::PALABRA) {

        nodo.tipo = TipoNodo::PALABRA;
        nodo.valor = tok.valorOriginal;
    }
    else if (tok.tipo == TipoToken::NUMERO) {

        nodo.tipo = TipoNodo::NUMERO;
        nodo.valor = tok.valorOriginal;
    }
    else {

        nodo.tipo = TipoNodo::PUNTUACION;
        nodo.valor = tok.valorOriginal;
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Stubs temporales
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseFraseInterrogativa() {
    return {};
}

NodoSintactico
AnalizadorSintactico::parseFraseExclamativa() {
    return {};
}

NodoSintactico
AnalizadorSintactico::parseFraseCita(
    TipoToken apertura,
    TipoToken cierre
) {
    return {};
}

NodoSintactico
AnalizadorSintactico::parseFraseParentetica() {
    return {};
}

NodoSintactico
AnalizadorSintactico::parseDialogo() {
    return {};
}

// ─────────────────────────────────────────────
// Imprimir árbol
// ─────────────────────────────────────────────
void AnalizadorSintactico::imprimirArbol(
    const NodoSintactico& nodo,
    int prof
) const {

    for (int i = 0; i < prof; i++) {

        if (i == prof - 1)
            cout << "  ├─ ";
        else
            cout << "  │  ";
    }

    if (prof == 0)
        cout << "  ";

    cout << colorNodo(nodo.tipo)
         << nombreNodo(nodo.tipo)
         << RESET;

    if (!nodo.valor.empty())
        cout << " \"" << nodo.valor << "\"";

    cout << "\n";

    for (const auto& hijo : nodo.hijos)
        imprimirArbol(hijo, prof + 1);
}