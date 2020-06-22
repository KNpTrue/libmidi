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
#include <midi.h>
#include <stdarg.h>

#define NULL    ((void *)0)

enum midi_event_type {
    EVT_TYPE_CHANNEL,
    EVT_TYPE_SYS_REALTIME,
    EVT_TYPE_SYS_COMMON,
    EVT_TYPE_UNKNOWN,
};

static struct midi_func gv_midi_func;

static struct midi_if {
    enum midi_if_type type;
    union {
        midi_in_event_cb_t in_event_cb;
        midi_out_send_cb_t out_send_cb;
    } cb;
    void *arg;

    int index;
} gv_ifs[MIDI_IF_COUNT_MAX];

static const unsigned char event_status_base[] = {
    /**
     * Status code of the system real-time message
    */
    [EVT_SYS_REALTIME_TIMING_CLOCK] = 0xf8, /* 11111000 */
    [EVT_SYS_REALTIME_RESERVED_F9] = 0xf9,  /* 11111001 */
    [EVT_SYS_REALTIME_SEQ_START] = 0xfa,    /* 11111010 */
    [EVT_SYS_REALTIME_SEQ_CONTINUE] = 0xfb, /* 11111011 */
    [EVT_SYS_REALTIME_SEQ_STOP] = 0xfc,     /* 11111000 */
    [EVT_SYS_REALTIME_RESERVED_FD] = 0xfd,  /* 11111101 */
    [EVT_SYS_REALTIME_ACTIVE_SENSE] = 0xfe, /* 11111110 */
    [EVT_SYS_REALTIME_RESET] = 0xff,        /* 11111111 */
    /**
     * Status code of the channel message
    */
    [EVT_CHAN_NOTE_OFF] = 0x80,             /* 1000nnnn */
    [EVT_CHAN_NOTE_ON] = 0x90,              /* 1001nnnn */
    [EVT_CHAN_POLY_AFTERTOUCH] = 0xa0,      /* 1010nnnn */
    [EVT_CHAN_CONTROL_CHANGE] = 0xb0,       /* 1011nnnn */
    [EVT_CHAN_PROGRAM_CHANGE] = 0xc0,       /* 1100nnnn */
    [EVT_CHAN_AFTERTOUCH] = 0xd0,           /* 1101nnnn */
    [EVT_CHAN_PITCH_BEND] = 0xe0,           /* 1110nnnn */
};

void midi_init(struct midi_func *midi_func)
{
    int i;

    MIDI_ASSERT(midi_func);
    MIDI_ASSERT(midi_func->memset);
    MIDI_ASSERT(midi_func->memcpy);

    for (i = 0; i < MIDI_IF_COUNT_MAX; i++) {
        gv_ifs[i].index = -1;
    }

    midi_func->memcpy(&gv_midi_func,
        midi_func, sizeof(struct midi_func));
}

struct midi_if *midi_if_create(enum midi_if_type type)
{
    int i;
    struct midi_if *pif;

    MIDI_ASSERT(type >= MIDI_IF_TYPE_IN && type < MIDI_IF_COUNT_MAX);

    for (i = 0; i < MIDI_IF_COUNT_MAX; i++) {
        pif = &gv_ifs[i];
        if (pif->index == -1) {
            gv_midi_func.memset(pif, 0, sizeof(struct midi_if));
            pif->type = type;
            pif->index = i;
            return pif;
        }
    }
    return NULL;
}

void midi_in_register_event_cb(struct midi_if *pif, midi_in_event_cb_t cb)
{
    MIDI_ASSERT(pif);
    MIDI_ASSERT(pif->type == MIDI_IF_TYPE_IN);
    MIDI_ASSERT(cb);

    pif->cb.in_event_cb = cb;
}

void midi_out_register_send_cb(struct midi_if *pif, midi_out_send_cb_t cb, void *arg)
{
    MIDI_ASSERT(pif);
    MIDI_ASSERT(pif->type == MIDI_IF_TYPE_OUT);
    MIDI_ASSERT(cb);

    pif->cb.out_send_cb = cb;
    pif->arg = arg;
}

static enum midi_event_type midi_event_get_type(enum midi_event evt)
{
    if (evt >= EVT_CHAN_MIN && evt <= EVT_CHAN_MAX) {
        return EVT_TYPE_CHANNEL;
    } else if (evt >= EVT_SYS_REALTIME_MIN && evt <= EVT_SYS_REALTIME_MAX) {
        return EVT_TYPE_SYS_REALTIME;
    } else {
        return EVT_TYPE_UNKNOWN;
    }
}

void midi_destory(struct midi_if *pif)
{
    if (!pif || pif != &gv_ifs[pif->index]) {
        return;
    }
    pif->index = -1;
}

static int midi_chan_msg_is_valid(struct midi_chan_msg *msg)
{
    return msg->chan > 0 && msg->chan < 17 &&
        !(msg->data[0] >> 7) && !(msg->data[1] >> 7);
}

int midi_out_report_event(struct midi_if *pif, enum midi_event evt, ...)
{
    int ret = 0;
    va_list ap;
    unsigned char buf[3];
    enum midi_event_type evt_type;
    struct midi_chan_msg *chan_msg;

    MIDI_ASSERT(pif);
    MIDI_ASSERT(pif == &gv_ifs[pif->index]);
    MIDI_ASSERT(pif->type == MIDI_IF_TYPE_OUT);
    MIDI_ASSERT(pif->cb.out_send_cb);

    evt_type = midi_event_get_type(evt);

    switch (evt_type) {
    case EVT_TYPE_CHANNEL:
        va_start(ap, evt);
        chan_msg = va_arg(ap, struct midi_chan_msg*);
        va_end(ap);
        if (!chan_msg || !midi_chan_msg_is_valid(chan_msg)) {
            ret = -1;
            break;
        }
        buf[0] = event_status_base[evt] | (chan_msg->chan - 1);
        buf[1] = chan_msg->data[0];
        buf[2] = chan_msg->data[1];
        pif->cb.out_send_cb(buf, 3, pif->arg);
        break;
    case EVT_TYPE_SYS_REALTIME:
        buf[0] = event_status_base[evt];
        pif->cb.out_send_cb(buf, 1, pif->arg);
        break;
    case EVT_TYPE_SYS_COMMON:
        /* TODO: Need to implement. */
    default:
        ret = -1;
        break;
    }
    return ret;
}

int midi_out_report_note(struct midi_if *pif, char chan, char onoff, char note, char v)
{
    struct midi_chan_msg msg;

    MIDI_ASSERT(pif);
    MIDI_ASSERT(chan >= 1 && chan <= 16);
    MIDI_ASSERT(onoff == 0 || onoff == 1);
    MIDI_ASSERT(note >= 0);
    MIDI_ASSERT(v >= 0);
    msg.chan = chan;
    msg.data[0] = note;
    msg.data[1] = v;
    return midi_out_report_event(pif, onoff ? EVT_CHAN_NOTE_ON : EVT_CHAN_NOTE_OFF, &msg);
}

int midi_out_report_control_change(struct midi_if *pif, char chan, char ctrl, char v)
{
    struct midi_chan_msg msg;

    MIDI_ASSERT(pif);
    MIDI_ASSERT(chan >= 1 && chan <= 16);
    MIDI_ASSERT(ctrl >= 0);
    MIDI_ASSERT(v >= 0);
    msg.chan = chan;
    msg.data[0] = ctrl;
    msg.data[1] = v;
    return midi_out_report_event(pif, EVT_CHAN_CONTROL_CHANGE, &msg);
}
