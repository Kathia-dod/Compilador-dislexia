#ifndef ANALIZADOR_LEXICO_H
#define ANALIZADOR_LEXICO_H

#include <string>
#include <vector>
using namespace std;

enum class TipoToken {
    PALABRA,
    NUMERO,
    EMAIL,               
    NUMERO_DECIMAL,       
    PALABRA_COMPUESTA,  
    URL,                
    FECHA,              
    HORA,              
    PORCENTAJE_VALOR,  
    OP_IGUAL_IGUAL,   
    OP_DIFERENTE,    
    OP_MAYOR_IGUAL,   
    OP_MENOR_IGUAL, 
    OP_FLECHA_DER,  
    OP_FLECHA_FAT, 
    OP_FLECHA_IZQ,   
    OP_AND,           
    OP_OR,           
    PIPE, 
    COMA,
    PUNTO,
    PUNTOS_SUSPENSIVOS, 
    PUNTO_Y_COMA,
    DOS_PUNTOS,
    SIGNO_INTERR_INI,     // ¿
    SIGNO_INTERR_FIN,     // ?
    SIGNO_EXCL_INI,       // ¡
    SIGNO_EXCL_FIN,       // !
    GUION_BAJO,           // _
    PARENTESIS_INI,       // (
    PARENTESIS_FIN,       // )
    CORCHETE_INI,         // [
    CORCHETE_FIN,         // ]
    LLAVE_INI,            // {
    LLAVE_FIN,            // }
    MAS,                  // +
    GUION,                // -
    IGUAL,                // =
    MAYOR_QUE,            // >
    MENOR_QUE,            // <
    BARRA,                // /
    AMPERSAND,            // &
    COMILLA_SIMPLE,       // '
    COMILLA_DOBLE,        // "
    ASTERISCO,            // *
    BARRA_INVERTIDA,      // '\'
    ARROBA,               // @
    NUMERAL,              // #
    VIRGULILLA,           // ~
    DOLAR,                // $
    EURO,                 // €
    PORCENTAJE,           // %
    DIFERENTE,            // ≠
    COMILLA_ANGULAR_INI,  // «
    COMILLA_ANGULAR_FIN,  // »
    RAYA,                 // —
    SALTO_LINEA,
    ESPACIO,
    DESCONOCIDO,
    FIN_ARCHIVO
};

// ─────────────────────────────────────────────
// Token
// ─────────────────────────────────────────────
struct Token {
    TipoToken tipo;
    string valorOriginal;
    string valorNormal;
    int linea;
    int columna;

    bool inicioOracion;     
    bool inicioLinea;       
    bool inicioParrafo;   

    bool esPalabraCompuesta; 
};

struct ErrorLexico {
    string descripcion;
    int linea;
    int columna;
};

class AnalizadorLexico;

// ─────────────────────────────────────────────
// BufferTokens
// ─────────────────────────────────────────────
class BufferTokens {
 public:
    explicit BufferTokens(vector<Token> tokens);

    Token siguiente();
    Token ver(int adelanto = 0) const;
    void retroceder();
    bool hayMas() const;

 private:
    vector<Token> tokens;
    size_t pos;
};

// ─────────────────────────────────────────────
// AnalizadorLexico
// ─────────────────────────────────────────────
class AnalizadorLexico {
 public:
    explicit AnalizadorLexico(const string& rutaArchivo);

    BufferTokens tokenizar();

    void resetear();

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

    bool esSignoDeApertura(TipoToken tipo) const;

    Token leerEmail(const string& localPart, int linea, int col);

    Token leerURL(const string& prefijo, int linea, int col);

    bool detectarAcentoInvalido(int linea, int col);
};

string nombreTipo(TipoToken tipo);

#endif
