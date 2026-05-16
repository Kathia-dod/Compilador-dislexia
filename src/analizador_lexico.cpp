#include "analizador_lexico.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cctype>
#include <unordered_map>
using namespace std;

// map de signos ASCII
static const unordered_map<char, TipoToken> MAP_ASCII = {
    {',', TipoToken::COMA},
    {'.', TipoToken::PUNTO},
    {';', TipoToken::PUNTO_Y_COMA},
    {':', TipoToken::DOS_PUNTOS},
    {'?', TipoToken::SIGNO_INTERR_FIN},
    {'!', TipoToken::SIGNO_EXCL_FIN},
    {'_', TipoToken::GUION_BAJO},
    {'(', TipoToken::PARENTESIS_INI},
    {')', TipoToken::PARENTESIS_FIN},
    {'[', TipoToken::CORCHETE_INI},
    {']', TipoToken::CORCHETE_FIN},
    {'{', TipoToken::LLAVE_INI},
    {'}', TipoToken::LLAVE_FIN},
    {'+', TipoToken::MAS},
    {'-', TipoToken::GUION},
    {'=', TipoToken::IGUAL},
    {'>', TipoToken::MAYOR_QUE},
    {'<', TipoToken::MENOR_QUE},
    {'/', TipoToken::BARRA},
    {'&', TipoToken::AMPERSAND},
    {'\'', TipoToken::COMILLA_SIMPLE},
    {'"', TipoToken::COMILLA_DOBLE},
    {'*', TipoToken::ASTERISCO},
    {'\\', TipoToken::BARRA_INVERTIDA},
    {'@', TipoToken::ARROBA},
    {'#', TipoToken::NUMERAL},
    {'~', TipoToken::VIRGULILLA},
    {'$', TipoToken::DOLAR},
    {'%', TipoToken::PORCENTAJE}, 
    
};

// map de signos multibyte UTF-8, secuencia de bytes como string, valor: TipoToken, simbolo
struct InfoMultibyte {
    TipoToken tipo;
    string representacion;
};

static const vector<pair<string, InfoMultibyte>> MAP_MULTIBYTE = {
    {string("\xC2\xBF"), {TipoToken::SIGNO_INTERR_INI, "¿"}},
    {string("\xC2\xA1"), {TipoToken::SIGNO_EXCL_INI, "¡"}},
    {string("\xC2\xAB"), {TipoToken::COMILLA_ANGULAR_INI, "«"}},
    {string("\xC2\xBB"), {TipoToken::COMILLA_ANGULAR_FIN, "»"}},
    {string("\xE2\x82\xAC"), {TipoToken::EURO, "€"}},
    {string("\xE2\x89\xA0"), {TipoToken::DIFERENTE, "≠"}},
    {string("\xE2\x80\x94"), {TipoToken::RAYA, "—"}},
};

