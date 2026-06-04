#include "analizador_lexico.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include <cctype>
#include <unordered_map>
using namespace std;

// map de signos ASCII de un solo byte
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
    {'^', TipoToken::DESCONOCIDO}, 
    {'`', TipoToken::DESCONOCIDO}, 
    {'|', TipoToken::PIPE},         
};


struct InfoMultibyte {
    TipoToken tipo;
    string representacion;
};

static const vector<pair<string, InfoMultibyte>> MAP_MULTIBYTE = {
    {string("\xC2\xBF"), {TipoToken::SIGNO_INTERR_INI,    "¿"}},
    {string("\xC2\xA1"), {TipoToken::SIGNO_EXCL_INI,      "¡"}},
    {string("\xC2\xAB"), {TipoToken::COMILLA_ANGULAR_INI, "«"}},
    {string("\xC2\xBB"), {TipoToken::COMILLA_ANGULAR_FIN, "»"}},
    {string("\xE2\x82\xAC"), {TipoToken::EURO,            "€"}},
    {string("\xE2\x89\xA0"), {TipoToken::DIFERENTE,       "≠"}},
    {string("\xE2\x80\x94"), {TipoToken::RAYA,            "—"}},
    {string("\xC2\xA7"), {TipoToken::DESCONOCIDO, "§"}},   
    {string("\xC2\xA4"), {TipoToken::DESCONOCIDO, "¤"}},   
    {string("\xC2\xA9"), {TipoToken::DESCONOCIDO, "©"}},  
    {string("\xC2\xAE"), {TipoToken::DESCONOCIDO, "®"}},  
    {string("\xE2\x84\xA2"), {TipoToken::DESCONOCIDO, "™"}}, 
    {string("\xC2\xAC"), {TipoToken::DESCONOCIDO, "¬"}},  
};

static const vector<string> ACENTOS_INVALIDOS_ES = {
    string("\xC3\xA0"), // à
    string("\xC3\xA8"), // è
    string("\xC3\xAC"), // ì
    string("\xC3\xB2"), // ò
    string("\xC3\xB9"), // ù
    string("\xC3\x80"), // À
    string("\xC3\x88"), // È
    string("\xC3\x8C"), // Ì
    string("\xC3\x92"), // Ò
    string("\xC3\x99"), // Ù
    string("\xCC\x80"), // combinacion grave (forma NFD)
};

static const vector<pair<string,string>> TABLA_MAYUS_MINUS = {
    {string("\xC3\x81"), string("\xC3\xA1")}, // Á -> á
    {string("\xC3\x89"), string("\xC3\xA9")}, // É -> é
    {string("\xC3\x8D"), string("\xC3\xAD")}, // Í -> í
    {string("\xC3\x93"), string("\xC3\xB3")}, // Ó -> ó
    {string("\xC3\x9A"), string("\xC3\xBA")}, // Ú -> ú
    {string("\xC3\x9C"), string("\xC3\xBC")}, // Ü -> ü
    {string("\xC3\x91"), string("\xC3\xB1")}, // Ñ -> ñ
};

static const vector<pair<string, TipoToken>> OPERADORES_COMPUESTOS = {
    {"==", TipoToken::OP_IGUAL_IGUAL},
    {"!=", TipoToken::OP_DIFERENTE},
    {">=", TipoToken::OP_MAYOR_IGUAL},
    {"<=", TipoToken::OP_MENOR_IGUAL},
    {"->", TipoToken::OP_FLECHA_DER},
    {"=>", TipoToken::OP_FLECHA_FAT},
    {"<-", TipoToken::OP_FLECHA_IZQ},
    {"&&", TipoToken::OP_AND},
    {"||", TipoToken::OP_OR},
};

