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

const int esp8266_n_diagram_lines = 17; // Load Wi-Fi library
#include <ESP8266WiFi.h>

#define LED_PIN_FREQ 16
#define LED_PIN_WIFI 2
#define HTTP_PORT 80
#define TIME_BTWN_FREQ_LED_MS 15000
#define NUM_PRINT_DELAY 150
unsigned long int time_led_freq = 0;

const char* wifi_ssid = "<your_ssid>";
const char* wifi_pass = "<your_password>";

#define WIFI_CONNECT_BLINKS 5
#define CLIENT_CONNECT_BLINKS 3
#define CLIENT_TIMEOUT_MS 2000
WiFiServer server(HTTP_PORT);
unsigned long int time_now = millis();
unsigned long int time_prev = 0;
String request;

const int allowed_output[] = {5, 4, 0, 14, 12, 13, 15};
const int allowed_output_n = 7;
const float allowed_freq_min = 100.0;
const float allowed_freq_max = 40000.0;
int old_pins[] = {0, 0, 0, 0, 0, 0, 0};
int output_pins[] = {32, 64, 0, 0, 0, 0, 0};
float old_freq = 0.0;
float output_freq = 1024.0;

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
    Serial.printf("Connecting to %s\r\n", wifi_ssid);
    WiFi.begin(wifi_ssid, wifi_pass);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\nWifi Connected\r\n");
    for (j=0; j<WIFI_CONNECT_BLINKS; ++j) {
        digitalWrite(LED_PIN_WIFI, LOW);
        delay(100);
        digitalWrite(LED_PIN_WIFI, HIGH);
        delay(300);
    }

    Serial.printf("IP address: ");
    Serial.println(WiFi.localIP());
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
    Serial.printf("HTTP Server Started\r\n");
}

void send_http_response(WiFiClient* client) {
    int j, k;
    int mapped_pin;
    const char* left_line;
    const char* right_line;
    char color_str[32];
    char input_str[128];
    char button_str[64];
    char form_start[64];
    char form_end[32];
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
    client->printf("  display: inline;\n");
    client->printf("}\n");
    client->printf("</style>\n");
    client->printf("<h1>ESP8266 PWM Settings</h1>\n");
    client->printf("<p>output_pin = [  ");
    for (j=0; j<allowed_output_n; ++j) {
        client->printf("%d:%d  ", allowed_output[j], output_pins[j]);
    }
    client->printf("]</p>\n");
    client->printf("<p>output_freq = %.2f Hz</p>\n", output_freq);
    client->printf("<br/>\n");
    client->printf("<h2>PWM Output Pin</h2>\n");
    for (j=0; j<esp8266_n_diagram_lines; ++j) {
        left_line = esp8266_pin_diagram[2*j];
        right_line = esp8266_pin_diagram[2*j+1];
        mapped_pin = esp8266_pin_map[2*j+1];
        snprintf(color_str, 32, "");
        snprintf(input_str, 128, "");
        snprintf(button_str, 64, "");
        snprintf(form_start, 64, "");
        snprintf(form_end, 32, "");
        for (k=0; k<allowed_output_n; ++k) {
            if (allowed_output[k] == mapped_pin) {
                break;
            }
        }
        if (k < allowed_output_n) {
            if (0 == output_pins[k]) {
                snprintf(input_str, 128, "<input name='duty' style='width: 4em' type='number' inputmode='numeric' step='1' min='0' max='255' value='%d'></input>\n", (256/(allowed_output_n+1))*(k+1));
                snprintf(button_str, 64, "<button type='submit'>enable pin</button>");
                snprintf(form_start, 64, "<form style='display:inline' method='get' action='/pin/%d'>", mapped_pin);
                snprintf(form_end, 32, "</form>");
            } else {
                snprintf(color_str, 32, " style='color: green;'");
                snprintf(input_str, 128, "<input name='duty' style='width: 4em' type='number' inputmode='numeric' step='1' min='0' max='255' value='%d'></input>\n", output_pins[k]);
                snprintf(button_str, 64, "<a href='/pin/%d?duty=0'><button>disable pin</button></a>", mapped_pin);
            }
        }
        client->printf("<pre><code>%s</code><code%s>", left_line, color_str);
        client->printf("%s</code></pre>%s", right_line, form_start);
        client->printf("%s%s%s<br/>\n", input_str, button_str, form_end);
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
    int new_pin, new_duty;
    float new_freq;
    j = request.indexOf("GET /pin");
    if (j >= 0) {
        sscanf(request.c_str()+j, "GET /pin/%d?duty=%d", &new_pin, &new_duty);
        Serial.printf("\r\nchanging pwm output pin %d (duty %d)...", new_pin, new_duty);
        for (j=0; j<allowed_output_n; ++j) {
            if (allowed_output[j] == new_pin) {
                output_pins[j] = new_duty;
                Serial.printf("done");
                break;
            }
        }
        Serial.println();
    }

    j = request.indexOf("GET /freq");
    if (j >= 0) {
        sscanf(request.c_str()+j, "GET /freq/?freq=%f", &new_freq);
        Serial.printf("\r\nchanging pwm freq to %f...", new_freq);
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
    int j, k;
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
            delay(NUM_PRINT_DELAY);
            digitalWrite(pin, HIGH);
            delay(NUM_PRINT_DELAY);
        }
        delay(4 * NUM_PRINT_DELAY);
    }
}

void show_led_freq() {
    int j;
    Serial.printf("\r\noutput pins [  ");
    for (j=0; j<allowed_output_n; ++j) {
        Serial.printf("%d:%d  ", allowed_output[j], output_pins[j]);
    }
    Serial.printf("] freq: %0.2f Hz\r\n", output_freq);
    led_number_print(LED_PIN_FREQ, (int) output_freq, 5);
}

void set_pwms() {
    int j, pin;
    bool changed = false;
    if (old_freq == output_freq) {
        for (j=0; j<allowed_output_n; ++j) {
            if (old_pins[j] != output_pins[j]) {
                changed = true;
                break;
            }
        }
    } else {
        changed = true;
    }
    if (changed) {
        Serial.printf("\r\n change detected, resetting pwm...");
        for (j=0; j<allowed_output_n; ++j) {
            pin = allowed_output[j];
            if (0 == output_pins[j]) {
                analogWrite(pin, 0);
            }
        }
        old_freq = output_freq;
        for (j=0; j<allowed_output_n; ++j) {
            old_pins[j] = output_pins[j];
        }

        for (j=0; j<allowed_output_n; ++j) {
            if (0 != output_pins[j]) {
                analogWrite(allowed_output[j], output_pins[j]);
            }
        }
        analogWriteRange(255);
        analogWriteFreq(output_freq);
        Serial.printf("done\r\n");
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
        Serial.printf("\r\n========\r\nclient connecting\r\n");
        for (j=0; j<CLIENT_CONNECT_BLINKS; ++j) {
            digitalWrite(LED_PIN_WIFI, LOW);
            delay(50);
            digitalWrite(LED_PIN_WIFI, HIGH);
            delay(50);
        }
        client_handle(&client);
        Serial.printf("\r\nclient disconnected\r\n");
    }

    set_pwms();
}
