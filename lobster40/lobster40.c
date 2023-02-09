#include <string.h>
#include "quantum.h"
#include "qp.h"
#include <hal_pal.h>
#include "noto11.qff.c"
#include "raw_hid.h"

painter_device_t display;
painter_font_handle_t font;


painter_image_handle_t loaded_image;
uint8_t raw_image_buffer[12000];
char track_title[64] = {0};
bool track_set = false;


typedef struct {
    double h;
    double s;
    double v;
} hsv;

enum hid_codes {
    UPDT_LAYER = 0x00, // LAYER UPDATE

};

void keyboard_post_init_kb(void) {
    setPinOutput(GP15); // Backlight Pin
    writePinHigh(GP15);
    font = qp_load_font_mem(font_noto11);
    // Below pins are CS, DC, RST
    display = qp_st7735_make_spi_device(128, 160, GP21, GP13, GP12, 4, 3);
    qp_init(display, QP_ROTATION_0);

    extern const hsv current_layer_colour(void);
    const hsv  lc = current_layer_colour();
    qp_rect(display, 0, 0, 129, 160, lc.h, lc.s, lc.v, true);

    extern const char *current_layer_string(void);
    const char        *layer_name = current_layer_string();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "layer: %s", layer_name);
    qp_drawtext(display, 4, 4, font, buf);
}


// Turn display on/off based on keeb power
void suspend_power_down_user(void) {
    writePinLow(GP15);
}


void suspend_wakeup_init_user(void) {
    writePinHigh(GP15);
}


void reset_display(void) {
    extern const hsv current_layer_colour(void);
    const hsv  lc = current_layer_colour();
    qp_rect(display, 0, 0, 129, 160, lc.h, lc.s, lc.v, true);


    extern const char *current_layer_string(void);
    const char        *layer_name = current_layer_string();
    char buf[64] = {0};
    snprintf(buf, sizeof(buf), "layer: %s", layer_name);

    qp_drawtext(display, 4, 4, font, buf);
}


static uint32_t last_layer_state   = 0;
uint8_t raw_hid_send_buffer[32] = {0};

void update_ui(void) {
    if (last_layer_state != layer_state) {
        last_layer_state   = layer_state;

        extern const hsv current_layer_colour(void);
        const hsv  lc = current_layer_colour();
        qp_rect(display, 0, 0, 129, 161, lc.h, lc.s, lc.v, true);

        extern const char *current_layer_string(void);
        const char        *layer_name = current_layer_string();
        char buf[64] = {0};
        snprintf(buf, sizeof(buf), "layer: %s", layer_name);

         if (track_set == true) {
            qp_drawtext(display, 4, (14 + font->line_height), font, track_title);
            loaded_image = qp_load_image_mem(raw_image_buffer);
            qp_drawimage(display, 14, 55, loaded_image);
            qp_close_image(loaded_image);
        }

        qp_drawtext(display, 4, 4, font, buf);
        raw_hid_send_buffer[0] = UPDT_LAYER;
        raw_hid_send_buffer[1] = layer_state; // Send new layer_state
        raw_hid_send_buffer[2] = lc.h;  // Send layer colour
        raw_hid_send_buffer[3] = lc.s;
        raw_hid_send_buffer[4] = lc.v;
        raw_hid_send(raw_hid_send_buffer, RAW_EPSIZE);
    }
}


void housekeeping_task_kb(void) {
    update_ui();
}


int point = 0;
int track_point = 0;

void display_image(void) {
    loaded_image = qp_load_image_mem(raw_image_buffer);
    qp_drawimage(display, 14, 55, loaded_image);
    qp_flush(display);
    qp_close_image(loaded_image);
    println("Displayed image?");
    qp_close_image(loaded_image);
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    // TODO: Figure out a better way of doing this :( it is bad rn
    if (data[0] == 23 && data[1] == 23 && data[2] == 24 && data[3] == 0 && data[4] == 0) {
        println("End of sequence");
        display_image();
        point = 0;
        return;
    }

    if (data[0] == 'R' && data[1] == 'T' && data[2] == 24) {
        println("Resetting...");
        reset_display();
        qp_flush(display);
        memset(raw_image_buffer, 0, 12000);
        point = 0;
        memset(track_title, 0, 64);
        return;
    }

    if (data[0] == 31 && data[1] == 20) { // title transmission
        println("Adding to title");
        for (int i = 2; i < length; i++) {
            track_title[track_point] = data[i];
            track_point++;
        }
        return;
    }

    if (data[0] == 24 && data[1] == 24 && data[2] == 24 && data[3] == 20) {
        println("Displaying title");
        qp_drawtext(display, 4, (14 + font->line_height), font, track_title);
        track_point = 0;
        track_set = true;
        return;
    }


    // Image loading from a stream of transmissions
    int n = sizeof raw_image_buffer / sizeof *raw_image_buffer;
    printf("Buffer size: %d Point: %d\n", n, point);
    for (int i = 0; i < length; i++) {
        raw_image_buffer[point] = data[i];
        point++;
    }
}
