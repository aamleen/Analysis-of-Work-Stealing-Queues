import matplotlib.pyplot as plt
import numpy as np

# Plot the data
# read data from text file
data = np.loadtxt('plotting_data1.txt')

# convert it to list
data = data.tolist()
time = [i*100 for i in range(len(data))]

# plot the data
plt.plot(time, data)
plt.xlabel('Time (nanosec)')
plt.ylabel('Total Tasks in Queue')
plt.title('Data Plot of 19 threads on matmul 8192')
plt.savefig('plot.png')