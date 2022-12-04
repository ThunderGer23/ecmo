//Vista buena

#include <LCDWIKI_GUI.h>    //Core graphics library
#include <LCDWIKI_KBV.h>    //Hardware-specific library
#include <LCDWIKI_TOUCH.h>  //touch screen library
#include <SoftwareSerial.h>
#include <String.h>
#include <SPI.h>
#include <SD.h>

LCDWIKI_KBV my_lcd(ILI9488, 40, 38, 39, 43, 41);  //model,cs,cd,wr,rd,reset
LCDWIKI_TOUCH my_touch(53, 52, 50, 51, 44);       //tcs,tclk,tdout,tdin,tirq

                                            /*  C     M   Y   K */
#define AZULJUERTE      0xDB1A16      /*   0, 255, 255      ALERTA*/
#define DARKPURPLE      0x30011F      /*   0,   0,   0      LETRAS*/
#define STARCOMMANDBLUE 0x37AFFA      /*   0,   0, 255      SELECT*/
#define AZULCLARO       0x277EB5      /* 255,   0,   0      FONDO*/
#define GREENEAGLE      0x015C66      /*   0, 255,   0      BOTON*/
#define XANADU          0x5E736F      /*   0, 255,   0      BOTONPRESIONADO*/
#define WHITE   0xFFFF
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0

/******************* UI details */
#define BUTTON_R 35
#define BUTTON_SPACING_X 35
#define BUTTON_SPACING_Y 10
#define EDG_Y 10
#define EDG_X 20

// We have a status line for like, is FONA working
#define STATUS_X 10
#define STATUS_Y 65
#define PIXEL_NUMBER  (my_lcd.Get_Display_Width()/4)
#define FILE_NUMBER 1
#define FILE_NAME_SIZE_MAX 20

uint32_t bmp_offset = 0;
uint16_t s_width = my_lcd.Get_Display_Width();  
uint16_t s_heigh = my_lcd.Get_Display_Height();
//int16_t PIXEL_NUMBER;

char file_name[FILE_NUMBER][FILE_NAME_SIZE_MAX];

uint16_t read_16(File fp){
  uint8_t low = fp.read();
  uint16_t high = fp.read();
  return (high<<8)|low;
}

uint32_t read_32(File fp){
  uint16_t low = read_16(fp);
  uint32_t high = read_16(fp);
  return (high<<8)|low;   
}

bool analysis_bpm_header(File fp){
  if(read_16(fp) != 0x4D42){    
    return false;
  }
  read_32(fp);  
  read_32(fp);  
  bmp_offset = read_32(fp);  
  read_32(fp);
  uint32_t bpm_width = read_32(fp);
  uint32_t bpm_heigh = read_32(fp);
  return ((bpm_width != s_width) || (bpm_heigh != s_heigh)) ? false: (read_16(fp) != 1) ? false: read_16(fp);
  return(read_32(fp) != 0) ? false: true;  
}

void draw_bmp_picture(File fp){  
  uint8_t bpm_data[PIXEL_NUMBER*3] = {0};
  uint16_t bpm_color[PIXEL_NUMBER];
  fp.seek(bmp_offset);
  for(uint16_t i = 0;i < s_heigh;i++){
    for(uint16_t j = 0;j<s_width/PIXEL_NUMBER;j++){
      uint16_t m = 0;
      fp.read(bpm_data,PIXEL_NUMBER*3);
      for(uint16_t k = 0;k<PIXEL_NUMBER;k++){
        bpm_color[k]= my_lcd.Color_To_565(bpm_data[m+2], bpm_data[m+1], bpm_data[m+0]);
        m +=3;
      }
      for(uint16_t l = 0;l<PIXEL_NUMBER;l++){
        my_lcd.Set_Draw_color(bpm_color[l]);
        my_lcd.Draw_Pixel(j*PIXEL_NUMBER+l,i);
      }    
    }
  }    
}

typedef struct _button_info {
  uint8_t button_name[10];
  uint8_t button_name_size;
  uint16_t button_name_colour;
  uint16_t button_colour;
  uint16_t button_x;
  uint16_t button_y;
} button_info;

