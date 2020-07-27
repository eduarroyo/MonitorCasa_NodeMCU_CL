#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h> //https://github.com/tzapu/WiFiManager
#include <ESP8266HTTPClient.h>
#include <FS.h>

#define NOMBRE_FICHERO "config.ini"

// Para usar el botón flash como entrada para borrar los datos de la wifi.
#define BT_FLASH 0
int inicioPulsarFlash = 0;

bool mensajeClienteIDNoConfigurado = false;
const char *url = "http://monitor-actividad.herokuapp.com/actualizar";
const char *host = "monitor-actividad.herokuapp.com";
char *json = "{nada por aqui}"; // Aquí pondremos el contenido json que enviamos al servidor periódicamente. He comprobado que si la inicializo como cadena vacía, no funciona.
const int periodoEjecucionMilis = 1000;
const int tiempoPulsacionResetMillis = 3000; // Mantener siempre a un valor >= 3*periodoEjecucionMilis
const int periodoSolicitudMilis = 15 * 60 * 1000;
unsigned long ultimaSolicitud = 0;

// Creamos una instancia de la clase WiFiManager
WiFiManager wifiManager;


/***************************************************************** MÉTODOS PARA LEER/ESCRIBIR CLIENTEID EN LA EEPROM */

void leerClienteId() {
    String clienteId;
  
    File f = SPIFFS.open(NOMBRE_FICHERO, "r");

    if (!f) {
        Serial.println("Error al abrir el fichero de configuración.");
    }
    else
    {
        Serial.println("Leyendo el fichero:");
        clienteId = f.readString();
        f.close();
        Serial.println("Lectura finalizada: " + clienteId);
        sprintf(json, "{\"monitorId\":\"%s\"}", clienteId.c_str());
        Serial.println("JSON: " + String(json));
    }
}

void establecerClienteId(String clienteIdNuevo) {
    
    File f = SPIFFS.open(NOMBRE_FICHERO, "w");
    if(!f) {
        Serial.println("Error al abrir el fichero.");
    } else {
        f.print(clienteIdNuevo);
        f.close();
    }

    leerClienteId();
}
/*********************************************************************************************************************/


/************************************************************************* MÉTODOS DE CONFIGURACIÓN DEL SERVICIO WEB */
// Servidor para recibir comandos mediante solicitudes HTTP.
ESP8266WebServer webServer(80);

void handleRoot() {
    if(webServer.args() == 1 && webServer.argName(0) == "id") {
        Serial.print("Solicitud de cambio de clienteId: ");
        Serial.println(webServer.arg("id"));
        establecerClienteId(webServer.arg("id"));
        Serial.print("Nuevo ID de cliente establecido: ");
        Serial.println(json);
        webServer.send(200, "text/plain", json);
        ultimaSolicitud = 0; // Para que envíe una actualización en el siguiente ciclo.
    } else if(webServer.args() == 0) {
        if(strlen(json) == 0) {
            webServer.send(200, "text/plain", "El ID de cliente no ha sido configurado todavía.");
        } else {
            char* mensaje;
            sprintf(mensaje, "Su ID de cliente es %s.", json);
            Serial.println(mensaje);
            webServer.send(200, "text/plain", mensaje);
        }
    } else {
        webServer.send(400, "text/plain", "Solicitud mal formada.");
    }
}

// Función que se ejecutara en URI desconocida
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

/*********************************************************************************************************************/

void setup() {
    Serial.begin(9600);

    // Configurar la entrada digital correspondiente al botón FLASH.
    pinMode(BT_FLASH, INPUT);

    // Configurar la salida digital correspondiente al LED de la placa.
    pinMode(LED_BUILTIN, OUTPUT);

    // Creamos AP y portal cautivo
    wifiManager.autoConnect("MonitorCasaCL-WiFi");
    InitServer();

    if(!SPIFFS.begin()) {
        Serial.println("Error al inicializar el sistema de ficheros.");
    } else /*if(!SPIFFS.format())*/ {
        //Serial.println("Error al formatear el sistema de ficheros");
    //} else {
        Serial.println("Sistema de ficheros inicializado correctamente");
    }

    // Leer el clienteId al arrancar.
    leerClienteId();
}

// Enciende el led durante <duracionMs> milisegundos y lo apaga durante otros tantos. Repite <repeticiones> veces.
void parpadeo(int repeticiones, int duracionMs) {
    for(int i= 0; i < repeticiones; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(duracionMs);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(duracionMs);
    }
}

HTTPClient http;
WiFiClient client;

void loop() {
    int httpCode = 0;

    if(digitalRead(BT_FLASH) == HIGH) // Si el botón FLASH no está pulsado, realizar el proceso normal.
    { 
        // Si el botón no está pulsado, ponemos el inicio de pulsación a 0.
        digitalWrite(LED_BUILTIN, HIGH); // Apagar el led.
        inicioPulsarFlash = 0;

        if (WiFi.status() != WL_CONNECTED) { // No conectado a WIFI.
            // Si no hay wifi no se pueden enviar los datos ni atender solicitudes web recibidas.
            Serial.println("Wifi no conectada.");
        } else {
            if(strlen(json) == 0) {
                if(!mensajeClienteIDNoConfigurado) {
                    Serial.println("El ID de cliente no ha sido configurado todavía.");
                    Serial.print("Configúrelo en ");
                    Serial.print(WiFi.localIP().toString());
                    Serial.println("/actualizar?id=<su-id-de-cliente>");
                    mensajeClienteIDNoConfigurado = true;
                }
            } else if(ultimaSolicitud == 0 || millis() - ultimaSolicitud > periodoSolicitudMilis) { // Ha transcurrido el periodo desde la última solicitud.
                http.begin(client, url);
                http.addHeader("Content-Type", "application/json");
                http.addHeader("Host", host);
                Serial.print("Contenido: ");
                Serial.println(json);
                httpCode = http.POST(json); // Realizar petición
                ultimaSolicitud = millis(); // Guardar el instante de la última solicitud.
                if(httpCode == 200) {
                    Serial.println("POST OK");
                    parpadeo(1, 100);
                } else if (httpCode > 0) {
                    Serial.print("POST NO OK: ");
                    Serial.println(httpCode);
                    // Dos parpadeos del led: error HTTP.  
                    parpadeo(2, 100);   
                } else {
                    Serial.print("POST ERROR: ");
                    Serial.println(http.errorToString(httpCode));
                    // Tres parpadeos del LED: Error al enviar la solicitud.
                    parpadeo(3, 100);
                }

                http.end();
                Serial.println("Cliente HTTP cerrado");
            }
                
            // Por último manejar solicitudes http recibidas.
            webServer.handleClient();
            //Serial.println("Solicitudes entrantes comprobadas.");
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
            mensajeClienteIDNoConfigurado = false;
            json = "";
            ESP.restart();
        }
    }

    //Serial.flush();
    // A dormir durante periodoEjecucionMillis
    //delay(periodoEjecucionMilis);
}