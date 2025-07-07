#include <Wire.h>
#include "Adafruit_TCS34725.h"

//类白平衡校准（滤光通道补偿）+目标颜色校准（容差处理）+RGB映射HSV色彩空间判定（S、V筛选）

// 常量定义：
// R_COMP, G_COMP, B_COMP：分别是红、绿、蓝通道的白平衡补偿系数
// hRed, hGreen, hBlue：红、绿、蓝三种颜色的基准色调（Hue角度值）

// 变量定义：
// r_raw, g_raw, b_raw, c_raw：传感器采集的原始颜色通道数据（16位整数）
// r_corr, g_corr, b_corr：应用补偿系数后的浮点颜色值（白平衡校正后）
// r_std, g_std, b_std：归一化后标准RGB值（8位，范围0~255）
// rf，bf，gf：临时红色浮点值，归一化到0~255范围（可能>255，需裁剪）
// h, s, v：HSV颜色空间参数，h色调(0~360度)，s饱和度(0~1)，v亮度(0~1)
// color_1：颜色识别结果ID，0=无色/环境，1=红，2=绿，3=蓝

Adafruit_TCS34725 tcs = Adafruit_TCS34725(
  TCS34725_INTEGRATIONTIME_154MS,
  TCS34725_GAIN_16X);

// 补偿系数（测得标准色板的rgb，将其对应颜色补偿至240左右的比例系数）
const float R_COMP = 1.25;
const float G_COMP = 1.78;
const float B_COMP = 2.10;

// 颜色基准色调(测得标准色板的h值)
const float hRed   = 344.0;
const float hGreen = 163.5;
const float hBlue  = 202.0;

int color_1 = 0;

// 判断是否在容差范围内（支持跨0度处理）（默认5%容差）
bool withinTolerance(float base, float val, float tolerancePercent = 5.0) {
  float tolerance = base * tolerancePercent / 100.0;
  float lower = base - tolerance;
  float upper = base + tolerance;
  if (lower < 0) lower += 360;
  if (upper > 360) upper -= 360;

  if (lower > upper) {
    return (val >= lower && val <= 360) || (val >= 0 && val <= upper);
  } else {
    return (val >= lower && val <= upper);
  }
}

// 获取原始RGBC
void readRawRGB(uint16_t& r, uint16_t& g, uint16_t& b, uint16_t& c) {
  tcs.getRawData(&r, &g, &b, &c);
}

// 应用补偿
void applyCompensation(float& r_corr, float& g_corr, float& b_corr,
                        uint16_t r_raw, uint16_t g_raw, uint16_t b_raw) {
  r_corr = r_raw * R_COMP;
  g_corr = g_raw * G_COMP;
  b_corr = b_raw * B_COMP;
}

// 标准化RGB到0-255
void normalizeRGB(uint8_t& r, uint8_t& g, uint8_t& b,
                  float r_corr, float g_corr, float b_corr, uint16_t c_raw) {
  float sum = c_raw > 0 ? c_raw : 1; // 防止除0

  float rf = 255.0 * r_corr / sum;
  float gf = 255.0 * g_corr / sum;
  float bf = 255.0 * b_corr / sum;
  // 裁剪最大值，避免 uint8_t 溢出
  r = (uint8_t)(rf > 255.0 ? 255.0 : rf);
  g = (uint8_t)(gf > 255.0 ? 255.0 : gf);
  b = (uint8_t)(bf > 255.0 ? 255.0 : bf);
}

// RGB转HSV
void rgbToHSV(uint8_t r, uint8_t g, uint8_t b,
              float& h, float& s, float& v) {
  float rf = r / 255.0, gf = g / 255.0, bf = b / 255.0;
  float maxc = max(rf, max(gf, bf));
  float minc = min(rf, min(gf, bf));
  float delta = maxc - minc;

  h = 0; v = maxc; s = 0;
  if (delta > 0) {
    if (maxc == rf) {
      h = 60 * fmod(((gf - bf) / delta), 6);
    } else if (maxc == gf) {
      h = 60 * (((bf - rf) / delta) + 2);
    } else {
      h = 60 * (((rf - gf) / delta) + 4);
    }
    if (h < 0) h += 360;
  }
  if (maxc > 0) {
    s = delta / maxc;
  }
}

// 判定颜色（加入门限rgb值220判定，加强对特殊反光材料的抗干扰能力）（手动输入10%容差）
int detectColor(float h, float s, float v, uint8_t r_std, uint8_t g_std, uint8_t b_std) {
  if (s * 100 < 55 || v * 100 < 80) return 0; // 环境色

  if (withinTolerance(hRed, h, 10) && r_std >= 220)   return 1; // 红色
  if (withinTolerance(hGreen, h, 10) && g_std >= 220) return 2; // 绿色
  if (withinTolerance(hBlue, h, 10) && b_std >= 220)  return 3; // 蓝色

  return 0; // 其他
}

void setup() {
  Serial.begin(115200);
  while (!Serial) delay(10);

  if (tcs.begin()) {
    Serial.println("TCS34725 初始化成功");
  } else {
    Serial.println("未找到 TCS34725，请检查接线！");
    while (1) delay(100);
  }
}

void loop() {
  uint16_t r_raw, g_raw, b_raw, c_raw;
  float r_corr, g_corr, b_corr;
  uint8_t r_std, g_std, b_std;
  float h, s, v;

  readRawRGB(r_raw, g_raw, b_raw, c_raw);
  applyCompensation(r_corr, g_corr, b_corr, r_raw, g_raw, b_raw);
  normalizeRGB(r_std, g_std, b_std, r_corr, g_corr, b_corr, c_raw);
  rgbToHSV(r_std, g_std, b_std, h, s, v);

  color_1 = detectColor(h, s, v, r_std, g_std, b_std);

  //调试输出
  Serial.print("原始 RGBC: ");
  Serial.print(r_raw); Serial.print(", ");
  Serial.print(g_raw); Serial.print(", ");
  Serial.print(b_raw); Serial.print(", ");
  Serial.println(c_raw);

  Serial.print("补偿标准 RGB: ");
  Serial.print(r_std); Serial.print(", ");
  Serial.print(g_std); Serial.print(", ");
  Serial.println(b_std);

  Serial.print("HSV: H=");
  Serial.print(h, 1); Serial.print(" S=");
  Serial.print(s * 100, 1); Serial.print("% V=");
  Serial.print(v * 100, 1); Serial.println("%");

  Serial.print("识别颜色 ID: ");
  Serial.println(color_1);

  Serial.println("----------");
  //delay(100);
}


