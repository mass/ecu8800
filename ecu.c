#include "ecu.h"

int main(void)
{
  if (init_power() ||
      init_flash_option_bytes())
    return 1; //TODO: Proper failure state

  //TODO: Everything below here...
  // - FLASH?
  // - RAMECC?
  // - SMM?
  // - VREFBUF?
  //
  // - SYSCFG_CLK_EN?
  // - SystemCoreClockUpdate
  // - __attribute__((interrupt)) ??
  //
  // - PWR / LDO setup
  // - PWR REG voltage scale
  // - HSI clock setup
  // - Init all the cpu/ahb/apb/pll/etc. clocks
  // - MCO pin

  //GPIOA->MODER =-
  RCC->AHB4ENR |= RCC_AHB4ENR_GPIOAEN;
  //TODO: Wait for clock to stablize?
  for (int i = 0; i < 10000; ++i);

  GPIOA->MODER &= ~(0x3u << (9*2));
  GPIOA->MODER |=  (0x1u << (9*2));
  GPIOA->OSPEEDR &= ~(0x3u << (9*2));
  GPIOA->OSPEEDR |= (0x3u << (9*2));

  int j = 0;
  for (;;)
  {
    ++j;
    GPIOA->ODR ^= (1<<9);
    //for (int i = 0; i < 1000; ++i);
  }
}

int init_power(void)
{
  // Enable LDO & disable SMPS. The low byte of PWR_CR3 must be written in a
  // single operation, seemingly before anything else. 0x42 is the reset
  // value (0x46) with SDEN disabled.
  PWR->CR3 = 0x42;

  // Wait for voltage levels to stabilize, after which we will have entered
  // the normal run mode of the hardware.
  while (!(PWR->CSR1 & PWR_CSR1_ACTVOSRDY)) {}

  // Configure PWR registers
  {
    // PWR_CR1
    // ALS
    PWR->CR1 &= ~PWR_CR1_AVDEN;     // Analog voltage detector
    PWR->CR1 |=  PWR_CR1_SVOS;      // Stop mode voltage scaling
    PWR->CR1 &= ~PWR_CR1_FLPS;      // Flash low-power mode
    PWR->CR1 |=  PWR_CR1_DBP;       // Disable backup domain write protection
    // PLS
    PWR->CR1 &= ~PWR_CR1_PVDEN;     // Programmable voltage detector
    PWR->CR1 &= ~PWR_CR1_LPDS;      // Low-power deepsleep w/ SVOS3

    // PWR_CR2
    PWR->CR2 &= ~PWR_CR2_MONEN;     // Vbat & Temp monitoring
    PWR->CR2 &= ~PWR_CR2_BREN;      // Backup regulator

    // PWR_CR3
    PWR->CR3 &= ~PWR_CR3_USBREGEN;  // USB regulator
    PWR->CR3 &= ~PWR_CR3_USB33DEN;  // Vdd33usb voltage level detector
    // VBRS
    PWR->CR3 &= ~PWR_CR3_VBE;       // Vbat charging
    // SMPSLEVEL, SMPSEXTHP, SMPSEN, LDOEN, BYPASS already written

    // PWR_CPUCR
    PWR->CPUCR |=  PWR_CPUCR_RUN_D3;  // Keep D3 in run mode
    // CSSF
    PWR->CPUCR &= ~PWR_CPUCR_PDDS_D3; // D3 power down deepsleep
    PWR->CPUCR &= ~PWR_CPUCR_PDDS_D2; // D2 power down deepsleep
    PWR->CPUCR &= ~PWR_CPUCR_PDDS_D1; // D1 power down deepsleep

    // PWR_WKUPCR
    // PWR_WKUPFR
    // PWR_WKUPEPR
  }

  // Set CPU voltage scaling to highest power mode (0b00)
  PWR->D3CR &= ~PWR_D3CR_VOS; // Voltage scaling selection
  while (!(PWR->D3CR & PWR_D3CR_VOSRDY) ||
         !(PWR->CSR1 & PWR_CSR1_ACTVOSRDY));
  if (PWR->CSR1 & PWR_CSR1_ACTVOS)
    return 1;

  // Clock gate unused peripherals
  {
    //TODO
  }

  return 0;
}

int init_flash_option_bytes(void)
{
  // Unlock option bytes control register
  if (FLASH->OPTCR & FLASH_OPTCR_OPTLOCK) {
    FLASH->OPTKEYR = 0x08192A3B;
    FLASH->OPTKEYR = 0x4C5D6E7F;
    if (FLASH->OPTCR & FLASH_OPTCR_OPTLOCK)
      return 1;
  }

  // Set option PRGs

  uint32_t optsr = FLASH->OPTSR_CUR;
  optsr &= ~FLASH_OPTSR_IO_HSLV;
  optsr &= ~FLASH_OPTSR_SECURITY;
  //FLASH_OPTSR_FZ_IWDG_SDBY
  //FLASH_OPTSR_FZ_IWDG_STOP
  //FLASH_OPTSR_NRST_STBY_D1
  //FLASH_OPTSR_NRST_STOP_D1
  //FLASH_OPTSR_IWDG1_SW
  optsr &= ~FLASH_OPTSR_BOR_LEV;
  optsr |= FLASH_OPTSR_BOR_LEV_1;
  FLASH->OPTSR_PRG = optsr;

  // FLASH->PRAR_PRG1
  // FLASH->SCAR_PRG1
  // FLASH->WPSN_PRG1

  uint32_t boot
     = ((0x0800 << FLASH_BOOT_ADD0_Pos) & FLASH_BOOT_ADD0_Msk)
     | ((0x1FF0 << FLASH_BOOT_ADD1_Pos) & FLASH_BOOT_ADD1_Msk);
  FLASH->BOOT_PRG = boot;

  uint32_t optsr2 = FLASH->OPTSR2_CUR;
  optsr2 |= FLASH_OPTSR2_CPUFREQ_BOOST;
  optsr2 &= ~FLASH_OPTSR2_TCM_AXI_SHARED;
  FLASH->OPTSR2_PRG = optsr2;

  // Start option bytes write and wait for finish
  while (FLASH->SR1 & FLASH_SR_CRC_BUSY);
  FLASH->OPTCR |= FLASH_OPTCR_OPTSTART;
  while (FLASH->SR1 & FLASH_SR_BSY);

  // Re-lock option bytes control register
  FLASH->OPTCR |= FLASH_OPTCR_OPTLOCK;
  if (!(FLASH->OPTCR & FLASH_OPTCR_OPTLOCK))
    return 1;

  // Validate writes were successful
  if (optsr   != FLASH->OPTSR_CUR ||
      boot    != FLASH->BOOT_CUR ||
      optsr2  != FLASH->OPTSR2_CUR)
    return 1;

  return 0;
}
