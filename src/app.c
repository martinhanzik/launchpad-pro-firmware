
#include "app.h"

#define BUTTON_COUNT 100

typedef struct color {
    u8 r, g, b;
} color;

typedef struct {
    u8 buttons[BUTTON_COUNT];
    u8 activeInRow[4];
} DATA;

DATA data;

#define COLOR_COUNT 10
color colors[COLOR_COUNT] = {
    {0, 0, 0},
    {63, 0, 0},
    {63, 31, 0},
    {63, 63, 0},
    {0, 63, 0},
    {0, 63, 31},
    {0, 0, 63},
    {31, 0, 63},
    {63, 0, 31},
    {63,63,63},
};

#define BUTTON(name, code) if (index == code) { name = value ? 1 : 0; }

u8 shift = 0;
u8 delete = 0;
u8 device = 0;
//______________________________________________________________________________

void draw(u8 even) {
    for (int i=0; i < 10; ++i)
    {
        for (int j=0; j < 10; ++j)
        {
            u8 index = j * 10 + i;
            color col = colors[data.buttons[index]];
            u8 active = data.activeInRow[8 - index / 10] == index % 10;
            if (active && even)
                hal_plot_led(TYPEPAD, index, col.r / 4, col.g / 4, col.b / 4);
            else
                hal_plot_led(TYPEPAD, index, col.r, col.g, col.b);
        }
    }

}

void app_surface_event(u8 type, u8 index, u8 value)
{
    switch (type)
    {
        case  TYPEPAD:
        {
            BUTTON(shift, 80);
            BUTTON(delete, 50);
            BUTTON(device, 97);

            u8 pad = (index % 10) != 0 && index > 10 && index < 90;

            if (shift && delete && device) {
                memset(&data, 0, sizeof(data));
                break;
            }

            // toggle it and store it off, so we can save to flash if we want to
            if (pad && shift && value)
            {
                data.buttons[index] = (data.buttons[index] + 1) % COLOR_COUNT;
            }

            if (!shift && pad && value)
            {
                data.activeInRow[8 - index / 10] = index % 10;
                hal_send_midi(DINMIDI, NOTEON, index, value);
                hal_send_midi(DINMIDI, NOTEOFF, index, value);
                hal_send_midi(USBMIDI, NOTEON, index, value);
                hal_send_midi(USBMIDI, NOTEOFF, index, value);
            }  
        }
        break;
            
        case TYPESETUP:
        {
            if (value)
            {
                hal_write_flash(0, &data, sizeof(data));
            }
        }
        break;
    }
}

//______________________________________________________________________________

void app_midi_event(u8 port, u8 status, u8 d1, u8 d2)
{}

//______________________________________________________________________________

void app_sysex_event(u8 port, u8 * data, u16 count)
{}

//______________________________________________________________________________

void app_aftertouch_event(u8 index, u8 value)
{}

//______________________________________________________________________________

void app_cable_event(u8 type, u8 value)
{
    // example - light the Setup LED to indicate cable connections
    if (type == MIDI_IN_CABLE)
    {
        hal_plot_led(TYPESETUP, 0, 0, value, 0); // green
    }
    else if (type == MIDI_OUT_CABLE)
    {
        hal_plot_led(TYPESETUP, 0, value, 0, 0); // red
    }
}

//______________________________________________________________________________

void app_timer_event()
{
#define DRAW_MS 240
    
    static u8 drawMs = DRAW_MS;
    static u8 drawEven = 1;
    
    if (++drawMs >= DRAW_MS)
    {
        drawMs = 0;
        drawEven = !drawEven;
        draw(drawEven);
    }
}

//______________________________________________________________________________

void app_init(const u16 *adc_raw)
{
    // example - load button states from flash
    hal_read_flash(0, &data, sizeof(data));
    memset(data.activeInRow, 0, sizeof(data.activeInRow));
    for (int i = 8; i > 6; i--) {
        for (int j = 1; j < 10; ++j) {
            data.buttons[i * 10 + j] = j;
        }
    }
}
