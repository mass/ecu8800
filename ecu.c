#include "ecu.h"

int main(void)
{
  if (init_flash_option_bytes() ||
      0 /*TODO: More inits*/)
    return 1; //TODO: Proper failure state

  //TODO: Everything below here...
  // - SYSCFG_CLK_EN?
  // - SystemCoreClockUpdate
  // - RAMECC?
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
	  for (int i = 0; i < 1000; ++i);
  }
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
