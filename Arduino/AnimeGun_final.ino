#include <Stepper.h>
#include <math.h>

//#define FULLCOLOR

#ifndef FULLCOLOR
#define MODES 4
#else
#define MODES 5
#endif

#define STEPS 360  //ステッピングモーターのステップ数
#define DISPTIME 250  //7セグLEDの表示時間の半分<ms>

/*ピン設定*/
#define MTRPIN1 4
#define MTRPIN2 7
#define LEDPIN  6 //red
#define LEDPIN2 5 //green
#define LEDPIN3 3 //blue
#define LOGIC 8      //74HC154の入力ピンA<8~11, 12:Enable>
#define SWITCH  2     //ボタンスイッチ
#define TRIGGER 13    //トリガー

#define DIAL 0    //可変抵抗の入力ピン

/*7セグLEDの文字表示用変数*/
/*
    7         ***
  8   6      *   *
    3         ***
  5   2      *   *
    4    1    ***    *
*/
const byte CharSet[] PROGMEM =
{
  B11111010,  //A
  B10011110,  //B
  B11011000,  //C
  B00111110,  //D
  B11011100,  //E
  B11010100,  //F
  B11011010,  //G
  B10110110,  //H
  B00000010,  //I
  B00111010,  //J
  B11010110,  //K
  B10011000,  //L
  B11110010,  //M
  B00010110,  //N
  B00011110,  //O
  B11110100,  //P
  B11111100,  //Q
  B00010100,  //R
  B10001110,  //S
  B00011010,  //U
  B10111010,  //V
  B10111110,  //W
  B10110110,  //X
  B10101110,  //Y
  B01110100   //Z
};

const byte CharSetNum[] PROGMEM = 
{
  B11111010,
  B00100010,
  B01111100,
  B01101110,
  B10100110,
  B11001110,
  B11011110,
  B01100010,
  B11111110,
  B11101110
};
//byte char0 = B11111010;
//byte char1 = B00100010;
//byte char2 = B01111100;
//byte char3 = B01101110;
//byte char4 = B10100110;
//byte char5 = B11001110;
//byte char6 = B11011110;
//byte char7 = B01100010;
//byte char8 = B11111110;
//byte char9 = B11101110;

byte mode = 0;  //表示内容の変更<7セグ>
int timeCnt = 0;  //何秒経過したかを保存<モーター出力off時>
int rgb[] = {255, 255, 255};  //フルカラーLED専用の色調整

/*ステッピングモータ用ライブラリ*/
Stepper stepper(STEPS, MTRPIN1, MTRPIN2);

void setup()
{
  /*スイッチの設定*/
  pinMode(SWITCH, INPUT);
  pinMode(TRIGGER, INPUT);
  digitalWrite(LOGIC + 4, HIGH);
  
  /*ロジックICの設定*/
  for(int i = 0; i < 5; i++)  pinMode(LOGIC + i, OUTPUT);
  
  /*モーターの速度を指定*/
  stepper.setSpeed(30);
  
  attachInterrupt(0, modeChange, FALLING);
}

void loop()
{
  /*静的変数*/
  static int preValue = 0;
  static int brightness = 255;
  static int hue = 0;
  
  static int count = 0;    //モーターの累積ステップ数
  static int frames = 12;  //フレーム数
  
  static int spd = 0;
  
  /*AD変換*/
  int value = analogRead(DIAL);
  
  if(digitalRead(TRIGGER) == LOW){  //銃のトリガーを引く
  
    //7セグLEDの無効化
    timeCnt = 0;                    //7セグLED表示のリセット
    digitalWrite(LOGIC + 4, HIGH);  //7セグLEDに使うロジックICを無効化

    if(value != preValue){
      spd = map(value, 0, 1023, 8, 60);  //モーターの速度に変換
      stepper.setSpeed(spd);
    }
    
    //モーターを回転させる
    stepper.step(1);
    count++;
    
    //ストロボフラッシュの生成
    //モーターが一定の角度だけ回転する度に光るようにする
    //角度<ステップ数> = モーター1回転のステップ数 / アニメ1枚の総フレーム数
    if(count >= (STEPS / frames)){    // the motor steps each frame of anime
      // full color
      if(MODES == 5)  lightOn(rgb[0], rgb[1], rgb[2]);
      // single color
      else            lightOn(brightness, 0, 0);  //digitalWrite(LEDPIN, HIGH);
      count = 0;
    }
    else{
      lightOff();
    }
    
  }
  else{
    switch(mode){
      //モーターの速度変更  
      case 0:
        lightOff();
        
        if(value != preValue){
          spd = map(value, 0, 1023, 8, 60);  //モーターの速度に変換
          stepper.setSpeed(spd);
        }
        
        //7セグLEDの表示
        if(timeCnt > DISPTIME){  //0.5sを超えると
          Write7seg(spd / 10, 1);
          Write7seg(spd % 10, 0);
        }else{
          Write7seg('S', 1);
          Write7seg('P', 0);
          timeCnt++;
        }
        break;
      //角度調整<モーターのステップ角を変更>
      case 1:
        if(value != preValue){
          stepper.setSpeed(30);
          stepper.step(value - preValue);
          
          stepper.setSpeed(spd);
        }
        
        Write7seg('D', 1);
        Write7seg('G', 0);
        timeCnt++;
        
        break;
      //明るさ調整
      case 2:        
        if(value != preValue){
          brightness = (value >> 2) / 10;  //10bitの値を8bitに
          if(MODES == 5)  HSVtoRGB(hue, 255, brightness);
          else            analogWrite(LEDPIN, brightness);
        }
        
        //7セグLEDの表示
        if(timeCnt > DISPTIME){  //0.5sを超えると
          Write7seg(brightness / 10, 1);
          Write7seg(brightness % 10, 0);
        }else{
          Write7seg('B', 1);
          Write7seg('R', 0);
          timeCnt++;
        }
        break;
      //フレーム数調整
      case 3:
        lightOff();
        
        if(value != preValue){
          frames = map(value, 0, 1023, 1, 48);
        }
        
        //7セグLEDの表示
        if(timeCnt > DISPTIME){  //0.5sを超えると
          Write7seg(frames / 10, 1);
          Write7seg(frames % 10, 0);
        }else{
          Write7seg('F', 1);
          Write7seg('L', 0);
          timeCnt++;
        }
        
        break;
      //色調整
      case 4:
        if(value != preValue){
          hue = map(value, 0, 1023, 0, 359);
          HSVtoRGB(hue, 255, brightness);
        }
        
        //7セグLEDの表示
        if(timeCnt > DISPTIME){  //0.5sを超えると
          Write7seg(hue / 100, 1);      //100の桁
          Write7seg(hue / 10 % 10, 0);  //10の桁
        }else{
          Write7seg('C', 1);
          Write7seg('L', 0);
          timeCnt++;
        }
        
        break;
    }
  }
  
  preValue = value;
}

