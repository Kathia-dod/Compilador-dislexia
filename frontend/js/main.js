document.addEventListener('DOMContentLoaded', () => {
  // Elementos
  const audioBtn = document.getElementById('btnPlayAudio');
  const speedSelect = document.getElementById('speedSelect');
  const palabraInput = document.getElementById('palabraInput');
  const btnEnviar = document.getElementById('btnEnviar');
  const resultadosDiv = document.getElementById('resultados');
  const palabraEsperadaDisplay = document.getElementById('palabraEsperadaDisplay');
  const palabraIngresadaDisplay = document.getElementById('palabraIngresadaDisplay');
  const scoreDisplay = document.getElementById('scoreDisplay');
  const nivelDisplay = document.getElementById('nivelDisplay');
  const erroresList = document.getElementById('erroresList');

  // Palabra esperada (hardcodeada para demo; en producción vendría del audio)
  const palabraEsperada = 'biblioteca';

  // --- Síntesis de voz (Web Speech API) ---
  function reproducirAudio(texto, velocidad = 1.0) {
    if (!window.speechSynthesis) {
      alert('Tu navegador no soporta síntesis de voz.');
      return;
    }
    const utterance = new SpeechSynthesisUtterance(texto);
    utterance.lang = 'es-ES';
    utterance.rate = parseFloat(velocidad);
    utterance.pitch = 1;
    utterance.volume = 1;
    window.speechSynthesis.cancel(); // detener cualquier reproducción anterior
    window.speechSynthesis.speak(utterance);
  }

  audioBtn.addEventListener('click', () => {
    const velocidad = speedSelect.value;
    reproducirAudio(palabraEsperada, velocidad);
  });

  // --- Envío al servidor ---
  btnEnviar.addEventListener('click', async () => {
    const palabraIngresada = palabraInput.value.trim();
    if (!palabraIngresada) {
      alert('Por favor, escribe la palabra que escuchaste.');
      return;
    }

    // Deshabilitar botón durante la carga
    btnEnviar.disabled = true;
    btnEnviar.textContent = 'Analizando...';

    try {
      const response = await fetch('/api/analizar', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          palabraEsperada,
          palabraIngresada
        })
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(errorData.error || 'Error en el servidor');
      }

      const data = await response.json();
      mostrarResultados(data);
    } catch (error) {
      alert('Error al analizar: ' + error.message);
      console.error(error);
    } finally {
      btnEnviar.disabled = false;
      btnEnviar.textContent = 'Analizar';
    }
  });

  // --- Renderizado de resultados ---
  function mostrarResultados(data) {
    // Mostrar sección
    resultadosDiv.classList.remove('hidden');

    // Palabras
    palabraEsperadaDisplay.innerHTML = `<strong>🔵 Esperada:</strong> ${data.palabraEsperada}`;
    palabraIngresadaDisplay.innerHTML = `<strong>🟢 Ingresada:</strong> ${data.palabraIngresada}`;

    // Score y nivel
    const score = (data.scoreGlobal * 100).toFixed(0);
    scoreDisplay.textContent = `Puntuación: ${score}% de acierto`;
    nivelDisplay.textContent = `Nivel de riesgo: ${data.nivelRiesgo}`;
    nivelDisplay.className = `nivel ${data.nivelRiesgo}`;

    // Errores
    erroresList.innerHTML = '';
    if (data.errores && data.errores.length > 0) {
      const titulo = document.createElement('p');
      titulo.textContent = 'Diferencias detectadas:';
      erroresList.appendChild(titulo);

      data.errores.forEach((err) => {
        const item = document.createElement('div');
        item.className = 'error-item';

        const letraSpan = document.createElement('span');
        letraSpan.className = `letra ${err.tipoError}`;
        letraSpan.textContent = err.letraIngresada || '∅';
        item.appendChild(letraSpan);

        const infoSpan = document.createElement('span');
        infoSpan.textContent = `Esperaba "${err.letraEsperada}" en posición ${err.posicion}`;
        item.appendChild(infoSpan);

        const tipoSpan = document.createElement('span');
        tipoSpan.className = 'tipo';
        tipoSpan.textContent = err.tipoError.replace('_', ' ');
        item.appendChild(tipoSpan);

        // Botón de audio (fonema correcto)
        const audioBtnItem = document.createElement('button');
        audioBtnItem.className = 'audio-btn';
        audioBtnItem.textContent = '🔊';
        audioBtnItem.title = 'Escuchar fonema correcto';
        audioBtnItem.addEventListener('click', () => {
          // Usar la letra esperada como fonema
          const fonema = err.letraEsperada;
          reproducirAudio(fonema, 1.0);
        });
        item.appendChild(audioBtnItem);

        erroresList.appendChild(item);
      });
    } else {
      const ok = document.createElement('p');
      ok.textContent = '✅ ¡Perfecto! No se detectaron errores.';
      ok.style.color = '#28a745';
      ok.style.fontWeight = 'bold';
      erroresList.appendChild(ok);
    }
  }

  // Enter para enviar
  palabraInput.addEventListener('keypress', (e) => {
    if (e.key === 'Enter') {
      btnEnviar.click();
    }
  });

  // Cargar palabra esperada (para demostración)
  palabraInput.placeholder = `Escribe "${palabraEsperada}"`;
});