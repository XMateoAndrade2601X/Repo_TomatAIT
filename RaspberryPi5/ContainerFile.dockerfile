FROM python:3.9-slim-bookworm

WORKDIR /app

# 1. Instalar librerías del sistema necesarias para video y gráficos
RUN apt-get update && apt-get install -y \
    libgl1 \
    libglib2.0-0 \
    v4l-utils \
    libsm6 \
    libxext6 \
    libxrender-dev \
    libgtk2.0-dev \
    && rm -rf /var/lib/apt/lists/*

# 2. Instalar librerías de Python
# Importante: paho-mqtt<2.0.0 evita errores de compatibilidad
RUN pip install --no-cache-dir ultralytics opencv-python pyserial "paho-mqtt<2.0.0"

# 3. Copiar los archivos del proyecto al contenedorr
COPY best.pt .
COPY main.py .

# 4. Comando de inicio
CMD ["python", "main.py"]