#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ESP8266HTTPClient.h>

// Para usar el botón flash como entrada para borrar los datos de la wifi.
#define BT_FLASH 0
int inicioPulsarFlash = 0;

const char *url = "http://monitor-actividad.herokuapp.com/actualizar";
const char *host = "monitor-actividad.herokuapp.com";
const char *json = "{\"monitorId\":\"%s\"}"; // <- Reemplazar los asteriscos por el código de cliente del dispositivo
const int periodoEjecucionMilis = 1000;
const int tiempoPulsacionResetMillis = 3000; // Mantener siempre a un valor >= 3*periodoEjecucionMilis
const int periodoSolicitudMilis = 10000;//15 * 60 * 1000;
String clienteId = "";
int ultimaSolicitud;

// Creamos una instancia de la clase WiFiManager
WiFiManager wifiManager;

// // Servidor para recibir comandos mediante solicitudes HTTP.
ESP8266WebServer webServer(80);

void handleRoot() {
    if(webServer.args() == 1 && webServer.argName(0) == "id") {
        clienteId = webServer.arg("id");
        Serial.println("Nuevo ID de cliente establecido: " + clienteId);
        webServer.send(200, clienteId.c_str());
    } else if(webServer.args() == 0) {
        if(clienteId.length() == 0) {
            Serial.println("El ID de cliente no ha sido configurado todavía.");
        } else {
            Serial.println("Su ID de cliente es " + clienteId);
        }
    } else {
        webServer.send(400, "Solicitud mal formada.");
    }
}

// Funcion que se ejecutara en URI desconocida
void handleNotFound() {
    webServer.send(404, "text/plain", "Not found");
}

void InitServer() {
    // Ruteo para '/'
    webServer.on("/", handleRoot);
    // Ruteo para URI desconocida
    webServer.onNotFound(handleNotFound);

    // Iniciar servidor
    webServer.begin();
    Serial.println("Servidor HTTP iniciado.");
}

void setup() {
    Serial.begin(9600);

    // Configurar la entrada digital correspondiente al botón FLASH.
    pinMode(BT_FLASH, INPUT);

    // Configurar la salida digital correspondiente al LED de la placa.
    pinMode(LED_BUILTIN, OUTPUT);

    // Creamos AP y portal cautivo
    wifiManager.autoConnect("ESP8266Temp");

    InitServer();
}

void loop() {
    HTTPClient http;
    WiFiClient client;
    char* datos = "";

    if(digitalRead(BT_FLASH) == HIGH) // Si el botón FLASH no está pulsado, realizar el proceso normal.
    { 
        // Si el botón no está pulsado, ponemos el inicio de pulsación a 0.
        digitalWrite(LED_BUILTIN, HIGH); // Apagar el led.
        inicioPulsarFlash = 0;

        if (WiFi.status() != WL_CONNECTED) { // No conectado a WIFI.
            // Si no hay wifi no se pueden enviar los datos ni atender solicitudes web recibidas.
            Serial.println("Wifi no conectada.");
        } else {
            if(clienteId.length() == 0) {
                Serial.println("El ID de cliente no ha sido configurado todavía.");
            } else if(millis() - ultimaSolicitud > periodoSolicitudMilis) { // Ha transcurrido el periodo desde la última solicitud.
                sprintf(datos, json, clienteId.c_str()); // Componer la cadena JSON que enviamos en el POST.
                Serial.println("Enviando actualización: " + String(datos));
                http.begin(client, url);
                Serial.print("[HTTP] POST... ");
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Cache-Control", "no-cache");
                http.addHeader("Host", host);
                int httpCode = http.POST(datos); // Realizar petición
                ultimaSolicitud = millis(); // Guardar el instante de la última solicitud.
                if(httpCode == 200) {
                    // Un parpadeo del LED: todo correcto.
                    digitalWrite(LED_BUILTIN, LOW);
                    delay(50);
                    digitalWrite(LED_BUILTIN, HIGH);
                } else if (httpCode > 0) {
                    Serial.printf("Código respuesta: %d\n", httpCode);       
                    // Dos parpadeos del led: error HTTP.     
                    digitalWrite(LED_BUILTIN, LOW);
                    delay(100);
                    digitalWrite(LED_BUILTIN, HIGH);
                    delay(100);
                    digitalWrite(LED_BUILTIN, LOW);
                    delay(100);
                    digitalWrite(LED_BUILTIN, HIGH);
                } else {
                    Serial.printf("Error: %s\n", http.errorToString(httpCode).c_str());
                    // Tres parpadeos del LED: Error al enviar la solicitud.
                    digitalWrite(LED_BUILTIN, LOW);
                    delay(100);
                    digitalWrite(LED_BUILTIN, HIGH);
                    delay(100);
                    digitalWrite(LED_BUILTIN, LOW);
                    delay(100);
                    digitalWrite(LED_BUILTIN, HIGH);
                    delay(100);
                    digitalWrite(LED_BUILTIN, LOW);
                    delay(100);
                    digitalWrite(LED_BUILTIN, HIGH);
                    delay(100);
                }

                http.end();
            }
                
            // Por último manejar solicitudes http recibidas.
            Serial.println("Atendiendo solicitudes web.");
            webServer.handleClient();
        }
    } else {
        
        // Comportamiento con el botón flash pulsado:
        // Si se mantiene pulsado 3 segundos, resetear el dispositivo.

        // La primera vez establecemos inicioPulsarFlash al instante de tiempo actual.
        if(inicioPulsarFlash == 0) {
            Serial.println("Pulsación de botón FLASH detectada.");
            inicioPulsarFlash = millis();
            digitalWrite(LED_BUILTIN, LOW); // Encender el LED mientras el botón FLASH esté pulsado.
        } 
        
        // Si el botón se mantiene pulsado durante 3'', resetear.
        if(millis()-inicioPulsarFlash > tiempoPulsacionResetMillis) {
            Serial.println("Pulsación larga de botón FLASH: Se borran los datos de configuración y se reinicia el dispositivo.");
            wifiManager.resetSettings();
            clienteId = "";
            ESP.restart();
        }
    }

    // A dormir durante periodoEjecucionMillis
    delay(periodoEjecucionMilis);
}