import numpy as np
import matplotlib.pyplot as plt

path = "cmake-build-debug/complex_f32.bin"
data = np.fromfile(path, dtype=np.float32)
if data.size % 2 != 0:
    data = data[:-1]

i = data[0::2]
q = data[1::2]

# 只画前 N 个样本，避免太密
N = 300000
i = i[:N]
q = q[:N]

t = np.arange(i.size)

plt.figure(figsize=(10, 4))
plt.plot(t, i, label="I")
plt.plot(t, q, label="Q", alpha=0.8)
plt.title("IQ waveform (time domain)")
plt.xlabel("Sample index")
plt.ylabel("Amplitude")
plt.legend()
plt.tight_layout()
plt.show()
