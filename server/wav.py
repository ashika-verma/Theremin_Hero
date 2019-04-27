import numpy as np
from scipy.io import wavfile
from scipy.signal import resample

#adapted from https://stackoverflow.com/questions/8299303/generating-sine-wave-sound-in-python
volume = 0.5     # range [0.0, 1.0]
fs = 44100       # sampling rate, Hz, must be integer
duration = 1.0   # in seconds, may be float
f = 430.0     # sine frequency, Hz, may be float

# generate samples, note conversion to float32 array
start=fs*duration

samples = (np.sin(2*np.pi*np.arange(fs*duration)*f/fs)).astype(np.float32)

volume = 0.5     # range [0.0, 1.0]
fs = 44100       # sampling rate, Hz, must be integer
duration = 1.0   # in seconds, may be float
f = 440.0        # sine frequency, Hz, may be float

samples2 =  (np.sin(2*np.pi*np.arange(start, start + fs*duration)*f/fs)).astype(np.float32)

print(samples)
print(samples2)
print(samples + samples2)

wavfile.write('output2.wav', 44100, samples2)
wavfile.write('output.wav', 44100, samples)
wavfile.write('output3.wav', 44100, samples + samples2)