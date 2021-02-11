/*
 * Copyright (c) 2020 Abel Sensors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <power/power.h>
#include <soc.h>
#include <init.h>

#include <stm32_ll_cortex.h>
#include <stm32_ll_pwr.h>
#include <stm32_ll_system.h>
#include <stm32_ll_bus.h>

#include <logging/log.h>

LOG_MODULE_DECLARE(soc, CONFIG_SOC_LOG_LEVEL);

/**
 * @brief Put processor into a power state.
 *
 * This function implements the SoC specific details necessary
 * to put the processor into available power states.
 */
void pm_power_state_set(struct pm_state_info info)
{
	if (info.state != PM_STATE_SUSPEND_TO_IDLE) {
			LOG_DBG("Unsupported power state %u", info.state);
			return;
	}

	switch (info.substate_id) {
	case 0:
		/* this corresponds to the STOP mode: */

#ifdef CONFIG_DEBUG
		/* Enable the Debug Module during STOP mode */
		LL_DBGMCU_EnableDBGStopMode();
#endif /* CONFIG_DEBUG */

		/** Request to enter STOP mode
		* Following procedure described in STM32L1xx Reference Manual
		* See PWR part, section Low-power modes, STOP mode
		*/

		/* Enable ultra low power mode */
		LL_PWR_EnableUltraLowPower();

		/** Set the regulator to low power before setting MODE_STOP.
		* If the regulator remains in "main mode",
		* it consumes more power without providing any additional feature. */
		LL_PWR_SetRegulModeLP(LL_PWR_REGU_LPMODES_LOW_POWER);

		/* Set STOP mode when CPU enters deepsleep */
		LL_PWR_SetPowerMode(LL_PWR_MODE_STOP);

		/* Set SLEEPDEEP bit of Cortex System Control Register */
		LL_LPM_EnableDeepSleep();

		/* enter SLEEP mode : WFE or WFI */
		k_cpu_idle();
		break;

	default:
		LOG_DBG("Unsupported power state substate-id %u",
			info.substate_id);
		break;
	}
}

/* Handle SOC specific activity after Low Power Mode Exit */
void pm_power_state_exit_post_ops(struct pm_state_info info)
{
	if (info.state != PM_STATE_SUSPEND_TO_IDLE) {
		LOG_DBG("Unsupported power substate-id %u", info.state);
	} else {
		switch (info.substate_id) {
		case 0:	/* STOP */

			LL_LPM_DisableSleepOnExit();
		default:
			LOG_DBG("Unsupported power substate-id %u",
				info.substate_id);
			break;
		}
	}

	/*
	 * System is now in active mode.
	 * Reenable interrupts which were disabled
	 * when OS started idling code.
	 */
	irq_unlock(0);
}


/* Initialize STM32 Power */
static int stm32_power_init(const struct device *dev)
{
	unsigned int ret;

	ARG_UNUSED(dev);

	ret = irq_lock();

	/* enable Power clock */
	LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_PWR);

	irq_unlock(ret);

	return 0;
}

SYS_INIT(stm32_power_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
