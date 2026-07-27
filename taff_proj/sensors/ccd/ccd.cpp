// 线性 CCD (TSL1401CL) 时序驱动 + ADC 采集
// 时序参考 WHEELTEC TSL1401 模块手册 / 数据手册:
//   SI 上升沿锁存(Hold), 之后每个 CLK 上升沿移出一个像素, 第129个时钟结束本帧
#include "sensors/ccd/ccd.h"
#include "ti_msp_dl_config.h"

namespace sensors::ccd {
namespace {

uint8_t  g_pix[kPixels] = {};
uint32_t g_expose = 30;   // 每像素曝光延时单位(可调, 见 set_exposure)

constexpr uint32_t kSetup = 8;   // SI/CLK 建立/保持延时单位
constexpr uint32_t kAdcTimeout = 20000;

// 微秒级忙等延时
// WHY: TSL1401 时序需要 us 级节拍, SysTick 只有 ms 精度, 故用空循环粗延时
inline void dly(volatile uint32_t n) {
    while (n--) { __asm volatile("nop"); }
}

// SI / CLK 电平控制(封装 DriverLib GPIO)
inline void si(bool level) {
    if (level) DL_GPIO_setPins(CCD_IO_PORT, CCD_IO_SI_PIN);
    else       DL_GPIO_clearPins(CCD_IO_PORT, CCD_IO_SI_PIN);
}
inline void clk(bool level) {
    if (level) DL_GPIO_setPins(CCD_IO_PORT, CCD_IO_CLK_PIN);
    else       DL_GPIO_clearPins(CCD_IO_PORT, CCD_IO_CLK_PIN);
}

bool adc_read(uint16_t& out) {
    DL_ADC12_startConversion(CCD_ADC_INST);
    uint32_t guard = kAdcTimeout;
    while (!(DL_ADC12_getRawInterruptStatus(
                 CCD_ADC_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED))) {
        if (guard-- == 0U) {
            DL_ADC12_enableConversions(CCD_ADC_INST);
            return false;
        }
    }
    out = DL_ADC12_getMemResult(CCD_ADC_INST, CCD_ADC_ADCMEM_0);
    DL_ADC12_clearInterruptStatus(
        CCD_ADC_INST, DL_ADC12_INTERRUPT_MEM0_RESULT_LOADED);
    DL_ADC12_enableConversions(CCD_ADC_INST);
    return true;
}

}  // namespace

void set_exposure(uint32_t units) { g_expose = units; }

void init() {
    DL_ADC12_enableConversions(CCD_ADC_INST);
    clk(true);   // 空闲态 CLK 高
    si(false);
}

const uint8_t* scan() {
    uint8_t frame[kPixels];

    // === 启动时序: 在 SI 上打一个脉冲锁存, 随后 128 个 CLK 移出像素 ===
    clk(true);  si(false); dly(kSetup);
    si(true);   clk(false); dly(kSetup);   // SI=1, CLK 下降沿 -> 锁存
    clk(true);  si(false); dly(kSetup);    // 下一个上升沿前把 SI 拉低

    for (int i = 0; i < kPixels; ++i) {
        clk(false);          // CLK 低电平期间 AO 输出当前像素电压, 采样
        dly(g_expose);       // 曝光/积分停留
        uint16_t v = 0;
        if (!adc_read(v)) return g_pix;
        frame[i] = static_cast<uint8_t>(v >> 4);  // 12bit -> 8bit(0..255)
        clk(true);           // 上升沿移位到下一像素
        dly(kSetup);
    }
    for (int i = 0; i < kPixels; ++i) g_pix[i] = frame[i];
    return g_pix;
}

const uint8_t* data() { return g_pix; }

}  // namespace sensors::ccd