string nombreTipo(TipoToken tipo) {
    static const unordered_map<TipoToken, string> nombres = {
        {TipoToken::PALABRA,               "TOKEN_PALABRA"},
        {TipoToken::NUMERO,                "TOKEN_NUMERO"},
        {TipoToken::EMAIL,                 "TOKEN_EMAIL"},
        {TipoToken::NUMERO_DECIMAL,        "TOKEN_NUMERO_DECIMAL"},
        {TipoToken::PALABRA_COMPUESTA,     "TOKEN_PALABRA_COMPUESTA"},
        {TipoToken::URL,                   "TOKEN_URL"},             
        {TipoToken::FECHA,                 "TOKEN_FECHA"},          
        {TipoToken::HORA,                  "TOKEN_HORA"},            
        {TipoToken::PORCENTAJE_VALOR,      "TOKEN_PORCENTAJE_VALOR"},
        {TipoToken::OP_IGUAL_IGUAL,        "TOKEN_OP_IGUAL_IGUAL"}, 
        {TipoToken::OP_DIFERENTE,          "TOKEN_OP_DIFERENTE"},
        {TipoToken::OP_MAYOR_IGUAL,        "TOKEN_OP_MAYOR_IGUAL"},
        {TipoToken::OP_MENOR_IGUAL,        "TOKEN_OP_MENOR_IGUAL"},
        {TipoToken::OP_FLECHA_DER,         "TOKEN_OP_FLECHA_DER"},
        {TipoToken::OP_FLECHA_FAT,         "TOKEN_OP_FLECHA_FAT"},
        {TipoToken::OP_FLECHA_IZQ,         "TOKEN_OP_FLECHA_IZQ"},
        {TipoToken::OP_AND,                "TOKEN_OP_AND"},
        {TipoToken::OP_OR,                 "TOKEN_OP_OR"},
        {TipoToken::PIPE,                  "TOKEN_PIPE"},            
        {TipoToken::COMA,                  "TOKEN_COMA"},
        {TipoToken::PUNTO,                 "TOKEN_PUNTO"},
        {TipoToken::PUNTOS_SUSPENSIVOS,    "TOKEN_PUNTOS_SUSPENSIVOS"},
        {TipoToken::PUNTO_Y_COMA,          "TOKEN_PUNTO_Y_COMA"},
        {TipoToken::DOS_PUNTOS,            "TOKEN_DOS_PUNTOS"},
        {TipoToken::SIGNO_INTERR_INI,      "TOKEN_INTERR_INI"},
        {TipoToken::SIGNO_INTERR_FIN,      "TOKEN_INTERR_FIN"},
        {TipoToken::SIGNO_EXCL_INI,        "TOKEN_EXCL_INI"},
        {TipoToken::SIGNO_EXCL_FIN,        "TOKEN_EXCL_FIN"},
        {TipoToken::GUION_BAJO,            "TOKEN_GUION_BAJO"},
        {TipoToken::PARENTESIS_INI,        "TOKEN_PARENTESIS_INI"},
        {TipoToken::PARENTESIS_FIN,        "TOKEN_PARENTESIS_FIN"},
        {TipoToken::CORCHETE_INI,          "TOKEN_CORCHETE_INI"},
        {TipoToken::CORCHETE_FIN,          "TOKEN_CORCHETE_FIN"},
        {TipoToken::LLAVE_INI,             "TOKEN_LLAVE_INI"},
        {TipoToken::LLAVE_FIN,             "TOKEN_LLAVE_FIN"},
        {TipoToken::MAS,                   "TOKEN_MAS"},
        {TipoToken::GUION,                 "TOKEN_GUION"},
        {TipoToken::IGUAL,                 "TOKEN_IGUAL"},
        {TipoToken::MAYOR_QUE,             "TOKEN_MAYOR_QUE"},
        {TipoToken::MENOR_QUE,             "TOKEN_MENOR_QUE"},
        {TipoToken::BARRA,                 "TOKEN_BARRA"},
        {TipoToken::AMPERSAND,             "TOKEN_AMPERSAND"},
        {TipoToken::COMILLA_SIMPLE,        "TOKEN_COMILLA_SIMPLE"},
        {TipoToken::COMILLA_DOBLE,         "TOKEN_COMILLA_DOBLE"},
        {TipoToken::ASTERISCO,             "TOKEN_ASTERISCO"},
        {TipoToken::BARRA_INVERTIDA,       "TOKEN_BARRA_INVERTIDA"},
        {TipoToken::ARROBA,                "TOKEN_ARROBA"},
        {TipoToken::NUMERAL,               "TOKEN_NUMERAL"},
        {TipoToken::VIRGULILLA,            "TOKEN_VIRGULILLA"},
        {TipoToken::DOLAR,                 "TOKEN_DOLAR"},
        {TipoToken::EURO,                  "TOKEN_EURO"},
        {TipoToken::PORCENTAJE,            "TOKEN_PORCENTAJE"},
        {TipoToken::DIFERENTE,             "TOKEN_DIFERENTE"},
        {TipoToken::COMILLA_ANGULAR_INI,   "TOKEN_COMILLA_ANGULAR_INI"},
        {TipoToken::COMILLA_ANGULAR_FIN,   "TOKEN_COMILLA_ANGULAR_FIN"},
        {TipoToken::RAYA,                  "TOKEN_RAYA"},
        {TipoToken::SALTO_LINEA,           "TOKEN_SALTO_LINEA"},
        {TipoToken::ESPACIO,               "TOKEN_ESPACIO"},
        {TipoToken::DESCONOCIDO,           "TOKEN_DESCONOCIDO"},
        {TipoToken::FIN_ARCHIVO,           "TOKEN_FIN_ARCHIVO"},
    };
    auto it = nombres.find(tipo);
    if (it != nombres.end()) return it->second;
    return "TOKEN_???";
}

