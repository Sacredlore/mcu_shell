#include "mcu_shell.h"
#include "ili9341.h"

/* array of initialization, bytes sent to ILI9341 */
const uint8_t glcd_sequence[] = {
    ILI9341_CMD_SOFTWARE_RESET, 0x80, // 0x01, __delay_ms(200);
    ILI9341_CMD_POWER_CONTROL_B, 3, 0x00, 0x8B, 0x30, // 0xCF
    ILI9341_CMD_POWER_ON_SEQUENCE_CONTROL, 4, 0x67, 0x03, 0x12, 0x81, // 0xED
    ILI9341_CMD_DRIVER_TIMING_CONTROL_A_E8, 3, 0x85, 0x10, 0x7A, // 0xE8
    ILI9341_CMD_POWER_CONTROL_A, 5, 0x39, 0x2C, 0x00, 0x34, 0x02, // 0xCB
    ILI9341_CMD_PUMP_RATIO_CONTROL, 1, 0x20, // 0xF7
    ILI9341_CMD_DRIVER_TIMING_CONTROL_B, 2, 0x00, 0x00, // 0xEA
    ILI9341_CMD_POWER_CONTROL_1, 1, 0x23, // 0xC0, power control VRH[5:0], ?, 4.60V -> 0x23 to 3.0V -> 0x03
    ILI9341_CMD_POWER_CONTROL_2, 1, 0x10, // 0xC1, power control SAP[2:0];BT[3:0], ?, 0x10 -> to 0x00
    ILI9341_CMD_VCOM_CONTROL_1, 2, 0x3F, 0x3C, // 0xC5, ?, 0x3F - 5.875V, 0x3C - 4,275V, reserved?
    ILI9341_CMD_VCOM_CONTROL_2, 1, 0xB7, // 0xC7 // ?
    ILI9341_CMD_MEMORY_ACCESS_CONTROL, 1, ILI9341_MV | ILI9341_BGR,
    ILI9341_CMD_COLMOD_PIXEL_FORMAT_SET, 1, 0x55, // 0x3A, 16-bits pixel
    ILI9341_CMD_FRAME_RATE_CONTROL_NORMAL, 2, 0x00, 0x1B, // 0xB1, fosc, 27 clocks
    ILI9341_CMD_VERT_SCROLL_START_ADDRESS, 1, 0x00, // 0x37, vertical scroll
    ILI9341_CMD_DISPLAY_FUNCTION_CONTROL, 2, 0x0A, 0xA2, // 0xB6
    ILI9341_CMD_ENABLE_3_GAMMA_CONTROL, 1, 0x00, // 0xF2, 3Gamma function disable
    ILI9341_CMD_GAMMA_SET, 1, 0x01, // 0x26, gamma curve
    ILI9341_CMD_POSITIVE_GAMMA_CORRECTION, 15, // 0xE0, set gamma
    0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
    ILI9341_CMD_NEGATIVE_GAMMA_CORRECTION, 15, // 0xE1, set gamma
    0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
    ILI9341_CMD_SLEEP_OUT, 0x81, // __delay_ms(120);
    ILI9341_CMD_DISPLAY_ON, 0x81, // __delay_ms(120);
    0x00 // end of list
};

struct {
	glcd_titlebar titlebar;
	glcd_label labels[5];
	glcd_readings readings[5];
	glcd_pager pager;
} glcd_gui =
{
	{ { 0, 0, 0, 0 }, GLCD_FONT_8X8, "TITLEBAR" },
	{
		{ { 0, 0, 0, 0 }, ALIGN_MID_LEFT, GLCD_FONT_8X8, "LABEL1"},
		{ { 0, 0, 0, 0 }, ALIGN_MID_LEFT, GLCD_FONT_8X8, "LABEL2"},
		{ { 0, 0, 0, 0 }, ALIGN_MID_LEFT, GLCD_FONT_8X8, "LABEL3"},
		{ { 0, 0, 0, 0 }, ALIGN_MID_LEFT, GLCD_FONT_8X8, "LABEL4"},
		{ { 0, 0, 0, 0 }, ALIGN_MID_LEFT, GLCD_FONT_8X8, "LABEL5"}
	},
	{
		{ { 0, 0, 0, 0 }, GLCD_FONT_8X8, "READING1:", "000.000"},
		{ { 0, 0, 0, 0 }, GLCD_FONT_8X8, "READING2:", "000.000"},
		{ { 0, 0, 0, 0 }, GLCD_FONT_8X8, "READING3:", "000.000"},
		{ { 0, 0, 0, 0 }, GLCD_FONT_8X8, "READING4:", "000.000"},
		{ { 0, 0, 0, 0 }, GLCD_FONT_8X8, "READING5:", "000.000"}
	},
	{ { 0, 0, 0, 0 }, 0, GLCD_FONT_8X8, "PAGER" }
};

int16_t display_x_max = 319, display_y_max = 239;
rectangle glcd_rect = { 0, 0, 319, 239 };
uint8_t display_orientation = ORIENT_LANDSCAPE;

uint8_t glcd_spi_send_8(uint8_t byte)
{
    __asm("movwf SSPBUF,c"); // first parameter in W
    __asm("spi_send_8_loop:"); // label
    __asm("btfss SSPSTAT,0"); // BF
    __asm("goto spi_send_8_loop");
    __asm("movf SSPBUF,w,c"); // return in W
}

