# -*- coding: utf-8 -*-
"""Get ALSA card and device list."""

import re

from io import open
from collections import namedtuple
from functools import partial
from os.path import exists

__all__ = (
    'AlsaCardInfo',
    'AlsaPcmInfo',
    'get_cards',
    'get_capture_devices',
    'get_playback_devices',
    'get_pcm_devices'
)

PROC_CARDS = '/proc/asound/cards'
PROC_DEVICES = '/proc/asound/pcm'

AlsaCardInfo = namedtuple('AlsaCardInfo', 'card_num id name')
AlsaPcmInfo = namedtuple('AlsaPcmInfo',
                         'card_num dev_num id name playback capture')


def get_cards():
    """Get card info from /proc/asound/cards."""

    if not exists(PROC_CARDS):
        raise IOError("'%s' does not exist. ALSA not loaded?" % PROC_CARDS)

    with open(PROC_CARDS, 'r', encoding='utf-8') as procfile:
        # capture card number, id and name
        cardline = re.compile(
            r'^\s*(?P<num>\d+)\s*'  # card number
            r'\[(?P<id>\w+)\s*\]:'  # card ID
            r'.*-\s(?P<name>.*)$')  # card name

        for line in procfile:
            match = cardline.match(line)

            if match:
                yield AlsaCardInfo(card_num=int(match.group('num')),
                                   id=match.group('id').strip(),
                                   name=match.group('name').strip())


def get_pcm_devices():
    """Get PCM device numbers and names from /proc/asound/pcm."""

    if not exists(PROC_DEVICES):
        raise IOError("'%s' does not exist. ALSA not loaded?" % PROC_DEVICES)

    with open(PROC_DEVICES, 'r', encoding='utf-8') as procfile:
        devnum = re.compile(r'(?P<card_num>\d+)-(?P<dev_num>\d+)')

        for line in procfile:
            fields = [l.strip() for l in line.split(':')]

            if len(fields) >= 3:
                match = devnum.match(fields[0])

                if match:
                    yield AlsaPcmInfo(card_num=int(match.group('card_num')),
                                      dev_num=int(match.group('dev_num')),
                                      id=fields[1],
                                      name=fields[2],
                                      playback='playback 1' in fields,
                                      capture='capture 1' in fields)


def get_devices(playback=True, capture=True):
    """Return iterable of (device string, card name, device name) tuples."""
    cards = {c.card_num: c for c in get_cards()}

    for dev in get_pcm_devices():
        card = cards[dev.card_num]
        if (playback and dev.playback) or (capture and dev.capture):
            yield ("hw:%s,%i" % (card.id, dev.dev_num), card.name, dev.name)


get_playback_devices = partial(get_devices, capture=False)
get_capture_devices = partial(get_devices, playback=False)
