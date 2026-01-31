# plot_iq.py
import numpy as np
import matplotlib.pyplot as plt

# 读取 test.txt 为原始字节（int8）
with open("cmake-build-debug/cfam.txt", "rb") as f:
    raw = np.frombuffer(f.read(), dtype=np.int8)

with open("cmake-build-debug/inputr.txt", "rb") as f:
    raw2 = np.frombuffer(f.read(), dtype=np.int8)

# 取前 N 个点画时域（避免太长）
N = min(len(raw), 500)

plt.figure(figsize=(12, 6))
plt.subplot(2, 1, 1)
plt.plot(raw[:N],  lw=0.8)
plt.title("cfam".format(N))
plt.legend()
plt.grid(True)

plt.subplot(2, 1, 2)
plt.plot(raw2[:N],  lw=0.8)
plt.title("after filter".format(N))
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()