uint16_t glcd_spi_send_16(uint16_t word)
{
    __asm("movf glcd_spi_send_16@word+1,w,c");
    __asm("movwf SSPBUF,c");
    __asm("spi_send_16_loop_1:"); // label
    __asm("btfss SSPSTAT,0");
    __asm("goto spi_send_16_loop_1");
    __asm("movf SSPBUF,w,c");
    __asm("movwf ??_glcd_spi_send_16,c");

    __asm("movf glcd_spi_send_16@word,w,c");
    __asm("movwf SSPBUF,c");
    __asm("spi_send_16_loop_2:"); // label
    __asm("btfss SSPSTAT,0"); // BF
    __asm("goto spi_send_16_loop_2");
    __asm("movf SSPBUF,w,c");
    __asm("movwf ?_glcd_spi_send_16,c"); // return in ?_glcd_spi_send_16
}

void glcd_read_identity(char *glcd_identity)
{

}

void touchscreen_read(uint16_t *x, uint16_t *y)
{
    //{
    //    uint32_t average_x = 0;
    //    uint32_t average_y = 0;
    //
    //    uint8_t x_raw[2] = {0x00, 0x00};
    //    uint8_t y_raw[2] = {0x00, 0x00};
    //
    //    touchscreen_select();
    //    for (int i = 0; i < 10; i++) {
    //        glcd_spi_send_8(0xD0); // TOUCHSCREEN_READ_X
    //        __delay_ms(10);
    //        SPI_TransmitReceive(x_raw, sizeof (x_raw));
    //        glcd_spi_send_8(0x90); // TOUCHSCREEN_READ_Y
    //        __delay_ms(10);
    //        SPI_TransmitReceive(y_raw, sizeof (y_raw));
    //        //        average_x += (((uint16_t) x_raw[0]) << 8) | ((uint16_t) x_raw[1]);
    //        //        average_y += (((uint16_t) y_raw[0]) << 8) | ((uint16_t) y_raw[1]);
    //        average_x = (((uint16_t) x_raw[0]) << 8) | ((uint16_t) x_raw[1]);
    //        average_y = (((uint16_t) y_raw[0]) << 8) | ((uint16_t) y_raw[1]);
    //    }
    //    touchscreen_unselect();
    //
    //    *x = (uint16_t) average_x;
    //    *y = (uint16_t) average_y;
}

void glcd_set_column(int16_t start_column, int16_t end_column) // set column address (X axis)
{
    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_COLUMN_ADDRESS_SET); // 0x2A

    glcd_write_data();
    glcd_spi_send_16(start_column);
    glcd_spi_send_16(end_column);
}

void glcd_set_page(int16_t start_page, int16_t end_page) // set page address (Y axis)
{
    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_PAGE_ADDRESS_SET); // 0x2B

    glcd_write_data();
    glcd_spi_send_16(start_page);
    glcd_spi_send_16(end_page);
}

void glcd_set_pixel(int16_t x0, int16_t y0, uint16_t color)
{
    glcd_select();
    glcd_set_column(x0, x0);
    glcd_set_page(y0, y0);

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_MEMORY_WRITE); // 0x2C

    glcd_write_data();
    glcd_spi_send_16(color);
    glcd_unselect();
}

void glcd_set_orientation(uint8_t orientation)
{
    if (orientation == ORIENT_PORTRAIT) {
        glcd_select();
        glcd_set_column(0, 0);
        glcd_set_page(0, 0);

        glcd_write_command();
        glcd_spi_send_8(ILI9341_CMD_MEMORY_ACCESS_CONTROL); // 0x36

        glcd_write_data();
        glcd_spi_send_8(0x08); // ILI9341_MV bit is not set
        glcd_unselect();
    }

    if (orientation == ORIENT_LANDSCAPE) {
        glcd_select();
        glcd_set_column(0, 0);
        glcd_set_page(0, 0);

        glcd_write_command();
        glcd_spi_send_8(ILI9341_CMD_MEMORY_ACCESS_CONTROL); // 0x36

        glcd_write_data();
        glcd_spi_send_8(0x28); // ILI9341_MV bit is set
        glcd_unselect();
    }
}

void glcd_draw_hline(int16_t x0, int16_t y0, int16_t length, uint16_t color)
{
    glcd_select();
    glcd_set_column(x0, x0 + length);
    glcd_set_page(y0, y0);

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_MEMORY_WRITE); // 0x2C

    glcd_write_data();
    for (int i = 0; i < length; i++)
        glcd_spi_send_16(color);
    glcd_unselect();
}

void glcd_draw_vline(int16_t x0, int16_t y0, int16_t length, uint16_t color)
{
    glcd_select();
    glcd_set_column(x0, x0); // set column address range
    glcd_set_page(y0, y0 + length); // set page address range

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_MEMORY_WRITE); // 0x2C

    glcd_write_data();
    for (int i = 0; i < length; i++)
        glcd_spi_send_16(color);
    glcd_unselect();
}

void glcd_clear_screen(uint16_t color) // fill screen
{
    uint16_t c_loop = display_x_max * (display_y_max / 8);

    glcd_select();
    glcd_set_column(0, display_x_max - 1); // set column address range
    glcd_set_page(0, display_y_max - 1); // set page address range	

    glcd_write_command();
    glcd_spi_send_8(ILI9341_CMD_MEMORY_WRITE); // 0x2C

    glcd_write_data();
    for (uint16_t i = 0; i < c_loop; i++) // write 320*240/8 = 9600
    {
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
        glcd_spi_send_16(color);
    }
    glcd_unselect();
}

