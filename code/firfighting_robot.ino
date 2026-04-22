#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <Servo.h>

const char* ssid = "ARD";
const char* password = "12345678";

#define IN1 D1
#define IN2 D2
#define IN3 D3
#define IN4 D4
#define ENA D5
#define ENB D6
#define RELAY D7

#define SERVO_PIN D0
Servo myServo;
bool servoRunning = false;

ESP8266WebServer server(80);

bool touchMode = true;
bool pressMode = false;

bool touchForward = false;
bool touchBackward = false;
bool touchLeft = false;
bool touchRight = false;

void stopMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}

void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}

void handleRoot() {
  String html = R"====(
<!DOCTYPE html>
<html>
<head>
<title>ESP8266 Robot</title>
<style>
body { text-align:center; font-family: Arial; }
button {
  width:120px; height:55px;
  font-size:18px;
  margin:6px;
  background:#ddd;
  border:none;
  border-radius:8px;
}
.active { background:darkgreen; color:white; }
</style>

<script>
function send(cmd, btn){
  fetch('/'+cmd);
  document.querySelectorAll('button').forEach(b=>b.classList.remove('active'));
  if(btn) btn.classList.add('active');
}
</script>

</head>
<body>

<h2>WiFi Robot Control</h2>

<button onclick="send('mode_touch',this)">Touch Mode</button>
<button onclick="send('mode_press',this)">Press Mode</button>
<br><br>

<button onclick="send('forward',this)">Forward</button><br>
<button onclick="send('left',this)">Left</button>
<button onclick="send('stop')">Stop</button>
<button onclick="send('right',this)">Right</button><br>
<button onclick="send('backward',this)">Backward</button>

<h3>Pump</h3>
<button onclick="send('pump_on',this)">Pump ON</button>
<button onclick="send('pump_off')">Pump OFF</button>

<h3>Servo</h3>
<button onclick="send('servo_on',this)">Servo ON</button>
<button onclick="send('servo_off')">Servo OFF</button>

</body>
</html>
)====";

  server.send(200, "text/html", html);
}

void setup() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(ENB, OUTPUT);
  pinMode(RELAY, OUTPUT);

  stopMotor();
  digitalWrite(RELAY, LOW);

  analogWrite(ENA, 900);
  analogWrite(ENB, 900);

  myServo.attach(SERVO_PIN);
  myServo.write(0);

  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  server.on("/", handleRoot);

  server.on("/mode_touch", [](){ touchMode = true; pressMode = false; server.send(200); });
  server.on("/mode_press", [](){ touchMode = false; pressMode = true; server.send(200); });

  server.on("/forward", [](){ if(touchMode){ touchForward=!touchForward; touchForward?forward():stopMotor(); } server.send(200); });
  server.on("/backward", [](){ if(touchMode){ touchBackward=!touchBackward; touchBackward?backward():stopMotor(); } server.send(200); });
  server.on("/left", [](){ if(touchMode){ touchLeft=!touchLeft; touchLeft?left():stopMotor(); } server.send(200); });
  server.on("/right", [](){ if(touchMode){ touchRight=!touchRight; touchRight?right():stopMotor(); } server.send(200); });

  server.on("/stop", [](){
    stopMotor();
    touchForward = touchBackward = touchLeft = touchRight = false;
    server.send(200);
  });

  server.on("/pump_on", [](){ digitalWrite(RELAY, HIGH); server.send(200); });
  server.on("/pump_off", [](){ digitalWrite(RELAY, LOW); server.send(200); });

  server.on("/servo_on", [](){ servoRunning = true; server.send(200); });
  server.on("/servo_off", [](){
    servoRunning = false;      // stop sweep
    // DO NOT reset position
    server.send(200);
  });

  server.begin();
}

/* -------- Servo variables for non-blocking sweep -------- */
int servoPos = 0;
int servoDir = 1;     // 1 = increasing, -1 = decreasing
unsigned long lastTime = 0;
int interval = 15;    // sweep speed (ms)

void loop() {
  server.handleClient();

  if (servoRunning) {
    unsigned long current = millis();
    if (current - lastTime >= interval) {
      lastTime = current;
      servoPos += servoDir;

      if (servoPos >= 180) servoDir = -1;
      if (servoPos <= 0) servoDir = 1;

      myServo.write(servoPos);
    }
  }
}
