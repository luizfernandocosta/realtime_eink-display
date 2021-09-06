//Including drivers
#include <Arduino.h>
#include <esp_task_wdt.h>
#include "epd_driver.h"
#include <Wire.h>
#include <SPI.h>

//Including Roboto fonts
#include "robotothin64.h"
#include "robotothin48.h"
#include "robotolight32.h"
#include "robotoregular16.h"
#include "robotoregular24.h"

//Including Wi-Fi client and NTP client
#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

WiFiUDP ntpUDP;

//Initializing NTPClient, using Brazil server with -3 offset
NTPClient timeClient(ntpUDP, "a.st1.ntp.br", -3 * 3600, 60000);

//Const for Wi-Fi (SSID and password)
const char *ssid = "pitsmoker";
const char *password = "pitsmoker";

//Framebuffer for e-ink display
uint8_t *framebuffer;

void setup()
{
    Serial.begin(115200);
    epd_init();
    char buf[128];

    framebuffer = (uint8_t *)ps_calloc(sizeof(uint8_t), EPD_WIDTH * EPD_HEIGHT / 2);
    if (!framebuffer)
    {
        Serial.println("alloc memory failed !!!");
        while (1)
            ;
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    // Connect to Wi-Fi
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");

    int cursor_x = 83;
    int cursor_y = 300;

    epd_poweron();
    epd_clear();
    epd_poweroff();

    epd_poweron();

    drawBox();
    drawTemperature(58);
    drawHumidity(88);
}

void loop()
{
    updateTime();
}

/* 
    Function to write all the figures that the display needs, since the figures doesn't count as
    "writing", I need to call a function and draw on the whole display.
*/
void drawBox()
{
    //Coordinates to draw the circle of temperature
    epd_draw_circle(155, 260, 120, 0x0, framebuffer);

    //Coordinates to draw the circle of humidity
    epd_draw_circle(465, 260, 120, 0x0, framebuffer);

    //Coordinates to draw the circle of air pressura
    epd_draw_circle(775, 260, 120, 0x0, framebuffer);

    //Drawing a rectangle on the edges of the display
    epd_draw_rect(10, 20, EPD_WIDTH - 20, EPD_HEIGHT - 40, 0, framebuffer);

    //Drawing the figures
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
}

/*
    Function that receives a int parameter and write it on the display, in this case it'll
    write temperature
*/
void drawTemperature(int temperatura)
{
    int cursor_x = 0;
    int cursor_y = 0;


/*
    Creating an area to refresh the data later, since I won't be redrawing the whole frame all the time, I
    need to declare an area so I can refresh only that part.

    It'll only refresh the measure area, leaving the rest intact.
*/
    Rect_t temperatureMeasureArea = {
        .x = 80,
        .y = 180,
        .width = 150,
        .height = 130};

    //Here I call a function to refresh that area, it'll refresh 4 cycles on 100 us.
    epd_clear_area_cycles(temperatureMeasureArea, 4, 100);

    //Writing the title "Temperature"
    cursor_x = 60;
    cursor_y = 130;
    writeln((GFXfont *)&RobotoRegular16, "Temperatura", &cursor_x, &cursor_y, NULL);


    cursor_y = 300;

    /*
        Here I'm converting the temperature received as int.
        The temperature futurely will come from a time series database (InfluxDB).

        This conversion is also valid for humidity and air pressure.

        I have to do this way because the writeln function on this display only accepts char *variable
        It's a little bit clunky, bit it works. 😁
    */
    char converteTemperatura[3];
    sprintf(converteTemperatura, "%d", temperatura);
    char *temperaturaConvertida = converteTemperatura;

    //atoi converte de char pointer para int

/*
    Condition to verify the number temperature, if it is less than of equal 9, it'll change the cursor
    Otherwise, it'll change to anoter location
*/
    if (temperatura <= 9)
        cursor_x = 123;
    if (temperatura >= 10)
        cursor_x = 83;

    writeln((GFXfont *)&RobotoThin64, temperaturaConvertida, &cursor_x, &cursor_y, NULL);

    //Writing ºC
    cursor_x = 140;
    cursor_y = 350;
    writeln((GFXfont *)&RobotoRegular24, "C", &cursor_x, &cursor_y, NULL);
}

/*
    Another function that receives an int parameter (humidity), and draw it.
*/
void drawHumidity(int umidade)
{
    int cursor_x = 0;
    int cursor_y = 0;

    //Same as the area created on temperature.
    Rect_t areaMedidaUmidade = {
        .x = 390,
        .y = 180,
        .width = 150,
        .height = 130
    };

    epd_clear_area_cycles(areaMedidaUmidade, 4, 100);

    //Writing the title "humidity"
    cursor_x = 400;
    cursor_y = 130;
    writeln((GFXfont *)&RobotoRegular16, "Umidade", &cursor_x, &cursor_y, NULL);

    //Writing humidity measure
    cursor_y = 300;

    //Same conversion of temperature
    char converteUmidade[3];
    sprintf(converteUmidade, "%d", umidade);
    char *umidadeConvertida = converteUmidade;

    //Same condition of temperature
    if (umidade <= 9)
        cursor_x = 430;
    if (umidade >= 10)
        cursor_x = 390;

    writeln((GFXfont *)&RobotoThin64, umidadeConvertida, &cursor_x, &cursor_y, NULL);

    //Writing humidity symbol
    cursor_x = 450;
    cursor_y = 350;
    writeln((GFXfont *)&RobotoRegular24, "%", &cursor_x, &cursor_y, NULL);
}

void drawAirPressure(int pressao)
{
    int cursor_x = 0;
    int cursor_y = 0;

    //Escrevendo pressão
    cursor_x = 710;
    cursor_y = 130;
    writeln((GFXfont *)&RobotoRegular16, "Pressao", &cursor_x, &cursor_y, NULL);

    //Escrevendo medida da pressão
    cursor_x = 660;
    cursor_y = 290;
    writeln((GFXfont *)&RobotoThin48, "1013", &cursor_x, &cursor_y, NULL);
}

void updateTime()
{
    timeClient.update();

    struct tm *ptm = gmtime((time_t *)&epochTime);

    unsigned long epochTime = timeClient.getEpochTime();

    int currentMinute = timeClient.getMinutes();
    int currentHour = timeClient.getHours();
    int monthDay = ptm->tm_mday;
    int currentMonth = ptm->tm_mon + 1;
    int currentYear = ptm->tm_year + 1900;

    Serial.println(monthDay);
    Serial.println(currentMonth);
    Serial.println(currentYear);

    drawTime(currentMinute, currentHour, monthDay, currentMonth, currentYear);

    delay(30000);
}

void drawTime(int currentMinute, int currentHour, int monthDay, int currentMonth, int currentYear)
{

    Rect_t minuteArea = {
        .x = 10,
        .y = 430,
        .width = 70,
        .height = 70
    };

    Rect_t hourArea = {
        .x = 390,
        .y = 180,
        .width = 150,
        .height = 130
    };

    epd_clear_area_cycles(minuteArea, 4, 100);

    char converteHoras[4];
    sprintf(converteHoras, "%d", currentHour);
    char *horaConvertida = converteHoras;

    char converteMinutos[3];
    if (currentMinute < 10)
        sprintf(converteMinutos, "0%d", currentMinute);
    if (currentMinute >= 10)
        sprintf(converteMinutos, "%d", currentMinute);
    char *minutoConvertido = converteMinutos;

    char converteDia[3];
    sprintf(converteDia, "%d", monthDay);
    char *diaConvertido = converteDia;

    char converteMes[3];
    if (currentMonth < 10)
        sprintf(converteMes, "0%d", currentMonth);
    if (currentMonth >= 10)
        sprintf(converteMes, "%d", currentMonth);
    char *mesConvertido = converteMes;

    char converteAno[3];
    sprintf(converteAno, "%d", currentYear);
    char *anoConvertido = converteAno;

    int cursor_x = 0;
    int cursor_y = 0;

    cursor_x = 10;
    cursor_y = 500;
    writeln((GFXfont *)&RobotoLight32, horaConvertida, &cursor_x, &cursor_y, NULL);

    cursor_x = 85;
    cursor_y = 495;
    writeln((GFXfont *)&RobotoLight32, ":", &cursor_x, &cursor_y, NULL);

    cursor_x = 100;
    cursor_y = 500;
    writeln((GFXfont *)&RobotoLight32, minutoConvertido, &cursor_x, &cursor_y, NULL);

    cursor_x = 630;
    cursor_y = 500;
    writeln((GFXfont *)&RobotoLight32, diaConvertido, &cursor_x, &cursor_y, NULL);

    cursor_x = 700;
    cursor_y = 500;
    writeln((GFXfont *)&RobotoLight32, "/", &cursor_x, &cursor_y, NULL);

    cursor_x = 720;
    cursor_y = 500;

    writeln((GFXfont *)&RobotoLight32, mesConvertido, &cursor_x, &cursor_y, NULL);

    cursor_x = 780;
    cursor_y = 500;

    writeln((GFXfont *)&RobotoLight32, "/", &cursor_x, &cursor_y, NULL);

    cursor_x = 800;
    cursor_y = 500;

    writeln((GFXfont *)&RobotoLight32, anoConvertido, &cursor_x, &cursor_y, NULL);
}