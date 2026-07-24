document.addEventListener('DOMContentLoaded', () => {
  // ====== Datos de los ejemplos (solo texto y audio) ======
  const EJEMPLOS = {
    1: {
      texto: "Un día, un perro llamado Dino fue al parque a jugar con su pelota blanda. De repente, vio a un gran oso pardo que bebía agua de una fuente. Dino, que era muy valiente pero un poco torpe, quiso jugar con él. Cuando llegó corriendo, el oso levantó su gran cabeza y gruñó con fuerza. Dino sintió un poco de miedo y dio un paso atrás. Pero el oso, en realidad, era amigable. Se puso a buscar una piedra para lanzársela a Dino y jugar a la pelota. Dino, después de la sorpresa, movió la cola y comió la comida que llevaba en su mochila. Al final, se pusieron a jugar juntos hasta que el sol se puso. Fue un día muy especial para el perro y el oso.",
      audio: "assets/Ejemplo1.m4a"
    },
    2: {
      texto: "Un hombre de pueblo llamado Francisco tenía un problema muy grande. Había perdido su libro favorito, un grueso volumen de historias de dragones. Su amigo Pedro, que era muy tranquilo, le prometió ayudar a encontrarlo. Primero, revisaron la biblioteca del pueblo, pero no estaba. Después, preguntaron en la plaza a una señora que paseaba a su perro. La señora les dijo que había visto un libro verde cerca del puente sobre el río. Corrieron al puente y, entre las flores y la hierba, encontraron el libro de Francisco, completamente mojado por la lluvia. Francisco, triste pero contento, lo secó al sol y prometió no volver a perderlo.",
      audio: "assets/Ejemplo2.m4a"
    },
    3: {
      texto: "Esta mañana, he ido a visitar a mi abuela a su casa en el campo. Ella me ha preparado un vaso de leche con galletas y me ha contado una historia muy antigua. Me ha dicho que hace muchos años, en su huerto, había una vaca que se llamaba \"Baca\" y un buey llamado \"Buey\". También había una higuera enorme donde los pájaros hacían sus nidos. Mi abuela dice que ha visto cosas asombrosas allí. Una vez, oyó un ruido extraño y vio una hoja que se movía sola. Resultó ser un erizo que había hecho su hogar bajo las hortensias. Después de la merienda, me puse a jugar con mi hermana y hasta nos atrevimos a subir al árbol más alto. Fue una hora inolvidable.",
      audio: "assets/Ejemplo3.m4a"
    }
  };

  // ====== Referencias DOM ======
  const ejemploSelect = document.getElementById('ejemploSelect');
  const textoInput = document.getElementById('textoInput');
  const btnPlay = document.getElementById('btnPlayAudio');
  const speedSelect = document.getElementById('speedSelect');
  const audioPlayer = document.getElementById('audioPlayer');
  const tiempoActual = document.getElementById('tiempoActual');
  const tiempoTotal = document.getElementById('tiempoTotal');
  const btnAnalizar = document.getElementById('btnAnalizar');

  const resultadosDiv = document.getElementById('resultados');
  const textoOriginalDisplay = document.getElementById('textoOriginalDisplay');
  const scoreDisplay = document.getElementById('scoreDisplay');
  const nivelDisplay = document.getElementById('nivelDisplay');
  const erroresList = document.getElementById('erroresList');

  // Variable global que almacena el texto de referencia (oculto al usuario)
  let currentReferenceText = '';

  // ====== Cargar ejemplo al seleccionar (sin mostrar el texto) ======
  function cargarEjemplo(id) {
    const ej = EJEMPLOS[id];
    if (!ej) return;

    // Guardar el texto de referencia en memoria (NUNCA se muestra en el textarea)
    currentReferenceText = ej.texto;

    // Cargar el audio correspondiente
    audioPlayer.src = ej.audio;
    audioPlayer.load();

    // Limpiar el área de texto y poner el placeholder
    textoInput.value = '';
    textoInput.placeholder = 'Escucha el audio y escribe aquí lo que entiendas…';

    // Ocultar resultados anteriores
    resultadosDiv.classList.add('hidden');

    // Resetear tiempos del reproductor
    tiempoActual.textContent = '0:00';
    tiempoTotal.textContent = '0:00';
    btnPlay.textContent = '▶ Reproducir';
  }

  ejemploSelect.addEventListener('change', (e) => {
    const id = parseInt(e.target.value);
    cargarEjemplo(id);
  });

  // Cargar el primer ejemplo por defecto (sin mostrar el texto)
  cargarEjemplo(1);

  // ====== Reproductor de audio (sin cambios) ======
  function formatTime(seconds) {
    if (isNaN(seconds)) return '0:00';
    const m = Math.floor(seconds / 60);
    const s = Math.floor(seconds % 60);
    return `${m}:${s.toString().padStart(2, '0')}`;
  }

  audioPlayer.addEventListener('loadedmetadata', () => {
    tiempoTotal.textContent = formatTime(audioPlayer.duration);
  });

  audioPlayer.addEventListener('timeupdate', () => {
    tiempoActual.textContent = formatTime(audioPlayer.currentTime);
  });

  btnPlay.addEventListener('click', () => {
    if (audioPlayer.paused) {
      audioPlayer.play();
      btnPlay.textContent = '⏸ Pausa';
    } else {
      audioPlayer.pause();
      btnPlay.textContent = '▶ Reproducir';
    }
  });

  audioPlayer.addEventListener('ended', () => {
    btnPlay.textContent = '▶ Reproducir';
    tiempoActual.textContent = formatTime(0);
  });

  speedSelect.addEventListener('change', () => {
    audioPlayer.playbackRate = parseFloat(speedSelect.value);
  });

  // ====== Análisis real con el backend C++ ======
  btnAnalizar.addEventListener('click', async () => {
    const textoUsuario = textoInput.value.trim();
    if (!textoUsuario) {
      alert('Por favor, escribe lo que hayas escuchado antes de analizar.');
      return;
    }
    if (!currentReferenceText) {
      alert('No hay texto de referencia. Selecciona un ejemplo primero.');
      return;
    }

    btnAnalizar.disabled = true;
    btnAnalizar.textContent = 'Analizando...';

    try {
      const response = await fetch('/api/analizar', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          textoReferencia: currentReferenceText,   // texto oculto en memoria
          textoUsuario: textoUsuario               // lo que escribió el usuario
        })
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(errorData.error || 'Error en el servidor');
      }

      const resultado = await response.json();
      mostrarResultados(resultado);
    } catch (error) {
      alert('Error al analizar: ' + error.message);
      console.error(error);
    } finally {
      btnAnalizar.disabled = false;
      btnAnalizar.textContent = '🔍 Analizar Texto';
    }
  });

  // ====== Renderizado de resultados (ahora muestra el texto de referencia) ======
  function mostrarResultados(resultado) {
    resultadosDiv.classList.remove('hidden');

    // --- Mostrar el texto de referencia (ahora visible) con resaltado ---
    const ref = resultado.palabraEsperada;  // viene del backend
    const errores = resultado.errores || [];
    const errorMap = new Map();
    errores.forEach(err => {
      errorMap.set(err.posicion, err);
    });

    let html = '';
    for (let i = 0; i < ref.length; i++) {
      const ch = ref[i];
      if (errorMap.has(i)) {
        const err = errorMap.get(i);
        const tipo = err.tipoError || 'OTRO';
        const desc = err.descripcion || '';
        const letraEsp = err.letraEsperada || ch;
        const letraIng = err.letraIngresada || '∅';
        html += `<span class="error-highlight ${tipo}" title="${desc} (esperado: ${letraEsp}, ingresado: ${letraIng})">${ch}</span>`;
      } else {
        html += ch;
      }
    }
    textoOriginalDisplay.innerHTML = html;

    // --- Score y nivel ---
    const score = (resultado.scoreGlobal * 100).toFixed(0);
    scoreDisplay.textContent = `Puntuación: ${score}% de acierto`;
    nivelDisplay.textContent = `Nivel de riesgo: ${resultado.nivelRiesgo}`;
    nivelDisplay.className = `nivel ${resultado.nivelRiesgo}`;

    // --- Lista detallada de errores ---
    erroresList.innerHTML = '';
    if (errores.length > 0) {
      const titulo = document.createElement('p');
      titulo.textContent = 'Diferencias detectadas:';
      erroresList.appendChild(titulo);

      errores.forEach((err) => {
        const item = document.createElement('div');
        item.className = 'error-item';

        const posSpan = document.createElement('span');
        posSpan.textContent = `Pos ${err.posicion}:`;
        item.appendChild(posSpan);

        const espSpan = document.createElement('span');
        espSpan.textContent = `"${err.letraEsperada}" →`;
        item.appendChild(espSpan);

        const usrSpan = document.createElement('span');
        usrSpan.textContent = `"${err.letraIngresada || '∅'}"`;
        item.appendChild(usrSpan);

        const tipoSpan = document.createElement('span');
        tipoSpan.className = 'tipo';
        tipoSpan.textContent = err.tipoError.replace('_', ' ');
        item.appendChild(tipoSpan);

        const descSpan = document.createElement('span');
        descSpan.textContent = err.descripcion || '';
        item.appendChild(descSpan);

        const audioBtn = document.createElement('button');
        audioBtn.className = 'audio-btn';
        audioBtn.textContent = '🔊';
        audioBtn.title = 'Escuchar fonema';
        audioBtn.addEventListener('click', () => {
          const utterance = new SpeechSynthesisUtterance(err.letraEsperada);
          utterance.lang = 'es-ES';
          utterance.rate = 1.0;
          window.speechSynthesis.cancel();
          window.speechSynthesis.speak(utterance);
        });
        item.appendChild(audioBtn);

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

  // ====== Tecla Enter+Ctrl en el textarea ======
  textoInput.addEventListener('keydown', (e) => {
    if (e.key === 'Enter' && e.ctrlKey) {
      btnAnalizar.click();
    }
  });
});