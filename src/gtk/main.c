#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtk.h>
#include <math.h>
#include "app_ds.h"
#include "savepng.h"
#include "../sdl/filter/filtre.h"
#include "SDL/SDL_image.h"
#include "SDL/SDL_rotozoom.h"
    

#define CHECK(pointer) \
    if(pointer == NULL) \
    exit(1);

struct {
    cairo_surface_t *image;  
} glob;


gchar filename1;
const char* filename;
GtkRange *range;
gdouble size1 =1;
gdouble size2 = 1;
GdkPixbuf *surface_pixbuf;
GdkPixbuf *redo;
struct Stack *undo;
cairo_surface_t *surface;
cairo_t *context;
double start_click = 0;
GtkWidget* window;
GtkWidget* drawarea;

int res_polygon =0;

void savefile(GtkButton *button, gpointer user_data)
{   
    gdk_pixbuf_save (surface_pixbuf, "snapshot.png", "png", NULL, NULL);     
}

void on_save(GtkButton *button, gpointer user_data)
{
    button=button;
    GtkWidget* dialog;
    GtkWidget* toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
    dialog = gtk_file_chooser_dialog_new("Save Text ",
            GTK_WINDOW(toplevel),
            GTK_FILE_CHOOSER_ACTION_SAVE,
            "Cancel", GTK_RESPONSE_CANCEL,
            "Save", GTK_RESPONSE_ACCEPT,
            NULL);
    switch (gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        case GTK_RESPONSE_ACCEPT:
            {
                gchar* filename;
                filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

                gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
                break;
            }
        default:
            break;
     }
    gtk_widget_destroy(dialog);

}

gboolean ctrl_z(GtkWidget* widget)
{
    /*
       This event handler is for ctrl+w, ctrl+y, ctrl+z operatins on the window.
       It is defined over here as it affects the sheet.
       */
    if(!is_empty_stack(undo))
    {
        redo = gdk_pixbuf_copy(surface_pixbuf);
        surface_pixbuf = gdk_pixbuf_copy(pop_stack(&undo));
        surface = gdk_cairo_surface_create_from_pixbuf (surface_pixbuf, 1, NULL);
        context = cairo_create(surface);
        cairo_set_source_surface(context, surface, 0, 0);
    }

    return TRUE;	    
}

gboolean ctrl_y(GtkWidget* widget)
{
    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    surface_pixbuf = gdk_pixbuf_copy(redo);
    surface = gdk_cairo_surface_create_from_pixbuf (surface_pixbuf, 1, NULL);
    return TRUE;
}

void openfile(GtkButton *button, gpointer user_data)
{

    button = button;
    g_print("Loading...\n");

    GtkWidget* toplevel = gtk_widget_get_toplevel(GTK_WIDGET(button));
    GtkFileFilter *filter = gtk_file_filter_new ();
    GtkWidget* dialog = gtk_file_chooser_dialog_new(("Open image"),
            GTK_WINDOW(toplevel),
            GTK_FILE_CHOOSER_ACTION_OPEN,
            "Open", GTK_RESPONSE_ACCEPT,
            "Cancel", GTK_RESPONSE_CANCEL,
            NULL);

    gtk_file_filter_add_pixbuf_formats (filter);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog),filter);

    switch (gtk_dialog_run(GTK_DIALOG(dialog)))
    {
        case GTK_RESPONSE_ACCEPT:
            {
                filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
                while(!is_empty_stack(undo))
                {
                    pop_stack(&undo);
                }

                undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
                glob.image = cairo_image_surface_create_from_png(filename);
                surface = glob.image;

                surface_pixbuf = gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
                gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
                //gtk_window_set_default_size(GTK_WINDOW(window), cairo_image_surface_get_width (surface), cairo_image_surface_get_height (surface));

                break;
            }
        default:
            break;
    }
    gtk_widget_destroy(dialog);
}

double red = 0;
double green = 0;
double blue = 0;





GtkWidget *color1;
GtkButton *status;
static struct Color pixel;
static guchar *current;
struct Queue *front;
int drawareaX;
int drawareaY;
struct Color foreground;
static gint rowstride;
static gint n_channels;
static guchar *pixels;

void floodFill(gint x, gint y, struct Color *color, gint p_size);

void draw_pixel(gint x, gint y, struct Color *color, gint p_size, cairo_t *context){
  /*
    Changes a Pixels color data int the Pixel buffer.
  */
  if(x >= 0 && x < drawareaX && y >= 0 && y < drawareaY){
    if(p_size == 0){
      cairo_rectangle(context, x, y, 1.5, 1.5);
      cairo_fill(context);
      
      current = pixels + y * rowstride + x * n_channels;
      current[0] = color->red;
      current[1] = color->green;
      current[2] = color->blue;
      
    }
    else{
        
      floodFill(x, y, color, p_size);
    }
  }
}

struct Color *getPixel(gint x, gint y){
  /*
    Returns a Pixel's color data from the Pixel buffer.
  */
  current = pixels + y * rowstride + x * n_channels;
  pixel.red = current[0];
  pixel.green = current[1];
  pixel.blue = current[2];
  return &pixel;
}