AnalizadorLexico::AnalizadorLexico(const string& rutaArchivo)
    : pos(0), lineaActual(1), columnaActual(1),
      esperandoInicioOracion(true), esperandoInicioLinea(true),
      esperandoInicioParrafo(true), saltosConsecutivos(0) {
    if (!cargarArchivo(rutaArchivo))
        throw runtime_error("No se pudo abrir el archivo: " + rutaArchivo);
}

bool AnalizadorLexico::cargarArchivo(const string& ruta) {
    ifstream archivo(ruta);
    if (!archivo.is_open()) return false;
    ostringstream buffer;
    buffer << archivo.rdbuf();
    contenido = buffer.str();
    return true;
}

void AnalizadorLexico::resetear() {
    pos = 0;
    lineaActual = 1;
    columnaActual = 1;
    esperandoInicioOracion = true;
    esperandoInicioLinea = true;
    esperandoInicioParrafo = true;
    saltosConsecutivos = 0;
    errores.clear();
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
            esperandoInicioLinea = true;
            saltosConsecutivos++;
            if (saltosConsecutivos >= 2) esperandoInicioParrafo = true;
        } else {
            columnaActual++;
        }
        pos++;
    }
}

void AnalizadorLexico::registrarError(const string& desc, int linea, int col) {
    errores.push_back({desc, linea, col});
}

const vector<ErrorLexico>& AnalizadorLexico::obtenerErrores() const { return errores; }
bool AnalizadorLexico::tieneErrores() const { return !errores.empty(); }

bool AnalizadorLexico::esPalabraChar(unsigned char c) {
    if (c >= 'a' && c <= 'z') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c < 128) return false;
    for (const auto& entrada : MAP_MULTIBYTE) {
        const string& sec = entrada.first;
        if (!sec.empty() && (unsigned char)sec[0] == c) return false;
    }
    return true;
}

bool AnalizadorLexico::esDigito(unsigned char c) { return c >= '0' && c <= '9'; }

string AnalizadorLexico::normalizar(const string& texto) {
    string resultado = "";
    size_t i = 0;
    while (i < texto.size()) {
        unsigned char uc = (unsigned char)texto[i];
        if (uc < 128) {
            resultado += (char)tolower(texto[i]);
            i++;
        } else {
            bool convertido = false;
            for (const auto& par : TABLA_MAYUS_MINUS) {
                size_t len = par.first.size();
                if (i + len <= texto.size() && texto.substr(i, len) == par.first) {
                    resultado += par.second;
                    i += len;
                    convertido = true;
                    break;
                }
            }
            if (!convertido) {
                int bytes = 1;
                if      ((uc & 0xE0) == 0xC0) bytes = 2;
                else if ((uc & 0xF0) == 0xE0) bytes = 3;
                else if ((uc & 0xF8) == 0xF0) bytes = 4;
                for (int j = 0; j < bytes && i < texto.size(); j++, i++)
                    resultado += texto[i];
            }
        }
    }
    return resultado;
}

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

bool AnalizadorLexico::detectarAcentoInvalido(int linea, int col) {
    for (const auto& seq : ACENTOS_INVALIDOS_ES) {
        size_t len = seq.size();
        if (pos + len <= contenido.size() && contenido.substr(pos, len) == seq) {
            registrarError("Acento invalido en español: '" + seq +
                           "' (posible confusion visual dislectica)", linea, col);
            for (size_t i = 0; i < len; i++) avanzar();
            return true;
        }
    }
    return false;
}