button_info phone_button[6] = {
  "Hola mundo", 4, DARKPURPLE, GREENEAGLE, EDG_X + BUTTON_R - 1, my_lcd.Get_Display_Height() - EDG_Y - 4 * BUTTON_SPACING_Y - 9 * BUTTON_R - 1,
  "2", 4, DARKPURPLE, STARCOMMANDBLUE, EDG_X + 3 * BUTTON_R + BUTTON_SPACING_X - 1, my_lcd.Get_Display_Height() - EDG_Y - 4 * BUTTON_SPACING_Y - 9 * BUTTON_R - 1,
  "3", 4, DARKPURPLE, AZULJUERTE, EDG_X + 5 * BUTTON_R + 2 * BUTTON_SPACING_X - 1, my_lcd.Get_Display_Height() - EDG_Y - 4 * BUTTON_SPACING_Y - 9 * BUTTON_R - 1,
  "4", 4, DARKPURPLE, XANADU, EDG_X + BUTTON_R - 1, my_lcd.Get_Display_Height() - EDG_Y - 3 * BUTTON_SPACING_Y - 7 * BUTTON_R - 1,
  "5", 4, DARKPURPLE, GREENEAGLE, EDG_X + 3 * BUTTON_R + BUTTON_SPACING_X - 1, my_lcd.Get_Display_Height() - EDG_Y - 3 * BUTTON_SPACING_Y - 7 * BUTTON_R - 1,
  "6", 4, DARKPURPLE, GREENEAGLE, EDG_X + 5 * BUTTON_R + 2 * BUTTON_SPACING_X - 1, my_lcd.Get_Display_Height() - EDG_Y - 3 * BUTTON_SPACING_Y - 7 * BUTTON_R - 1,
};

uint16_t text_x = 7, text_y = 10, text_x_add = 6 * phone_button[0].button_name_size, text_y_add = 8 * phone_button[0].button_name_size;
uint16_t n = 0;

void show_string(uint8_t *str, int16_t x, int16_t y, uint8_t csize, uint16_t fc, uint16_t bc, boolean mode) {
  my_lcd.Set_Text_Mode(mode);
  my_lcd.Set_Text_Size(csize);
  my_lcd.Set_Text_colour(fc);
  my_lcd.Set_Text_Back_colour(bc);
  my_lcd.Print_String(str, x, y);
}