gboolean check_pixel(gint x, gint y, struct Color *fgcolor, struct Color *bgcolor, gint p_size, gint ogx, gint ogy){
  if(x >= 0 && x < drawareaX && y >= 0 && y < drawareaY)
    {
      current = pixels + y * rowstride + x * n_channels;
      if(fgcolor->red == current[0] && fgcolor->green == current[1] && fgcolor->blue == current[2])
	{
	  return FALSE;
	}
      if(p_size == 0)
	{
	  if(bgcolor->red == current[0] && bgcolor->green == current[1] && bgcolor->blue == current[2])
	    {
	      return TRUE;
	    }
	}
      else
	{
	  if(abs(x - ogx) <= p_size && abs(y-ogy) <= p_size)
	    return TRUE;
	  return FALSE;
	}
    }
  return FALSE;
}

void floodFill(gint x, gint y, struct Color *color, gint p_size){
  struct Point *current_point = NULL;
  gint west, east, valid_north, valid_south;
  struct Color *base_color = getPixel(x,y);
  if(!check_pixel(x, y ,color, base_color, p_size, x, y)){
    if(p_size > 0){
      return;
    }
  }
  front = push_queue(x,y, NULL);

  cairo_t *context = cairo_create(surface);
  cairo_set_source_rgb(context, red, green, blue);
      
  while(!is_queue_empty())
    {
      valid_north = 0;
      valid_south = 0;
      current_point = pop_queue();
      for(west = current_point->x; check_pixel(west, current_point->y, color, base_color, p_size, x, y); west--);
      west--;
      for(east = current_point->x; check_pixel(east, current_point->y, color, base_color, p_size, x, y); east++);
      east++;
      
      for(west = west + 1 ; west < east ; west++)
	{
	  draw_pixel(west, current_point->y, color, 0, context);
	  if(check_pixel(west, current_point->y - 1, color, base_color, p_size, x, y))
	    {
	      if(valid_north == 0)
		{
		  front = push_queue(west, current_point->y - 1, front);
		  valid_north = 1;
		}
	    }
	  else
	    {
	      valid_north = 0;
	    }
	  if(check_pixel(west, current_point->y + 1, color, base_color, p_size, x, y))
	    {
	      if(valid_south == 0)
		{
		  front = push_queue(west, current_point->y + 1, front);
		  valid_south = 1;
		}
	    }
	  else
	    valid_south = 0;
	}
    }

  cairo_destroy(context);
  free(current_point);
  free(front);
}





//static void do_drawing(cairo_t *cr);

/*gboolean on_draw(GtkWidget *widget, cairo_t* context ,gpointer user_data)
{   
  //surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,gtk_widget_get_allocated_width(widget),gtk_widget_get_allocated_height(widget));

  if (filename == NULL)
    {
      cairo_set_source_rgba(context, 0.5, 0.5, 0.1,1);
      cairo_set_source_surface(context, surface, 0, 0);
      //do_drawing(context);
      cairo_paint(context);
     
      return TRUE;
    }
  else
    {
      cairo_set_source_surface(context, glob.image, 0, 0);
      cairo_paint(context);
      printf("%i\n",cairo_image_surface_get_width (glob.image));
      printf("%i\n",cairo_image_surface_get_height(glob.image));
      if (cairo_image_surface_get_width (glob.image)!= 0 && cairo_image_surface_get_height(glob.image)!=0)
	surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (glob.image),cairo_image_surface_get_height(glob.image));
      else
	surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,gtk_widget_get_allocated_width(widget),gtk_widget_get_allocated_height(widget));
      return TRUE;
    }
    }*/

gboolean on_draw(GtkWidget *widget, cairo_t* context ,gpointer user_data)
{   
    //surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,gtk_widget_get_allocated_width(widget),gtk_widget_get_allocated_height(widget));

    if (filename == NULL)
    {
        cairo_set_source_rgba(context, 0.5, 0.5, 0.1,1);
        cairo_set_source_surface(context, surface, 0, 0);
        //do_drawing(context);
        cairo_paint(context);
        if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
            surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
        else
            surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,gtk_widget_get_allocated_width(widget),gtk_widget_get_allocated_height(widget));
        return TRUE;
    }
    else
    {
        cairo_set_source_surface(context, glob.image, 0, 0);
        cairo_paint(context);
        if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
            surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
        else
            surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,gtk_widget_get_allocated_width(widget),gtk_widget_get_allocated_height(widget));
        return TRUE;
    }

}

size_t highlighted = 0;
size_t pipetted = 0;
size_t rectangled = 0;
size_t triangled = 0;
size_t losanged = 0;
size_t circled = 0;
size_t bucketed = 0;
size_t erased = 0;
size_t stared = 0;
size_t polygoned = 0;

void tool_reset()
{
    polygoned = 0;
    rectangled = 0;
    triangled = 0;
    bucketed = 0;
    erased = 0;
    pipetted = 0;
    circled = 0;
    losanged = 0;
    stared = 0;
    highlighted = 0;
}

void return_draw()
{
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/pencil.png");
    gtk_button_set_image(status, imageStatus);
    tool_reset();
    filename = NULL;
}

void website_button()
{
    printf("Opening website...\n");
    int a = system("xdg-open https://akaagi.github.io/Pinte_Website/accueil.html &");
    if (a)
    {}
}

/*static gboolean on_draw(GtkWidget *da, GdkEvent *event, cairo_t* cr, gpointer data)
  {
  (void)event; (void)data;
  GdkPixbuf *pix;
  GError *err = NULL;
  pix = gdk_pixbuf_new_from_file("esfse", &err);
  if(err)
  {
  printf("Error : %s\n", err->message);
  g_error_free(err);
  return FALSE;
  }
//    cr = gdk_cairo_create (da->window);
gdk_cairo_set_source_pixbuf(cr, pix, 0, 0);
cairo_paint(cr);
//    cairo_fill (cr);
cairo_destroy (cr);
return FALSE;
}*/

