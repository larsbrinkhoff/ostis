#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include <SDL_image.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

#include "screen.h"
#include "shifter.h"
#include "debug/display.h"
#include "diag.h"

struct monitor {
  const char *name;
  int width, height, columns, lines;
  int crop_offset_x, crop_offset_y;
  int crop_width, crop_height;
  SDL_Surface *screen;
  SDL_Texture *texture;
  SDL_Window *window;
  SDL_Renderer *renderer;
  char *rgbimage, *rasterpos, *next_line;
};

static int disable = 0;
int screen_window_id;
static SDL_Texture *rasterpos_indicator[2];
static int rasterpos_indicator_cnt = 0;
static int screen_grabbed = 0;
static int screen_fullscreen = 0;
static int screen_delay_value = 0;

static int ppm_fd;
static int framecnt;
static int64_t last_framecnt_usec;
static int usecs_per_framecnt_interval = 1;
int64_t last_vsync_ticks = 0;

struct monitor monitor[] = {
  {
    .name = "Colour Monitor",
    .width = 1024,
    .height = 626,
    .columns = 1024,
    .lines = 313,
    .crop_offset_x = 64,
    .crop_offset_y = 33,
    .crop_width = 768,
    .crop_height = 560,
    .texture = NULL
  },
  {
    .name = "Monochrome Monitor",
    .width = 896,
    .height = 501,
    .columns = 896,
    .lines = 501,
    .crop_offset_x = 0,
    .crop_offset_y = 0,
    .crop_width = 896,
    .crop_height = 501,
    .texture = NULL
  }
};

int monitors = 0;
struct monitor mon[2];

HANDLE_DIAGNOSTICS(screen)

void screen_make_texture(const char *scale)
{
  static const char *old_scale = "";
  int i;

  if(monitors == 0) {
    if(monitor_sc1224)
      mon[monitors++] = monitor[0];
    if(monitor_sm124)
      mon[monitors++] = monitor[1];
  }

  if(strcmp(scale, old_scale) == 0)
    return;

  for(i = 0; i < monitors; i++) {
    if(mon[i].texture != NULL)
      SDL_DestroyTexture(mon[i].texture);

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scale);
    mon[i].texture = SDL_CreateTexture(mon[i].renderer,
				       SDL_PIXELFORMAT_RGB24,
				       SDL_TEXTUREACCESS_STREAMING,
				       mon[i].columns, mon[i].lines);
  }
}

SDL_Texture *screen_generate_rasterpos_indicator(int color)
{
  Uint32 rmask, gmask, bmask, amask;
  SDL_Surface *rscreen;
  SDL_Texture *rtext;
  int i;
  char *p;
  
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
  amask = 0x00000000;
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
#else
  amask = 0x00000000;
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
#endif

  rscreen = SDL_CreateRGBSurface(0, 4, 1, 24, rmask, gmask, bmask, amask);

  p = rscreen->pixels;
  
  for(i=0;i<rscreen->w;i++) {
    p[i*rscreen->format->BytesPerPixel+0] = (color<<16)&0xff;
    p[i*rscreen->format->BytesPerPixel+1] = (color<<8)&0xff;
    p[i*rscreen->format->BytesPerPixel+2] = color&0xff;
  }

  rtext = SDL_CreateTexture(mon[0].renderer, SDL_PIXELFORMAT_BGR24,
                            SDL_TEXTUREACCESS_STREAMING,
                            4, 1);
  
  SDL_UpdateTexture(rtext, NULL, rscreen->pixels, rscreen->pitch);
  
  return rtext;
}

