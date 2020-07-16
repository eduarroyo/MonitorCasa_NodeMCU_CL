#include <ESP8266WiFi.h>

const char* ssid = "TVSANTAELLA-ARENAL";
const char* password = "3fFxEUJb";

WiFiServer server(80);
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

  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) //Si hay un cliente presente
  { 
    Serial.println("Nuevo Cliente");
    
    //esperamos hasta que hayan datos disponibles
    while(!client.available()&&client.connected())
    {
    delay(1);
    }
    
    // Leemos la primera línea de la petición del cliente.
    String linea1 = client.readStringUntil('r');
    Serial.println(linea1);
    
    client.flush(); 
                
    Serial.println("Enviando respuesta...");   
    //Encabezado http    
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");// La conexión se cierra después de finalizar de la respuesta
    client.println();
    //Pagina html  para en el navegador
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head><title>MonitorCasa NodeMCU CL</title>");
    client.println("<body>");
    client.println("<h1 align='center'>MonitorCasa NodeMCU CL</h1>");
    client.println("<div style='text-align:center;'>");
    client.println("<br />");
    client.println("Sirviendo esta página desde NodeMCU.");
    client.println("</div>");
    client.println("</body>");
    client.println("</html>");
    
    delay(1);
    Serial.println("respuesta enviada");
    Serial.println();
  }
}