void initialize_glcd(void)
{
    glcd_reset();
    glcd_select();
    for (uint8_t *p = &glcd_sequence; *p != 0x00;) {
        glcd_write_command();
        glcd_spi_send_8(*p++);

        if (*p == 0x80) {
            __delay_ms(200);
            p++;
            continue;
        }
        if (*p == 0x81) {
            __delay_ms(120);
            p++;
            continue;
        }

        glcd_write_data();
        for (uint8_t i = *p++; i > 0; i--) {
            glcd_spi_send_8(*p++);
        }
    }
    glcd_unselect();
}

long gcd_euclidis(long a, long b) {
	long nod = 1L;
	long tmp;

	if (a == 0L)
		return b;
	if (b == 0L)
		return a;
	if (a == b)
		return a;
	if (a == 1L || b == 1L)
		return 1L;

	while (a != 0 && b != 0) {
		if (a % 2L == 0L && b % 2L == 0L) {
			nod *= 2L;
			a /= 2L;
			b /= 2L;
			continue;
		}
		if (a % 2L == 0L && b % 2L != 0L) {
			a /= 2L;
			continue;
		}
		if (a % 2L != 0L && b % 2L == 0L) {
			b /= 2L;
			continue;
		}
		if (a > b) {
			tmp = a;
			a = b;
			b = tmp;
		}
		tmp = a;
		a = (b - a) / 2L;
		b = tmp;
	}
	if (a == 0)
		return nod * b;
	else
		return nod * a;
}

long gcd_euclidis_bitshift(long a, long b) {
	long nod = 1L;
	long tmp;

	if (a == 0L)
		return b;
	if (b == 0L)
		return a;
	if (a == b)
		return a;
	if (a == 1L || b == 1L)
		return 1L;

	while (a != 0 && b != 0) {
		if (((a & 1L) | (b & 1L)) == 0L) {
			nod <<= 1L;
			a >>= 1L;
			b >>= 1L;
			continue;
		}
		if (((a & 1L) == 0L) && (b & 1L)) {
			a >>= 1L;
			continue;
		}
		if ((a & 1L) && ((b & 1L) == 0L)) {
			b >>= 1L;
			continue;
		}
		if (a > b) {
			tmp = a;
			a = b;
			b = tmp;
		}
		tmp = a;
		a = (b - a) >> 1L;
		b = tmp;
	}
	if (a == 0)
		return nod * b;
	else
		return nod * a;
}

void draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) // Bresenham's line algorithm
{
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = (dx > dy ? dx : -dy) / 2;

    while (x0 != x1 || y0 != y1) {
        glcd_set_pixel(x0, y0, color);

        int e2 = err;
        if (e2 > -dx) {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dy) {
            err += dx;
            y0 += sy;
        }
    }
}

void draw_circle(int16_t x0, int16_t y0, int16_t radius, uint16_t color) // midpoint circle algorithm
{
    int f = 1 - radius;
    int ddF_x = 0;
    int ddF_y = -2 * radius;
    int x = 0;
    int y = radius;

    glcd_set_pixel(x0, y0 + radius, color);
    glcd_set_pixel(x0, y0 - radius, color);
    glcd_set_pixel(x0 + radius, y0, color);
    glcd_set_pixel(x0 - radius, y0, color);

    while (x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x + 1;
        glcd_set_pixel(x0 + x, y0 + y, color);
        glcd_set_pixel(x0 - x, y0 + y, color);
        glcd_set_pixel(x0 + x, y0 - y, color);
        glcd_set_pixel(x0 - x, y0 - y, color);
        glcd_set_pixel(x0 + y, y0 + x, color);
        glcd_set_pixel(x0 - y, y0 + x, color);
        glcd_set_pixel(x0 + y, y0 - x, color);
        glcd_set_pixel(x0 - y, y0 - x, color);
    }
}

void font_settings(uint8_t font_id, uint8_t *char_width, uint8_t *char_height) {
	switch (font_id) {
	case GLCD_FONT_8X16:
		*char_height = 16;
		*char_width = 8;
		break;
	case GLCD_FONT_8X14:
		*char_height = 14;
		*char_width = 8;
		break;
	case GLCD_FONT_8X12:
		*char_height = 12;
		*char_width = 8;
		break;
	case GLCD_FONT_8X8:
		*char_height = 8;
		*char_width = 8;
		break;
	}
}

void draw_character8x12(uint8_t c, int16_t x0, int16_t y0) {
	uint8_t x, y, w;

	for (y = 0; y < 12; y++) {
		w = font8x12[c][y];
		for (x = 0; x < 8; x++) {
			if (w & 1) { // character
				glcd_set_pixel(x + x0, y + y0, GLCD_NORMAL_FONT_COLOR);
			} else { // background
				glcd_set_pixel(x + x0, y + y0, GLCD_COMPONENT_BACKGROUND_COLOR);
			}
			w = w >> 1;
		}
	}
}

void draw_character8x8(uint8_t c, int16_t x0, int16_t y0) {
	uint8_t x, y, w;

	for (y = 0; y < 8; y++) {
		w = font8x8[c][y];
		for (x = 0; x < 8; x++) {
			if (w & 1) { // character
				glcd_set_pixel(x + x0, y + y0, GLCD_NORMAL_FONT_COLOR);
			} else { // background
				glcd_set_pixel(x + x0, y + y0, GLCD_COMPONENT_BACKGROUND_COLOR);
			}
			w = w >> 1;
		}
	}
}

