#include <iostream>
#include <vector>
#include "analizador_lexico.h"
using namespace std;

string nombreTipo(TipoToken tipo) {
    switch (tipo) {
        case TipoToken::PALABRA:return "TOKEN_PALABRA";
        case TipoToken::NUMERO:return "TOKEN_NUMERO";
        case TipoToken::COMA: return "TOKEN_COMA";
        case TipoToken::PUNTO: return "TOKEN_PUNTO";
        case TipoToken::PUNTO_Y_COMA: return "TOKEN_PUNTO_Y_COMA";
        case TipoToken::DOS_PUNTOS: return "TOKEN_DOS_PUNTOS";
        case TipoToken::SIGNO_INTERR_INI: return "TOKEN_INTERR_INI";
        case TipoToken::SIGNO_INTERR_FIN: return "TOKEN_INTERR_FIN";
        case TipoToken::SIGNO_EXCL_INI: return "TOKEN_EXCL_INI";
        case TipoToken::SIGNO_EXCL_FIN: return "TOKEN_EXCL_FIN";
        case TipoToken::GUION_BAJO: return "TOKEN_GUION_BAJO";
        case TipoToken::PARENTESIS_INI: return "TOKEN_PARENTESIS_INI";
        case TipoToken::PARENTESIS_FIN: return "TOKEN_PARENTESIS_FIN";
        case TipoToken::CORCHETE_INI: return "TOKEN_CORCHETE_INI";
        case TipoToken::CORCHETE_FIN: return "TOKEN_CORCHETE_FIN";
        case TipoToken::MAS: return "TOKEN_MAS";
        case TipoToken::IGUAL: return "TOKEN_IGUAL";
        case TipoToken::MAYOR_QUE: return "TOKEN_MAYOR_QUE";
        case TipoToken::MENOR_QUE: return "TOKEN_MENOR_QUE";
        case TipoToken::BARRA: return "TOKEN_BARRA";
        case TipoToken::AMPERSAND: return "TOKEN_AMPERSAND";
        case TipoToken::COMILLA_SIMPLE: return "TOKEN_COMILLA_SIMPLE";
        case TipoToken::COMILLA_DOBLE: return "TOKEN_COMILLA_DOBLE";
        case TipoToken::ASTERISCO: return "TOKEN_ASTERISCO";
        case TipoToken::BARRA_INVERTIDA: return "TOKEN_BARRA_INVERTIDA";
        case TipoToken::ARROBA: return "TOKEN_ARROBA";
        case TipoToken::NUMERAL: return "TOKEN_NUMERAL";
        case TipoToken::VIRGULILLA: return "TOKEN_VIRGULILLA";
        case TipoToken::DOLAR: return "TOKEN_DOLAR";
        case TipoToken::EURO: return "TOKEN_EURO";
        case TipoToken::PORCENTAJE: return "TOKEN_PORCENTAJE";
        case TipoToken::DIFERENTE: return "TOKEN_DIFERENTE";
        case TipoToken::COMILLA_ANGULAR_INI: return "TOKEN_COMILLA_ANGULAR_INI";
        case TipoToken::COMILLA_ANGULAR_FIN: return "TOKEN_COMILLA_ANGULAR_FIN";
        case TipoToken::RAYA: return "TOKEN_RAYA";
        case TipoToken::SALTO_LINEA: return "TOKEN_SALTO_LINEA";
        case TipoToken::ESPACIO: return "TOKEN_ESPACIO";
        case TipoToken::DESCONOCIDO: return "TOKEN_DESCONOCIDO";
        case TipoToken::FIN_ARCHIVO: return "TOKEN_FIN_ARCHIVO";
        default: return "TOKEN_???";
    }
}

int main(int argc, char* argv[]) {
    string rutaEntrada = "input/texto_prueba.txt";
    if (argc >= 2) rutaEntrada = argv[1];

    cout << "Leyendo archivo: " << rutaEntrada << "\n\n";

    try {
        AnalizadorLexico lexer(rutaEntrada);
        vector<Token> tokens = lexer.tokenizar();

        cout << "TOKENS: \n";
        for (const Token& t : tokens) {
            if (t.tipo == TipoToken::ESPACIO || t.tipo == TipoToken::SALTO_LINEA)
                continue;
            cout << "[" << nombreTipo(t.tipo) << "] " << "\"" << t.valor << "\"" << "  (linea " << t.linea << ", col " << t.columna << ")\n";
        }

        cout << "\nTotal de tokens: " << tokens.size() << "\n";

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}