Token AnalizadorLexico::leerEmail(const string& localPart, int linea, int col) {
    string email = localPart;
    email += '@';
    avanzar(); 
    while (pos < contenido.size()) {
        char c = actual();
        unsigned char uc = (unsigned char)c;
        if ((uc >= 'a' && uc <= 'z') || (uc >= 'A' && uc <= 'Z') ||
            (uc >= '0' && uc <= '9') || c == '.' || c == '-') {
            email += c;
            avanzar();
        } else {
            break;
        }
    }
    return {TipoToken::EMAIL, email, normalizar(email), linea, col, false, false, false, false};
}

Token AnalizadorLexico::leerURL(const string& prefijo, int linea, int col) {
    string url = prefijo;
    url += ':'; avanzar();
    url += '/'; avanzar();
    url += '/'; avanzar();
    while (pos < contenido.size()) {
        char c = actual();
        if (c == ' ' || c == '\t' || c == '\n' || c == '\0') break;
        url += c;
        avanzar();
    }
    return {TipoToken::URL, url, url, linea, col, false, false, false, false};
}

Token AnalizadorLexico::leerPalabra(int linea, int col) {
    string original = "";

    while (pos < contenido.size() && esPalabraChar((unsigned char)actual())) {
        unsigned char uc = (unsigned char)actual();
        if (uc < 128) {
            original += actual();
            avanzar();
        } else {
            bool invalido = false;
            for (const auto& seq : ACENTOS_INVALIDOS_ES) {
                size_t len = seq.size();
                if (pos + len <= contenido.size() && contenido.substr(pos, len) == seq) {
                    registrarError("Acento invalido en español dentro de palabra: '" +
                                   seq + "'", lineaActual, columnaActual);
                    invalido = true;
                    break;
                }
            }
            (void)invalido;
            int bytes = 1;
            if      ((uc & 0xE0) == 0xC0) bytes = 2;
            else if ((uc & 0xF0) == 0xE0) bytes = 3;
            else if ((uc & 0xF8) == 0xF0) bytes = 4;
            for (int i = 0; i < bytes && pos < contenido.size(); i++) {
                original += actual();
                avanzar();
            }
        }
    }

    if ((original == "http" || original == "https") &&
        pos + 2 < contenido.size() &&
        contenido[pos] == ':' && contenido[pos+1] == '/' && contenido[pos+2] == '/') {
        return leerURL(original, linea, col);
    }

    if (pos < contenido.size() && actual() == '@') {
        return leerEmail(original, linea, col);
    }

    if (pos < contenido.size() && actual() == '-') {
        char siguienteAlGuion = (pos + 1 < contenido.size()) ? contenido[pos + 1] : '\0';
        bool siguienteEsLetra = (siguienteAlGuion >= 'a' && siguienteAlGuion <= 'z') ||
                                 (siguienteAlGuion >= 'A' && siguienteAlGuion <= 'Z') ||
                                 ((unsigned char)siguienteAlGuion >= 128);
        if (siguienteEsLetra) {
            original += '-';
            avanzar();
            while (pos < contenido.size() && esPalabraChar((unsigned char)actual())) {
                unsigned char uc2 = (unsigned char)actual();
                if (uc2 < 128) {
                    original += actual();
                    avanzar();
                } else {
                    int bytes = 1;
                    if      ((uc2 & 0xE0) == 0xC0) bytes = 2;
                    else if ((uc2 & 0xF0) == 0xE0) bytes = 3;
                    else if ((uc2 & 0xF8) == 0xF0) bytes = 4;
                    for (int i = 0; i < bytes && pos < contenido.size(); i++) {
                        original += actual();
                        avanzar();
                    }
                }
            }
            bool esInicioOracion = esperandoInicioOracion;
            bool esInicioLinea   = esperandoInicioLinea;
            bool esInicioParrafo = esperandoInicioParrafo;
            esperandoInicioOracion = false;
            esperandoInicioLinea   = false;
            esperandoInicioParrafo = false;
            saltosConsecutivos = 0;
            return {TipoToken::PALABRA_COMPUESTA, original, normalizar(original),
                    linea, col, esInicioOracion, esInicioLinea, esInicioParrafo, true};
        }
    }

    bool esInicioOracion = esperandoInicioOracion;
    bool esInicioLinea   = esperandoInicioLinea;
    bool esInicioParrafo = esperandoInicioParrafo;
    esperandoInicioOracion = false;
    esperandoInicioLinea   = false;
    esperandoInicioParrafo = false;
    saltosConsecutivos = 0;

    return {TipoToken::PALABRA, original, normalizar(original),
            linea, col, esInicioOracion, esInicioLinea, esInicioParrafo, false};
}

