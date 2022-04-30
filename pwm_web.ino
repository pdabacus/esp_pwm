const char* esp8266_pin_diagram[] = {
    "              ______________________","_________          ",
    "    --        | a0       _   ____   ","    d0  |   --     ",
    "    --        | rsv     | | |       ","    d1  |   gpio5  ",
    "    --        | rsv     | |_| [=]   ","    d2  |   gpio4  ",
    "    --        | sd3                 ","    d3  |   gpio0  ",
    "    --        | sd2     [-------]   ","    d4  |   --     ",
    "    --        | sd1     [-------]   ","    3v3 |   3.3v   ",
    "    --        | cmd     [-------]   ","    gnd |   ground ",
    "    --        | sd0     [-------]   ","    d5  |   gpio14 ",
    "    --        | clk     [-------]   ","    d6  |   gpio12 ",
    "    ground    | gnd                 ","    d7  |   gpio13 ",
    "    3.3v      | 3v3      |=|        ","    d8  |   gpio15 ",
    "    --        | en                  ","    rx  |   --     ",
    "    --        | rst      _______    ","    tx  |   --     ",
    "    ground    | gnd      |     |    ","    gnd |   ground ",
    "    volt in   | vin      | usb |    ","    3v3 |   3.3v   ",
    "              ______________________","_________          "
};

const int esp8266_pin_map[] = {
    -1, -1,
    -1, -1,
    -1, 5,
    -1, 4,
    -1, 0,
    -1, -1,
    -1, -1,
    -1, -1,
    -1, 14,
    -1, 12,
    -1, 13,
    -1, 15,
    -1, -1,
    -1, -1,
    -1, -1,
    -1, -1,
    -1, -1
};

const int esp8266_n_diagram_lines = 17;

// Load Wi-Fi library
#include <ESP8266WiFi.h>

#define LED_PIN_FREQ 16
#define LED_PIN_WIFI 2
#define HTTP_PORT 80
#define TIME_BTWN_FREQ_LED_MS 15000
unsigned long int time_led_freq = 0;

const char* wifi_ssid = "<YOUR_SSID>";
const char* wifi_pass = "<YOUR_WIFI_PASSWORD>";

#define WIFI_CONNECT_BLINKS 5
#define CLIENT_CONNECT_BLINKS 3
#define CLIENT_TIMEOUT_MS 2000
WiFiServer server(HTTP_PORT);
unsigned long int time_now = millis();
unsigned long int time_prev = 0;
String request;

const int allowed_output[] = {0, 4, 5, 12, 13, 14, 15};
const int allowed_output_n = 7;
const float allowed_freq_min = 100.0;
const float allowed_freq_max = 40000.0;
int old_pin = 0;
int output_pin = 5;
float old_freq = 0.0;
float output_freq = 123.0;

void led_number_print(int pin, int num, int num_digits);