/*static void do_drawing(cairo_t *cr)
  {
  cairo_set_source_surface(cr, glob.image, 10, 10);
  cairo_paint(cr);    
  }*/


double mouseX;
double mouseY;
double previousX, previousY;
int acc = 0;
GdkRGBA couleur;


void on_color1_color_set(GtkColorButton *cb)
{
    gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(cb), &couleur);
    red = couleur.red;
    green = couleur.green;
    blue = couleur.blue;
}

void erase_white()
{
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/eraser.png");
    gtk_button_set_image(status, imageStatus);
    tool_reset();
    erased = 1;
}

void value_changed(GtkWidget *scale, gpointer user_data) {

    size1 = gtk_range_get_value(GTK_RANGE(scale));

}

void value_changed2(GtkWidget *scale, gpointer user_data)
{
    size2 = gtk_range_get_value(GTK_RANGE(scale));
}

void flood_fill()
{
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/fill.png");
    gtk_button_set_image(status, imageStatus);
    tool_reset();
    bucketed = 1;

}

void get_rect()
{
    tool_reset();
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/rect.png");
    gtk_button_set_image(status, imageStatus);
    rectangled = 1;
}

void get_triangle()
{
    tool_reset();
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/triangle.png");
    gtk_button_set_image(status, imageStatus);
    triangled = 1;
}

void get_losange()
{
    tool_reset();
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/losange.png");
    gtk_button_set_image(status, imageStatus);
    losanged = 1;
}

void get_circle()
{   
    tool_reset();
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/circle.png");
    gtk_button_set_image(status, imageStatus);
    circled = 1;
}

void get_pipette()
{
    tool_reset();
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/pipette.png");
    gtk_button_set_image(status, imageStatus);
    pipetted = 1;
}

void get_polygon()
{  
    tool_reset();
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/polygon.png");
    gtk_button_set_image(status, imageStatus);
    polygoned = 1;
}

void get_star()
{  
    tool_reset();
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/star.png");
    gtk_button_set_image(status, imageStatus);

    stared = 1;
}

void get_highlight()
{
    tool_reset();
    highlighted = 1;
    GtkWidget *imageStatus = gtk_image_new_from_file("icons/surligneur.png");
    gtk_button_set_image(status, imageStatus);
}


void draw_triangle(int size, GdkEventButton *event)
{
    GdkEventMotion * e = (GdkEventMotion *) event;
    int mousex= e->x;
    int mousey = e->y;
    cairo_t *cr = cairo_create(surface);
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_set_line_width(cr, size1);
    int a_x = mousex;
    int a_y = mousey;
    int b_x = mousex + size;
    int b_y = mousey;
    int c_x = mousex + size/2;
    int c_y = mousey - sqrt((size * size) - (size*size)/4);


    cairo_move_to(cr, a_x, a_y);
    cairo_line_to(cr, b_x, b_y);
    cairo_stroke(cr);

    cairo_move_to(cr, a_x, a_y);
    cairo_line_to(cr, c_x, c_y);
    cairo_stroke(cr);

    cairo_move_to(cr, b_x, b_y);
    cairo_line_to(cr, c_x, c_y);
    cairo_stroke(cr);
}

void draw_losange(int size, GdkEventButton *event)
{
    GdkEventMotion * e = (GdkEventMotion *) event;
    int mousex= e->x;
    int mousey = e->y;
    cairo_t *cr = cairo_create(surface);
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_set_line_width(cr, size1);
    int a_x = mousex;
    int a_y = mousey;
    int b_x = mousex + size;
    int b_y = mousey;
    int c_x = mousex + size/2;
    int c_y = mousey - size/4;
    int d_x = mousex + size/2;
    int d_y = mousey + size/4;


    cairo_move_to(cr, a_x, a_y);
    cairo_line_to(cr, c_x, c_y);
    cairo_stroke(cr);

    cairo_move_to(cr, a_x, a_y);
    cairo_line_to(cr, d_x, d_y);
    cairo_stroke(cr);

    cairo_move_to(cr, c_x, c_y);
    cairo_line_to(cr, b_x, b_y);
    cairo_stroke(cr);

    cairo_move_to(cr, d_x, d_y);
    cairo_line_to(cr, b_x, b_y);
    cairo_stroke(cr);
}


void draw_rectangle(int size, GdkEventButton *event)
{
    GdkEventMotion * e = (GdkEventMotion *) event;
    int mousex= e->x;
    int mousey = e->y;
    cairo_t *cr = cairo_create(surface);
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_set_line_width(cr, size1);
    int left_x = mousex;
    int right_x = mousex + size;
    int top_y = mousey;
    int bot_y = mousey + size;
    cairo_move_to(cr, left_x, bot_y);
    cairo_line_to(cr, right_x, bot_y);
    cairo_stroke(cr);

    cairo_move_to(cr, left_x, top_y);
    cairo_line_to(cr, right_x, top_y);
    cairo_stroke(cr);

    cairo_move_to(cr, left_x, bot_y+2);
    cairo_line_to(cr, left_x, top_y-2);
    cairo_stroke(cr);

    cairo_move_to(cr, right_x, bot_y+2);
    cairo_line_to(cr, right_x, top_y-2);
    cairo_stroke(cr);
}

