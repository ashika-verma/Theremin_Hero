import numpy as np
import soundfile as sf

path = "/var/jail/home/kgarner/"

#adapted from https://stackoverflow.com/questions/8299303/generating-sine-wave-sound-in-python
fs = 44100       # sampling rate, Hz, must be integer
block_length = .1 # in seconds, may be float


def create_sample(frequency: float, start: float, end: float):
  """
  Creates a sine wave at the frequency, frequency, with inputs
  in the range [start, end]

  Arguments:
    - frequency (float): the frequency for this start wave
    - start (float): the starting input (x) for the wave
    - end (float): the ending x for this wave
  """
  return np.sin(2 * np.pi * np.arange(start, end) * frequency / fs)


def create_song(notes: list):
  """
  Creates a new song as a numpy array from a notes list. 
  This notes list is in the following format:

  notes ::= (INT "," INT)*
  INT ::= [0-9]+

  The first integer is a frequency, and the second corresponds
  to its duration in 100ms intervals.
  """
  song = []
  last_angle, start = 0.0, 0.0

  for note in notes:

    freq, dur = note.split(",")
  
    freq = float(freq)
    dur = int(dur)

    if freq == 0:
      continue

    start = (fs * last_angle) / (2 * np.pi * freq) + 1
    end = start + block_length * fs * dur
    sample = create_sample(freq, start, end)

    song.append(sample)

    sin1, sin2 = sample[-1], sample[-2]
    last_angle = np.arcsin(sin1) if sin1 >= sin2 else np.pi - np.arcsin(sin1)

  return np.concatenate(song)


def write_song_from_string(name: str, song: str):
  """
  Creates an OGG file from a notes string. 

  Arguments:
    - name (string): the name of the file to be written
    - song (string): the song contents
  """
  if song.endswith(";"):
    song = song[:-1]
    
  notes = song.split(";")
  song = create_song(notes)
  sf.write(path + name + ".ogg", song, 44100, format="OGG")

