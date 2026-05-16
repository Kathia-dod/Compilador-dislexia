#include <iostream>
#include <vector>
#include "analizador_lexico.h"
using namespace std;

int main(int argc, char* argv[]) {
    string rutaEntrada = "input/prueba1.txt";
    if (argc >= 2) 
        rutaEntrada = argv[1];

    cout << "Leyendo archivo: " << rutaEntrada << "\n\n";

    try {
        AnalizadorLexico lexer(rutaEntrada);
        BufferTokens buffer = lexer.tokenizar();

        // mostrar errores
        if (lexer.tieneErrores()) {
            cout << "ERRORES LEXICOS:\n";
            for (const ErrorLexico& e : lexer.obtenerErrores()) {
                cout << "  [linea " << e.linea << ", col " << e.columna << "] " << e.descripcion << "\n";
            }
            cout << "\n";
        }

        cout << "TOKENS GENERADOS:\n";

        // consumir via buffer en lugar de iterar el vector directo
        while (buffer.hayMas()) {
            Token t = buffer.siguiente();

            if (t.tipo == TipoToken::ESPACIO || t.tipo == TipoToken::SALTO_LINEA)
                continue;

            cout << "[" << nombreTipo(t.tipo) << "] " << "original=\"" << t.valorOriginal << "\" " << "normal=\""   << t.valorNormal   << "\" " << "(linea " << t.linea << ", col " << t.columna << ")";

            // marcar inicio de oraciones
            if (t.inicioOracion) 
                cout << " [INICIO_ORACION]";
            
            if (t.inicioLinea) 
                cout << " [INICIO_LINEA]";
            
            if (t.inicioParrafo) 
                cout << " [INICIO_PARRAFO]";
            
            cout << "\n";
        }

    } catch (const exception& e) {
        cerr << "Error fatal: " << e.what() << "\n";
        return 1;
    }

    return 0;
}