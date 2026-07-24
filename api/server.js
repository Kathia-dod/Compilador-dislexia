const express = require('express');
const cors = require('cors');
const bodyParser = require('body-parser');
const { spawn } = require('child_process');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(bodyParser.json({ limit: '10mb' }));
app.use(bodyParser.urlencoded({ extended: true }));

// Ruta absoluta al ejecutable C++ (compilado)
const COMPILER_PATH = path.join(__dirname, '..', 'backend', 'build', 'compilador');

// Endpoint principal de análisis
app.post('/api/analizar', async (req, res) => {
  try {
    const { textoReferencia, textoUsuario } = req.body;

    if (!textoReferencia || typeof textoReferencia !== 'string') {
      return res.status(400).json({ error: 'Se requiere "textoReferencia"' });
    }
    if (!textoUsuario || typeof textoUsuario !== 'string') {
      return res.status(400).json({ error: 'Se requiere "textoUsuario"' });
    }

    // Verificar que el ejecutable existe
    if (!fs.existsSync(COMPILER_PATH)) {
      return res.status(500).json({
        error: 'Ejecutable no encontrado. Compile el backend con CMake.'
      });
    }

    // Invocar al ejecutable C++ pasando los dos textos por stdin
    const child = spawn(COMPILER_PATH, [], { stdio: ['pipe', 'pipe', 'pipe'] });

    // Escribir en stdin: primera línea = referencia, segunda = usuario
    child.stdin.write(textoReferencia + '\n');
    child.stdin.write(textoUsuario + '\n');
    child.stdin.end();

    let stdout = '';
    let stderr = '';

    child.stdout.on('data', (data) => {
      stdout += data.toString();
    });

    child.stderr.on('data', (data) => {
      stderr += data.toString();
    });

    child.on('close', (code) => {
      if (code !== 0) {
        console.error('Error en el ejecutable C++:', stderr);
        return res.status(500).json({
          error: 'Error en el motor de análisis',
          detalle: stderr
        });
      }

      try {
        // El ejecutable debe emitir un JSON válido en su salida estándar
        const resultado = JSON.parse(stdout);
        return res.json(resultado);
      } catch (parseError) {
        console.error('Error parseando JSON:', parseError);
        console.error('Salida recibida:', stdout);
        return res.status(500).json({
          error: 'Respuesta inválida del motor de análisis'
        });
      }
    });

  } catch (error) {
    console.error('Error en el servidor:', error);
    res.status(500).json({ error: 'Error interno del servidor' });
  }
});

// Ruta de salud
app.get('/api/health', (req, res) => {
  res.json({ status: 'OK', timestamp: new Date().toISOString() });
});

// Servir archivos del frontend
app.use(express.static(path.join(__dirname, '..', 'frontend')));

// Ruta principal
app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, '..', 'frontend', 'index.html'));
});

app.listen(PORT, () => {
  console.log(`API escuchando en http://localhost:${PORT}`);
});