/*7セグLED用の表示関数, 計算上は表示で2msほど経過*/
void Write7seg(int value, byte digit){   // 数値,  桁数<0:1桁目, 1:2桁目>
  byte unit = pgm_read_byte_near(CharSetNum + value);
  
  for(int i = 0; i < 8; i++){
    if((unit >> i) & B00000001)  PORTB = (PORTB & B11100000) | (i + 8 * digit);
    else                         PORTB = (PORTB & B11100000) | B10000;
    delayMicroseconds(500); //delay(1);
    
  }
  
  //PORTB = (PORTB & B11100000) | B10000;
}

/*7セグLED用の表示関数<文字>, 計算上は表示で2msほど経過*/
void Write7seg(char moji, byte digit){
  byte unit = pgm_read_byte_near(CharSet + (byte)(moji - 'A'));
  
  for(int i = 0; i < 8; i++){
    if((unit >> i) & B00000001)  PORTB = (PORTB & B11100000) | (i + 8 * digit);
    else                         PORTB = (PORTB & B11100000) | B10000;
    delayMicroseconds(500); //delay(1);
  }
  
  PORTB = (PORTB & B11100000) | B10000;
}

/*HSVをRGBに変換する関数*/
void HSVtoRGB(int h, int s, int v){  //引数は、色相<0~359>、彩度<0~255>、明度<0~255>の順
  float f;
  int i, p, q, t;
  
  i = (int)floor(h / 60.0f) % 6;
  f = (float)(h / 60.f) - (float)floor(h / 60.0f);
  p = (int)round(v * (1.0f - (s / 255.0f)));
  q = (int)round(v * (1.0f - (s / 255.0f) * f));
  t = (int)round(v * (1.0f - (s / 255.0f) * (1.0f - f)));
  
  switch(i){
    case 0:  rgb[0] = v; rgb[1] = t; rgb[2] = p; break;
    case 1:  rgb[0] = q; rgb[1] = v; rgb[2] = p; break;
    case 2:  rgb[0] = p; rgb[1] = v; rgb[2] = t; break;
    case 3:  rgb[0] = p; rgb[1] = q; rgb[2] = v; break;
    case 4:  rgb[0] = t; rgb[1] = p; rgb[2] = v; break;
    case 5:  rgb[0] = v; rgb[1] = p; rgb[2] = q; break;
  }
  
  analogWrite(LEDPIN, rgb[0]);  //6ピンは赤
  analogWrite(LEDPIN2, rgb[1]);  //5ピンは緑
  analogWrite(LEDPIN3, rgb[2]);  //3ピンは青
}

void lightOn(int r, int g, int b){
  analogWrite(LEDPIN, r);  //6ピンは赤
  analogWrite(LEDPIN2, g);  //5ピンは緑
  analogWrite(LEDPIN3, b);  //3ピンは青
//  digitalWrite(LEDPIN, HIGH);  //6ピンは赤
//  digitalWrite(LEDPIN2, HIGH);  //5ピンは緑
//  digitalWrite(LEDPIN3, HIGH);  //3ピンは青
}

void lightOff(){
  digitalWrite(LEDPIN, LOW);  //LEDライトを消す
  digitalWrite(LEDPIN2, LOW);  //LED2ライトを消す
  digitalWrite(LEDPIN3, LOW);  //LED3ライトを消す
}

void modeChange(){
  mode = (mode + 1) % MODES;
  timeCnt = 0;
}
