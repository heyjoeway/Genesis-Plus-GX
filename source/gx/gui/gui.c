/****************************************************************************
 *  menu.c
 *
 *  GUI Engine, using GX hardware
 *
 *  Eke-Eke (2009)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ***************************************************************************/

#include "shared.h"
#include "gui.h"
#include "font.h"

#ifdef HW_RVL
gx_texture *w_pointer;
#endif

/* background color */
static GXColor bg_color;

/* prompt window button data */
static butn_data button_text_data =
{
  {NULL,NULL},
  {Button_text_png,Button_text_over_png}
};

/*****************************************************************************/
/*  Generic GUI routines                                                     */
/*****************************************************************************/

/* Allocate texture images data */
void GUI_InitMenu(gui_menu *menu)
{
  int i;
  gui_item *item;
  gui_butn *button;
  gui_image *image;

  /* background elements */
  for (i=0; i<menu->max_images; i++)
  {
    image = &menu->bg_images[i];
    image->texture = gxTextureOpenPNG(image->data,0);
  }

  for (i=0; i<2; i++)
  {
    /* key helpers */
    item = menu->helpers[i];
    if (item)
      item->texture = gxTextureOpenPNG(item->data,0);

    /* arrows */
    button = menu->arrows[i];
    if (button)
    {
      if (!button->data->texture[0])
        button->data->texture[0] = gxTextureOpenPNG(button->data->image[0],0);
      if (!button->data->texture[1])
        button->data->texture[1] = gxTextureOpenPNG(button->data->image[1],0);

      /* initial state */
      button->state &= ~BUTTON_VISIBLE;
      if (((i==0) && (menu->offset != 0)) || ((i==1) && (menu->offset + menu->max_buttons) < menu->max_items))
        button->state |= BUTTON_VISIBLE;
    }
  }

  /* menu buttons */
  for (i=0; i<menu->max_buttons; i++)
  {
    button = &menu->buttons[i];
    if (button->data)
    {
      if (!button->data->texture[0])
        button->data->texture[0] = gxTextureOpenPNG(button->data->image[0],0);
      if (!button->data->texture[1])
        button->data->texture[1] = gxTextureOpenPNG(button->data->image[1],0);
    }
  }

  /* menu items */
  for (i=0; i<menu->max_items; i++)
  {
    item = &menu->items[i];
    if (item->data)
      item->texture = gxTextureOpenPNG(item->data,0);
  }
}

/* release allocated memory */
void GUI_DeleteMenu(gui_menu *menu)
{
  int i;
  gui_butn *button;
  gui_item *item;
  gui_image *image;

  /* background elements */
  for (i=0; i<menu->max_images; i++)
  {
    image = &menu->bg_images[i];
    gxTextureClose(&image->texture);
  }

  for (i=0; i<2; i++)
  {
    /* key helpers */
    item = menu->helpers[i];
    if (item)
      gxTextureClose(&item->texture);

    /* arrows */
    button = menu->arrows[i];
    if (button)
    {
      gxTextureClose(&button->data->texture[0]);
      gxTextureClose(&button->data->texture[1]);
    }
  }

  /* menu buttons */
  for (i=0; i<menu->max_buttons; i++)
  {
    button = &menu->buttons[i];
    if (button->data)
    {
      gxTextureClose(&button->data->texture[0]);
      gxTextureClose(&button->data->texture[1]);
    }
  }

  /* menu items */
  for (i=0; i<menu->max_items; i++)
  {
    item = &menu->items[i];
    gxTextureClose(&item->texture);
  }
}

/* set default background */
void GUI_SetBgColor(GXColor color)
{
  bg_color.r = color.r;
  bg_color.g = color.g;
  bg_color.b = color.b;
  bg_color.a = color.a;
}