string nombreTipo(TipoToken tipo) {
    static const unordered_map<TipoToken, string> nombres = {
        {TipoToken::PALABRA, "TOKEN_PALABRA"},
        {TipoToken::NUMERO, "TOKEN_NUMERO"},
        {TipoToken::COMA, "TOKEN_COMA"},
        {TipoToken::PUNTO, "TOKEN_PUNTO"},
        {TipoToken::PUNTOS_SUSPENSIVOS, "TOKEN_PUNTOS_SUSPENSIVOS"},
        {TipoToken::PUNTO_Y_COMA, "TOKEN_PUNTO_Y_COMA"},
        {TipoToken::DOS_PUNTOS, "TOKEN_DOS_PUNTOS"},
        {TipoToken::SIGNO_INTERR_INI, "TOKEN_INTERR_INI"},
        {TipoToken::SIGNO_INTERR_FIN, "TOKEN_INTERR_FIN"},
        {TipoToken::SIGNO_EXCL_INI, "TOKEN_EXCL_INI"},
        {TipoToken::SIGNO_EXCL_FIN, "TOKEN_EXCL_FIN"},
        {TipoToken::GUION_BAJO, "TOKEN_GUION_BAJO"},
        {TipoToken::PARENTESIS_INI, "TOKEN_PARENTESIS_INI"},
        {TipoToken::PARENTESIS_FIN, "TOKEN_PARENTESIS_FIN"},
        {TipoToken::CORCHETE_INI, "TOKEN_CORCHETE_INI"},
        {TipoToken::CORCHETE_FIN, "TOKEN_CORCHETE_FIN"},
        {TipoToken::LLAVE_INI, "TOKEN_LLAVE_INI"},
        {TipoToken::LLAVE_FIN, "TOKEN_LLAVE_FIN"},
        {TipoToken::MAS, "TOKEN_MAS"},
        {TipoToken::GUION, "TOKEN_GUION"},
        {TipoToken::IGUAL, "TOKEN_IGUAL"},
        {TipoToken::MAYOR_QUE, "TOKEN_MAYOR_QUE"},
        {TipoToken::MENOR_QUE, "TOKEN_MENOR_QUE"},
        {TipoToken::BARRA, "TOKEN_BARRA"},
        {TipoToken::AMPERSAND, "TOKEN_AMPERSAND"},
        {TipoToken::COMILLA_SIMPLE, "TOKEN_COMILLA_SIMPLE"},
        {TipoToken::COMILLA_DOBLE, "TOKEN_COMILLA_DOBLE"},
        {TipoToken::ASTERISCO, "TOKEN_ASTERISCO"},
        {TipoToken::BARRA_INVERTIDA, "TOKEN_BARRA_INVERTIDA"},
        {TipoToken::ARROBA, "TOKEN_ARROBA"},
        {TipoToken::NUMERAL, "TOKEN_NUMERAL"},
        {TipoToken::VIRGULILLA, "TOKEN_VIRGULILLA"},
        {TipoToken::DOLAR, "TOKEN_DOLAR"},
        {TipoToken::EURO, "TOKEN_EURO"},
        {TipoToken::PORCENTAJE, "TOKEN_PORCENTAJE"},
        {TipoToken::DIFERENTE, "TOKEN_DIFERENTE"},
        {TipoToken::COMILLA_ANGULAR_INI, "TOKEN_COMILLA_ANGULAR_INI"},
        {TipoToken::COMILLA_ANGULAR_FIN, "TOKEN_COMILLA_ANGULAR_FIN"},
        {TipoToken::RAYA, "TOKEN_RAYA"},
        {TipoToken::SALTO_LINEA, "TOKEN_SALTO_LINEA"},
        {TipoToken::ESPACIO, "TOKEN_ESPACIO"},
        {TipoToken::DESCONOCIDO, "TOKEN_DESCONOCIDO"},
        {TipoToken::FIN_ARCHIVO, "TOKEN_FIN_ARCHIVO"},
    };

    auto it = nombres.find(tipo);
    if (it != nombres.end()) return it->second;
    return "TOKEN_???";
}

AnalizadorLexico::AnalizadorLexico(const string& rutaArchivo) : pos(0), lineaActual(1), columnaActual(1),
      esperandoInicioOracion(true),
      esperandoInicioLinea(true),
      esperandoInicioParrafo(true),
      saltosConsecutivos(0) {
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
            // al encontrar un salto de linea se activa la bandera de inicio de linea y se cuenta cuantos saltos consecutivos hay para detectar parrafos
            esperandoInicioLinea = true;
            saltosConsecutivos++;
            if (saltosConsecutivos >= 2) {
                esperandoInicioParrafo = true;
            }
        } else {
            columnaActual++;
        }
        pos++;
    }
}

void AnalizadorLexico::registrarError(const string& desc, int linea, int col) {
    errores.push_back({desc, linea, col});
}

const vector<ErrorLexico>& AnalizadorLexico::obtenerErrores() const {
    return errores;
}

bool AnalizadorLexico::tieneErrores() const {
    return !errores.empty();
}

bool AnalizadorLexico::esPalabraChar(unsigned char c) {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c < 128) return false;

    for (const auto& entrada : MAP_MULTIBYTE) {
        const string& sec = entrada.first;
        if (!sec.empty() && (unsigned char)sec[0] == c) {
            return false;
        }
    }

    return true; 
}

bool AnalizadorLexico::esDigito(unsigned char c) {
    return c >= '0' && c <= '9';
}

// normaliza a minusculas respetando multibyte
string AnalizadorLexico::normalizar(const string& texto) {
    string resultado = "";
    size_t i = 0;
    while (i < texto.size()) {
        unsigned char uc = (unsigned char)texto[i];
        if (uc < 128) {
            resultado += (char)tolower(texto[i]);
            i++;
        } else {
            // caracteres multibyte se copian sin tocar
            int bytes = 1;
            if ((uc & 0xE0) == 0xC0) bytes = 2;
            else if ((uc & 0xF0) == 0xE0) bytes = 3;
            else if ((uc & 0xF8) == 0xF0) bytes = 4;
            for (int j = 0; j < bytes && i < texto.size(); j++, i++) {
                resultado += texto[i];
            }
        }
    }
    return resultado;
}