boolean is_pressed(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t px, int16_t py) {
  if ((px > x1 && px < x2) && (py > y1 && py < y2)) {
    return true;
  } else {
    return false;
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  my_lcd.Init_LCD();
  my_touch.TP_Init(my_lcd.Get_Rotation(), my_lcd.Get_Display_Width(), my_lcd.Get_Display_Height());
  my_lcd.Fill_Screen(AZULCLARO);
  Serial.print(my_lcd.Get_Display_Width());
  Serial.print(my_lcd.Get_Display_Height());
  if(PIXEL_NUMBER != 60){
    strcpy(file_name[0],"01.bmp");
  }
  //Init SD_Card
  pinMode(48, OUTPUT);
   
  if (!SD.begin(48)) {
    my_lcd.Set_Text_Back_colour(BLUE);
    my_lcd.Set_Text_colour(WHITE);    
    my_lcd.Set_Text_Size(1);
    my_lcd.Print_String("SD Card Init fail!",0,0);
  }
  //DrawLog();
  my_lcd.Set_Rotation(3);
  my_touch.TP_Set_Rotation(3);
  Menu();
}

void DrawLog(){
  File bmp_file;
  for(int i = 0;i<FILE_NUMBER;i++){
  bmp_file = SD.open(file_name[i]);
  if(!bmp_file){
    my_lcd.Set_Text_Back_colour(BLUE);
    my_lcd.Set_Text_colour(WHITE);    
    my_lcd.Set_Text_Size(1);
    my_lcd.Print_String("didnt find BMPimage!",0,10);
    while(1);
  }
  if(!analysis_bpm_header(bmp_file)){  
    my_lcd.Set_Text_Back_colour(BLUE);
    my_lcd.Set_Text_colour(WHITE);    
    my_lcd.Set_Text_Size(1);
    my_lcd.Print_String("bad bmp picture!",0,0);
    return;
  }
  draw_bmp_picture(bmp_file);
  bmp_file.close();   
  }
}

void Menu(){
  char S[2] = " ";
  S[0] = 246;
  char S1[2] = " ";
  my_lcd.Fill_Screen(AZULCLARO);
  my_lcd.Draw_Round_Rectangle(5, 5, 210, 55, 10);
  show_string("00:00:00", 15, 17, 4, GREENEAGLE, DARKPURPLE, 1);
  my_lcd.Fill_Circle(25, 95, 20);
  show_string(">", 17, 85, 3, GREENEAGLE, DARKPURPLE, 1);
  my_lcd.Fill_Circle(75, 95, 20);
  show_string("<", 65, 85, 3, GREENEAGLE, DARKPURPLE, 1);
  my_lcd.Fill_Circle(140, 95, 20);  
  show_string("^", 133, 90, 3, GREENEAGLE, DARKPURPLE, 1);
  my_lcd.Fill_Circle(190, 95, 20);
  my_lcd.Set_Draw_color(0, 172, 177);

  int xinit = 185;
  int yinit = 90;
  int veces = 5;
  for(int a = 0; a < veces; a++){
    for(int i = xinit; i <= xinit+2; i++){
      for (int j = yinit; j <= yinit+2; j++){
        my_lcd.Draw_Pixel(i, j);         
      }
    }
    if (a >= 2){
      yinit -= 3;
      xinit += 3;
    }else {
      xinit += 3;
      yinit += 3;
    }
  }
  
  //show_string("V", 185, 90, 2, GREENEAGLE, DARKPURPLE, 1);
  
  show_string("Temperatura", 35, 130, 3, GREENEAGLE, DARKPURPLE, 1);
  my_lcd.Draw_Round_Rectangle(5, 165, 85, 215, 10);
  show_string("00", 15, 177, 4, GREENEAGLE, DARKPURPLE, 1);
  my_lcd.Draw_Char(64, 177, S[0], GREENEAGLE, DARKPURPLE, 3, 1);
  show_string("Deseada", 5, 220, 2, GREENEAGLE, DARKPURPLE, 1);
  my_lcd.Draw_Round_Rectangle(190, 165, 270, 215, 10);
  show_string("00", 200, 177, 4, GREENEAGLE, DARKPURPLE, 1);
  my_lcd.Draw_Char(250, 177, S[0], GREENEAGLE, DARKPURPLE, 3, 1);
  show_string("Real", 209, 220, 2, GREENEAGLE, DARKPURPLE, 1);
  // my_lcd.Fill_Circle(25, 95, 20);
  // show_string(">", 17, 85, 3, GREENEAGLE, DARKPURPLE, 1);
  // my_lcd.Fill_Circle(75, 95, 20);
  // show_string("<", 65, 85, 3, GREENEAGLE, DARKPURPLE, 1);
  // my_lcd.Fill_Circle(140, 95, 20);  
  // show_string("^", 133, 90, 3, GREENEAGLE, DARKPURPLE, 1);
  // my_lcd.Fill_Circle(190, 95, 20);
  // show_string("V", 185, 90, 2, GREENEAGLE, DARKPURPLE, 1);
}

void loop() {
  // put your main code here, to run repeatedly:
  // uint16_t i;
  uint16_t px = 0;
  uint16_t py = 0;
  my_touch.TP_Scan(0);
  if (my_touch.TP_Get_State() & TP_PRES_DOWN) {
    px = my_touch.x;
    py = my_touch.y;
  }  

  // for (i = 0; i < sizeof(phone_button) / sizeof(button_info); i++) {
  //   if (is_pressed(phone_button[i].button_x - BUTTON_R, phone_button[i].button_y - BUTTON_R, phone_button[i].button_x + BUTTON_R, phone_button[i].button_y + BUTTON_R, px, py)) {
  //     my_lcd.Fill_Round_Rectangle(phone_button[i].button_x, phone_button[i].button_y, phone_button[i].button_x+25, phone_button[i].button_y+12, BUTTON_R);
  //     show_string(phone_button[i].button_name, phone_button[i].button_x - strlen(phone_button[i].button_name) * phone_button[i].button_name_size * 6 / 2 + phone_button[i].button_name_size / 2 + 1, phone_button[i].button_y - phone_button[i].button_name_size * 8 / 2 + phone_button[i].button_name_size / 2 + 1, phone_button[i].button_name_size, GREENEAGLE, DARKPURPLE, 1);
  //     delay(100);
  //     my_lcd.Set_Draw_color(phone_button[i].button_colour);
  //     my_lcd.Fill_Round_Rectangle(phone_button[i].button_x, phone_button[i].button_y, phone_button[i].button_x+25, phone_button[i].button_y+12, BUTTON_R);
  //     show_string(phone_button[i].button_name, phone_button[i].button_x - strlen(phone_button[i].button_name) * phone_button[i].button_name_size * 6 / 2 + phone_button[i].button_name_size / 2 + 1, phone_button[i].button_y - phone_button[i].button_name_size * 8 / 2 + phone_button[i].button_name_size / 2 + 1, phone_button[i].button_name_size, phone_button[i].button_name_colour, DARKPURPLE, 1);
  //     if (i < 12) {
  //       if (n < 13) {
  //         show_string(phone_button[i].button_name, text_x, text_y, phone_button[i].button_name_size, GREENEAGLE, DARKPURPLE, 1);
  //         text_x += text_x_add - 1;
  //         n++;
  //       }
  //     } else if (12 == i) {
  //       my_lcd.Set_Draw_color(AZULJUERTE);
  //       my_lcd.Fill_Rectangle(0, 48, my_lcd.Get_Display_Width() - 1, 60);
  //       show_string("Calling ended", CENTER, 52, 1, GREENEAGLE, DARKPURPLE, 1);
  //     } else if (13 == i) {
  //       my_lcd.Set_Draw_color(AZULJUERTE);
  //       my_lcd.Fill_Rectangle(0, 48, my_lcd.Get_Display_Width() - 1, 60);
  //       show_string("Calling...", CENTER, 52, 1, GREENEAGLE, DARKPURPLE, 1);
  //     } else if (14 == i) {
  //       if (n > 0) {
  //         my_lcd.Set_Draw_color(AZULJUERTE);
  //         text_x -= (text_x_add - 1);
  //         my_lcd.Fill_Rectangle(text_x, text_y, text_x + text_x_add - 1, text_y + text_y_add - 2);
  //         n--;
  //       }
  //     }
  //   }
  // }
}