/* Menu Rendering */
void GUI_DrawMenu(gui_menu *menu)
{
  int i;
  gui_item *item;
  gui_butn *button;
  gui_image *image;

  /* background color */
  if (menu->screenshot)
  {
    gxClearScreen((GXColor)BLACK);
    gxDrawScreenshot(128);
  }
  else
  {
    gxClearScreen(bg_color);
  }

  /* background elements */
  for (i=0; i<menu->max_images; i++)
  {
    image = &menu->bg_images[i];
    if (image->state & IMAGE_REPEAT)
      gxDrawTextureRepeat(image->texture,image->x,image->y,image->w,image->h,image->alpha);
    else if (image->state & IMAGE_VISIBLE)
      gxDrawTexture(image->texture,image->x,image->y,image->w,image->h,image->alpha);
  }

  /* menu title */
  FONT_write(menu->title, 22,10,56,640,(GXColor)WHITE);

  /* draw buttons + items */
  for (i=0; i<menu->max_buttons; i++)
  {
    button = &menu->buttons[i];

    if (button->state & BUTTON_VISIBLE)
    {
      /* draw button + items */ 
      item = &menu->items[menu->offset +i];
      if ((i == menu->selected) && (button->state & BUTTON_ACTIVE))
      {
        if (button->data) gxDrawTexture(button->data->texture[1],button->x-4,button->y-4,button->w+8,button->h+8,255);

        if (item->texture)
        {
          gxDrawTexture(item->texture, item->x-4,item->y-4,item->w+8,item->h+8,255);
          FONT_writeCenter(item->text,18,button->x+4,item->x-4,button->y+(button->h - 36)/2+18,(GXColor)DARK_GREY);
        }
        else
        {
          FONT_writeCenter(item->text,18,item->x+2,item->x+item->w+2,button->y+(button->h-18)/2+18,(GXColor)DARK_GREY);
        }

        /* update help comment */
        if (menu->helpers[1]) strcpy(menu->helpers[1]->comment,item->comment);
      }
      else
      {
        if (button->data)
          gxDrawTexture(button->data->texture[0],button->x,button->y,button->w, button->h,255);

        if (item->texture)
        {
          gxDrawTexture(item->texture,item->x,item->y,item->w,item->h,255);
          FONT_writeCenter(item->text,16,button->x+8,item->x,button->y+(button->h - 32)/2+16,(GXColor)DARK_GREY);
        }
        else
        {
          FONT_writeCenter(item->text,16,item->x,item->x+item->w,button->y+(button->h - 16)/2+16,(GXColor)DARK_GREY);
        }
      }
    }
  }

  /* draw arrow */
  for (i=0; i<2; i++)
  {
    button = menu->arrows[i];
    if (button)
    {
      if (button->state & BUTTON_VISIBLE)
      {
        if (menu->selected == (menu->max_buttons + i))
          gxDrawTexture(button->data->texture[1],button->x-2,button->y-2,button->w+4,button->h+4,255);
        else
          gxDrawTexture(button->data->texture[0],button->x,button->y,button->w, button->h,255);
      }
    }
  }

  /* left comment */
  item = menu->helpers[0];
  if (item)
  {
    gxDrawTexture(item->texture,item->x,item->y,item->w,item->h,255);
    FONT_write(item->comment,16,item->x+item->w+6,item->y+(item->h-16)/2 + 16,640,(GXColor)WHITE);
  }

  /* right comment */
  item = menu->helpers[1];
  if (item)
  {
    if (menu->selected  < menu->max_buttons)
    {
      gxDrawTexture(item->texture,item->x,item->y,item->w,item->h,255);
      FONT_alignRight(item->comment,16,item->x-6,item->y+(item->h-16)/2+16,(GXColor)WHITE);
    }
  }
}

