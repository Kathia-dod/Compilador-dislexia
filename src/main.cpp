// main.cpp

#include <iostream>
#include <iomanip>

#include "analizador_lexico.h"
#include "analizador_sintactico.h"

using namespace std;

// ─────────────────────────────────────────────
// Colores ANSI
// ─────────────────────────────────────────────
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define CYAN    "\033[36m"

void imprimirSeparador(const string& titulo) {
    cout << "\n";
    cout << CYAN << BOLD;
    cout << "========================================\n";
    cout << " " << titulo << "\n";
    cout << "========================================\n";
    cout << RESET;
}

void mostrarTokens(BufferTokens buffer) {
    cout << GREEN << "TOKENS GENERADOS:\n\n" << RESET;

    while (buffer.hayMas()) {

        Token t = buffer.siguiente();

        // ignorar espacios visualmente
        if (t.tipo == TipoToken::ESPACIO ||
            t.tipo == TipoToken::SALTO_LINEA)
            continue;

        cout << left << setw(28)
             << ("[" + nombreTipo(t.tipo) + "]");

        cout << " original=\""
             << t.valorOriginal << "\"";

        cout << " normal=\""
             << t.valorNormal << "\"";

        cout << " (L"
             << t.linea
             << ":C"
             << t.columna
             << ")";

        if (t.inicioOracion)
            cout << "  [INICIO_ORACION]";

        if (t.inicioLinea)
            cout << "  [INICIO_LINEA]";

        if (t.inicioParrafo)
            cout << "  [INICIO_PARRAFO]";

        cout << "\n";
    }
}

void mostrarErroresLexicos(const AnalizadorLexico& lexer) {

    if (!lexer.tieneErrores())
        return;

    cout << RED << BOLD;
    cout << "\nERRORES LEXICOS:\n";
    cout << RESET;

    for (const auto& e : lexer.obtenerErrores()) {

        cout << RED
             << "  [Linea "
             << e.linea
             << ", Col "
             << e.columna
             << "] "
             << e.descripcion
             << RESET
             << "\n";
    }
}

void mostrarErroresSintacticos(const AnalizadorSintactico& parser) {

    if (!parser.tieneErrores())
        return;

    cout << RED << BOLD;
    cout << "\nERRORES SINTACTICOS:\n";
    cout << RESET;

    for (const auto& e : parser.obtenerErrores()) {

        cout << RED
             << "  [Linea "
             << e.linea
             << ", Col "
             << e.columna
             << "] "
             << e.descripcion
             << RESET
             << "\n";
    }
}

int main(int argc, char* argv[]) {

    string rutaEntrada = "input/prueba2.txt";

    if (argc >= 2)
        rutaEntrada = argv[1];

    cout << BOLD
         << "\nCOMPILADOR PARA DETECCION DE DIFICULTADES DE LECTURA\n"
         << RESET;

    cout << "Archivo de entrada: "
         << rutaEntrada
         << "\n";

    try {

        // ─────────────────────────────────────
        // ANALISIS LEXICO
        // ─────────────────────────────────────
        imprimirSeparador("ANALISIS LEXICO");

        AnalizadorLexico lexer(rutaEntrada);

        BufferTokens bufferTokens = lexer.tokenizar();

        mostrarTokens(bufferTokens);

        mostrarErroresLexicos(lexer);

        // IMPORTANTE:
        // El buffer ya fue consumido al imprimir tokens,
        // por eso necesitamos volver a tokenizar.
        BufferTokens bufferParser = lexer.tokenizar();

        // ─────────────────────────────────────
        // ANALISIS SINTACTICO
        // ─────────────────────────────────────
        imprimirSeparador("ANALISIS SINTACTICO");

        AnalizadorSintactico parser(move(bufferParser));

        NodoSintactico ast = parser.analizar();

        parser.imprimirArbol(ast);

        mostrarErroresSintacticos(parser);

        // ─────────────────────────────────────
        // RESUMEN FINAL
        // ─────────────────────────────────────
        imprimirSeparador("RESUMEN");

        cout << "Errores lexicos: "
             << lexer.obtenerErrores().size()
             << "\n";

        cout << "Errores sintacticos: "
             << parser.obtenerErrores().size()
             << "\n";

        if (!lexer.tieneErrores() &&
            !parser.tieneErrores()) {

            cout << GREEN
                 << "\nAnalisis completado correctamente.\n"
                 << RESET;
        }
        else {

            cout << YELLOW
                 << "\nAnalisis finalizado con errores.\n"
                 << RESET;
        }

    }
    catch (const exception& e) {

        cerr << RED
             << "\nERROR FATAL: "
             << e.what()
             << RESET
             << "\n";

        return 1;
    }

    return 0;
}
