
#include <string.h>
#include "app.h"

#define BUTTON_COUNT 100
#define ROW_COUNT 10

typedef struct color
{
  u8 r, g, b;
} color;

typedef struct
{
  u8 buttons[BUTTON_COUNT];
  u8 rowHasActiveCount[ROW_COUNT];
} DATA;

DATA data;
u8 activeInRow[ROW_COUNT];

#define COLOR_COUNT 10
color colors[COLOR_COUNT] = {
    { 0,  0,  0 },
    { 63, 0,  0 },
    { 63, 31, 0 },
    { 63, 63, 0 },
    { 0,  63, 0 },
    { 0,  63, 31 },
    { 0,  0,  63 },
    { 31, 0,  63 },
    { 63, 0,  31 },
    { 63, 63, 63 },
};

#define BUTTON(name, code) if (index == code) { name = value ? 1 : 0; }

u8 inSetup = 0;
u8 inColorSelect = 0;

u8 shift = 0;
u8 delete = 0;
u8 device = 0;
u8 sends = 0;
//______________________________________________________________________________

void draw(u8 even)
{
  if (inColorSelect)
  {
    for (int i = 0; i < 10; ++i)
      for (int j = 0; j < 10; ++j)
        hal_plot_led(TYPEPAD, j * 10 + i, 0, 0, 0);

    for (u8 color = 0; color < COLOR_COUNT; ++color)
    {
      hal_plot_led(TYPEPAD, 10 + color, colors[color].r, colors[color].g, colors[color].b);
    }

    return;
  }

  for (int i = 0; i < 10; ++i)
  {
    for (int j = 0; j < 10; ++j)
    {
      u8 index = j * 10 + i;
      color col = colors[data.buttons[index]];
      u8 active = activeInRow[index / 10] == index % 10;
      if (data.rowHasActiveCount[j] && active && even)
        hal_plot_led(TYPEPAD, index, col.r / 4, col.g / 4, col.b / 4);
      else
        hal_plot_led(TYPEPAD, index, col.r, col.g, col.b);
    }
  }

  if (inSetup)
  {
    for (int i = 0; i < ROW_COUNT; ++i) {
      if (data.rowHasActiveCount[i])
        hal_plot_led(TYPEPAD, 10 * i, 63, 63, 63);
    }
  }
}

void app_surface_event(u8 type, u8 index, u8 value)
{
  u8 row = index / 10;
  u8 col = index % 10;

  switch (type)
  {
    case TYPEPAD:
    {
      BUTTON(shift, 80);
      BUTTON(delete, 50);
      BUTTON(device, 97);
      BUTTON(sends, 7);

      u8 pad = col != 0 && row > 0 && row < 9;

      if (inSetup)
      {
        if (delete && device && value)
        {
          memset(&data, 0, sizeof(data));
          break;
        }

        if (sends && value) {
          if (index % 10 == 0) {
            data.rowHasActiveCount[index / 10] = !data.rowHasActiveCount[index / 10];
            if (!data.rowHasActiveCount[index / 10])
              activeInRow[index / 10] = 0;
          }
        }

        if (!inColorSelect && pad && shift && value)
        {
          inColorSelect = index;
          break;
        }

        if (inColorSelect && value)
        {
          if (index >= 10 && index < 10 + COLOR_COUNT)
          {
            data.buttons[inColorSelect] = index - 10;
            inColorSelect = 0;
          }
          if (shift)
            inColorSelect = 0;
          break;
        }

        break;
      }

      // NOT SETUP
      if (!shift && pad)
      {
        if (value)
        {
          if (data.rowHasActiveCount[row])
            activeInRow[row] = col == 9 ? 0 : col;
          hal_send_midi(DINMIDI, NOTEON, index, value);
          hal_send_midi(USBMIDI, NOTEON, index, value);
        }
        else
        {
          hal_send_midi(DINMIDI, NOTEOFF, index, value);
          hal_send_midi(USBMIDI, NOTEOFF, index, value);
        }
      }
    }
      break;

    case TYPESETUP:
    {
      if (value)
      {
        inSetup = !inSetup;
        if (inSetup)
        {
          hal_plot_led(TYPESETUP, 0, 63, 0, 0);
        }
        else
        {
          inColorSelect = 0;
          hal_plot_led(TYPESETUP, 0, 0, 0, 0);
          hal_write_flash(0, &data, sizeof(data));
        }
      }
    }
      break;
  }
}

//______________________________________________________________________________

void app_midi_event(u8 port, u8 status, u8 d1, u8 d2)
{}

//______________________________________________________________________________

void app_sysex_event(u8 port, u8* data, u16 count)
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

void app_init(const u16* adc_raw)
{
  // example - load button states from flash
  hal_read_flash(0, &data, sizeof(data));
}