/* Menu Transitions effect */
void GUI_DrawMenuFX(gui_menu *menu, u8 speed, u8 out)
{
  int i,temp,xoffset,yoffset;
  int max_offset = 0;
  u8 item_alpha = 255;
  GXColor text_color = DARK_GREY;
  gui_item *item;
  gui_butn *button;
  gui_image *image;

  /* find maximal offset */
  for (i=0; i<menu->max_images; i++)
  {
    image = &menu->bg_images[i];

    if (image->state & IMAGE_SLIDE_LEFT)
    {
      temp = image->x + image->w;
      if (max_offset < temp) max_offset = temp;
    }
    else if (image->state & IMAGE_SLIDE_RIGHT)
    {
      temp = 640 - image->x;
      if (max_offset < temp) max_offset = temp;
    }


    if (image->state & IMAGE_SLIDE_TOP)
    {
      temp = image->y + image->h;
      if (max_offset < temp) max_offset = temp;
    }
    else if (image->state & IMAGE_SLIDE_BOTTOM)
    {
      temp = 480 - image->y;
      if (max_offset < temp) max_offset = temp;
    }
  }

  temp = max_offset;

  /* Alpha steps */
  int alpha = out ? 255 : 0;
  int alpha_step = (255 * speed) / max_offset;
  if (out) alpha_step = -alpha_step;

  /* Let's loop until final position has been reached */
  while (temp > 0)
  {
    /* background color */
    if (menu->screenshot)
    {
      gxClearScreen((GXColor)BLACK);
      if (alpha > 127) gxDrawScreenshot(128);
      else gxDrawScreenshot(255 - alpha);
    }
    else
    {
      gxClearScreen(bg_color);
    }

    /* background images */
    for (i=0; i<menu->max_images; i++)
    {
      image = &menu->bg_images[i];

      /* X offset */
      if (image->state & IMAGE_SLIDE_LEFT)
        xoffset = out ? (temp - max_offset) : (-temp);
      else if (image->state & IMAGE_SLIDE_RIGHT)
        xoffset = out ? (max_offset - temp) : (temp);
      else
        xoffset = 0;

      /* Y offset */
      if (image->state & IMAGE_SLIDE_TOP)
        yoffset = out ? (temp - max_offset) : (-temp);
      else if (image->state & IMAGE_SLIDE_BOTTOM)
        yoffset = out ? (max_offset - temp) : (temp);
      else
        yoffset = 0;

      /* draw image */
      if ((image->state & IMAGE_FADE) && ((out && (image->alpha > alpha)) || (!out && (image->alpha < alpha))))
      {
        /* FADE In-Out */
        if (image->state & IMAGE_REPEAT) gxDrawTextureRepeat(image->texture,image->x+xoffset,image->y+yoffset,image->w,image->h,alpha);
        else if (image->state & IMAGE_VISIBLE)gxDrawTexture(image->texture,image->x+xoffset,image->y+yoffset,image->w,image->h,alpha);
      }
      else
      {
        if (image->state & IMAGE_REPEAT) gxDrawTextureRepeat(image->texture,image->x+xoffset,image->y+yoffset,image->w,image->h,image->alpha);
        else if (image->state & IMAGE_VISIBLE)gxDrawTexture(image->texture,image->x+xoffset,image->y+yoffset,image->w,image->h,image->alpha);
      }
    }

    /* menu title */
    if (menu->bg_images[2].state & IMAGE_SLIDE_TOP)
    {
      FONT_write(menu->title, 22,10,out ? (56 + temp - max_offset) : (56 -temp),640,(GXColor)WHITE);
    }
    else
    {
      FONT_write(menu->title, 22,10,56,640,(GXColor)WHITE);
    }

    /* draw buttons + items */
    for (i=0; i<menu->max_buttons; i++)
    {
      button = &menu->buttons[i];

      if (button->state & BUTTON_VISIBLE)
      {
        /* X offset */
        if (button->state & BUTTON_SLIDE_LEFT)
          xoffset = out ? (temp - max_offset) : (-temp);
        else if (button->state & BUTTON_SLIDE_RIGHT)
          xoffset = out ? (max_offset - temp) : (temp);
        else
          xoffset = 0;

        /* Y offset */
        if (button->state & BUTTON_SLIDE_TOP)
          yoffset = out ? (temp - max_offset) : (-temp);
        else if (button->state & BUTTON_SLIDE_BOTTOM)
          yoffset = out ? (max_offset - temp) : (temp);
        else
          yoffset = 0;

        /* Alpha transparency */
        if (button->state & BUTTON_FADE)
        {
          item_alpha = alpha;
          text_color.a = alpha;
        }
        else
        {
          item_alpha = 255;
          text_color.a = 255;
        }

        /* draw button + items */ 
        item = &menu->items[menu->offset + i];

        if (button->data) gxDrawTexture(button->data->texture[0],button->x+xoffset,button->y+yoffset,button->w, button->h,item_alpha);
        if (item->texture)
        {
          gxDrawTexture(item->texture,item->x+xoffset,item->y+yoffset,item->w,item->h,item_alpha);
          FONT_writeCenter(item->text,16,button->x+xoffset+8,item->x+xoffset,button->y+yoffset+(button->h - 32)/2+16,text_color);
        }
        else
        {
          FONT_writeCenter(item->text,16,item->x+xoffset,item->x+item->w+xoffset,button->y+yoffset+(button->h - 16)/2+16,text_color);
        }
      }
    }

    /* update offset */
    temp -= speed;

    /* update alpha */
    alpha += alpha_step;
    if (alpha > 255) alpha = 255;
    else if (alpha < 0) alpha = 0;

    /* copy EFB to XFB */
    gxSetScreen ();
  }

  /* final position */
  if (!out) 
  {
    GUI_DrawMenu(menu);
    gxSetScreen ();
  }
  else if (menu->screenshot)
  {
    gxClearScreen((GXColor)BLACK);
    gxDrawScreenshot(255);
    gxSetScreen ();
  }
}

