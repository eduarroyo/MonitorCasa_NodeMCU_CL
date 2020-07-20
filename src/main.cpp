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
const char *json = "{\"monitorId\":%d}"; // <- Reemplazar los asteriscos por el código de identificación del usuario de Telegram
const int periodoEjecucionMilis = 2000;
const int periodoSolicitudMilis = 10000;//15 * 60 * 1000;
int telegramId = 0;
int ultimaSolicitud;

// Creamos una instancia de la clase WiFiManager
WiFiManager wifiManager;

// // Servidor para recibir comandos mediante solicitudes HTTP.
// ESP8266WebServer webServer(80);

// // Funcion que se ejecutara en la URI '/'
// void handleRoot()
// {
//     webServer.send(200, "text/plain", "Hola mundo!");
// }

// // Funcion que se ejecutara en URI desconocida
// void handleNotFound()
// {
//     webServer.send(404, "text/plain", "Not found");
// }

// void InitServer()
// {
//     // Ruteo para '/'
//     webServer.on("/", handleRoot);

//     // Ruteo para '/inline' usando función lambda
//     webServer.on("/inline", []() {
//         webServer.send(200, "text/plain", "Esto también funciona");
//     });

//     // Ruteo para URI desconocida
//     webServer.onNotFound(handleNotFound);

//     // Iniciar servidor
//     webServer.begin();
//     Serial.println("HTTP server started");
// }

void setup()
{
    Serial.begin(9600);

    // Initialize the button.
    pinMode(BT_FLASH, INPUT);

    // Creamos AP y portal cautivo
    wifiManager.autoConnect("ESP8266Temp");

    //InitServer();
}

void loop()
{
    HTTPClient http;
    WiFiClient client;
    char* datos = "";

    //webServer.handleClient();

    if(digitalRead(BT_FLASH) == LOW)
    {
        // Pulsar botón flash durante 3'' para resetear la configuración.
        if(inicioPulsarFlash == 0) {
            inicioPulsarFlash = millis();
        } else if(millis()-inicioPulsarFlash > 3000) {
            Serial.printf("Se borran los datos de configuración y se reinicia el dispositivo.");
            wifiManager.resetSettings();
            telegramId = 0;
            ESP.restart();
        }
    }
    else 
    {
        inicioPulsarFlash = 0;
        if(millis() - ultimaSolicitud > periodoSolicitudMilis)
        {
            if (WiFi.status() == WL_CONNECTED)
            {
                sprintf(datos, json, telegramId);
                http.begin(client, url);
                Serial.print("[HTTP] POST... ");
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Cache-Control", "no-cache");
                http.addHeader("Host", host);
                int httpCode = http.POST(datos); // Realizar petición

                if (httpCode > 0)
                {
                    Serial.printf("Código respuesta: %d\n", httpCode);
                    // if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                    // {
                    //     String payload = http.getString(); // Obtener respuesta
                    //     Serial.println(payload);           // Mostrar respuesta por serial
                    // }
                    ultimaSolicitud = millis();
                }
                else
                {
                    Serial.printf("Error: %s\n", http.errorToString(httpCode).c_str());
                }

                http.end();
            }
            else
            {
                Serial.printf("Wifi no conectada");
            }
        }

        delay(periodoEjecucionMilis);
    }
}