import numpy as np
import soundfile as sf
from scipy.signal import blackmanharris, fftconvolve

# from: https://gist.github.com/endolith/255291

def freq_from_fft(sig, fs):
    """
    Estimate frequency from peak of FFT
    """
    # Compute Fourier transform of windowed signal
    windowed = sig * blackmanharris(len(sig))
    f = np.rfft(windowed)

    # Find the peak and interpolate to get a more accurate peak
    i = argmax(abs(f))  # Just use this for less-accurate, naive version
    true_i = parabolic(log(abs(f)), i)[0]

    # Convert to equivalent frequency
    return fs * true_i / len(windowed)


def to_esp_encoding(sig, fs):
    return "".join([str(freq_from_fft(sub_sig, fs)) + "," + "1;" for sub_sig in np.split(sig, len(sig)/(fs*0.1))])
    