/* Basic Fading */
void GUI_FadeOut()
{
  int alpha = 0;
  while (alpha < 256)
  {
    gxDrawRectangle(0, 0, 640, 480, alpha, (GXColor)BLACK);
    gxSetScreen();
    alpha +=3;
  }
}

/* Window Prompt  */
/* prompt window slides in & out */
int GUI_WindowPrompt(gui_menu *parent, char *title, char *items[], u8 nb_items)
{
  int i, ret, quit = 0;
  s32 selected = 0;
  s32 old;
  butn_data *data = &button_text_data;
  s16 p;

#ifdef HW_RVL
  int x,y;
  struct orient_t orient;
#endif

  /* initialize buttons data */
  data->texture[0] = gxTextureOpenPNG(data->image[0],0);
  data->texture[1] = gxTextureOpenPNG(data->image[1],0);

  /* initialize texture window */
  gx_texture *window = gxTextureOpenPNG(Frame_s1_png,0);
  gx_texture *top = gxTextureOpenPNG(Frame_s1_title_png,0);

  /* get initial positions */
  int w = data->texture[0]->width;
  int h = data->texture[0]->height;
  int xwindow = (640 - window->width)/2;
  int ywindow = (480 - window->height)/2;
  int xpos = xwindow + (window->width - w)/2;
  int ypos = (window->height - top->height - (h*nb_items) - (nb_items-1)*20)/2;
  ypos = ypos + ywindow + top->height;

  /* set initial vertical offset */
  int yoffset = ywindow + window->height;

  /* reset help comment */
  if (parent->helpers[1]) strcpy(parent->helpers[1]->comment,"");

  /* slide in */
  while (yoffset > 0)
  {
    /* draw parent menu */
    GUI_DrawMenu(parent);

    /* draw window */
    gxDrawTexture(window,xwindow,ywindow-yoffset,window->width,window->height,230);
    gxDrawTexture(top,xwindow,ywindow-yoffset,top->width,top->height,255);

    /* draw title */
    FONT_writeCenter(title,20,xwindow,xwindow+window->width,ywindow+(top->height-20)/2+20-yoffset,(GXColor)WHITE);

    /* draw buttons + text */
    for (i=0; i<nb_items; i++)
    {
      gxDrawTexture(data->texture[0],xpos,ypos+i*(20 + h)-yoffset,w,h,255);
      FONT_writeCenter(items[i],18,xpos,xpos+w,ypos+i*(20 + h)+(h + 18)/2- yoffset,(GXColor)DARK_GREY);
    }

    /* update display */
    gxSetScreen ();

    /* slide speed */
    yoffset -= 60;
  }

  /* draw menu  */
  while (quit == 0)
  {
    /* draw parent menu (should have been initialized first) */
    GUI_DrawMenu(parent);

    /* draw window */
    gxDrawTexture(window,xwindow,ywindow,window->width,window->height,230);
    gxDrawTexture(top,xwindow,ywindow,top->width,top->height,255);

    /* draw title */
    FONT_writeCenter(title,20,xwindow,xwindow+window->width,ywindow+(top->height-20)/2+20,(GXColor)WHITE);

    /* draw buttons + text */
    for (i=0; i<nb_items; i++)
    {
      if (i==selected)
      {
        gxDrawTexture(data->texture[1],xpos-4,ypos+i*(20+h)-4,w+8,h+8,255);
        FONT_writeCenter(items[i],22,xpos,xpos+w,ypos+i*(20+h)+(h+22)/2,(GXColor)DARK_GREY);
      }
      else
      {
        gxDrawTexture(data->texture[0],xpos,ypos+i*(20 + h),w,h,255);
        FONT_writeCenter(items[i],18,xpos,xpos+w,ypos+i*(20+h)+(h+18)/2,(GXColor)DARK_GREY);
      }
    }

    old = selected;
    p = m_input.keys;

#ifdef HW_RVL
    if (Shutdown)
    {
      gxTextureClose(&w_pointer);
      GUI_DeleteMenu(parent);
      GUI_FadeOut();
      shutdown();
      SYS_ResetSystem(SYS_POWEROFF, 0, 0);
    }
    else if (m_input.ir.valid)
    {
      /* get cursor position */
      x = m_input.ir.x;
      y = m_input.ir.y;

      /* draw wiimote pointer */
      WPAD_Orientation(0,&orient);
      gxResetAngle(orient.roll);
      gxDrawTexture(w_pointer,x,y,w_pointer->width,w_pointer->height,255);
      gxResetAngle(0.0);

      /* check for valid buttons */
      selected = -1;
      for (i=0; i<nb_items; i++)
      {
        if ((x>=xpos)&&(x<=(xpos+w))&&(y>=ypos+i*(20 + h))&&(y<=(ypos+i*(20+h)+h)))
        {
          selected = i;
          break;
        }
      }
    }
    else
    {
      /* reinitialize selection */
      if (selected == -1) selected = 0;
    }
#endif

    /* update screen */
    gxSetScreen ();

    /* update selection */
    if (p&PAD_BUTTON_UP)
    {
      if (selected > 0) selected --;
    }
    else if (p&PAD_BUTTON_DOWN)
    {
      if (selected < (nb_items -1)) selected ++;
    }

    /* sound fx */
    if (selected != old)
    {
      if (selected >= 0)
      {
        ASND_SetVoice(ASND_GetFirstUnusedVoice(),VOICE_MONO_16BIT,22050,0,(u8 *)button_over_pcm,button_over_pcm_size,
                      ((int)config.sfx_volume * 255) / 100,((int)config.sfx_volume * 255) / 100,NULL);
      }
    }

    if (p & PAD_BUTTON_A)
    {
      if (selected >= 0)
      {
        quit = 1;
        ret = selected;
      }
    }
    else if (p & PAD_BUTTON_B)
    {
      quit = 1;
      ret = -1;
    }
  }

  /* reset initial vertical offset */
  yoffset = 0;

  /* slide out */
  while (yoffset < (ywindow + window->height))
  {
    /* draw parent menu */
    GUI_DrawMenu(parent);

    /* draw window + header */
    gxDrawTexture(window,xwindow,ywindow-yoffset,window->width,window->height,230);
    gxDrawTexture(top,xwindow,ywindow-yoffset,top->width,top->height,255);

    /* draw title */
    FONT_writeCenter(title,20,xwindow,xwindow+window->width,ywindow+(top->height-20)/2+20-yoffset,(GXColor)WHITE);

    /* draw buttons + text */
    for (i=0; i<nb_items; i++)
    {
      gxDrawTexture(data->texture[0],xpos,ypos+i*(20+h)-yoffset,w,h,255);
      FONT_writeCenter(items[i],18,xpos,xpos+w,ypos+i*(20+h)+(h+18)/2-yoffset,(GXColor)WHITE);
    }

    yoffset += 60;
    gxSetScreen ();
  }

  /* final position */
  GUI_DrawMenu(parent);
  gxSetScreen ();

  /* close textures */
  gxTextureClose(&window);
  gxTextureClose(&data->texture[0]);
  gxTextureClose(&data->texture[1]);

  return ret;
}