void draw_text(char text[], uint8_t font_id, int16_t x, int16_t y) {
	uint8_t length;

	length = (uint8_t) strlen(text);
	switch (font_id) {
	case GLCD_FONT_8X12:
		for (int i = 0; i < length; i++) {
			draw_character8x12(text[i], x + (i * 8), y); //  x + i * spacing
		}
		break;
	case GLCD_FONT_8X8:
		for (int i = 0; i < length; i++) {
			draw_character8x8(text[i], x + (i * 8), y); //  x + i * spacing
		}
		break;
	}
}

void outline_text_rectangle(uint8_t length, uint8_t font_id, rectangle *rect) {
	rect->x0 = 0;
	rect->y0 = 0;
	rect->x1 = 0;
	rect->y1 = 0;

	switch (font_id) {
	case GLCD_FONT_8X12:
		rect->x1 = (length * 8) - 1; // length * spacing
		rect->y1 = 12 - 1;           // height
		break;
	case GLCD_FONT_8X8:
		rect->x1 = (length * 8) - 1; // length * spacing
		rect->y1 = 8 - 1;            // height
		break;
	}
}

void draw_rectangle(rectangle rect_a, uint16_t color) {
	rectangle rect_b;

	rect_b.x0 = min(rect_a.x0, rect_a.x1);
	rect_b.y0 = min(rect_a.y0, rect_a.y1);
	rect_b.x1 = max(rect_a.x0, rect_a.x1);
	rect_b.y1 = max(rect_a.y0, rect_a.y1);

	glcd_draw_hline(rect_b.x0, rect_b.y0, rect_b.x1 - rect_b.x0 + 1, color); // top
	glcd_draw_vline(rect_b.x1, rect_b.y0, rect_b.y1 - rect_b.y0 + 1, color); // right
	glcd_draw_hline(rect_b.x0, rect_b.y1, rect_b.x1 - rect_b.x0 + 1, color); // bottom
	glcd_draw_vline(rect_b.x0, rect_b.y0, rect_b.y1 - rect_b.y0 + 1, color); // left
}

uint16_t rectangle_width(rectangle rect_a) {
	return fabs(max(rect_a.x0, rect_a.x1) - min(rect_a.x0, rect_a.x1)) + 1;
}

uint16_t rectangle_height(rectangle rect_a) {
	return fabs(max(rect_a.y0, rect_a.y1) - min(rect_a.y0, rect_a.y1)) + 1;
}

void draw_fill_rectangle(rectangle rect_a, uint16_t color) {
	rectangle rect_b;
	uint16_t width, height;

	rect_b.x0 = min(rect_a.x0, rect_a.x1);
	rect_b.y0 = min(rect_a.y0, rect_a.y1);
	rect_b.x1 = max(rect_a.x0, rect_a.x1);
	rect_b.y1 = max(rect_a.y0, rect_a.y1);

	width = rectangle_width(rect_b);
	height = rectangle_height(rect_b);

	if (width < height)
		while (width--)
			glcd_draw_vline(rect_b.x0 + width, rect_b.y0, height, color);
	else
		while (height--)
			glcd_draw_hline(rect_b.x0, rect_b.y0 + height, width, color);
}

bool rectangle_has_volume(rectangle rect_a) {
	if ((rect_a.x0 == rect_a.x1) || (rect_a.y0 == rect_a.y1))
		return false;

	return true;
}

bool rectangle_contains_point(int16_t x0, int16_t y0, rectangle rect_a) {
	rectangle rect_b;

	rect_b.x0 = min(rect_a.x0, rect_a.x1);
	rect_b.y0 = min(rect_a.y0, rect_a.y1);
	rect_b.x1 = max(rect_a.x0, rect_a.x1);
	rect_b.y1 = max(rect_a.y0, rect_a.y1);

//	if (x0 > rect_b.x0 && x0 < rect_b.x1 && y0 > rect_b.y0 && y0 < rect_b.y1) // without contiguity
//		return true;

	if (x0 >= rect_b.x0 && x0 <= rect_b.x1 && y0 >= rect_b.y0 && y0 <= rect_b.y1) // with contiguity
		return true;

	return false;
}

void scale_rectangle(rectangle rect_a, rectangle *rect_b, int16_t scale_x, int16_t scale_y) {
	rectangle rect_c;

	rect_c.x0 = min(rect_a.x0, rect_a.x1);
	rect_c.y0 = min(rect_a.y0, rect_a.y1);
	rect_c.x1 = max(rect_a.x0, rect_a.x1);
	rect_c.y1 = max(rect_a.y0, rect_a.y1);

	rect_b->x0 = rect_c.x0 - scale_x;
	rect_b->y0 = rect_c.y0 - scale_y;
	rect_b->x1 = rect_c.x1 + scale_x;
	rect_b->y1 = rect_c.y1 + scale_y;
}

void offset_rectangle(rectangle rect_a, rectangle *rect_b, int16_t offset_x, int16_t offset_y) {
	rect_b->x0 = rect_a.x0 + offset_x;
	rect_b->y0 = rect_a.y0 + offset_y;
	rect_b->x1 = rect_a.x1 + offset_x;
	rect_b->y1 = rect_a.y1 + offset_y;
}

void union_rectangle(rectangle rect_a, rectangle rect_b, rectangle *rect_c) {
	rect_c->x0 = min(rect_a.x0, rect_b.x0);
	rect_c->y0 = min(rect_a.y0, rect_b.y0);
	rect_c->x1 = max(rect_a.x1, rect_b.x1);
	rect_c->y1 = max(rect_a.y1, rect_b.y1);
}