void screen_init()
{
  /* should be rewritten with proper error checking */
  Uint32 rmask, gmask, bmask, amask;
  SDL_Surface *icon;
  int i;
  
  HANDLE_DIAGNOSTICS_NON_MMU_DEVICE(screen, "SCRN");

  if(disable) return;
  
#if SDL_BYTEORDER != SDL_BIG_ENDIAN
  amask = 0x00000000;
  rmask = 0x00ff0000;
  gmask = 0x0000ff00;
  bmask = 0x000000ff;
#else
  amask = 0x00000000;
  rmask = 0x000000ff;
  gmask = 0x0000ff00;
  bmask = 0x00ff0000;
#endif

  if(monitors == 0) {
    if(monitor_sc1224)
      mon[monitors++] = monitor[0];
    if(monitor_sm124)
      mon[monitors++] = monitor[1];
  }

  for(i = 0; i < monitors; i++) {
    if (crop_screen) {
      mon[i].width = mon[i].crop_width;
      mon[i].height = mon[i].crop_height;
    }
  
    mon[i].window = SDL_CreateWindow(mon[i].name,
				     SDL_WINDOWPOS_UNDEFINED,
				     SDL_WINDOWPOS_UNDEFINED,
				     mon[i].width,
				     mon[i].height,
				     SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    // NOTE: We always allocate memory for 501 lines, in case someone
    // decides to use a colour monitor, but switch to high resolution
    // near the end of the screen.  In that case, we'd try to output
    // 501 lines and overflow the buffer.
    mon[i].screen = SDL_CreateRGBSurface(0, mon[i].columns, 501,
					 24, rmask, gmask, bmask, amask);
    mon[i].renderer = SDL_CreateRenderer(mon[0].window, -1, 0);
    screen_window_id = SDL_GetWindowID(mon[i].window);
    DEBUG("screen_window_id == %d", screen_window_id);
    screen_make_texture(SDL_SCALING_NEAREST);

    if(mon[i].screen == NULL) {
      FATAL("Did not get a video mode");
    }

    icon = IMG_Load("logo-main.png");
    SDL_SetWindowIcon(mon[i].window, icon);
  }
  
  if(ppmoutput) {
    ppm_fd = open("ostis.ppm", O_WRONLY|O_CREAT|O_TRUNC, 0644);
  }

  mon[0].rgbimage = mon[0].screen->pixels;
  rasterpos_indicator[0] = screen_generate_rasterpos_indicator(0xffffff);
  rasterpos_indicator[1] = screen_generate_rasterpos_indicator(0xff0000);

  framecnt = 0;

  screen_vsync();

  if(debugger) {
    debug_raise_window();
  }
}

float screen_fps()
{
  if(usecs_per_framecnt_interval) {
    return 1000000*64.0/usecs_per_framecnt_interval;
  } else {
    return 0;
  }
}

int screen_framecnt(int c)
{
  if(c == -1) {
    framecnt = 0;
  }
  return framecnt;
}

static void screen_build_ppm()
{
  int x,y,c;
  char header[80];
  unsigned char frame[384*288*3];

  c = 0;

  for(y=0;y<288;y++) {
    for(x=0;x<384;x++) {
      frame[c*3+0] = mon[0].rgbimage[((y+12)*512+x+32)*6+2];
      frame[c*3+1] = mon[0].rgbimage[((y+12)*512+x+32)*6+1];
      frame[c*3+2] = mon[0].rgbimage[((y+12)*512+x+32)*6+0];
      c++;
    }
  }

  sprintf(header, "P6\n%d %d\n255\n", 384, 288);
  if(write(ppm_fd, header, strlen(header)) != strlen(header))
    WARNING(write);
  if (write(ppm_fd, frame, 384*288*3) != 384*288*3)
    WARNING(write);
}

void screen_copyimage(unsigned char *src)
{
}

void screen_clear()
{
}

int screen_get_vsync()
{
  return (mon[0].rasterpos - mon[0].rgbimage) / 6;
}

void screen_swap(int indicate_rasterpos)
{
  SDL_Rect dst,src;
  
  if(disable) return;

    if(debugger) {
      display_swap_screen();
    }
    SDL_UpdateTexture(mon[0].texture, NULL, mon[0].screen->pixels,
		      mon[0].screen->pitch);
    if(crop_screen) {
      src.x = mon[0].crop_offset_x;
      src.y = mon[0].crop_offset_y;
      src.w = mon[0].crop_width;
      src.h = mon[0].crop_height;
      SDL_RenderCopy(mon[0].renderer, mon[0].texture, &src, NULL);
    } else {
      SDL_RenderCopy(mon[0].renderer, mon[0].texture, NULL, NULL);
    }
    if(indicate_rasterpos) {
      int rasterpos = screen_get_vsync();
      dst.x = 2*(rasterpos%512)-8;
      dst.y = 2*(rasterpos/512);
      dst.w = 8;
      dst.h = 2;
      SDL_RenderCopy(mon[0].renderer, rasterpos_indicator[rasterpos_indicator_cnt&1], NULL, &dst);
      rasterpos_indicator_cnt++;
    }
    SDL_RenderPresent(mon[0].renderer);
}

void screen_disable(int yes)
{
  if(yes) {
    disable = 1;
  } else {
    disable = 0;
  }
}

int screen_check_disable()
{
  return disable;
}

static void set_screen_grabbed(int grabbed)
{
  int w, h;

  if(grabbed) {
    SDL_SetWindowGrab(mon[0].window, SDL_TRUE);
    SDL_GetWindowSize(mon[0].window, &w, &h);
    SDL_WarpMouseInWindow(mon[0].window, w/2, h/2);
    SDL_ShowCursor(0);
    screen_grabbed = 1;
  } else {
    SDL_SetWindowGrab(mon[0].window, SDL_FALSE);
    SDL_ShowCursor(1);
    screen_grabbed = 0;
  }
}

void screen_toggle_grab()
{
  set_screen_grabbed(!screen_grabbed);
}

void screen_toggle_fullscreen()
{
  screen_fullscreen = !screen_fullscreen;
  SDL_SetWindowFullscreen(mon[0].window,
			  screen_fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
  set_screen_grabbed(screen_fullscreen);
}

void screen_draw(int r, int g, int b)
{
  *mon[0].rasterpos++ = r;
  *mon[0].rasterpos++ = g;
  *mon[0].rasterpos++ = b;
}

static int64_t usec_count() {
  static struct timeval tv;

  gettimeofday(&tv,NULL);
  return tv.tv_sec * 1000000 + tv.tv_usec;
}

void screen_vsync(void)
{
  int64_t current_ticks = 0;
  int64_t remaining = 0;

  TRACE("Vsync");

  if(ppmoutput) {
    screen_build_ppm();
  }
  if(screen_delay_value) {
    current_ticks = usec_count();
    remaining = current_ticks - last_vsync_ticks;
    while(remaining < screen_delay_value) {
      current_ticks = usec_count();
      remaining = current_ticks - last_vsync_ticks;
    }
    last_vsync_ticks = current_ticks;
  }
  screen_swap(SCREEN_NORMAL);

  framecnt++;
  if((framecnt&0x3f) == 0) {
    current_ticks = usec_count();
    usecs_per_framecnt_interval = current_ticks - last_framecnt_usec;
    last_framecnt_usec = current_ticks;
  }

  mon[0].rasterpos = mon[0].rgbimage;
  mon[0].next_line = mon[0].rgbimage + mon[0].screen->pitch;
}

void screen_hsync(void)
{
  mon[0].rasterpos = mon[0].next_line;
  mon[0].next_line += mon[0].screen->pitch;
}

void screen_set_delay(int delay_value)
{
  INFO("Set update speed to %d ms", delay_value/1000);
  screen_delay_value = delay_value;
}