int GUI_UpdateMenu(gui_menu *menu)
{
  u16 p;
  int ret = 0;
  int selected = menu->selected;
  int max_items = menu->max_items;
  int max_buttons = menu->max_buttons;
  gui_butn *button;

#ifdef HW_RVL
  int i,x,y;
  struct orient_t orient;
  if (Shutdown)
  {
    GUI_DeleteMenu(menu);
    GUI_FadeOut();
    shutdown();
    SYS_ResetSystem(SYS_POWEROFF, 0, 0);
  }
  else if (m_input.ir.valid)
  {
    /* get cursor position */
    x = m_input.ir.x;
    y = m_input.ir.y;

    /* draw wiimote pointer */
    WPAD_Orientation(0,&orient);
    gxResetAngle(orient.roll);
    gxDrawTexture(w_pointer,x,y,w_pointer->width,w_pointer->height,255);
    gxResetAngle(0.0);

    /* check for valid buttons */
    selected = max_buttons + 2;
    for (i=0; i<max_buttons; i++)
    {
      button = &menu->buttons[i];
      if ((button->state & BUTTON_ACTIVE)&&(x>=button->x)&&(x<=(button->x+button->w))&&(y>=button->y)&&(y<=(button->y+button->h)))
      {
        selected = i;
        break;
      }
    }

    for (i=0; i<2; i++)
    {
      button = menu->arrows[i];
      if (button)
      {
        if (button->state & BUTTON_VISIBLE)
        {
          if ((x<=(button->x+button->w))&&(y>=button->y)&&(y<=(button->y+button->h)))
          {
            selected = max_buttons + i;
            break;
          }
        }
      }
    }
  }
  else
  {
    /* reinitialize selection */
    if (selected >= menu->max_buttons) selected = 0;
  }
#endif

  /* update screen */
  gxSetScreen ();

  /* update menu */
  p = m_input.keys;

  if (selected < max_buttons)
  {
    button = &menu->buttons[selected];
    if (p & PAD_BUTTON_UP)
    {
      selected -= button->shift[0];
      if (selected < 0)
      {
        selected = 0;
        if (menu->offset)
          menu->offset --;
      }
    }
    else if (p & PAD_BUTTON_DOWN)
    {
      selected += button->shift[1];
      if (selected >= max_buttons)
      {
        selected = max_buttons - 1;
        if ((menu->offset + selected < (max_items - 1)))
          menu->offset ++;
      }
    }
    else if (p & PAD_BUTTON_LEFT)
    {
      if (button->state & BUTTON_SHIFT)
      {
        /* menu clicked */
        ret = 2;
      }
      else
      {
        selected -= button->shift[2];
        if (selected < 0)
        {
          selected = 0;
          if (menu->offset)
            menu->offset --;
        }
      }
    }
    else if (p & PAD_BUTTON_RIGHT)
    {
      if (button->state & BUTTON_SHIFT)
      {
        /* menu clicked */
        ret = 1;
      }
      else
      {
        selected += button->shift[3];
        if (selected >= max_buttons)
        {
          selected = max_buttons - 1;
          if ((menu->offset + selected < (max_items - 1)))
            menu->offset ++;
        }
      }
    }
  }

  if (p & PAD_BUTTON_A)
  {
    if (selected < max_buttons) ret = 1; /* menu clicked */
    else if (selected == max_buttons) menu->offset --; /* up arrow */
    else if (selected == (max_buttons+1))menu->offset ++; /* down arrow */
  }
  else if ((p & PAD_BUTTON_B) || (p & PAD_TRIGGER_Z))
  {
    /* quit menu */
    ret = -1;
  }

  /* selected item has changed ? */
  if (menu->selected != selected)
  {
    if (selected < max_buttons)
    {
      /* sound fx */
      button = &menu->buttons[selected];
      if (button->state & BUTTON_OVER_SFX)
      {
        ASND_SetVoice(ASND_GetFirstUnusedVoice(),VOICE_MONO_16BIT,22050,0,(u8 *)button_over_pcm,button_over_pcm_size,
                      ((int)config.sfx_volume * 255) / 100,((int)config.sfx_volume * 255) / 100,NULL);
      }
    }
    else if (selected < (max_buttons + 2))
    {
      /* sound fx */
      button = menu->arrows[selected-max_buttons];
      if (button->state & BUTTON_OVER_SFX)
      {
        ASND_SetVoice(ASND_GetFirstUnusedVoice(),VOICE_MONO_16BIT,22050,0,(u8 *)button_over_pcm,button_over_pcm_size,
                      ((int)config.sfx_volume * 255) / 100,((int)config.sfx_volume * 255) / 100,NULL);
      }
    }

    /* update selection */
    menu->selected = selected;
  }

  /* update arrows buttons status (items list) */
  button = menu->arrows[0];
  if (button)
  {
    if (menu->offset > 0) button->state |= BUTTON_VISIBLE;
    else button->state &= ~BUTTON_VISIBLE;
  }
  button = menu->arrows[1];
  if (button)
  {
    if ((menu->offset + max_buttons) < max_items) button->state |= BUTTON_VISIBLE;
    else button->state &= ~BUTTON_VISIBLE;
  }

  if (ret > 0)
  {
    if (selected < max_buttons)
    {
      /* sound fx */
      button = &menu->buttons[selected];
      if (button->state & BUTTON_SELECT_SFX)
      {
        ASND_SetVoice(ASND_GetFirstUnusedVoice(),VOICE_MONO_16BIT,22050,0,(u8 *)button_select_pcm,button_select_pcm_size,
                      ((int)config.sfx_volume * 255) / 100,((int)config.sfx_volume * 255) / 100,NULL);
      }
    }
  }

  return ret;
}