void between_rectangle(rectangle rect_a, rectangle rect_b, rectangle *rect_c) {
	rect_c->x0 = max(rect_a.x0, rect_b.x0);
	rect_c->y0 = max(rect_a.y0, rect_b.y0);
	rect_c->x1 = min(rect_a.x1, rect_b.x1);
	rect_c->y1 = min(rect_a.y1, rect_b.y1);
}

bool rectangles_intersect(rectangle rect_a, rectangle rect_b, rectangle *rect_c) {
	int16_t intersect_left = max(rect_a.x0, rect_b.x0);
	int16_t intersect_top = max(rect_a.y0, rect_b.y0);
	int16_t intersect_right = min(rect_a.x1, rect_b.x1);
	int16_t intersect_bottom = min(rect_a.y1, rect_b.y1);

	int16_t intersect_width  = intersect_right - intersect_left;
	int16_t intersect_height = intersect_bottom - intersect_top;

    if (intersect_width >= 0 && intersect_height >= 0) { // with contiguity
    	if (rect_c != NULL) {
    		rect_c->x0 = max(rect_a.x0, rect_b.x0); // left
    		rect_c->y0 = max(rect_a.y0, rect_b.y0); // top
    		rect_c->x1 = min(rect_a.x1, rect_b.x1); // right
    		rect_c->y1 = min(rect_a.y1, rect_b.y1); // bottom
    	}
    	return true;
    }

    return false;
}

bool rectangles_fit(rectangle rect_a, rectangle rect_b) {
	if (rectangle_contains_point(rect_b.x0, rect_b.y0, rect_a) &&
			rectangle_contains_point(rect_b.x1, rect_b.y1, rect_a))
		return true;

	return false;
}

bool rectangles_equal(rectangle rect_a, rectangle rect_b) {
	if (rectangle_width(rect_a) == rectangle_width(rect_b) &&
			rectangle_height(rect_a) == rectangle_height(rect_b))
		return true;

	return false;
}

bool rectangles_fit_equal(rectangle rect_a, rectangle rect_b) {
	rectangle rect_c;
	rectangle rect_d;

	rect_c.x0 = min(rect_a.x0, rect_a.x1);
	rect_c.y0 = min(rect_a.y0, rect_a.y1);
	rect_c.x1 = max(rect_a.x0, rect_a.x1);
	rect_c.y1 = max(rect_a.y0, rect_a.y1);

	rect_d.x0 = min(rect_b.x0, rect_b.x1);
	rect_d.y0 = min(rect_b.y0, rect_b.y1);
	rect_d.x1 = max(rect_b.x0, rect_b.x1);
	rect_d.y1 = max(rect_b.y0, rect_b.y1);

	if (rect_c.x0 == rect_d.x0 && rect_c.y0 == rect_d.y0
			&& rect_c.x1 == rect_d.x1 && rect_c.y1 == rect_d.y1)
		return true;

	return false;
}

void stretch_rectangle_left(rectangle rect_a, rectangle *rect_b, int16_t stretch) {
	rect_b->x0 = min(rect_a.x0, rect_a.x1) + stretch;
	rect_b->y0 = min(rect_a.y0, rect_a.y1);
	rect_b->x1 = max(rect_a.x0, rect_a.x1);
	rect_b->y1 = max(rect_a.y0, rect_a.y1);
}

void stretch_rectangle_top(rectangle rect_a, rectangle *rect_b, int16_t stretch) {
	rect_b->x0 = min(rect_a.x0, rect_a.x1);
	rect_b->y0 = min(rect_a.y0, rect_a.y1) + stretch;
	rect_b->x1 = max(rect_a.x0, rect_a.x1);
	rect_b->y1 = max(rect_a.y0, rect_a.y1);
}

void stretch_rectangle_right(rectangle rect_a, rectangle *rect_b, int16_t stretch) {
	rect_b->x0 = min(rect_a.x0, rect_a.x1);
	rect_b->y0 = min(rect_a.y0, rect_a.y1);
	rect_b->x1 = max(rect_a.x0, rect_a.x1) + stretch;
	rect_b->y1 = max(rect_a.y0, rect_a.y1);
}

void stretch_rectangle_bottom(rectangle rect_a, rectangle *rect_b, int16_t stretch) {
	rect_b->x0 = min(rect_a.x0, rect_a.x1);
	rect_b->y0 = min(rect_a.y0, rect_a.y1);
	rect_b->x1 = max(rect_a.x0, rect_a.x1);
	rect_b->y1 = max(rect_a.y0, rect_a.y1) + stretch;
}

