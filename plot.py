import numpy as np
import matplotlib.pyplot as plt

original = [25401.547, 27759.804]
FF = [25150.045, 26046.664]

# plot speedup graph
speedup = []
for i in range(len(original)):
    speedup.append(original[i]/FF[i])

fig, axs = plt.subplots(1, 2, figsize=(10, 5))
fig.suptitle('Speedup')
axs[0].plot(range(1, 3), speedup, label=f'delta = {delta[i]}')
axs[0].set_xlabel('Iteration')
axs[0].set_ylabel('Speedup')

plt.show()