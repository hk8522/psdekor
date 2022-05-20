/*
 * buzzer.c
 *
 * This buzzer module only supports output frequencies in the
 * range 200Hz <= f <= 7kHz; peripheral clock must be 2MHz or higher
 * Created: 19.11.2013 15:13:19
 *  Author: huber
 */

#include "platform.h"
#include "buzzer/buzzer.h"
#include "utils/debug.h"

static volatile enum buz_state_e {
	BUZZER_UNINITIALIZED,
	BUZZER_STOPPED,
	BUZZER_RUNNING,
	BUZZER_STOPPING
}buz_state = BUZZER_UNINITIALIZED;

static struct tone_setting *current = NULL;
static volatile u8 number_of_tones = 0;
static volatile u16 repetitions = 0;
static volatile const char *current_sequence = NULL;
static volatile const char *current_tone = NULL;
static volatile u8 repeat_tone = 0;
static volatile u16 fading_step = 0;
static volatile u16 ccreg_buf = 0;
static volatile u8 control = 0;
static volatile u8 fade_divider = 0;

#define REPEAT_TONE_bp 2
#define REPEAT_TONE_bm (1 << REPEAT_TONE_bp)

static struct tone_setting dummy_tone = {
	.key = 0,
	.divider = TC_CLKSEL_OFF_gc,
	.ccreg = U16_C(200),
	.per = U16_C(100),
	.repetitions = 0,
	.fade_step = 0,
	.control = FADE_DISABLED_gc
};

/*!
 * \brief Tries to find a tone by it's key
 * \param key Key identifier to search for.
 * \param tone pointer to address of the tone entry that
 *             has been looked for.
 * \return ERR_NOTFOUND if tone settings couldn't be found.
 *         ERR_REQUEST if buzzer sub-system has not been
 *         initialized.
 *         ERR_NONE if tone settings could be found.
 *
 * On error, the tone pointer will not be modified.
 */
static s8 find_tone (char key, struct tone_setting **tone);

/*!
 * \brief Prepares buzzer module to play next tone
 * This function prepares the next tone to be played. If there
 * is some error (invalid sequence) a dummy tone will be used
 * (no sound) and playback will continue until finished.
 */
static void play_next_tone (void);

ISR(TCD1_CCA_vect)
{
	/* HINT: Capture Compare interrupt
	 *       This is a bad workaround - usually capture compare unit
	 *       should directly drive the pin (which should use a tiny
	 *       FET/transistor to drive loads (as the buzzer). However, since
	 *       this hasn't been done, CPU is necessary to toggle the pin.
	 *       There is no way to check which edge this is; If interrupts are
	 *       disabled too long, interrupts could be missed, which produces
	 *       frequency components (hearable!!!).
	 *       These problems are least propable if the compare match value is
	 *       half of the top value.
	 *       THIS IS A HARDWARE ISSUE!!!
	 */
	DLOG("Should not occure!\r\n");
}

ISR(TCD1_OVF_vect)
{
	if (repetitions != 0) {
		/* Repeat tone a number of times (configuration) */
		--repetitions;
		switch(control & FADE_CTRL_MASK_gc) {
		case FADE_IN_gc:
			ccreg_buf += fading_step;
			break;
		case FADE_OUT_gc:
			ccreg_buf -= fading_step;
			break;
		}
		TCD1.CCABUF = (ccreg_buf >> fade_divider);
	} else {
		/* Stop repeating and check if there is another tone
		 * else quit */
		play_next_tone();

		if (repetitions == 0) {
			buz_state = BUZZER_STOPPED;
		}
	}
}

s8 buzzerInitialize (struct tone_setting tone[], const u8 size)
{
	u32 clk;

	switch (buz_state) {

	case BUZZER_UNINITIALIZED:
		break;

	case BUZZER_RUNNING:
	case BUZZER_STOPPED:
	case BUZZER_STOPPING:
	default:
		return ERR_REQUEST;
	}

	sysclk_enable_peripheral_clock (&TCD1);
	clk = sysclk_get_peripheral_bus_hz (&TCD1);

	if (unlikely ((tone == NULL) || size == 0))
		return ERR_PARAM;

	if (clk < U32_C(2000000))
		return ERR_REQUEST;

	current = tone;
	number_of_tones = size;

	TCD1.CTRLA = TC_CLKSEL_OFF_gc;                          /* Timer OFF */
	TCD1.CTRLB = TC_WGMODE_DSBOTTOM_gc | TC1_CCAEN_bm;      /* CCAEN = true, WGMODE = DSBOTTOM */
	TCD1.CTRLC = 0x00;                                      /* CMPx = 0 */
	TCD1.CTRLD = 0x00;                                      /* EVACT = OFF, EVDLY = 0, EVSEL = OFF */
	TCD1.CTRLE = 0x00;                                      /* BYTEM = NORMAL */
	TCD1.INTCTRLA = TC_OVFINTLVL_MED_gc;                    /* ERRINTLVL = OFF, OVFINTLVL = MED */
	TCD1.INTCTRLB = 0x00;                                   /* CCAINTLVL = HIGH */
	TCD1.CNT = U16_C(0);
	TCD1.PER = U16_C(0);
	TCD1.CCA = U16_C(0);

	buz_state = BUZZER_STOPPED;

	return ERR_NONE;
}