void align_rectangle_in(rectangle rect_a, rectangle rect_b, uint8_t align, rectangle *rect_c) { // align rect_b to rect_a
	uint16_t a_width, a_height;
	uint16_t b_width, b_height;
	uint16_t rect_a_min_x, rect_a_min_y;

	a_width = rectangle_width(rect_a);
	a_height = rectangle_height(rect_a);
	b_width = rectangle_width(rect_b);
	b_height = rectangle_height(rect_b);

	rect_a_min_x = min(rect_a.x0, rect_a.x1);
	rect_a_min_y = min(rect_a.y0, rect_a.y1);

	switch (align) {
	case ALIGN_CENTER:
		rect_c->x0 = rect_a_min_x + a_width / 2 - b_width / 2;
		rect_c->y0 = rect_a_min_y + a_height / 2 - b_height / 2;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_MID_LEFT:
		rect_c->x0 = rect_a_min_x;
		rect_c->y0 = rect_a_min_y + a_height / 2 - b_height / 2;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_MID_TOP:
		rect_c->x0 = rect_a_min_x + a_width / 2 - b_width / 2;
		rect_c->y0 = rect_a_min_y;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_MID_RIGHT:
		rect_c->x0 = rect_a_min_x + a_width - b_width;
		rect_c->y0 = rect_a_min_y + a_height / 2 - b_height / 2;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_MID_BOTTOM:
		rect_c->x0 = rect_a_min_x + a_width / 2 - b_width / 2;
		rect_c->y0 = rect_a_min_y + a_width - b_width;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_LEFT_TOP:
		rect_c->x0 = rect_a_min_x;
		rect_c->y0 = rect_a_min_y;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_LEFT_BOTTOM:
		rect_c->x0 = rect_a_min_x;
		rect_c->y0 = rect_a_min_y + a_height - b_height;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_RIGHT_TOP:
		rect_c->x0 = rect_a_min_x + a_width - b_width;
		rect_c->y0 = rect_a_min_y;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_RIGHT_BOTTOM:
		rect_c->x0 = rect_a_min_x + a_width - b_width;
		rect_c->y0 = rect_a_min_y + a_height - b_height;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	}
}

void align_rectangle_out(rectangle rect_a, rectangle rect_b, uint8_t align, rectangle *rect_c) {
	uint16_t a_width, a_height;
	uint16_t b_width, b_height;
	uint16_t rect_a_min_x, rect_a_min_y;

	a_width = rectangle_width(rect_a);
	a_height = rectangle_height(rect_a);
	b_width = rectangle_width(rect_b);
	b_height = rectangle_height(rect_b);

	rect_a_min_x = min(rect_a.x0, rect_a.x1);
	rect_a_min_y = min(rect_a.y0, rect_a.y1);

	switch (align) {
	case ALIGN_CENTER:
		rect_c->x0 = rect_a_min_x + a_width / 2 - b_width / 2;
		rect_c->y0 = rect_a_min_y + a_height / 2 - b_height / 2;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_MID_LEFT:
		rect_c->x0 = (rect_a_min_x - b_width) + 1; // contiguity
		rect_c->y0 = rect_a_min_y + a_height / 2 - b_height / 2;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_MID_TOP:
		rect_c->x0 = rect_a_min_x + a_width / 2 - b_width / 2;
		rect_c->y0 = (rect_a_min_y - b_height) + 1; // contiguity
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_MID_RIGHT:
		rect_c->x0 = (rect_a_min_x + a_width) - 1; // contiguity
		rect_c->y0 = rect_a_min_y + a_height / 2 - b_height / 2;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_MID_BOTTOM:
		rect_c->x0 = rect_a_min_x + a_width / 2 - b_width / 2;
		rect_c->y0 = (rect_a_min_y + a_width) - 1; // contiguity
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_LEFT_TOP:
		rect_c->x0 = rect_a_min_x;
		rect_c->y0 = (rect_a_min_y - b_height) + 1; // contiguity
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_LEFT_BOTTOM:
		rect_c->x0 = rect_a_min_x;
		rect_c->y0 = (rect_a_min_y + a_height) - 1; // contiguity
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_RIGHT_TOP:
		rect_c->x0 = rect_a_min_x + a_width - b_width;
		rect_c->y0 = (rect_a_min_y - b_height) + 1; // contiguity
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	case ALIGN_RIGHT_BOTTOM:
		rect_c->x0 = rect_a_min_x + a_width - b_width;
		rect_c->y0 = (rect_a_min_y + a_height) - 1; // contiguity;
		rect_c->x1 = rect_c->x0 + b_width - 1;
		rect_c->y1 = rect_c->y0 + b_height - 1;
		break;
	}
}

void stroke_inside_rectangle(rectangle rect_a, rectangle *rect_b) {
	rect_b->x0 = min(rect_a.x0, rect_a.x1) + 1;
	rect_b->y0 = min(rect_a.y0, rect_a.y1) + 1;
	rect_b->x1 = max(rect_a.x0, rect_a.x1) - 1;
	rect_b->y1 = max(rect_a.y0, rect_a.y1) - 1;
}

void stroke_outside_rectangle(rectangle rect_a, rectangle *rect_b) {
	rect_b->x0 = min(rect_a.x0, rect_a.x1) - 1;
	rect_b->y0 = min(rect_a.y0, rect_a.y1) - 1;
	rect_b->x1 = max(rect_a.x0, rect_a.x1) + 1;
	rect_b->y1 = max(rect_a.y0, rect_a.y1) + 1;
}

void rectangle_same_width(rectangle rect_a, rectangle rect_b, rectangle *rect_c) {
	uint16_t width_a  = rectangle_width(rect_a);
	uint16_t width_b  = rectangle_width(rect_b);

	if (width_a > width_b)
		stretch_rectangle_right(rect_a, rect_c, width_a - width_b);

	if (width_a < width_b)
		stretch_rectangle_left(rect_a, rect_c, width_a - width_b);
}

void rectangle_same_height(rectangle rect_a, rectangle rect_b, rectangle *rect_c) {
	uint16_t height_a  = rectangle_height(rect_a);
	uint16_t height_b  = rectangle_height(rect_b);

	if (height_a > height_b)
		stretch_rectangle_bottom(rect_a, rect_c, height_a - height_b);

	if (height_a < height_b)
		stretch_rectangle_top(rect_a, rect_c, height_a - height_b);
}