void setup() {
    Serial.begin(9600);

    // set all possible pwm output pins to on
    pinMode(LED_PIN_FREQ, OUTPUT);
    pinMode(LED_PIN_WIFI, OUTPUT);
    int j;
    for (j=0; j<allowed_output_n; ++j) {
        pinMode(allowed_output[j], OUTPUT);
    }

    set_pwms();
    old_freq = 0.0;

    // connect to wifi
    Serial.printf("Connecting to %s\n", wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWifi Connected\n");
    Serial.printf("IP address: ");
    Serial.println(WiFi.localIP());
    for (j=0; j<WIFI_CONNECT_BLINKS; ++j) {
        digitalWrite(LED_PIN_WIFI, LOW);
        delay(100);
        digitalWrite(LED_PIN_WIFI, HIGH);
        delay(100);
    }

    uint32 ip = WiFi.localIP();
    led_number_print(LED_PIN_WIFI, (0xff & ip), 3);
    delay(500);
    led_number_print(LED_PIN_WIFI, (0xff & ip >> 8), 3);
    delay(500);
    led_number_print(LED_PIN_WIFI, (0xff & ip >> 16), 3);
    delay(500);
    led_number_print(LED_PIN_WIFI, (0xff & ip >> 24), 3);
    delay(500);

    // start http server
    server.begin();
    Serial.printf("HTTP Server Started\n");
}

void send_http_response(WiFiClient* client) {
    int j, k;
    int mapped_pin;
    const char* left_line;
    const char* right_line;
    char color_str[32];
    char button_str[64];
    client->printf("HTTP/1.1 200 OK\n");
    client->printf("Content-type: text/html\n");
    client->printf("Connection: close\n");
    client->printf("\n");
    client->printf("<!doctype html>\n");
    client->printf("<html>\n");
    client->printf("<head>\n");
    client->printf("</head>\n");
    client->printf("<body>\n");
    client->printf("<style>\n");
    client->printf("pre {\n");
    client->printf("  margin: 0;\n");
    client->printf("}\n");
    client->printf("</style>\n");
    client->printf("<h1>ESP8266 PWM Settings</h1>\n");
    client->printf("<p>output_pin = %d</p>\n", output_pin);
    client->printf("<p>output_freq = %.2f Hz</p>\n", output_freq);
    client->printf("<br/>\n");
    client->printf("<h2>PWM Output Pin</h2>\n");
    for (j=0; j<esp8266_n_diagram_lines; ++j) {
        left_line = esp8266_pin_diagram[2*j];
        right_line = esp8266_pin_diagram[2*j+1];
        mapped_pin = esp8266_pin_map[2*j+1];
        snprintf(color_str, 32, "");
        snprintf(button_str, 64, "");
        for (k=0; k<allowed_output_n; ++k) {
            if (allowed_output[k] == mapped_pin) {
                snprintf(button_str, 64, "<a href='/pin/%d'><button>set pin</button></a>", mapped_pin);
                break;
            }
        }
        if (esp8266_pin_map[2*j+1] == output_pin) {
            snprintf(button_str, 64, "");
            snprintf(color_str, 32, " style='color: green;'");
        }
        client->printf("<pre><code>%s</code><code%s>%s</code>%s</pre>\n", left_line, color_str, right_line, button_str);
    }
    
    client->printf("<h2>PWM Output Frequency</h2>\n");
    client->printf("<form method='get' action='/freq/'>\n");
    client->printf("<input name='freq' type='number' inputmode='numeric' ");
    client->printf("step='0.1' min='%0.1f' max='%0.1f' value='%0.1f'></input>\n", allowed_freq_min, allowed_freq_max, output_freq);
    client->printf("<button type='submit'>set frequency</button>\n");
    client->printf("</form>\n");
    client->printf("</body>\n");
    client->printf("</html>\n");
    client->printf("\n");
}

void parse_setting_changes() {
    int j;
    int new_pin;
    float new_freq;
    j = request.indexOf("GET /pin");
    if (j >= 0) {
        sscanf(request.c_str()+j, "GET /pin/%d", &new_pin);
        Serial.printf("changing pwm output pin to %d...", new_pin);
        for (j=0; j<allowed_output_n; ++j) {
            if (allowed_output[j] == new_pin) {
                output_pin = new_pin;
                Serial.printf("done");
                break;
            }
        }
        Serial.println();
    }

    j = request.indexOf("GET /freq");
    if (j >= 0) {
        sscanf(request.c_str()+j, "GET /freq/?freq=%f", &new_freq);
        Serial.printf("changing pwm freq to %f...", new_freq);
        if (allowed_freq_min <= new_freq && new_freq <= allowed_freq_max) {
            output_freq = new_freq;
            Serial.printf("done");
        }
        Serial.println();
    }
}

void client_handle(WiFiClient* client) {
    String request_current_line = "";
    char request_current_char;
    time_now = millis();
    time_prev = time_now;
    while (client->connected() && time_now-time_prev < CLIENT_TIMEOUT_MS) {
        time_now = millis();
        if (client->available()) {
            request_current_char = client->read();
            Serial.write(request_current_char);
            request += request_current_char;
            if (request_current_char == '\n') {
                if (request_current_line.length() == 0) {
                    parse_setting_changes();
                    send_http_response(client);
                    break;
                } else {
                    request_current_line = "";
                }
            } else if (request_current_char != '\r') {
                request_current_line += request_current_char;
            }
        }
    }

    request = "";
    client->stop();
}

void led_number_print(int pin, int num, int num_digits) {
    int j, k, n;
    int digits[10];
    if (num_digits > 8) {
        num_digits = 8;
    }
    
    j = 10;
    while (num > 0 && j > 0) {
        digits[--j] = num % 10;
        num /= 10;
    }
    
    for (; num_digits && j<10; ++j, --num_digits) {
        for (k=0; k<digits[j]; ++k) {
            digitalWrite(pin, LOW);
            delay(100);
            digitalWrite(pin, HIGH);
            delay(100);
        }
        delay(400);
    }
}

void show_led_freq() {
    Serial.printf("\noutput pin %d freq: %0.2f Hz\n", output_pin, output_freq);
    led_number_print(LED_PIN_FREQ, (int) output_freq, 5);
}

void set_pwms() {
    int j, pin;
    if (old_freq != output_freq || old_pin != output_pin) {
        for (j=0; j<allowed_output_n; ++j) {
            pin = allowed_output[j];
            if (pin != output_pin) {
                analogWrite(pin, 0);
            }
        }
        old_freq = output_freq;
        old_pin = output_pin;
        
        analogWrite(output_pin, 127);
        analogWriteRange(255);
        analogWriteFreq(output_freq);
    }
}

void loop() {
    int j;
    digitalWrite(LED_PIN_FREQ, HIGH);
    digitalWrite(LED_PIN_WIFI, HIGH);

    time_now = millis();
    if (time_now - time_led_freq > TIME_BTWN_FREQ_LED_MS) {
        show_led_freq();
        time_led_freq = millis();
    }
    
    WiFiClient client = server.available();
    if (client) {
        Serial.printf("\n========\nclient connecting\n");
        for (j=0; j<CLIENT_CONNECT_BLINKS; ++j) {
            digitalWrite(LED_PIN_WIFI, LOW);
            delay(100);
            digitalWrite(LED_PIN_WIFI, HIGH);
            delay(100);
        }
        client_handle(&client);
        Serial.printf("\nclient disconnected\n");
    }

    set_pwms();

}
