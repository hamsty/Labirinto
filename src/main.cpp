#include <Arduino.h>
#if MESA == 0
#include <WiFi.h>
#include <WiFiClient.h>
#include <ESPmDNS.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#endif
#include <Cursor12x12.h>
#include <Adafruit_NeoPixel.h>
#include <SPIFFS.h>
#include <FS.h>
#include <gamesirg3s.h>

using namespace std;

#define NUM_SPACE 48
#define DATAPIN 15
#define GAME_VELOCITY 100
#define START pair<int, int>(1, 5)
#define END pair<int, int>(NUM_SPACE - 2, NUM_SPACE - 2 - 5)
#define CHARCOLOR 0xFFE0U
#define FINALRGB565 0xF800

#if MESA == 0
const char *casa = "Talles";
const char *senha = "talles12345";
static AsyncWebServer server(80);
#endif

static JoystickClient *client;

static bool jogoIniciado = false;
static bool jogoTerminado = false;

static Cursor12x12 display(NUM_SPACE, DATAPIN, NEO_GRB + NEO_KHZ800);
static pair<int, int> posicao;
static pair<int, int> posicaoAnterior;

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

void initJogo()
{
  drawImage(1, 1, "/labirinto.bin");
  display.drawRect(0, 0, NUM_SPACE, NUM_SPACE, 0x0000);
  display.drawPixel(END.first, END.second, FINALRGB565);
  posicao = START;
  display.drawPixel(posicao.first, posicao.second, CHARCOLOR);
  display.setCursorPos(0, 0);
}

bool canMove(int16_t x, int16_t y)
{
  if (display.getPixel(x, y) == 0x0000)
  {
    return false;
  }
  return true;
}

void moveChar()
{
  display.drawPixel(posicaoAnterior.first, posicaoAnterior.second, 0xFFFF);
  display.drawPixel(posicao.first, posicao.second, CHARCOLOR);
  display.setCursorPos(posicao.first - 5, posicao.second - 5);
}

void layerWin()
{
  display.fillScreen(0X00);
  drawImage(0, 0, "/trofeu.bin");
}

void andar()
{
  pair<int, int> xy = client->getXY();
  int x = xy.first;
  int y = xy.second;
  posicaoAnterior = posicao;
  posicao = {posicao.first + x, posicao.second + y};
  if (canMove(posicao.first, posicao.second))
  {
    if (posicaoAnterior != posicao)
    {
      moveChar();
    }
    if (posicao == END)
    {
      jogoTerminado = true;
      layerWin();
    }
    if (client->bPressed())
    {
      jogoIniciado = false;
    }
  }
  else
  {
    posicao = posicaoAnterior;
  }
}

unsigned long tempo = 0;

void loop()
{
  if (!client->isConnected())
  {
    client->connectToServer();
  }
  else
  {
    if (!jogoIniciado)
    {
      Serial.println("Jogo Iniciado!");
      jogoIniciado = true;
      initJogo();
    }
    else
    {
      client->update();
      if (!jogoTerminado)
      {
        andar();
      }
      else
      {
        if (client->bPressed() || client->aPressed())
        {
          jogoTerminado = false;
          jogoIniciado = false;
        }
      }
    }
#if MESA == 1
    display.show();
#endif
    delay(GAME_VELOCITY);
  }
}