void initialize_gui(rectangle glcd_rect) {
	uint8_t char_width, char_height;
	int16_t grid_horizontal = 320;
	int16_t grid_vertical = 240;
	int8_t grid_step = 10;
	rectangle components_place;

	draw_fill_rectangle(glcd_rect, GLCD_BACKGROUND_COLOR); // grid
	for (uint16_t i = 0; i <= grid_vertical; i += grid_step) { // grid lines x-axis parallel
		glcd_draw_hline(0, i, grid_horizontal + 1, MAYA_COMPONENT_BACKGROUND);
	}
	for (uint16_t i = 0; i <= grid_horizontal; i += grid_step) { // grid lines y-axis parallel
		glcd_draw_vline(i, 0, grid_vertical + 1, MAYA_COMPONENT_BACKGROUND);
	}
	draw_rectangle(glcd_rect, GLCD_BORDER_COLOR);

	font_settings(glcd_gui.titlebar.font_id, &char_width, &char_height); // titlebar
	glcd_gui.titlebar.bound.x0 = glcd_rect.x0; // calculate coordinates
	glcd_gui.titlebar.bound.y0 = glcd_rect.y0;
	glcd_gui.titlebar.bound.x1 = glcd_rect.x1;
	glcd_gui.titlebar.bound.y1 = 2 + 1 + char_height;

	for (uint8_t i = 0; i < (sizeof(glcd_gui.labels) / sizeof(glcd_label)); i++) { // labels
		font_settings(glcd_gui.labels[0].font_id, &char_width, &char_height);
		glcd_gui.labels[i].bound.x0 = 0; // calculate coordinates
		glcd_gui.labels[i].bound.y0 = 0;
		glcd_gui.labels[i].bound.x1 = 2 + 1 + char_width * (uint8_t)sizeof(glcd_gui.labels[i].text);
		glcd_gui.labels[i].bound.y1 = 2 + 1 + char_height;
	}

	for (uint8_t i = 0; i < (sizeof(glcd_gui.readings) / sizeof(glcd_readings)); i++) { // readings
		font_settings(glcd_gui.readings[0].font_id, &char_width, &char_height);
		glcd_gui.readings[i].bound.x0 = 0; // calculate coordinates
		glcd_gui.readings[i].bound.y0 = 0;
		glcd_gui.readings[i].bound.x1 = 2 + 1 + char_width * ((uint8_t)sizeof(glcd_gui.readings[i].text) + (uint8_t)sizeof(glcd_gui.readings[i].value));
		glcd_gui.readings[i].bound.y1 = 2 + 1 + char_height;
	}

	font_settings(glcd_gui.pager.font_id, &char_width, &char_height); // pager
	glcd_gui.pager.bound.x0 = glcd_rect.x0; // calculate coordinates
	glcd_gui.pager.bound.y0 = glcd_rect.y1 - (2 + 1 + char_height);
	glcd_gui.pager.bound.x1 = glcd_rect.x1;
	glcd_gui.pager.bound.y1 = glcd_rect.y1;

	between_rectangle(glcd_gui.titlebar.bound, glcd_gui.pager.bound, &components_place);
	for (uint8_t i = 0; i < (sizeof(glcd_gui.labels) / sizeof(glcd_label)); i++) { // layout labels
		align_rectangle_in(components_place, glcd_gui.labels[i].bound, ALIGN_MID_TOP, &glcd_gui.labels[i].bound);
		offset_rectangle(glcd_gui.labels[i].bound, &glcd_gui.labels[i].bound, 0, 15*i+20);
	}

	for (uint8_t i = 0; i < (sizeof(glcd_gui.readings) / sizeof(glcd_readings)); i++) { // layout readings
		align_rectangle_in(components_place, glcd_gui.readings[i].bound, ALIGN_MID_TOP, &glcd_gui.readings[i].bound);
		offset_rectangle(glcd_gui.readings[i].bound, &glcd_gui.readings[i].bound, 0, 15*i+120);
	}
}

void titlebar_draw(glcd_titlebar *titlebar) {
	rectangle area, outline;

	draw_rectangle(titlebar->bound, GLCD_BORDER_COLOR); // draw border
	stroke_inside_rectangle(titlebar->bound, &area); // calculate component area
	draw_fill_rectangle(area, GLCD_COMPONENT_BACKGROUND_COLOR); // fill component area
//
	outline_text_rectangle((uint8_t)strlen(titlebar->text), titlebar->font_id, &outline); // calculate text outline
	align_rectangle_in(area, outline, ALIGN_CENTER, &outline); // align text outline
	draw_text(titlebar->text, titlebar->font_id, outline.x0, outline.y0); // draw text
}

void label_draw(glcd_label *label) { // draws text
	rectangle area, outline;

	draw_rectangle(label->bound, GLCD_BORDER_COLOR); // draw border
	stroke_inside_rectangle(label->bound, &area); // calculate component area
	draw_fill_rectangle(area, GLCD_COMPONENT_BACKGROUND_COLOR); // fill component area

	outline_text_rectangle((uint8_t) strlen(label->text), label->font_id, &outline); // calculate text outline
	align_rectangle_in(area, outline, label->align, &outline); // align text outline
	draw_text(label->text, label->font_id, outline.x0, outline.y0); // draw text
}

void label_set_text(glcd_label *label, char text[]) {
	strcpy(label->text, text);
}

