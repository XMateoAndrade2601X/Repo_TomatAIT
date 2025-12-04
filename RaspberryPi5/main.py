import cv2
from ultralytics import YOLO
import sys
import paho.mqtt.client as mqtt
import time

# ================= CONFIGURACIÓN =================
# IP DEL BROKER (Tu Laptop) - Asegúrate de que sea la correcta
BROKER_ADDRESS = "172.20.10.9" 
TOPIC = "robotica/frutas"

# Umbral de confianza para considerar válida la detección de madurez(75%)
CONF_UMBRAL = 0.75 
# =================================================

print("Cargando modelo YOLO (best.pt)...")
try:
    model = YOLO('./best.pt') 
except Exception as e:
    print(f"❌ Error cargando modelo: {e}")
    sys.exit(1)

# Cliente MQTT
client = mqtt.Client("RaspberryPi_Auto")

def run_auto_system():
    print(f"Conectando a MQTT en {BROKER_ADDRESS}...")
    try:
        # Nota: Si usas una versión muy nueva de paho-mqtt, el callback puede variar.
        # Este código funciona con la versión instalada en el Containerfile (<2.0.0)
        client.connect(BROKER_ADDRESS, 1883, 60)
        print("✅ CONEXIÓN MQTT EXITOSA.")
    except Exception as e:
        print(f"❌ Error conectando al Broker: {e}")
        sys.exit(1)

    print("Iniciando cámara...")
    # Configuración para velocidad (MJPG)
    cap = cv2.VideoCapture(0, cv2.CAP_V4L2)
    cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M', 'J', 'P', 'G'))
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, 800)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 600)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)

    if not cap.isOpened():
        print("❌ Error crítico: No se detecta cámara USB.")
        sys.exit(1)

    print("\n✅ SISTEMA VISUAL ACTIVO.")
    print("👁️ Esperando tomate estático...")

    while True:
        ret, frame = cap.read()
        if not ret: continue

        # Mostrar ventana de video
        cv2.imshow("Camara Clasificadora", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'): break

        # Inferencia con YOLO (verbose=False para limpiar consola)
        results = model(frame, verbose=False)
        
        mejor_clase = None
        mayor_conf = 0.0

        # Buscar la mejor detección
        for result in results:
            for box in result.boxes:
                conf = float(box.conf)
                # Solo si supera el 75% de confianza
                if conf > CONF_UMBRAL and conf > mayor_conf:
                    mayor_conf = conf
                    cls_id = int(box.cls)
                    mejor_clase = model.names[cls_id]

        # Si encontramos un tomate válido
        if mejor_clase:
            print(f"✅ DETECTADO: {mejor_clase} ({mayor_conf:.2f})")
            
            # 1. Enviar mensaje a la Laptop
            client.publish(TOPIC, mejor_clase)
            print(f"📡 Enviado '{mejor_clase}'. Esperando ciclo mecánico...")

            # 2. COOLDOWN (Tiempo de espera)
            # Esperamos 6 segundos mientras el Arduino mueve la banda y clasifica
            time.sleep(6) 
            
            # 3. Limpiar buffer de cámara (para tener imagen fresca al volver)
            for _ in range(5): cap.read()
            print("👁️ Buscando siguiente tomate...")

    cap.release()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    run_auto_system()