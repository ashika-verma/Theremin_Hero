import numpy as np
import soundfile as sf

path = "/var/jail/home/kgarner/"

#adapted from https://stackoverflow.com/questions/8299303/generating-sine-wave-sound-in-python
fs = 44100       # sampling rate, Hz, must be integer
block_length = 1 # in seconds, may be float

def create_sample(frequency: float, duration: int):
  return np.sin(2 * np.pi * np.arange(0, fs * duration) * frequency / fs)

def create_song(notes: list):
  song = []

  for note in notes:
    freq, dur = note.split(",")

    freq = float(freq)
    dur = int(dur)

    song.append(create_sample(freq, dur))

  return np.concatenate(song)

def write_song_from_string(name: str, song: str):
  notes = song.split(";")
  song = create_song(notes)
  sf.write(path + name + ".ogg", song, 44100, format="OGG")