Token AnalizadorLexico::leerNumero(int linea, int col) {
    string num = "";
    while (pos < contenido.size() && esDigito((unsigned char)actual())) {
        num += actual();
        avanzar();
    }

    if (pos < contenido.size() && (actual() == '/' || actual() == '-') &&
        num.size() <= 4) {
        char sep = actual();
        if (pos + 1 < contenido.size() && esDigito((unsigned char)contenido[pos+1])) {
            string fecha = num;
            fecha += sep; avanzar();
            while (pos < contenido.size() && esDigito((unsigned char)actual())) {
                fecha += actual(); avanzar();
            }

            if (pos < contenido.size() && (actual() == '/' || actual() == '-') &&
                pos + 1 < contenido.size() && esDigito((unsigned char)contenido[pos+1])) {
                fecha += actual(); avanzar();
                while (pos < contenido.size() && esDigito((unsigned char)actual())) {
                    fecha += actual(); avanzar();
                }
            }
            bool esInicioOracion = esperandoInicioOracion;
            bool esInicioLinea   = esperandoInicioLinea;
            bool esInicioParrafo = esperandoInicioParrafo;
            esperandoInicioOracion = false;
            esperandoInicioLinea   = false;
            esperandoInicioParrafo = false;
            saltosConsecutivos = 0;
            return {TipoToken::FECHA, fecha, fecha,
                    linea, col, esInicioOracion, esInicioLinea, esInicioParrafo, false};
        }
    }

    if (pos < contenido.size() && actual() == ':' &&
        pos + 1 < contenido.size() && esDigito((unsigned char)contenido[pos+1])) {
        string hora = num;
        hora += ':'; avanzar();
        while (pos < contenido.size() && esDigito((unsigned char)actual())) {
            hora += actual(); avanzar();
        }
        bool esInicioOracion = esperandoInicioOracion;
        bool esInicioLinea   = esperandoInicioLinea;
        bool esInicioParrafo = esperandoInicioParrafo;
        esperandoInicioOracion = false;
        esperandoInicioLinea   = false;
        esperandoInicioParrafo = false;
        saltosConsecutivos = 0;
        return {TipoToken::HORA, hora, hora,
                linea, col, esInicioOracion, esInicioLinea, esInicioParrafo, false};
    }

    if (pos < contenido.size() && actual() == '.' &&
        pos + 1 < contenido.size() && esDigito((unsigned char)contenido[pos + 1])) {
        num += '.';
        avanzar();
        while (pos < contenido.size() && esDigito((unsigned char)actual())) {
            num += actual();
            avanzar();
        }

        if (pos < contenido.size() && actual() == '%') {
            num += '%'; avanzar();
            bool esInicioOracion = esperandoInicioOracion;
            bool esInicioLinea   = esperandoInicioLinea;
            bool esInicioParrafo = esperandoInicioParrafo;
            esperandoInicioOracion = false;
            esperandoInicioLinea   = false;
            esperandoInicioParrafo = false;
            saltosConsecutivos = 0;
            return {TipoToken::PORCENTAJE_VALOR, num, num,
                    linea, col, esInicioOracion, esInicioLinea, esInicioParrafo, false};
        }
        bool esInicioOracion = esperandoInicioOracion;
        bool esInicioLinea   = esperandoInicioLinea;
        bool esInicioParrafo = esperandoInicioParrafo;
        esperandoInicioOracion = false;
        esperandoInicioLinea   = false;
        esperandoInicioParrafo = false;
        saltosConsecutivos = 0;
        return {TipoToken::NUMERO_DECIMAL, num, num,
                linea, col, esInicioOracion, esInicioLinea, esInicioParrafo, false};
    }

    // [MEJORA-C] detectar porcentaje entero: 50%, 75%
    if (pos < contenido.size() && actual() == '%') {
        num += '%'; avanzar();
        bool esInicioOracion = esperandoInicioOracion;
        bool esInicioLinea   = esperandoInicioLinea;
        bool esInicioParrafo = esperandoInicioParrafo;
        esperandoInicioOracion = false;
        esperandoInicioLinea   = false;
        esperandoInicioParrafo = false;
        saltosConsecutivos = 0;
        return {TipoToken::PORCENTAJE_VALOR, num, num,
                linea, col, esInicioOracion, esInicioLinea, esInicioParrafo, false};
    }

    bool esInicioOracion = esperandoInicioOracion;
    bool esInicioLinea   = esperandoInicioLinea;
    bool esInicioParrafo = esperandoInicioParrafo;
    esperandoInicioOracion = false;
    esperandoInicioLinea   = false;
    esperandoInicioParrafo = false;
    saltosConsecutivos = 0;
    return {TipoToken::NUMERO, num, num,
            linea, col, esInicioOracion, esInicioLinea, esInicioParrafo, false};
}