/// -----------
void label1_set_text(char text[]) {
	strcpy(glcd_gui.labels[0].text, text);
    label_draw(&glcd_gui.labels[0]);
}
void label2_set_text(char text[]) {
	strcpy(glcd_gui.labels[1].text, text);
    label_draw(&glcd_gui.labels[1]);
}
void label3_set_text(char text[]) {
	strcpy(glcd_gui.labels[2].text, text);
    label_draw(&glcd_gui.labels[2]);
}
void label4_set_text(char text[]) {
	strcpy(glcd_gui.labels[3].text, text);
    label_draw(&glcd_gui.labels[3]);
}
void label5_set_text(char text[]) {
	strcpy(glcd_gui.labels[4].text, text);
    label_draw(&glcd_gui.labels[4]);
}
/// -----------

void label_set_align(glcd_label *label, uint8_t align) {
	label->align = align;
}

void readings_draw(glcd_readings *readings) { // draws text and value
	rectangle area, outline;

	draw_rectangle(readings->bound, GLCD_BORDER_COLOR); // draw border
	scale_rectangle(readings->bound, &area, -1, -1); // calculate component area
	draw_fill_rectangle(area, GLCD_COMPONENT_BACKGROUND_COLOR); // fill component area

	outline_text_rectangle((uint8_t) strlen(readings->text), readings->font_id, &outline); // calculate text outline
	align_rectangle_in(area, outline, ALIGN_MID_LEFT, &outline); // align text outline
	draw_text(readings->text, readings->font_id, outline.x0 + 1, outline.y0 + 1); // draw text

	outline_text_rectangle((uint8_t) strlen(readings->value), readings->font_id, &outline); // calculate text outline
	align_rectangle_in(area, outline, ALIGN_MID_RIGHT, &outline); // align text outline
	draw_text(readings->value, readings->font_id, outline.x0 + 1, outline.y0 + 1); // draw text
}

void readings_set_text(glcd_readings *readings, char text[]) {
	strcpy(readings->text, text);
}

void readings_set_value(glcd_readings *readings, char value[]) {
	strcpy(readings->value, value);
}

uint8_t percentage_from(uint16_t value, uint8_t percent) {
	return value * percent / 100; // multiply before divide
}

void pager_draw(glcd_pager *pager) {
	rectangle area, outline;
	//uint8_t ss_pixels = 50; // scale/stretch pixels
	uint8_t ss_pixels = percentage_from(rectangle_width(pager->bound), 10); // scale/stretch pixels

	scale_rectangle(pager->bound, &pager->bound, -ss_pixels, 0);
	draw_rectangle(pager->bound, GLCD_BORDER_COLOR); // draw border
	scale_rectangle(pager->bound, &area, -1, -1); // calculate component area
	draw_fill_rectangle(area, GLCD_COMPONENT_BACKGROUND_COLOR); // fill area
	outline_text_rectangle((uint8_t)strlen(pager->text), pager->font_id, &outline); // calculate text outline
	align_rectangle_in(area, outline, ALIGN_CENTER, &outline); // align text outline
	draw_text(pager->text, pager->font_id, outline.x0, outline.y0); // draw text

	stretch_rectangle_right(pager->bound, &area, -((ss_pixels + rectangle_width(pager->bound)) - 1));
	draw_rectangle(area, GLCD_BORDER_COLOR);
	scale_rectangle(area, &area, -1, -1); // scale area
	draw_fill_rectangle(area, GLCD_COMPONENT_BACKGROUND_COLOR); // fill area
	outline_text_rectangle(1, pager->font_id, &outline);
	align_rectangle_in(area, outline, ALIGN_CENTER, &outline);
	draw_text("\x11", pager->font_id, outline.x0, outline.y0);

	stretch_rectangle_left(pager->bound, &area, +((ss_pixels + rectangle_width(pager->bound)) - 1));
	draw_rectangle(area, GLCD_BORDER_COLOR);
	scale_rectangle(area, &area, -1, -1); // scale area
	draw_fill_rectangle(area, GLCD_COMPONENT_BACKGROUND_COLOR); // fill area
	outline_text_rectangle(1, pager->font_id, &outline);
	align_rectangle_in(area, outline, ALIGN_CENTER, &outline);
	draw_text("\x10", pager->font_id, outline.x0, outline.y0);
}

void glcd_demo(void)
{
    char string_print[256];

    titlebar_draw(&glcd_gui.titlebar);
	label_draw(&glcd_gui.labels[0]);
	label_draw(&glcd_gui.labels[1]);
	label_draw(&glcd_gui.labels[2]);
	label_draw(&glcd_gui.labels[3]);
	label_draw(&glcd_gui.labels[4]);
	readings_draw(&glcd_gui.readings[0]);
	readings_draw(&glcd_gui.readings[1]);
	readings_draw(&glcd_gui.readings[2]);
	readings_draw(&glcd_gui.readings[3]);
	readings_draw(&glcd_gui.readings[4]);
	pager_draw(&glcd_gui.pager);
    
    label_set_text(&glcd_gui.labels[0], "select/write");
	label_set_text(&glcd_gui.labels[1], "frequency/phase");
	label_set_text(&glcd_gui.labels[2], "register");
	label_set_text(&glcd_gui.labels[3], "value");
    label_set_text(&glcd_gui.labels[4], "CRC");
    	
    label_draw(&glcd_gui.labels[0]);
	label_draw(&glcd_gui.labels[1]);
	label_draw(&glcd_gui.labels[2]);
	label_draw(&glcd_gui.labels[3]);
    label_draw(&glcd_gui.labels[4]);
}
