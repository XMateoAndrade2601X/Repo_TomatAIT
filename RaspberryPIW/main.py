import network
import time
from machine import UART, Pin
from umqtt.simple import MQTTClient

# ================= CONFIGURACIÓN =================
SSID = "iPhone de Joe Jose"       # <--- PON TU WIFI EXACTO
PASSWORD = "jugodemaracuya"     # <--- PON TU CLAVE EXACTA
BROKER_IP = "172.20.10.9"       # <--- LA IP DE TU LAPTOPp
TOPIC = "robotica/frutas"       # El canal que escucha
# =================================================

# Comunicación Cableada con Arduino
# GP0 (TX de Pico) va al RX de Arduino
# GP1 (RX de Pico) va al TX de Arduino
uart = UART(0, baudrate=9600, tx=Pin(0), rx=Pin(1))

# 1. Conectar al WiFi
wlan = network.WLAN(network.STA_IF)
wlan.active(True)
wlan.connect(SSID, PASSWORD)
print("Conectando al WiFi...")
intentos = 0
while not wlan.isconnected() and intentos < 20:
    time.sleep(1)
    intentos += 1
    print(".")
if wlan.isconnected():
    print('✅ WiFi Conectado:', wlan.ifconfig())
else:
    print('❌ Error: No se pudo conectar al WiFi')

# Función que se activa cuando llega el mensaje desde la Laptop
def mensaje_recibido(topic, msg):
    try:
        texto = msg.decode('utf-8')
        print(f"📩 Recibido del WiFi: {texto}")
        
        # Convertir la palabra completa en una letra para el Arduino
        letra = 'N' # N de nada
        
        if texto == "ripe": 
            letra = 'R'
        elif texto == "unripe" or texto == "half_ripe": 
            letra = 'U'
        elif texto == "mold" or texto == "rotten": 
            letra = 'M'
            
        if letra != 'N':
            # Enviar la letra al Arduino por el cable
            uart.write(letra + '\n')
            print(f"➡️ Enviada letra '{letra}' al Arduino")
            
    except Exception as e:
        print(f"Error procesando mensaje: {e}")

# 2. Conectar al Servidor (Tu Laptop)
try:
    client = MQTTClient("Pico_Mensajero", BROKER_IP)
    client.set_callback(mensaje_recibido)
    client.connect()
    client.subscribe(TOPIC)
    print(f"✅ Conectado a la Laptop. Esperando órdenes...")
except Exception as e:
    print(f"❌ Error conectando a MQTT (Laptop): {e}")

# Bucle infinito: Solo escucha y pasa mensajes
while True:
    try:
        client.check_msg() # Revisar si hay mensajes nuevos
        time.sleep(0.01)
    except OSError:
        pass