void draw_circle(int size, GdkEventButton *event)
{
    GdkEventMotion * e = (GdkEventMotion *) event;
    cairo_t *cr = cairo_create(surface);
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_set_line_width(cr, size1);  
    cairo_translate(cr, size1/2, size1/2);
    cairo_arc(cr, e->x, e->y, size2*40, 0, 2 * M_PI);
    cairo_stroke_preserve(cr);
}

void draw_star(int size, GdkEventButton *event)
{
    GdkEventMotion * e = (GdkEventMotion *) event;
    int x= e->x;
    int y = e->y;
    cairo_t *cr = cairo_create(surface);
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_set_line_width(cr, size1);
    int extex[5];
    int extey[5];
    int intex[5];
    int intey[5];
    for (int i = 0; i < 5; i++)
    {
        intex[i] = size / 3 * cos(2* M_PI * i / 5 + M_PI/2) + x;
        intey[i] = size / 3 * sin(2 * M_PI * i / 5 + M_PI/2) + y;
        extex[i] = size * cos(2*M_PI * i / 5 + 2 * M_PI/10 + M_PI/2) +x;
        extey[i] = size * sin(2*M_PI * i / 5 + 2 * M_PI/10 + M_PI/2) + y;
    }
    for (int i = 1; i < 5; i++)
    {
        cairo_move_to(cr, extex[i], extey[i]);
        cairo_line_to(cr, intex[i], intey[i]);
        cairo_stroke(cr);
        cairo_move_to(cr, extex[i - 1], extey[i -1]);
        cairo_line_to(cr, intex[i], intey[i]);
        cairo_stroke(cr);
    }
    cairo_move_to(cr, extex[0], extey[0]);
    cairo_line_to(cr, intex[0], intey[0]);
    cairo_stroke(cr);
    cairo_move_to(cr, extex[4], extey[4]);
    cairo_line_to(cr, intex[0], intey[0]);
    cairo_stroke(cr);
}

void draw_polygon(int size, int n, GdkEventButton *event)
{
    GdkEventMotion * e = (GdkEventMotion *) event;
    int x= e->x;
    int y = e->y;
    cairo_t *cr = cairo_create(surface);
    cairo_set_source_rgb(cr, red, green, blue);
    cairo_set_line_width(cr, size1);
    int intex[n];
    int intey[n];
    for (int i = 0; i < n; i++)
    {
        intex[i] = size * cos(2* M_PI * i / n + M_PI/2) + x;
        intey[i] = size * sin(2 * M_PI * i / n + M_PI/2) + y;
    }
    for (int i = 0; i < n - 1; i++)
    {
        cairo_move_to(cr, intex[i], intey[i]);
        cairo_line_to(cr, intex[i + 1], intey[i + 1]);
        cairo_stroke(cr);

    }
    cairo_move_to(cr, intex[n - 1], intey[n - 1]);
    cairo_line_to(cr, intex[0], intey[0]);
    cairo_stroke(cr);
}

