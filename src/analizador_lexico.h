#ifndef ANALIZADOR_LEXICO_H
#define ANALIZADOR_LEXICO_H

#include <string>
#include <vector>
using namespace std;

enum class TipoToken {
    PALABRA,
    NUMERO,
    COMA,
    PUNTO,
    PUNTOS_SUSPENSIVOS, //...
    PUNTO_Y_COMA,
    DOS_PUNTOS,
    SIGNO_INTERR_INI, // ¿
    SIGNO_INTERR_FIN, // ?
    SIGNO_EXCL_INI, // ¡
    SIGNO_EXCL_FIN, // !
    GUION_BAJO,     // _
    PARENTESIS_INI, // (
    PARENTESIS_FIN, // )
    CORCHETE_INI,   // [
    CORCHETE_FIN,   // ]
    LLAVE_INI,   // {
    LLAVE_FIN,   // }
    MAS,        // +
    GUION,      // -
    IGUAL,      // =
    MAYOR_QUE,      // >
    MENOR_QUE,      // <
    BARRA,      // /
    AMPERSAND,  // &
    COMILLA_SIMPLE, // '
    COMILLA_DOBLE,  // "
    ASTERISCO,      // *
    BARRA_INVERTIDA,  
    ARROBA,   // @
    NUMERAL,  // #
    VIRGULILLA,  // ~
    DOLAR, // $
    EURO,  // € 
    PORCENTAJE, // %
    DIFERENTE,  // ≠ 
    COMILLA_ANGULAR_INI, // « (UTF-8: 0xC2 0xAB)
    COMILLA_ANGULAR_FIN, // » (UTF-8: 0xC2 0xBB)
    RAYA,               // — (UTF-8: 0xE2 0x80 0x94)
    SALTO_LINEA,
    ESPACIO,
    DESCONOCIDO,
    FIN_ARCHIVO
};

struct Token {
    TipoToken tipo;
    string valorOriginal;
    string valorNormal;
    int linea;
    int columna;

    bool inicioOracion;
    bool inicioLinea;
    bool inicioParrafo;
};

struct ErrorLexico {
    string descripcion;
    int linea;
    int columna;
};

class AnalizadorLexico;

class BufferTokens {
 public:
    explicit BufferTokens(vector<Token> tokens);

    Token siguiente();
    Token ver(int adelanto = 0);
    void retroceder();
    bool hayMas() const;

 private:
    vector<Token> tokens;
    size_t pos;
};

class AnalizadorLexico {
 public:
    explicit AnalizadorLexico(const string& rutaArchivo);

    BufferTokens tokenizar();

    const vector<ErrorLexico>& obtenerErrores() const;
    bool tieneErrores() const;

 private:
    string contenido;
    size_t pos;

    int lineaActual;
    int columnaActual;

    bool esperandoInicioOracion;
    bool esperandoInicioLinea;
    bool esperandoInicioParrafo;

    int saltosConsecutivos;

    vector<ErrorLexico> errores;

    bool cargarArchivo(const string& ruta);

    Token siguienteToken();

    bool esPalabraChar(unsigned char c);
    bool esDigito(unsigned char c);
    void avanzar();
    char actual();
    Token leerPalabra(int linea, int col);
    Token leerNumero(int linea, int col);
    bool leerSignoMultibyte(int linea, int col, Token& resultado);
    string normalizar(const string& texto);
    void registrarError(const string& desc, int linea, int col);

    // devuelve true si el tipo de token es un signo de apertura
    bool esSignoDeApertura(TipoToken tipo) const;
};

string nombreTipo(TipoToken tipo);

#endif