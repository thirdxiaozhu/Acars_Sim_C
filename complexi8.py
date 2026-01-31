# plot_iq.py
import numpy as np
import matplotlib.pyplot as plt

# 读取 test.txt 为原始字节（int8）
with open("cmake-build-debug/complex_i8.txt", "rb") as f:
    raw = np.frombuffer(f.read(), dtype=np.int8)

# 交织 I/Q：I0,Q0,I1,Q1,...
i = raw[0::2]
q = raw[1::2]

# 取前 N 个点画时域（避免太长）
N = min(len(i), 300000)

plt.figure(figsize=(12, 6))
plt.plot(i[:N], label="I", lw=0.8)
plt.plot(q[:N], label="Q", lw=0.8)
plt.title("I/Q waveform (first {} samples)".format(N))
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()