gboolean on_click(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    //gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    if (highlighted == 1)
    {
        cairo_t *context = cairo_create(surface);

        if(GDK_BUTTON_PRESS)
        {
            if (start_click == 1)
            {
                undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
                start_click = 0;
            }
            //printf("test\n");
            cairo_set_line_width(context, size1);


            if(acc != 0)
            {
                previousX = mouseX;
                previousY = mouseY;
                if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
                    surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
                //surface_pixbuf = gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
                //gdk_pixbuf_save (surface_pixbuf, "snapshot.png", "png", NULL, NULL);

            }
            GdkEventMotion * e = (GdkEventMotion *) event;
            if (acc == 0)
            {
                cairo_set_source_rgba(context, red, green, blue, 0.5);
                previousX = e->x;
                previousY = e->y;
                //cairo_translate(context, size1/2, size1/2);
                //cairo_arc(context, previousX, previousY, size1, 0, 2*M_PI);
                cairo_rectangle(context, previousX, previousY, size1/40, size1/40);
                //cairo_fill_preserve(context);

            }

            mouseX= e->x;
            mouseY = e->y;


            cairo_set_source_rgba(context, red, green, blue, 0.5);
            cairo_move_to(context, previousX, previousY);
            cairo_line_to(context, mouseX, mouseY);
            cairo_stroke(context);
            acc = 1;


            cairo_destroy(context);

            gtk_widget_queue_draw_area(widget, 0, 0,
                    gtk_widget_get_allocated_width(widget),
                    gtk_widget_get_allocated_height(widget));

            gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
            return TRUE;
        }
    }
    else if (pipetted == 1)
    {	
        rowstride = gdk_pixbuf_get_rowstride(surface_pixbuf);
        n_channels = gdk_pixbuf_get_n_channels(surface_pixbuf);
        pixels = gdk_pixbuf_get_pixels(surface_pixbuf);


        GdkEventMotion * e = (GdkEventMotion *) event;
        struct Color *cur = getPixel(e->x,e->y);
        red = (double)cur->red/255;
        green = (double)cur->green/255;
        blue = (double)cur->blue/255;

        couleur.red = red;
        couleur.blue = blue;
        couleur.green= green;
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(color1), &couleur);

        //g_print("%f %f %f\n\n", red, green, blue);
    }

    else if (rectangled == 1)
    {
        undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
        draw_rectangle(100 * size2, event);
    }
    else if (polygoned == 1)
    {
        undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
        draw_polygon(100 * size2,res_polygon, event);
    }
    else if (stared == 1)
    {
        undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
        draw_star(100 * size2, event);
    }
    else if (triangled == 1)
    {
        undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
        draw_triangle(100 * size2, event);
    }
    else if (losanged == 1)
    {
        undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
        draw_losange(100 * size2, event);
    }
    else if (circled == 1)
    {
        undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
        draw_circle(100 * size2, event);
    }
    else if (bucketed == 1)
    {
        undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);

        GdkEventMotion * e = (GdkEventMotion *) event;
        mouseX= e->x;
        mouseY = e->y;

        rowstride = gdk_pixbuf_get_rowstride(surface_pixbuf);
        n_channels = gdk_pixbuf_get_n_channels(surface_pixbuf);
        pixels = gdk_pixbuf_get_pixels(surface_pixbuf);

        drawareaX = gtk_widget_get_allocated_width(drawarea);
        drawareaY = gtk_widget_get_allocated_height(drawarea);
        foreground.red = red*255;
        foreground.green = green*255;
        foreground.blue = blue*255;
        floodFill(mouseX, mouseY, &foreground, 0);
    }
    else if (erased == 0) 
    {
        cairo_t *context = cairo_create(surface);

        if(GDK_BUTTON_PRESS)
        {
            if (start_click == 1)
            {
                undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
                start_click = 0;
            }
            //printf("test\n");
            cairo_set_line_width(context, size1);


            if(acc != 0)
            {
                previousX = mouseX;
                previousY = mouseY;
                if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
                    surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
                //surface_pixbuf = gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
                //gdk_pixbuf_save (surface_pixbuf, "snapshot.png", "png", NULL, NULL);

            }
            GdkEventMotion * e = (GdkEventMotion *) event;
            if (acc == 0)
            {
                cairo_set_source_rgb(context, red, green, blue);
                previousX = e->x;
                previousY = e->y;
                //cairo_translate(context, size1/2, size1/2);
                //cairo_arc(context, previousX, previousY, size1, 0, 2*M_PI);
                cairo_rectangle(context, previousX, previousY, size1/40, size1/40);
                //cairo_fill_preserve(context);

            }

            mouseX= e->x;
            mouseY = e->y;


            cairo_set_source_rgb(context, red, green, blue);
            cairo_move_to(context, previousX, previousY);
            cairo_line_to(context, mouseX, mouseY);
            cairo_stroke(context);
            acc = 1;


            cairo_destroy(context);

            gtk_widget_queue_draw_area(widget, 0, 0,
                    gtk_widget_get_allocated_width(widget),
                    gtk_widget_get_allocated_height(widget));

            gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
            return TRUE;
        }
    }
    else
    {
        if(GDK_BUTTON_PRESS)
        {
            if (start_click == 1)
            {
                undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
                start_click = 0;
            }

            //printf("test\n");
            cairo_t *context = cairo_create(surface);
            cairo_set_line_width(context, 10);


            if(acc != 0)
            {
                previousX = mouseX;
                previousY = mouseY;
                if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
                    surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
                //surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
            }
            GdkEventMotion * e = (GdkEventMotion *) event;
            if (acc == 0)
            {
                cairo_set_source_rgb(context, 1, 1, 1);
                previousX = e->x;
                previousY = e->y;
                //cairo_translate(context, size1/2, size1/2);
                //cairo_arc(context, previousX, previousY, size1, 0, 2*M_PI);
                cairo_rectangle(context, previousX, previousY, 10, 10);
                //cairo_fill_preserve(context);

            }
            mouseX= e->x;
            mouseY = e->y;


            cairo_set_source_rgb(context, 1, 1, 1);
            cairo_move_to(context, previousX, previousY);
            cairo_line_to(context, mouseX, mouseY);
            cairo_stroke(context);
            acc = 1;


            cairo_destroy(context);

            gtk_widget_queue_draw_area(widget, 0, 0,
                    gtk_widget_get_allocated_width(widget),
                    gtk_widget_get_allocated_height(widget));

            gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
            return TRUE;
        }
    }
    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return FALSE;
}

gboolean on_click_release(GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
    acc = 0;
    start_click = 1;
    return TRUE;
}



gboolean refresh(GtkWidget *widget, GdkEventExpose*event, gpointer user_data)
{
    gtk_widget_queue_draw_area(widget, 0, 0,
            gtk_widget_get_allocated_width(widget),
            gtk_widget_get_allocated_height(widget));
    return TRUE;
}


gboolean loadblank(GtkWidget* widget)
{
    while(!is_empty_stack(undo))
    {
        pop_stack(&undo);
    }

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png("blank");
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));

    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean loadgrid(GtkWidget* widget)
{
    while(!is_empty_stack(undo))
    {
        pop_stack(&undo);
    }

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png("grid");
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));

    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}