bool AnalizadorLexico::leerSignoMultibyte(int linea, int col, Token& resultado) {
    for (const auto& entrada : MAP_MULTIBYTE) {
        const string& secuencia = entrada.first;
        const InfoMultibyte& info = entrada.second;
        size_t len = secuencia.size();
        if (pos + len <= contenido.size() && contenido.substr(pos, len) == secuencia) {
            for (size_t i = 0; i < len; i++) avanzar();
            if (info.tipo == TipoToken::DESCONOCIDO) {
                registrarError("Simbolo legal/tecnico fuera del alfabeto español: '" +
                               info.representacion + "'", linea, col);
            }
            resultado = {info.tipo, info.representacion, info.representacion,
                         linea, col, false, false, false, false};
            return true;
        }
    }
    return false;
}

Token AnalizadorLexico::siguienteToken() {
    char c = actual();
    int linea = lineaActual;
    int col   = columnaActual;

    if (c == '\0') return {TipoToken::FIN_ARCHIVO, "", "", linea, col, false, false, false, false};
    if (c == ' ' || c == '\t') {
        avanzar();
        return {TipoToken::ESPACIO, " ", " ", linea, col, false, false, false, false};
    }
    if (c == '\n') {
        avanzar();
        return {TipoToken::SALTO_LINEA, "\\n", "\\n", linea, col, false, false, false, false};
    }

    if (esPalabraChar((unsigned char)c)) return leerPalabra(linea, col);
    if (esDigito((unsigned char)c))      return leerNumero(linea, col);

    Token multibyte;
    if (leerSignoMultibyte(linea, col, multibyte)) {
        if (multibyte.tipo == TipoToken::SIGNO_INTERR_INI ||
            multibyte.tipo == TipoToken::SIGNO_EXCL_INI)
            esperandoInicioOracion = true;

        if (multibyte.tipo == TipoToken::RAYA) {
            char anteriorR = (pos >= 4) ? contenido[pos - 4] : '\0';
            char siguienteR = actual();
            if (!(esDigito((unsigned char)anteriorR) && esDigito((unsigned char)siguienteR)))
                esperandoInicioOracion = true;
        }

        if (!esSignoDeApertura(multibyte.tipo)) saltosConsecutivos = 0;
        return multibyte;
    }

    if (pos + 1 < contenido.size()) {
        string dos(1, c);
        dos += contenido[pos + 1];
        for (const auto& op : OPERADORES_COMPUESTOS) {
            if (dos == op.first) {
                avanzar(); avanzar();
                saltosConsecutivos = 0;
                return {op.second, dos, dos, linea, col, false, false, false, false};
            }
        }
    }

    avanzar();
    auto it = MAP_ASCII.find(c);
    if (it != MAP_ASCII.end()) {
        string rep(1, c);
        TipoToken tipo = it->second;

        if (tipo == TipoToken::DESCONOCIDO) {
            registrarError("Caracter fuera del alfabeto español: '" + rep + "'", linea, col);
            saltosConsecutivos = 0;
            return {TipoToken::DESCONOCIDO, rep, rep, linea, col, false, false, false, false};
        }

        bool marcarInicioOracion = false;

        if (tipo == TipoToken::PUNTO) {
            char anterior = (pos >= 2) ? contenido[pos - 2] : '\0';
            char siguiente = actual();
            bool anteriorAlnum = isalnum((unsigned char)anterior);
            bool siguienteAlnum = isalnum((unsigned char)siguiente);
            if (anteriorAlnum && siguienteAlnum)
                marcarInicioOracion = false;
            else if (siguiente == '.')
                marcarInicioOracion = false;
            else
                marcarInicioOracion = true;
        }
        else if (tipo == TipoToken::SIGNO_INTERR_FIN ||
                 tipo == TipoToken::SIGNO_EXCL_FIN ||
                 tipo == TipoToken::DOS_PUNTOS)
            marcarInicioOracion = true;

        else if (tipo == TipoToken::GUION) {
            char anteriorG = (pos >= 2) ? contenido[pos - 2] : '\0';
            char siguienteG = actual();
            bool entreDigitos = esDigito((unsigned char)anteriorG) && esDigito((unsigned char)siguienteG);
            bool sigueEspacio = (siguienteG == ' ' || siguienteG == '\t');
            if (!entreDigitos && sigueEspacio)
                marcarInicioOracion = true;
        }

        if (marcarInicioOracion) esperandoInicioOracion = true;
        if (!esSignoDeApertura(tipo)) saltosConsecutivos = 0;

        return {tipo, rep, rep, linea, col, false, false, false, false};
    }

    string rep(1, c);
    registrarError("Caracter desconocido: '" + rep + "'", linea, col);
    saltosConsecutivos = 0;
    return {TipoToken::DESCONOCIDO, rep, rep, linea, col, false, false, false, false};
}

