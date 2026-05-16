#include "analizador_lexico.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cctype>
using namespace std;

AnalizadorLexico::AnalizadorLexico(const string& rutaArchivo) : pos(0), lineaActual(1), columnaActual(1) {
    if (!cargarArchivo(rutaArchivo)) {
        throw runtime_error("No se pudo abrir el archivo: " + rutaArchivo);
    }
}

bool AnalizadorLexico::cargarArchivo(const string& ruta) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) return false;
    ostringstream buffer;
    buffer << archivo.rdbuf();
    contenido = buffer.str();
    return true;
}

char AnalizadorLexico::actual() {
    if (pos >= contenido.size()) return '\0';
    return contenido[pos];
}

void AnalizadorLexico::avanzar() {
    if (pos < contenido.size()) {
        if (contenido[pos] == '\n') {
            lineaActual++;
            columnaActual = 1;
        } else {
            columnaActual++;
        }
        pos++;
    }
}

bool AnalizadorLexico::esPalabraChar(unsigned char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c > 127);
}

bool AnalizadorLexico::esDigito(unsigned char c) {
    return c >= '0' && c <= '9';
}

Token AnalizadorLexico::leerPalabra(int linea, int col) {
    string palabra = "";
    while (pos < contenido.size() && esPalabraChar((unsigned char)actual())) {
        unsigned char uc = (unsigned char)actual();
        if (uc < 128) {
            palabra += (char)tolower(actual());
            avanzar();
        } else {
            // Detectar cuantos bytes ocupa el caracter UTF-8
            int bytes = 1;
            if((uc & 0xE0) == 0xC0) bytes = 2;
            else if((uc & 0xF0) == 0xE0) bytes = 3;
            else if((uc & 0xF8) == 0xF0) bytes = 4;

            for (int i = 0; i < bytes && pos < contenido.size(); i++) {
                palabra += actual();
                avanzar();
            }
        }
    }
    return {TipoToken::PALABRA, palabra, linea, col};
}

Token AnalizadorLexico::leerNumero(int linea, int col) {
    string num = "";
    while (pos < contenido.size() && esDigito((unsigned char)actual())) {
        num += actual();
        avanzar();
    }
    return {TipoToken::NUMERO, num, linea, col};
}

Token AnalizadorLexico::siguienteToken() {
    char c = actual();
    int linea = lineaActual;
    int col = columnaActual;

    if (c == '\0') return {TipoToken::FIN_ARCHIVO, "", linea, col};

// Espacio en blanco, tabulacion, salto de linea
    if (c == ' ' || c == '\t') {
        avanzar();
        return {TipoToken::ESPACIO, " ", linea, col};
    }

    if (c == '\n') {
        avanzar();
        return {TipoToken::SALTO_LINEA, "\\n", linea, col};
    }

// Palabras normales o con UTF-8
    if (esPalabraChar((unsigned char)c)) {
        return leerPalabra(linea, col);
    }

// Numeros
    if (esDigito((unsigned char)c)) {
        return leerNumero(linea, col);
    }

// Signos multibyte UTF-8
    if ((unsigned char)c == 0xC2 && pos + 1 < contenido.size()) {
        unsigned char sig = (unsigned char)contenido[pos + 1];
        if (sig == 0xBF) { avanzar(); avanzar(); return {TipoToken::SIGNO_INTERR_INI, "¿", linea, col}; }
        if (sig == 0xA1) { avanzar(); avanzar(); return {TipoToken::SIGNO_EXCL_INI, "¡", linea, col}; }
        if (sig == 0xAB) { avanzar(); avanzar(); return {TipoToken::COMILLA_ANGULAR_INI, "«", linea, col}; }
        if (sig == 0xBB) { avanzar(); avanzar(); return {TipoToken::COMILLA_ANGULAR_FIN, "»", linea, col}; }
    }

// Signos que empiezan en 0xE2 (3 bytes)
    if ((unsigned char)c == 0xE2 && pos + 2 < contenido.size()) {
        unsigned char b2 = (unsigned char)contenido[pos + 1];
        unsigned char b3 = (unsigned char)contenido[pos + 2];

    // € = 0xE2 0x82 0xAC
        if (b2 == 0x82 && b3 == 0xAC) { avanzar(); avanzar(); avanzar(); return {TipoToken::EURO, "€", linea, col}; }
    // ≠ = 0xE2 0x89 0xA0
        if (b2 == 0x89 && b3 == 0xA0) { avanzar(); avanzar(); avanzar(); return {TipoToken::DIFERENTE, "≠", linea, col}; }
    // — = 0xE2 0x80 0x94
        if (b2 == 0x80 && b3 == 0x94) { avanzar(); avanzar(); avanzar(); return {TipoToken::RAYA, "—", linea, col}; }
}

// Signos ASCII de un solo byte
    avanzar();
    switch (c) {
        case ',': return {TipoToken::COMA, ",", linea, col};
        case '.': return {TipoToken::PUNTO, ".", linea, col};
        case ';': return {TipoToken::PUNTO_Y_COMA, ";", linea, col};
        case ':': return {TipoToken::DOS_PUNTOS, ":", linea, col};
        case '?': return {TipoToken::SIGNO_INTERR_FIN,"?", linea, col};
        case '!': return {TipoToken::SIGNO_EXCL_FIN, "!", linea, col};
        case '_': return {TipoToken::GUION_BAJO, "_", linea, col};
        case '(': return {TipoToken::PARENTESIS_INI, "(", linea, col};
        case ')': return {TipoToken::PARENTESIS_FIN, ")", linea, col};
        case '[':  return {TipoToken::CORCHETE_INI, "[", linea, col};
        case ']':  return {TipoToken::CORCHETE_FIN, "]", linea, col};
        case '+': return {TipoToken::MAS, "+", linea, col};
        case '=': return {TipoToken::IGUAL, "=", linea, col};
        case '>': return {TipoToken::MAYOR_QUE, ">", linea, col};
        case '<': return {TipoToken::MENOR_QUE, "<", linea, col};
        case '/': return {TipoToken::BARRA, "/", linea, col};
        case '&': return {TipoToken::AMPERSAND, "&", linea, col};
        case '\'': return {TipoToken::COMILLA_SIMPLE, "'", linea, col};
        case '"': return {TipoToken::COMILLA_DOBLE, "\"", linea, col};
        case '*': return {TipoToken::ASTERISCO, "*", linea, col};
        case '\\': return {TipoToken::BARRA_INVERTIDA, "\\", linea, col};
        case '@': return {TipoToken::ARROBA, "@", linea, col};
        case '#': return {TipoToken::NUMERAL, "#", linea, col};
        case '~': return {TipoToken::VIRGULILLA, "~", linea, col};
        case '$':  return {TipoToken::DOLAR, "$", linea, col};
        case '%':  return {TipoToken::PORCENTAJE, "%", linea, col};
        default: return {TipoToken::DESCONOCIDO, string(1, c), linea, col};
    }
}

vector<Token> AnalizadorLexico::tokenizar() {
    vector<Token> tokens;
    pos = 0;
    lineaActual = 1;
    columnaActual = 1;

    while (true) {
        Token t = siguienteToken();
        tokens.push_back(t);
        if (t.tipo == TipoToken::FIN_ARCHIVO) break;
    }

    return tokens;
}