static s8 find_tone (char key, struct tone_setting **tone)
{
	u8 i;

	if (buz_state == BUZZER_UNINITIALIZED)
		return ERR_REQUEST;

	for (i = 0; i < number_of_tones; ++i) {
		if (key == current[i].key) {
			*tone = &current[i];
			return ERR_NONE;
		}
	}

	return ERR_NOTFOUND;
}

static void play_next_tone (void)
{
	static struct tone_setting *tone = &dummy_tone;
	char key = *current_tone;

	if (key == tone->key) {
		IRQ_INC_DISABLE();
		repetitions = tone->repetitions;
		TCD1.CCABUF = tone->ccreg;
		ccreg_buf = (tone->ccreg << fade_divider);
		IRQ_DEC_ENABLE();
		++current_tone;
	} else if (key != 0) {
		find_tone (key, &tone);

		IRQ_INC_DISABLE();
		control = tone->control;
		fading_step = tone->fade_step;
		fade_divider = (control & FADE_DIV_MASK_gc);
		TCD1.PERBUF = tone->per;
		TCD1.CCABUF = tone->per;
		ccreg_buf = (tone->ccreg << fade_divider);
		TCD1.CTRLA = tone->divider;
		repetitions = tone->repetitions;
		TCD1.INTFLAGS |= TC1_OVFIF_bm; /* Reset interrupt flag */
		IRQ_DEC_ENABLE();
		++current_tone;
	} else if (repeat_tone) {
		current_tone = current_sequence;
		/* Just to be sure that there are no infinite call stacks... */
		repeat_tone = 0;
		play_next_tone();
		repeat_tone = 1;
	} else {
		buz_state = BUZZER_STOPPING;
	}

	if (buz_state != BUZZER_RUNNING) {
		IRQ_INC_DISABLE();
		TCD1.CTRLA = TC_CLKSEL_OFF_gc;
		tone = &dummy_tone;

		TCD1.INTFLAGS |= TC1_OVFIF_bm; /* Reset interrupt flag */
		IRQ_DEC_ENABLE();

		buz_state = BUZZER_STOPPED;
	}
}

s8 buzzerStart (const char *sequence, bool_t repeat)
{
	if (unlikely(sequence == 0)) {
		return ERR_PARAM;
	}

	switch (buz_state) {
	case BUZZER_RUNNING:
		buzzerStop ();
	case BUZZER_STOPPING:
		while (buz_state != BUZZER_STOPPED) {}
	case BUZZER_STOPPED:
		current_tone = sequence;
		current_sequence = sequence;
		buz_state = BUZZER_RUNNING;
		repeat_tone = repeat ? 1 : 0;
		play_next_tone ();
		return ERR_NONE;

	case BUZZER_UNINITIALIZED:
	default:
		return ERR_REQUEST;
	}

	return ERR_INTERNAL;
}

s8 buzzerStop (void)
{
	s8 err;

	switch (buz_state) {
	case BUZZER_RUNNING:
		IRQ_INC_DISABLE();
		repetitions = 0;
		repeat_tone = 0;
		buz_state = BUZZER_STOPPING;
		IRQ_DEC_ENABLE();
	case BUZZER_STOPPING:
		/* busy waiting */
		while (buz_state == BUZZER_STOPPING) {}
	case BUZZER_STOPPED:
		err = ERR_NONE;
		break;

	case BUZZER_UNINITIALIZED:
	default:
		err = ERR_REQUEST;
	}

	return err;
}

void buzzerWaitTillFinished (void)
{
	switch (buz_state) {
	case BUZZER_RUNNING:
		cpu_irq_disable();
		while (buz_state == BUZZER_RUNNING) {
			SLEEP_CPU_LOCKED();
			cpu_irq_disable();
		}
		cpu_irq_enable();
	case BUZZER_STOPPING:
		cpu_irq_disable();
		while (buz_state == BUZZER_STOPPING) {
			SLEEP_CPU_LOCKED();
			cpu_irq_disable();
		}
		cpu_irq_enable();
	case BUZZER_STOPPED:
		break;

	case BUZZER_UNINITIALIZED:
	default:
		break;
	}
}

bool_t buzzerIsRunning (void)
{
	return (buz_state == BUZZER_RUNNING);
}
