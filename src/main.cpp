#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "*****";
const char* password = "*****";
const char* url = "http://monitor-actividad.herokuapp.com/actualizar";
const char* host = "monitor-actividad.herokuapp.com";
const char* payload = "{\"monitorId\":*****}";
const int periodoSolicitudMilis = 15*60*1000;

IPAddress ip;

void setup() {
  Serial.begin(9600);
  delay(10);

  // Configuración del GPIO2
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  Serial.println();
  Serial.println();
  Serial.println("Conectandose a la red: ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi conectado");
  
  ip = WiFi.localIP();
  Serial.print("IP:");
  Serial.println(ip);
}

void loop() {
  HTTPClient http;
  WiFiClient client;
 
  if(WiFi.status() == WL_CONNECTED) {
    http.begin(client, url);
    Serial.print("[HTTP] POST... ");
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Cache-Control", "no-cache");
    http.addHeader("Host", host);
    int httpCode = http.POST(payload);  // Realizar petición

    if (httpCode > 0) {
      Serial.printf("Código respuesta: %d\n", httpCode);
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
        String payload = http.getString();   // Obtener respuesta
        Serial.println(payload);   // Mostrar respuesta por serial
      }
    } else {
      Serial.printf("Error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
  } else {
    Serial.printf("Wifi no conectada");
  }
 
  delay(periodoSolicitudMilis);
}