// devuelve true si el token es un signo de apertura
bool AnalizadorLexico::esSignoDeApertura(TipoToken tipo) const {
    switch (tipo) {
        case TipoToken::PARENTESIS_INI:
        case TipoToken::CORCHETE_INI:
        case TipoToken::LLAVE_INI:
        case TipoToken::COMILLA_SIMPLE:
        case TipoToken::COMILLA_DOBLE:
        case TipoToken::COMILLA_ANGULAR_INI:
        case TipoToken::SIGNO_INTERR_INI:
        case TipoToken::SIGNO_EXCL_INI:
            return true;
        default:
            return false;
    }
}

// lee la palabra guardando: original y normalizada
Token AnalizadorLexico::leerPalabra(int linea, int col) {
    string original = "";

    while (pos < contenido.size() && esPalabraChar((unsigned char)actual())) {
        unsigned char uc = (unsigned char)actual();
        if (uc < 128) {
            original += actual();
            avanzar();
        } else {
            int bytes = 1;
            if ((uc & 0xE0) == 0xC0) bytes = 2;
            else if ((uc & 0xF0) == 0xE0) bytes = 3;
            else if ((uc & 0xF8) == 0xF0) bytes = 4;
            for (int i = 0; i < bytes && pos < contenido.size(); i++) {
                original += actual();
                avanzar();
            }
        }
    }

    bool esInicioOracion = esperandoInicioOracion;
    bool esInicioLinea = esperandoInicioLinea;
    bool esInicioParrafo = esperandoInicioParrafo;

    esperandoInicioOracion = false;
    esperandoInicioLinea = false;
    esperandoInicioParrafo = false;
    saltosConsecutivos = 0;

    return {TipoToken::PALABRA, original, normalizar(original), linea, col, esInicioOracion, esInicioLinea, esInicioParrafo};
}

Token AnalizadorLexico::leerNumero(int linea, int col) {
    string num = "";
    while (pos < contenido.size() && esDigito((unsigned char)actual())) {
        num += actual();
        avanzar();
    }
    // los numeros marcan INICIO_LINEA
    bool esInicioOracion = esperandoInicioOracion;
    bool esInicioLinea = esperandoInicioLinea;
    bool esInicioParrafo = esperandoInicioParrafo;

    esperandoInicioOracion = false;
    esperandoInicioLinea = false;
    esperandoInicioParrafo = false;
    saltosConsecutivos = 0;

    return {TipoToken::NUMERO, num, num, linea, col, esInicioOracion, esInicioLinea, esInicioParrafo};
}

// lógica multibyte 
bool AnalizadorLexico::leerSignoMultibyte(int linea, int col, Token& resultado) {
    for (const auto& entrada : MAP_MULTIBYTE) {
        const string& secuencia = entrada.first;
        const InfoMultibyte& info = entrada.second;
        size_t len = secuencia.size();

        if (pos + len <= contenido.size() && contenido.substr(pos, len) == secuencia) {
            for (size_t i = 0; i < len; i++) 
                avanzar();
            resultado = {info.tipo, info.representacion, info.representacion, linea, col, false, false, false};
            return true;
        }
    }
    return false;
}