gboolean grey(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image(filename);

    __grayscale(image);

    SDL_Surface *bmp = image;
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean invert(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image(filename);

    __negative(image);

    SDL_Surface *bmp = image;
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean cyann(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image(filename);

    __cyan(image);

    SDL_Surface *bmp = image;
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean rosee(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image(filename);

    __rose(image);

    SDL_Surface *bmp = image;
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}
gboolean yelloww(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image(filename);

    __yellow(image);

    SDL_Surface *bmp = image;
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean BAW(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image(filename);

    __blackandwhite(image);

    SDL_Surface *bmp = image;
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean mirrorr(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image(filename);

    __mirror(image);

    SDL_Surface *bmp = image;
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean rotaright(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    //gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image("saveauto");

    image = __rotationr(image);

    SDL_SaveBMP(image, ".imgbmp");

    SDL_Surface *bmp = load_image(".imgbmp");
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));

    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean rotaleft(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    //gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);
    init_sdl();
    SDL_Surface *image = load_image("saveauto");

    image = __rotationl(image);

    SDL_SaveBMP(image, ".imgbmp");

    SDL_Surface *bmp = load_image(".imgbmp");
    SDL_SavePNG(bmp, filename);
    SDL_FreeSurface(image);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png(filename);
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));

    gdk_pixbuf_save (surface_pixbuf, "saveauto", "png", NULL, NULL);
    return TRUE;
}

gboolean blank(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png("./images/blanksheet.png");
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    return TRUE;
}

gboolean quad(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png("./images/quadrillage.png");
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    return TRUE;
}

gboolean part(GtkWidget *widget)
{
    char* filename = ".temp_filter";
    gdk_pixbuf_save (surface_pixbuf, filename, "png", NULL, NULL);

    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    glob.image = cairo_image_surface_create_from_png("./images/partition.png");
    surface = glob.image;
    if (cairo_image_surface_get_width (surface)!= 0 && cairo_image_surface_get_height(surface)!=0)
        surface_pixbuf =  gdk_pixbuf_get_from_surface(surface,0,0,cairo_image_surface_get_width (surface),cairo_image_surface_get_height(surface));
    return TRUE;
}

GtkEntry *spin1;

gboolean value_changed3(GtkWidget* widget)
{
    res_polygon = 0;
    int acc=1;
    const gchar* s= gtk_entry_get_text(spin1);
    size_t len = strlen(s);
    size_t i=0;
    if (len==3)
    {
        i = 1;
    }
    else
    {
        i=0;
    }
    for (;len>i;len--)
    {
        if (s[len - 1] >='0' && s[len - 1] <= '9')
        {
            res_polygon += acc*(s[len - 1] - '0');
            acc *=10;
        }
        else
        {
            return 0;
        }
    }

    if (s[0]=='-')
    {
        return 0;;
    }

    if ( (s[0]!= '-') && (s[0] <'0' || s[0] > '9'))
    {
        return 0;
    }

    return 0;
}


void create_window(GtkApplication *app, gpointer data)
{

    GtkWidget *window;
    //GtkWidget *drawarea;
    GtkWidget *load;
    GtkButton *pen;
    GtkButton *erase;
    GtkWidget *web;
    GtkWidget *save;
    GtkWidget *hscale;
    GtkWidget *hscale2;
    GtkPaned *grid;
    GtkPaned *grid2;
    GtkButton *retour;
    GtkButton *annul;
    GtkButton *bucket;
    GtkButton *rect;
    GtkButton *triangle;
    GtkButton *losange;
    GtkButton *circle;
    GtkButton *star;
    GtkButton *polygon;
    GtkButton *pipette;
    GtkButton *surligneur;
    GtkWidget *new;
    GtkWidget *newgrid;
    GtkWidget *menu;
    //GtkEntry *spin1;
    GtkButton *enter;
    //FILTERS
    GtkWidget *filter1;
    GtkWidget *filter2;
    GtkWidget *filter3;
    GtkWidget *filter4;
    GtkWidget *filter5;
    GtkWidget *filter6;
    GtkWidget *filter7;

    GtkButton *quadrillage;
    GtkButton *blanksheet;
    GtkButton *partition;

    GtkButton *leftrot;
    GtkButton *rightrot;

    GtkAdjustment* adjustement = gtk_adjustment_new(1.0,1.0,10.0,1.0,1.0, 0.0);
    GtkAdjustment* adjustement2 = gtk_adjustment_new(1.0,1.0,10.0,1.0,1.0, 0.0);

    GtkBuilder *builder = gtk_builder_new_from_file("pinte.glade");
    CHECK(builder)
        window = GTK_WIDGET(gtk_builder_get_object(builder, "window"));
    CHECK(window)
        triangle = GTK_BUTTON(gtk_builder_get_object(builder, "triangle"));
    CHECK(triangle)
        star = GTK_BUTTON(gtk_builder_get_object(builder, "star"));
    CHECK(star)

        polygon = GTK_BUTTON(gtk_builder_get_object(builder, "polygon"));
    CHECK(polygon)
        spin1=GTK_ENTRY(gtk_builder_get_object(builder,"spin1"));
    CHECK(spin1)
        enter=GTK_BUTTON(gtk_builder_get_object(builder,"enter"));
    CHECK(enter)
        drawarea = GTK_WIDGET(gtk_builder_get_object(builder, "drawarea"));
    CHECK(drawarea)
        color1 = GTK_WIDGET(gtk_builder_get_object(builder, "color1"));
    CHECK(color1)
        pen = GTK_BUTTON(gtk_builder_get_object(builder, "pen"));
    CHECK(pen)
        newgrid = GTK_WIDGET(gtk_builder_get_object(builder, "newgrid"));
    CHECK(pen)

        load = GTK_WIDGET(gtk_builder_get_object(builder, "load"));
    CHECK(load)
        erase = GTK_BUTTON(gtk_builder_get_object(builder, "erase"));
    CHECK(erase)
        leftrot = GTK_BUTTON(gtk_builder_get_object(builder, "leftrot"));
    CHECK(leftrot)
        rightrot = GTK_BUTTON(gtk_builder_get_object(builder, "rightrot"));
    CHECK(rightrot)

        web = GTK_WIDGET(gtk_builder_get_object(builder, "web"));
    CHECK(web)
        menu = GTK_WIDGET(gtk_builder_get_object(builder, "menu"));
    CHECK(menu)
        status = GTK_BUTTON(gtk_builder_get_object(builder, "status"));
    CHECK(status)


        grid = GTK_PANED(gtk_builder_get_object(builder, "grid"));
    CHECK(grid)
        grid2 = GTK_PANED(gtk_builder_get_object(builder, "grid2"));
    CHECK(grid2)

        save = GTK_WIDGET(gtk_builder_get_object(builder, "save"));
    CHECK(save)
        retour = GTK_BUTTON(gtk_builder_get_object(builder, "return"));
    CHECK(retour)
        annul = GTK_BUTTON(gtk_builder_get_object(builder, "cancel"));
    CHECK(annul)
        bucket = GTK_BUTTON(gtk_builder_get_object(builder, "bucket"));
    CHECK(bucket)
        rect =  GTK_BUTTON(gtk_builder_get_object(builder, "rectangle"));
    CHECK(rect)
        losange =  GTK_BUTTON(gtk_builder_get_object(builder, "losange"));
    CHECK(losange)
        circle =  GTK_BUTTON(gtk_builder_get_object(builder, "circle"));
    CHECK(circle)
        pipette =  GTK_BUTTON(gtk_builder_get_object(builder, "pipette"));
    CHECK(pipette)
        surligneur =  GTK_BUTTON(gtk_builder_get_object(builder, "surligneur"));
    CHECK(surligneur)

        new = GTK_WIDGET(gtk_builder_get_object(builder, "new"));
    CHECK(new)
        //FILTERS
        filter1 = GTK_WIDGET(gtk_builder_get_object(builder, "filter1"));
    CHECK(filter1)
        filter2 = GTK_WIDGET(gtk_builder_get_object(builder, "filter2"));
    CHECK(filter2)
        filter3 = GTK_WIDGET(gtk_builder_get_object(builder, "filter3"));
    CHECK(filter3)
        filter4 = GTK_WIDGET(gtk_builder_get_object(builder, "filter4"));
    CHECK(filter4)
        filter5 = GTK_WIDGET(gtk_builder_get_object(builder, "filter5"));
    CHECK(filter5)
        filter6 = GTK_WIDGET(gtk_builder_get_object(builder, "filter6"));
    CHECK(filter6)
        filter7 = GTK_WIDGET(gtk_builder_get_object(builder, "filter7"));
    CHECK(filter7)

        quadrillage = GTK_BUTTON(gtk_builder_get_object(builder, "quadrillage"));
    CHECK(quadrillage)
        blanksheet = GTK_BUTTON(gtk_builder_get_object(builder, "blanksheet"));
    CHECK(blanksheet)
        partition = GTK_BUTTON(gtk_builder_get_object(builder, "partition"));
    CHECK(partition)

        //ICON ON GLADE
        GtkWidget *imageBucket = gtk_image_new_from_file("icons/fill.png");
    gtk_button_set_image(bucket, imageBucket);

    GtkWidget *imagePen = gtk_image_new_from_file("icons/pencil.png");
    gtk_button_set_image(pen, imagePen);

    GtkWidget *imageErase = gtk_image_new_from_file("icons/eraser.png");
    gtk_button_set_image(erase, imageErase);

    GtkWidget *imageStatus = gtk_image_new_from_file("icons/pencil.png");
    gtk_button_set_image(status, imageStatus);

    GtkWidget *imageRectangle = gtk_image_new_from_file("icons/rect.png");
    gtk_button_set_image(rect, imageRectangle);

    GtkWidget *imageCircle = gtk_image_new_from_file("icons/circle.png");
    gtk_button_set_image(circle, imageCircle);

    GtkWidget *imageTriangle = gtk_image_new_from_file("icons/triangle.png");
    gtk_button_set_image(triangle, imageTriangle);

    GtkWidget *imageLosange = gtk_image_new_from_file("icons/losange.png");
    gtk_button_set_image(losange, imageLosange);

    GtkWidget *imagePipette = gtk_image_new_from_file("icons/pipette.png");
    gtk_button_set_image(pipette, imagePipette);

    GtkWidget *imagePolygon = gtk_image_new_from_file("icons/polygon.png");
    gtk_button_set_image(polygon, imagePolygon);

    GtkWidget *imageStar = gtk_image_new_from_file("icons/star.png");
    gtk_button_set_image(star, imageStar);

    GtkWidget *imageAnnul = gtk_image_new_from_file("icons/redo.png");
    gtk_button_set_image(annul, imageAnnul);

    GtkWidget *imageRetour = gtk_image_new_from_file("icons/undo.png");
    gtk_button_set_image(retour, imageRetour);

    GtkWidget *imageBlank = gtk_image_new_from_file("icons/blanksheet.png");
    gtk_button_set_image(blanksheet, imageBlank);

    GtkWidget *imageQuad = gtk_image_new_from_file("icons/quadrillage.png");
    gtk_button_set_image(quadrillage, imageQuad);

    GtkWidget *imageMusic = gtk_image_new_from_file("icons/partition.png");
    gtk_button_set_image(partition, imageMusic);

    GtkWidget *imageSurligneur = gtk_image_new_from_file("icons/surligneur.png");
    gtk_button_set_image(surligneur, imageSurligneur);

    hscale = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(adjustement));
    gtk_container_add(GTK_CONTAINER(grid), hscale);

    hscale2 = gtk_scale_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(adjustement2));
    gtk_container_add(GTK_CONTAINER(grid2), hscale2);

    //g_signal_connect(G_OBJECT(drawarea), "draw",G_CALLBACK(on_draw_event), NULL); 

    gtk_widget_add_events(drawarea, 
            GDK_BUTTON_PRESS_MASK |
            GDK_BUTTON_MOTION_MASK |
            GDK_BUTTON_RELEASE_MASK |
            GDK_KEY_PRESS_MASK);
    gtk_widget_add_events(gtk_widget_get_toplevel (drawarea), 
            GDK_BUTTON_MOTION_MASK | GDK_BUTTON_PRESS_MASK |
            GDK_BUTTON_MOTION_MASK |
            GDK_BUTTON_RELEASE_MASK );
    g_signal_connect(load, "activate", G_CALLBACK(openfile), NULL);
    g_signal_connect(pen, "clicked", G_CALLBACK(return_draw), NULL);
    g_signal_connect(erase, "clicked", G_CALLBACK(erase_white), NULL);
    g_signal_connect(bucket, "clicked", G_CALLBACK(flood_fill), NULL);
    g_signal_connect(rect, "clicked", G_CALLBACK(get_rect), NULL);
    g_signal_connect(losange, "clicked", G_CALLBACK(get_losange), NULL);
    g_signal_connect(circle, "clicked", G_CALLBACK(get_circle), NULL);
    g_signal_connect(star, "clicked", G_CALLBACK(get_star), NULL);
    g_signal_connect(polygon, "clicked", G_CALLBACK(get_polygon), NULL);
    g_signal_connect(triangle, "clicked", G_CALLBACK(get_triangle), NULL);
    g_signal_connect(pipette, "clicked", G_CALLBACK(get_pipette), NULL);
    g_signal_connect(rightrot, "clicked", G_CALLBACK(rotaright), NULL);
    g_signal_connect(leftrot, "clicked", G_CALLBACK(rotaleft), NULL);
    g_signal_connect(drawarea, "button-press-event", G_CALLBACK(on_click), NULL); //blc
    g_signal_connect(drawarea, "motion-notify-event", G_CALLBACK(on_click), NULL); //important
    g_signal_connect(drawarea, "button-release-event", G_CALLBACK(on_click_release), NULL);
    g_signal_connect(color1, "color-set", G_CALLBACK(on_color1_color_set), NULL);
    g_signal_connect(G_OBJECT(drawarea), "draw", G_CALLBACK(on_draw), NULL);
    g_signal_connect(web, "activate", G_CALLBACK(website_button), NULL);
    g_signal_connect(save, "activate", G_CALLBACK(on_save),NULL);
    g_signal_connect(hscale, "value-changed", G_CALLBACK(value_changed), NULL);
    g_signal_connect(hscale2, "value-changed", G_CALLBACK(value_changed2), NULL);
    g_signal_connect(retour, "clicked", G_CALLBACK(ctrl_z), NULL);
    g_signal_connect(annul, "clicked", G_CALLBACK(ctrl_y), NULL);
    g_signal_connect(window, "motion-notify-event", G_CALLBACK(refresh), NULL);
    //g_signal_connect(drawarea, "button-release-event", G_CALLBACK(refresh), NULL);
    g_signal_connect(gtk_widget_get_toplevel(menu), "button-release-event", G_CALLBACK(refresh), NULL);

    g_signal_connect(enter,"clicked",G_CALLBACK(value_changed3), NULL);

    g_signal_connect(new, "activate", G_CALLBACK(loadblank), NULL);
    g_signal_connect(newgrid, "activate", G_CALLBACK(loadgrid), NULL);
    //FILTERS
    g_signal_connect(filter1, "activate", G_CALLBACK(grey), NULL);
    g_signal_connect(filter2, "activate", G_CALLBACK(invert), NULL);
    g_signal_connect(filter3, "activate", G_CALLBACK(cyann), NULL);
    g_signal_connect(filter4, "activate", G_CALLBACK(rosee), NULL);
    g_signal_connect(filter5, "activate", G_CALLBACK(yelloww), NULL);
    g_signal_connect(filter6, "activate", G_CALLBACK(BAW), NULL);
    g_signal_connect(filter7, "activate", G_CALLBACK(mirrorr), NULL);

    g_signal_connect(quadrillage, "clicked", G_CALLBACK(quad), NULL);
    g_signal_connect(blanksheet, "clicked", G_CALLBACK(blank), NULL);
    g_signal_connect(partition, "clicked", G_CALLBACK(part), NULL);

    g_signal_connect(surligneur, "clicked", G_CALLBACK(get_highlight), NULL);

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_show_all(window);

    surface = gdk_window_create_similar_surface(
            gtk_widget_get_parent_window(drawarea),
            CAIRO_CONTENT_COLOR,
            gtk_widget_get_allocated_width(drawarea),
            gtk_widget_get_allocated_height(drawarea));

    //gtk_widget_set_app_paintable(drawarea, TRUE);

    cairo_t *context = cairo_create(surface);
    cairo_set_source_rgba(context, 1, 1, 1, 1);
    cairo_paint(context);
    surface_pixbuf = gdk_pixbuf_get_from_surface(surface,0,0,gtk_widget_get_allocated_width(drawarea),gtk_widget_get_allocated_height(drawarea));
    undo = push_stack(gdk_pixbuf_copy(surface_pixbuf), undo);
    cairo_destroy(context);

    gtk_main();
}


int main(int argc, char *argv[])
{

    GtkApplication *app;
    app = gtk_application_new("test.test", G_APPLICATION_FLAGS_NONE);
    g_signal_connect(app, "activate", G_CALLBACK(create_window), NULL);
    int status= g_application_run(G_APPLICATION(app), argc, argv);

    g_object_unref(app);


    return status;
}
