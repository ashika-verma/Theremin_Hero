import numpy as np
import soundfile as sf

path = "/var/jail/home/kgarner/"

#adapted from https://stackoverflow.com/questions/8299303/generating-sine-wave-sound-in-python
fs = 44100       # sampling rate, Hz, must be integer
block_length = 1 # in seconds, may be float


def create_sample(frequency: float, start: float, end: float):
  return np.sin(2 * np.pi * np.arange(start, end) * frequency / fs)


def create_song(notes: list):
  song = []
  last_angle, start = 0.0, 0.0

  for note in notes:

    freq, dur = note.split(",")

    freq = float(freq)
    dur = int(dur)

    start = (fs * last_angle) / (2 * np.pi * freq) + 1
    end = start + block_length * fs * dur
    sample = create_sample(freq, start, end)

    song.append(sample)

    sin1, sin2 = sample[-1], sample[-2]
    last_angle = np.arcsin(sin1) if sin1 >= sin2 else np.pi - np.arcsin(sin1)

  return np.concatenate(song)


def write_song_from_string(name: str, song: str):
  notes = song.split(";")
  song = create_song(notes)
  sf.write(path + name + ".ogg", song, 44100, format="OGG")


if __name__ == "__main__":
  # write_song_from_string("song", "290,1;310,1;330,1;350,1;370,1;390,1;410,1")
  pass