int GUI_RunMenu(gui_menu *menu)
{
  int update = 0;

  /* update menu */
  while (!update)
  {
    GUI_DrawMenu(menu);
    update = GUI_UpdateMenu(menu);
  }

  if (update == 2) return (-2-menu->offset-menu->selected);
  else if (update == 1) return (menu->offset + menu->selected);
  else return -1;
 }

/* basic slide effect for option menus */
void GUI_SlideMenuTitle(gui_menu *m, int title_offset)
{
#ifdef HW_RVL
  gui_butn *button;
  int i,x,y;
  struct orient_t orient;
#endif

  char title[64];
  strcpy(title,m->title);

  while (title_offset > 0)
  {
    strcpy(m->title,title+title_offset);
    m->title[strlen(title)-title_offset-1] = 0;
    GUI_DrawMenu(m);
#ifdef HW_RVL
    if (m_input.ir.valid)
    {
      /* get cursor position */
      x = m_input.ir.x;
      y = m_input.ir.y;

      /* draw wiimote pointer */
      WPAD_Orientation(0,&orient);
      gxResetAngle(orient.roll);
      gxDrawTexture(w_pointer,x,y,w_pointer->width,w_pointer->height,255);
      gxResetAngle(0.0);

      /* check for valid buttons */
      m->selected = m->max_buttons + 2;
      for (i=0; i<m->max_buttons; i++)
      {
        button = &m->buttons[i];
        if ((button->state & BUTTON_ACTIVE)&&(x>=button->x)&&(x<=(button->x+button->w))&&(y>=button->y)&&(y<=(button->y+button->h)))
        {
          m->selected = i;
          break;
        }
      }

      for (i=0; i<2; i++)
      {
        button = m->arrows[i];
        if (button)
        {
          if (button->state & BUTTON_VISIBLE)
          {
            if ((x<=(button->x+button->w))&&(y>=button->y)&&(y<=(button->y+button->h)))
            {
              m->selected = m->max_buttons + i;
              break;
            }
          }
        }
      }
    }
    else
    {
      /* reinitialize selection */
      if (m->selected >= m->max_buttons) m->selected = 0;
    }
#endif
    gxSetScreen ();
    usleep(6000);
    title_offset--;
  }
  strcpy(m->title,title);
}