#include <AFMotor.h>
#include <Servo.h>

// ==========================================
//   CONFIGURACIÓN DE HARDWARE (ARDUINO MEGA)
// ==========================================

// 1. MOTOR DE LA CINTA (Shield L293D - Puerto M1)
AF_DCMotor motorCinta(1); 

// 2. SERVOS
Servo servoMaduro;  // Pin 9 
Servo servoVerde;   // Pin 10

const int PIN_SERVO_MADURO = 9;
const int PIN_SERVO_VERDE  = 10;

// 3. SENSORES Y BOTONES
const int switchPin = A7;   // Interruptor Maestro
const int trigPin   = A8;   // Sensor Ultrasonico Trig
const int echoPin   = A9;   // Sensor Ultrasonico Echo
const int pinLed    = A10;  // Led indicador

// ====================================================
// ⚠️ CALIBRACIÓN DE TIEMPOS
// ====================================================
const int DISTANCIA_STOP = 12; 

// VELOCIDAD DE TRABAJO (BAJADA A 70)
const int VELOCIDAD_TRABAJO = 70;

// TIEMPOS DE VIAJE
// Base para MADURO: 2 segundos
const unsigned long TIEMPO_VIAJE_MADURO = 2000; 

// Base para VERDE: 1 segundo MÁS que el Maduro (Antes era +2000)
// Esto hace que active 1 segundo antes que la configuración previa
const unsigned long TIEMPO_VIAJE_VERDE  = TIEMPO_VIAJE_MADURO + 1000; 

// Tiempos de retención del brazo
const unsigned long TIEMPO_RETENCION_MADURO = 3000; 
const unsigned long TIEMPO_RETENCION_VERDE  = 5000; 

// Tiempo de posicionamiento bajo cámara
const unsigned long TIEMPO_POSICIONAMIENTO = 800; 
// ====================================================

void setup() {
  Serial.begin(9600);
  Serial.println("✅ SISTEMA TOMATAIT: LISTO (VELOCIDAD 70)");

  // Comunicación con PICO W
  Serial1.begin(9600); 

  // CONFIGURACIÓN MOTOR
  motorCinta.setSpeed(VELOCIDAD_TRABAJO); 
  motorCinta.run(RELEASE);  

  pinMode(switchPin, INPUT_PULLUP);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(pinLed, OUTPUT);

  servoMaduro.attach(PIN_SERVO_MADURO);
  servoVerde.attach(PIN_SERVO_VERDE);
  
  // === POSICIONES INICIALES DIFERENTES ===
  // Maduro: Reposo en 180 (Lado A)
  servoMaduro.write(180);
  
  // Verde: Reposo en 0 (Lado B - Contrario)
  servoVerde.write(0);
}

void loop() {
  // === 1. VERIFICACIÓN MAESTRA ===
  if (digitalRead(switchPin) == HIGH) {
      apagarSistema();
      return; 
  }

  // === 2. BANDA AVANZANDO ===
  digitalWrite(pinLed, HIGH); 
  
  // Velocidad constante a 70
  motorCinta.setSpeed(VELOCIDAD_TRABAJO);
  motorCinta.run(FORWARD); 
    
  // --- FILTRO DE SENSOR ---
  long distancia = medirDistancia();
  
  if (distancia > 0 && distancia <= DISTANCIA_STOP) {
      // Doble confirmación
      delay(50); 
      long confirmacion = medirDistancia();

      if (confirmacion > 0 && confirmacion <= DISTANCIA_STOP) {
          
          Serial.println("👀 TOMATE DETECTADO. POSICIONANDO...");
          
          // Avanzamos para ponerlo bajo la cámara
          if (!esperarSeguro(TIEMPO_POSICIONAMIENTO)) return;

          // PAUSA LÓGICA PARA FOTO (Banda sigue rodando suave a 70)
          Serial.println("📸 Esperando clasificación IA (En movimiento)...");
          
          while(Serial1.available() > 0) { Serial1.read(); }

          bool datoRecibido = false;
          char clasificacion = 'N';

          while (!datoRecibido) {
            if (digitalRead(switchPin) == HIGH) { apagarSistema(); return; }

            if (Serial1.available() > 0) {
              clasificacion = Serial1.read();
              if (clasificacion != '\n' && clasificacion != '\r' && clasificacion != ' ') {
                 datoRecibido = true;
              }
            }
            delay(10);
          }

          Serial.print("📩 Clasificación:  "); Serial.println(clasificacion);

          // === ACTUACIÓN SERVOS ===
          motorCinta.setSpeed(VELOCIDAD_TRABAJO);
          motorCinta.run(FORWARD);

          if (clasificacion == 'R') { 
            Serial.println("⏳ Viajando a MADURO...");
            if (!esperarSeguro(TIEMPO_VIAJE_MADURO)) return;
            
            Serial.println("💥 SERVO MADURO (Normal)");
            // Maduro usa lógica normal (180 -> 90 -> 180)
            golpearNormal(servoMaduro, TIEMPO_RETENCION_MADURO); 
          } 
          else if (clasificacion == 'U' || clasificacion == 'H') { 
            Serial.println("⏳ Viajando a VERDE...");
            if (!esperarSeguro(TIEMPO_VIAJE_VERDE)) return;
            
            Serial.println("💥 SERVO VERDE (Invertido)");
            // Verde usa lógica invertida (0 -> 90 -> 0)
            golpearInvertido(servoVerde, TIEMPO_RETENCION_VERDE); 
          }
          else {
            Serial.println("⚠️ Descarte.");
            if (!esperarSeguro(5000)) return;
          }

          Serial.println("♻️ Fin de ciclo. Esperando salida del sensor...\n");
          esperarSeguro(2000); 
      }
  }
  
  delay(50); 
}

// ==========================================
//   FUNCIONES AUXILIARES
// ==========================================

void apagarSistema() {
    digitalWrite(pinLed, LOW);
    motorCinta.run(RELEASE); 
    // Guardar brazos en sus respectivos lados
    servoMaduro.write(180);
    servoVerde.write(0);
}

bool esperarSeguro(unsigned long tiempoMs) {
    unsigned long inicio = millis();
    while (millis() - inicio < tiempoMs) {
        if (digitalRead(switchPin) == HIGH) { 
            apagarSistema();
            return false; 
        }
        delay(10);
    }
    return true; 
}

long medirDistancia() {
  digitalWrite(trigPin, LOW); delayMicroseconds(2);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duracion = pulseIn(echoPin, HIGH, 30000); 
  if (duracion == 0) return 999; 
  return (duracion * 0.034 / 2);
}

// --- FUNCIÓN GOLPE NORMAL (Para Maduro) ---
// Reposo: 180 -> Activo: 90
void golpearNormal(Servo &s, unsigned long tiempoRetencion) {
  s.write(135);  
  if(!esperarSeguro(tiempoRetencion)) return;   
  s.write(180); 
  if(!esperarSeguro(500)) return;   
}

// --- FUNCIÓN GOLPE INVERTIDO (Para Verde) ---
// Reposo: 0 -> Activo: 90
void golpearInvertido(Servo &s, unsigned long tiempoRetencion) {
  s.write(25);  
  if(!esperarSeguro(tiempoRetencion)) return;   
  s.write(0); 
  if(!esperarSeguro(500)) return;   
}