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
            p.valorOriginal = tok.valorOriginal;
            p.valor = tok.valorOriginal;
            p.linea = tok.linea;
            p.columna = tok.columna;

            nodo.hijos.push_back(p);

            consumir();

            break;
        }

        if (tok.tipo == TipoToken::SALTO_LINEA)
            break;

        NodoSintactico e = parseElemento();

        if (!e.valor.empty() || !e.hijos.empty())
            nodo.hijos.push_back(e);
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Elemento (dispatcher central de la gramática)
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseElemento() {

    saltarEspacios();

    Token tok = actual();

    switch (tok.tipo) {

        case TipoToken::SIGNO_INTERR_INI:
            return parseFraseInterrogativa();

        case TipoToken::SIGNO_EXCL_INI:
            return parseFraseExclamativa();

        case TipoToken::COMILLA_DOBLE:
            return parseFraseCita(TipoToken::COMILLA_DOBLE,
                                   TipoToken::COMILLA_DOBLE);

        case TipoToken::COMILLA_SIMPLE:
            return parseFraseCita(TipoToken::COMILLA_SIMPLE,
                                   TipoToken::COMILLA_SIMPLE);

        case TipoToken::COMILLA_ANGULAR_INI:
            return parseFraseCita(TipoToken::COMILLA_ANGULAR_INI,
                                   TipoToken::COMILLA_ANGULAR_FIN);

        case TipoToken::PARENTESIS_INI:
            return parseFraseParentetica();

        case TipoToken::RAYA:
            return parseDialogo();

        default:
            return parseContenidoGeneral();
    }
}

// ─────────────────────────────────────────────
// Contenido general (caso base: hoja del árbol)
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseContenidoGeneral() {

    Token tok = actual();

    consumir();

    NodoSintactico nodo;

    nodo.linea = tok.linea;
    nodo.columna = tok.columna;

    if (tok.tipo == TipoToken::PALABRA ||
        tok.tipo == TipoToken::PALABRA_COMPUESTA) {

        nodo.tipo = TipoNodo::PALABRA;
        nodo.valor = tok.valorNormal;
        nodo.valorOriginal = tok.valorOriginal;
    }
    else if (tok.tipo == TipoToken::NUMERO ||
             tok.tipo == TipoToken::NUMERO_DECIMAL) {

        nodo.tipo = TipoNodo::NUMERO;
        nodo.valor = tok.valorOriginal;
        nodo.valorOriginal = tok.valorOriginal;    }
    else {

        nodo.tipo = TipoNodo::PUNTUACION;
        nodo.valor = tok.valorOriginal;
        nodo.valorOriginal = tok.valorOriginal;
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Utilidad: ¿el token actual es el cierre esperado?
// ─────────────────────────────────────────────
bool AnalizadorSintactico::esTokenDeCierre(
    TipoToken esperado
) const {
    return actual().tipo == esperado;
}

// ─────────────────────────────────────────────
// Frase interrogativa: ¿ ... ?
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseFraseInterrogativa() {

    Token apertura = actual();

    NodoSintactico nodo;
    nodo.tipo = TipoNodo::FRASE_INTERROGATIVA;
    nodo.linea = apertura.linea;
    nodo.columna = apertura.columna;

    consumir(); // ¿

    while (!esFinLogico() &&
           !esTokenDeCierre(TipoToken::SIGNO_INTERR_FIN) &&
           actual().tipo != TipoToken::SALTO_LINEA) {

        nodo.hijos.push_back(parseElemento());
    }

    if (esTokenDeCierre(TipoToken::SIGNO_INTERR_FIN)) {
        consumir(); // ?
    } else {
        registrarError(
            "Falta cierre '?' para interrogacion abierta",
            apertura.linea, apertura.columna
        );
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Frase exclamativa: ¡ ... !
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseFraseExclamativa() {

    Token apertura = actual();

    NodoSintactico nodo;
    nodo.tipo = TipoNodo::FRASE_EXCLAMATIVA;
    nodo.linea = apertura.linea;
    nodo.columna = apertura.columna;

    consumir(); // ¡

    while (!esFinLogico() &&
           !esTokenDeCierre(TipoToken::SIGNO_EXCL_FIN) &&
           actual().tipo != TipoToken::SALTO_LINEA) {

        nodo.hijos.push_back(parseElemento());
    }

    if (esTokenDeCierre(TipoToken::SIGNO_EXCL_FIN)) {
        consumir(); // !
    } else {
        registrarError(
            "Falta cierre '!' para exclamacion abierta",
            apertura.linea, apertura.columna
        );
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Frase entre comillas (genérica: " ', « »)
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseFraseCita(
    TipoToken apertura,
    TipoToken cierre
) {
    Token tokApertura = actual();

    NodoSintactico nodo;
    nodo.tipo = TipoNodo::FRASE_CITA;
    nodo.linea = tokApertura.linea;
    nodo.columna = tokApertura.columna;

    consumir(); // comilla de apertura

    while (!esFinLogico() &&
           !esTokenDeCierre(cierre) &&
           actual().tipo != TipoToken::SALTO_LINEA) {

        nodo.hijos.push_back(parseElemento());
    }

    if (esTokenDeCierre(cierre)) {
        consumir(); // comilla de cierre
    } else {
        registrarError(
            "Falta cierre de comilla para cita abierta",
            tokApertura.linea, tokApertura.columna
        );
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Frase parentética: ( ... )
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseFraseParentetica() {

    Token apertura = actual();

    NodoSintactico nodo;
    nodo.tipo = TipoNodo::FRASE_PARENTETICA;
    nodo.linea = apertura.linea;
    nodo.columna = apertura.columna;

    consumir(); // (

    while (!esFinLogico() &&
           !esTokenDeCierre(TipoToken::PARENTESIS_FIN) &&
           actual().tipo != TipoToken::SALTO_LINEA) {

        nodo.hijos.push_back(parseElemento());
    }

    if (esTokenDeCierre(TipoToken::PARENTESIS_FIN)) {
        consumir(); // )
    } else {
        registrarError(
            "Falta cierre ')' para parentesis abierto",
            apertura.linea, apertura.columna
        );
    }

    return nodo;
}

// ─────────────────────────────────────────────
// Diálogo: — ...   (cierra en PUNTO o SALTO_LINEA,
// sin consumirlos: los maneja parseOracion)
// ─────────────────────────────────────────────
NodoSintactico
AnalizadorSintactico::parseDialogo() {

    Token apertura = actual();

    NodoSintactico nodo;
    nodo.tipo = TipoNodo::DIALOGO;
    nodo.linea = apertura.linea;
    nodo.columna = apertura.columna;

    consumir(); // —

    while (!esFinLogico() &&
           actual().tipo != TipoToken::PUNTO &&
           actual().tipo != TipoToken::SALTO_LINEA) {

        nodo.hijos.push_back(parseElemento());
    }

    return nodo;
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

    if (!nodo.valorOriginal.empty())
        cout << " \"" << nodo.valorOriginal << "\"";
    else if (!nodo.valor.empty())
        cout << " \"" << nodo.valor << "\"";

    cout << "\n";

    for (const auto& hijo : nodo.hijos)
        imprimirArbol(hijo, prof + 1);
}