Token AnalizadorLexico::siguienteToken() {
    char c = actual();
    int linea = lineaActual;
    int col = columnaActual;

    if (c == '\0') return {TipoToken::FIN_ARCHIVO, "", "", linea, col, false, false, false};

    if (c == ' ' || c == '\t') {
        avanzar();
        return {TipoToken::ESPACIO, " ", " ", linea, col, false, false, false};
    }

    if (c == '\n') {
        avanzar();
        return {TipoToken::SALTO_LINEA, "\\n", "\\n", linea, col, false, false, false};
    }

    if (esPalabraChar((unsigned char)c)) {
        return leerPalabra(linea, col);
    }

    if (esDigito((unsigned char)c)) {
        return leerNumero(linea, col);
    }

    // intentar multibyte antes de ASCII
    Token multibyte;
    if (leerSignoMultibyte(linea, col, multibyte)) {
        if (multibyte.tipo == TipoToken::SIGNO_INTERR_INI || multibyte.tipo == TipoToken::SIGNO_EXCL_INI) {
            esperandoInicioOracion = true;
        }
        // la raya (—) marca inicio de oracion como dialogo, excepto si esta entre dos digitos
        if (multibyte.tipo == TipoToken::RAYA) {
            char anteriorR = (pos >= 4) ? contenido[pos - 4] : '\0';
            char siguienteR = actual();
            if (!(esDigito((unsigned char)anteriorR) && esDigito((unsigned char)siguienteR))) {
                esperandoInicioOracion = true;
            }
        }
        // si el signo multibyte es de apertura, no consume las banderas de contexto, las deja activas para que la primera palabra real las reciba
        if (!esSignoDeApertura(multibyte.tipo)) {
            saltosConsecutivos = 0;
        }
        return multibyte;
    }

    // signo ASCII via map
    avanzar();
    auto it = MAP_ASCII.find(c);
    if (it != MAP_ASCII.end()) {
        string rep(1, c);

        // punto y signo de exclamación/interrogación marcan fin de oración
        TipoToken tipo = it->second; 
        bool marcarInicioOracion = false;

        if (tipo == TipoToken::PUNTO) {

            char anterior = '\0';
            char siguiente = actual();

            if (pos >= 2) 
                anterior = contenido[pos - 2];

            bool anteriorAlnum = isalnum((unsigned char)anterior);
            bool siguienteAlnum = isalnum((unsigned char)siguiente);

    // Caso 1: prueba1.txt 3.14 U.S.A
            if (anteriorAlnum && siguienteAlnum) {
                marcarInicioOracion = false;
            }

            else if (siguiente == '.') {
                marcarInicioOracion = false;
            }

    // Caso normal: fin de oración
            else {
                marcarInicioOracion = true;
            }
        }

        else if (tipo == TipoToken::SIGNO_INTERR_FIN || tipo == TipoToken::SIGNO_EXCL_FIN || tipo == TipoToken::DOS_PUNTOS) 
            marcarInicioOracion = true;

        // el guion (-) marca inicio de oracion como dialogo o cita, excepto si esta entre dos digitos (ej: 999-123-456 telefono) o si no le sigue un espacio
        // (ej: -esta roto- es cita en medio de oracion, no dialogo)

        else if (tipo == TipoToken::GUION) {
            char anteriorG = (pos >= 2) ? contenido[pos - 2] : '\0';
            char siguienteG = actual();
            bool entreDigitos = esDigito((unsigned char)anteriorG) && esDigito((unsigned char)siguienteG);
            bool sigueEspacio = (siguienteG == ' ' || siguienteG == '\t');
            if (!entreDigitos && sigueEspacio) {
                marcarInicioOracion = true;
            }
        }

        if (marcarInicioOracion) 
            esperandoInicioOracion = true;

        // si el signo ASCII es de apertura, no consume las banderas de contexto, las deja activas para que la primera palabra real las reciba.
        // para signos que no son de apertura, si se resetea el contador de saltos.
        if (!esSignoDeApertura(tipo)) {
            saltosConsecutivos = 0;
        }

        return {tipo, rep, rep, linea, col, false, false, false};
    }

    // caracter desconocido: registra error y sigue
    string rep(1, c);
    registrarError("Caracter desconocido: '" + rep + "'", linea, col);
    saltosConsecutivos = 0;
    return {TipoToken::DESCONOCIDO, rep, rep, linea, col, false, false, false};
}

BufferTokens AnalizadorLexico::tokenizar() {
    vector<Token> tokens;
    pos = 0;
    lineaActual = 1;
    columnaActual = 1;
    esperandoInicioOracion = true;
    esperandoInicioLinea = true;
    esperandoInicioParrafo = true;
    saltosConsecutivos = 0;

    // se detectan los puntos suspensivos como un token propio para evitar que el punto individual los procese mal.
    while (true) {
        // deteccion de puntos suspensivos con lookahead de 2 posiciones: si el caracter actual y los dos siguientes son '.', se consume como una unidad y marca inicio de oracion 
        if (actual() == '.' &&
            pos + 1 < contenido.size() && contenido[pos + 1] == '.' &&
            pos + 2 < contenido.size() && contenido[pos + 2] == '.') {

            int linea = lineaActual;
            int col = columnaActual;
            avanzar(); avanzar(); avanzar(); // consumir los tres puntos
            esperandoInicioOracion = true;
            saltosConsecutivos = 0;
            tokens.push_back({TipoToken::PUNTOS_SUSPENSIVOS, "...", "...", linea, col, false, false, false});
            continue;
        }

        Token t = siguienteToken();
        tokens.push_back(t);
        if (t.tipo == TipoToken::FIN_ARCHIVO) 
            break;
    }

    return BufferTokens(tokens);
}

//BufferTokens
BufferTokens::BufferTokens(vector<Token> tokens) : tokens(move(tokens)), pos(0) {}

Token BufferTokens::siguiente() {
    if (pos < tokens.size()) return tokens[pos++];
    return tokens.back(); // devuelve FIN_ARCHIVO 
}

// adelanto = 0 es el token actual, 1 es el siguiente, etc
Token BufferTokens::ver(int adelanto) {
    size_t objetivo = pos + adelanto;
    if (objetivo < tokens.size()) return tokens[objetivo];
    return tokens.back();
}

void BufferTokens::retroceder() {
    if (pos > 0) 
        pos--;
}

bool BufferTokens::hayMas() const {
    return pos < tokens.size() && tokens[pos].tipo != TipoToken::FIN_ARCHIVO;
}