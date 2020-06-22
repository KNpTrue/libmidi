/**
 * Copyright (c) 2020 KNpTrue
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/
#ifndef __MIDI_H_
#define __MIDI_H_

/**
 * Midi interface.
*/

#include <midi_config.h>

#ifdef	__cplusplus
extern "C" {
#endif /* __cplusplus */

/**
 * @brief MIDI interface.
*/
struct midi_if;

/**
 * @enum Type of MIDI interface.
*/
enum midi_if_type {
    MIDI_IF_TYPE_IN,        /**< MIDI in */
    MIDI_IF_TYPE_OUT,       /**< MIDI out */

    MIDI_IF_TYPE_COUNT,     /**< count of MIDI types */
};

/*
 * As noted elsewhere, libmidi is event-driven, and handles received data
 * via a callback mechanism. The enumeration values of event_type describe
 * the different types of events for which callers may register.
*/
enum midi_event {
    /*
     * System real-time messages.
    */
    EVT_SYS_REALTIME_MIN = 0,
    EVT_SYS_REALTIME_TIMING_CLOCK = EVT_SYS_REALTIME_MIN,
    EVT_SYS_REALTIME_RESERVED_F9,
    EVT_SYS_REALTIME_SEQ_START,
    EVT_SYS_REALTIME_SEQ_CONTINUE,
    EVT_SYS_REALTIME_SEQ_STOP,
    EVT_SYS_REALTIME_RESERVED_FD,
    EVT_SYS_REALTIME_ACTIVE_SENSE,
    EVT_SYS_REALTIME_RESET,
    EVT_SYS_REALTIME_MAX = EVT_SYS_REALTIME_RESET,

    /*
     * Channel messages.
    */
    EVT_CHAN_MIN,
    EVT_CHAN_NOTE_OFF = EVT_CHAN_MIN,
    EVT_CHAN_NOTE_ON,
    EVT_CHAN_POLY_AFTERTOUCH,
    EVT_CHAN_CONTROL_CHANGE,
    EVT_CHAN_PROGRAM_CHANGE,
    EVT_CHAN_AFTERTOUCH,
    EVT_CHAN_PITCH_BEND,
    EVT_CHAN_MAX = EVT_CHAN_PITCH_BEND,

    /*
     * Placeholder whose value is equal to the total number of event
     * that we support above.
    */
    EVT_COUNT
};

/**
 * @brief APIs depended on by libmidi are passed to libmidi during initialization.
*/
struct midi_func {
    void (*memset)(void *s, int c, unsigned int n);
    void *(*memcpy)(void *dest, const void *src, unsigned int n);
};

/**
 * @brief MIDI channel message to report.
*/
struct midi_chan_msg {
    unsigned char chan;     /**< MIDI channel */
    unsigned char data[2];  /**< Data bytes */ 
};

typedef void (*midi_in_event_cb_t)(struct midi_if*, enum midi_event, void *arg);
typedef void (*midi_out_send_cb_t)(void *buf, unsigned int len, void *arg);

/**
 * @brief Initialize MIDI.
 *
 * @param midi_func is a point to struct midi_func
*/
void midi_init(struct midi_func *midi_func);

/**
 * @brief Create a MIDI interface.
 *
 * @param type is an enumeration of MIDI interface types
 * @return a point to MIDI interface
 *
 * @see midi_if_destory
*/
struct midi_if *midi_if_create(enum midi_if_type type);

/**
 * @brief Destory a MIDI interface.
 *
 * @param pif is a point to MIDI interface
 *
 * @see midi_if_create
*/
void midi_if_destory(struct midi_if *pif);

/**
 * @brief Register event callback to MIDI IN interface.
 *
 * @param pif is a point to MIDI IN interface
 * @param cb is an event callback.
*/
void midi_if_in_register_event_cb(struct midi_if *pif, midi_in_event_cb_t cb);

/**
 * @brief Register send callback to MIDI OUT interface.
 *
 * @param pif is a point to MIDI OUT interface
 * @param cb is a send callback
 * @param arg is the parameter of the send callback
*/
void midi_if_out_register_send_cb(struct midi_if *pif, midi_out_send_cb_t cb, void *arg);

/**
 * @brief Analysis MIDI IN data stream to trigger MIDI events.
 *
 * @param pif is a point to MIDI IN interface
 * @param data is the data to analysis
 * @param len is the length of the data
*/
void midi_in_recv(struct midi_if *pif, void *data, unsigned int len);

/**
 * @brief Send event to MIDI OUT interface.
 *
 * @param pif is a point to MIDI OUT interface
 * @param evt is the event to report
 * @param [...]
 *  when evt >= EVT_CHAN_MIN && evt <= EVT_CHAN_MAX, the argument is a point to struct midi_chan_msg
 * @return 0 on success, or -1 on failed
*/
int midi_out_report_event(struct midi_if *pif, enum midi_event evt, ...);

#ifdef	__cplusplus
}
#endif /* __cplusplus */

#endif /* __MIDI_H_ */
