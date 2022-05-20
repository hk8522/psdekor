/*
 * buzzer.h
 *
 * Created: 19.11.2013 15:13:28
 *  Author: huber
 */


#ifndef BUZZER_H_
#define BUZZER_H_

#define FADE_DIV_1_gc           0
#define FADE_DIV_2_gc           1
#define FADE_DIV_4_gc           2
#define FADE_DIV_8_gc           3
#define FADE_DIV_MASK_gc        3

#define FADE_ENABLE_bp          2
#define FADE_ENABLE_bm          (1 << FADE_ENABLE_bp)
#define FADE_DIRECTION_bp       3
#define FADE_IN_gc              (FADE_ENABLE_bm)
#define FADE_OUT_gc             (FADE_ENABLE_bm | (1 << FADE_DIRECTION_bp))
#define FADE_CTRL_MASK_gc       (FADE_ENABLE_bm | (1 << FADE_DIRECTION_bp))
#define FADE_DISABLED_gc        0x00

struct tone_setting {
	char key;
	u8 divider;
	u16 per;
	u16 repetitions; /**< Number of square waves that will be played */
	u16 ccreg;
	u16 fade_step;
	u8 control;
};

/*!
 * \brief Initializes the buzzer sub-system
 * \param tone Array of tone_setting structs that define all tones
 *             that can be played
 * \param size Number of entries in tone.
 * \return ERR_REQUEST if buzzer is already running or frequency is too low.
 *         ERR_PARAM if parameters are illegal
 *         ERR_NONE if buzzer sub-system has been initialized successfully.
 */
extern s8 buzzerInitialize (struct tone_setting tone[], const u8 size);

/*!
 * \brief Starts to playback the specified sequence
 * \param sequence The sequence to play back (string of symbols
 *                 defined at initialization). Illegal symbols will
 *                 be replaced by dummy_tone (silence).
 * \param repeat Indicates if the current sequence should be repeated.
 *               Currently this is not supported and will therefore be
 *               ignored.
 * \return ERR_REQUEST if buzzer sub.-system has not been initialized.
 *         ERR_INTERNAL if the sub-system is in some invalid state.
 *         ERR_PARAM if sequence is NULL.
 *         ERR_NONE if playback has been initialized.
 *
 * If a playback is currently running, it will be stopped and
 * the new sequence will be started.
 */
extern s8 buzzerStart (const char *sequence, bool_t repeat);

/*!
 * \brief Stops a running playback
 * \return ERR_REQUEST if buzzer sub-system has not been initialized.
 *         ERR_NONE if buzzer sub-system has been stopped.
 *
 * It's safe to call this functions after playback has stopped or
 * or even if playback has never been started!
 */
extern s8 buzzerStop (void);

/*!
 * \brief Blocks CPU until buzzer is finished playing current sequence
 *
 * It's safe to call this function if playback is not running or
 * has never been started. It will then just return immediately.
 */
extern void buzzerWaitTillFinished (void);

/*!
 * \brief Checks if buzzer is currently running.
 * \return TRUE if buzzer is still running (playback) else FALSE.
 */
extern bool_t buzzerIsRunning (void);


#endif /* BUZZER_H_ */