BufferTokens AnalizadorLexico::tokenizar() {
    vector<Token> tokens;
    pos = 0; lineaActual = 1; columnaActual = 1;
    esperandoInicioOracion = true; esperandoInicioLinea = true;
    esperandoInicioParrafo = true; saltosConsecutivos = 0;

    while (true) {
        if (actual() == '.' &&
            pos + 1 < contenido.size() && contenido[pos + 1] == '.' &&
            pos + 2 < contenido.size() && contenido[pos + 2] == '.') {
            int l = lineaActual, col = columnaActual;
            avanzar(); avanzar(); avanzar();
            esperandoInicioOracion = true;
            saltosConsecutivos = 0;
            tokens.push_back({TipoToken::PUNTOS_SUSPENSIVOS, "...", "...",
                              l, col, false, false, false, false});
            continue;
        }

        if (esDigito((unsigned char)actual())) {
            size_t tmpPos = pos;
            string tmpNum = "";
            while (tmpPos < contenido.size() && esDigito((unsigned char)contenido[tmpPos]))
                tmpNum += contenido[tmpPos++];
            if (tmpNum.size() <= 2 && tmpPos < contenido.size() && contenido[tmpPos] == '.' &&
                tmpPos + 1 < contenido.size() && esDigito((unsigned char)contenido[tmpPos+1])) {
                int l = lineaActual, col = columnaActual;
                string fecha = "";
                while (pos < contenido.size() && esDigito((unsigned char)actual())) {
                    fecha += actual(); avanzar();
                }
                if (pos < contenido.size() && actual() == '.') {
                    fecha += actual(); avanzar();
                }
                while (pos < contenido.size() && esDigito((unsigned char)actual())) {
                    fecha += actual(); avanzar();
                }
                if (pos < contenido.size() && actual() == '.') {
                    fecha += actual(); avanzar();
                }
                while (pos < contenido.size() && esDigito((unsigned char)actual())) {
                    fecha += actual(); avanzar();
                }
                bool esInicioOracion = esperandoInicioOracion;
                bool esInicioLinea   = esperandoInicioLinea;
                bool esInicioParrafo = esperandoInicioParrafo;
                esperandoInicioOracion = false;
                esperandoInicioLinea   = false;
                esperandoInicioParrafo = false;
                saltosConsecutivos = 0;
                tokens.push_back({TipoToken::FECHA, fecha, fecha, l, col, esInicioOracion, esInicioLinea, esInicioParrafo, false});
                continue;
            }
        }

        Token t = siguienteToken();
        tokens.push_back(t);
        if (t.tipo == TipoToken::FIN_ARCHIVO) break;
    }
    return BufferTokens(tokens);
}

// ─────────────────────────────────────────────
// BufferTokens
// ─────────────────────────────────────────────
BufferTokens::BufferTokens(vector<Token> tokens) : tokens(move(tokens)), pos(0) {}

Token BufferTokens::siguiente() {
    if (pos < tokens.size()) return tokens[pos++];
    return tokens.back();
}

Token BufferTokens::ver(int adelanto) const {
    size_t objetivo = pos + adelanto;
    if (objetivo < tokens.size()) return tokens[objetivo];
    return tokens.back();
}

void BufferTokens::retroceder() { if (pos > 0) pos--; }

bool BufferTokens::hayMas() const {
    return pos < tokens.size() && tokens[pos].tipo != TipoToken::FIN_ARCHIVO;
}
