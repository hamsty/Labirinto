#include <Arduino.h>
#if MESA == 0
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#endif
#include <Adafruit_NeoPixel.h>
#include <SPIFFS.h>
#include <FS.h>
#include <gamesirg3s.h>

#if MESA == 0
const char *casa = "Talles";
const char *senha = "talles12345";
static AsyncWebServer server(80);
#endif

void drawImage(int16_t x, int16_t y, String file)
{
  if (SPIFFS.exists(file))
  {
    fs::File hello_kitty = SPIFFS.open(file);
    uint8_t buffer[2];
    hello_kitty.read(buffer, 2);
    uint16_t height;
    memcpy(&height, buffer, sizeof(uint16_t));
    hello_kitty.read(buffer, 2);
    uint16_t width;
    memcpy(&width, buffer, sizeof(uint16_t));
    uint16_t bitmap[width * height];
    for (int i = 0; i < width * height; i++)
    {
      hello_kitty.read(buffer, 2);
      memcpy(&bitmap[i], buffer, sizeof(uint16_t));
    }
    display.drawRGBBitmap(x, y, bitmap, width, height);
  }
  else
  {
    Serial.println("Arquivo indisponível!");
  }
}

void setup()
{
  Serial.begin(115600);
  if (!SPIFFS.begin(true))
  {
    Serial.println("SPIFFS Mount Failed");
    return;
  }
#if MESA == 0
  WiFi.begin(casa, senha);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/index.html", "text/html", false); });
  server.on("/mesa.js", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/mesa.js", "text/javascript", false); });
  server.on("/show", HTTP_GET, [](AsyncWebServerRequest *request)
            {
    String img;
    for (int i = 0; i < 144; i++)
    {
      char linha[50];
      uint32_t pixel = display.getPixel(i);
      uint8_t red = pixel >> 16, green = pixel >> 8, blue = pixel;
      sprintf(linha, "RGB(%d,%d,%d)|", red, green, blue);
      img.concat(linha);
    }
    request->send_P(200, "text/plain", img.c_str()); });
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/favicon.ico", "image/x-icon", false); });
  server.begin();
  Serial.println("Servidor Iniciado");
  Serial.print("Acessa em casa http://");
  Serial.println(WiFi.localIP());
#endif
  client = new JoystickClient();
  while (!client->isFind())
  {
  }
  Serial.println("Joystick Achado!");
}

void loop()
{
  // put your main code here, to run repeatedly:
}