import sys
import matplotlib
matplotlib.use('Agg')
from matplotlib import pyplot as plt
""" %matplotlib inline
import pylab as plt """

#bash should call as python ./plotting_script.py benchmark_name param delta1 delta2 ..
benchmark_name= sys.argv[1]
benchmark_param= sys.argv[2]
delta= sys.argv[3:]
delta= list(map(int, delta))

print(benchmark_name, benchmark_param, delta)


Kernel_time= []
totalPush= []
totalSteal= []
F_ratio = []
F_totalPush = []
F_totalSteal = []
F_Kernel_time = []

def reset_vals_temp():
    global totalPush
    global totalSteal
    global Kernel_time

    totalPush=[]
    totalSteal=[]
    Kernel_time=[]

def reset_vals_final():
    global F_ratio
    global F_totalPush
    global F_totalSteal
    global F_Kernel_time

    F_ratio=[]
    F_totalSteal=[]
    F_Kernel_time=[]
    F_totalPush=[]

def readfile(fd):
    lines=fd.readlines()
    index=0
    for line in lines:
        words=line.split()
        if 'totalPush' in words:
            values= lines[index+1].split()
            Kernel_time.append(float(values[0]))
            totalPush.append(float(values[1]))
            totalSteal.append(float(values[2]))
        index+=1

div_factor=5
def calculate_avg(delta):
    avg_kernel_time=sum(Kernel_time)/div_factor
    avg_totalSteal=int(sum(totalSteal))/div_factor
    avg_totalPush=int(sum(totalPush))/div_factor
    reset_vals_temp()

    if(avg_totalPush<=0):
        print(benchmark_name,benchmark_param,delta)

    steal_percent=(float(avg_totalSteal)/float(avg_totalPush))*100.0
    F_Kernel_time.append(avg_kernel_time)
    F_totalSteal.append(avg_totalSteal)
    F_ratio.append(steal_percent)
    F_totalPush.append(avg_totalPush)

def open_file(file_name):
    for j in delta:
        for i in range(1,6):
            filename= file_name + str(j)+ "_"+str(i)+".txt"
            fd= open(filename, 'r')
            readfile(fd)
            fd.close
        calculate_avg(j)

def Hclib_openFile(file_name):
    for i in range(1,6):
        filename= file_name + str(i)+".txt"
        fd= open(filename, 'r')
        readfile(fd)
        fd.close
    calculate_avg(0)

def set_plot(title,ylab):
    ax=plt.axes()
    #ax.set_facecolor('#cccccc')
    plt.title(title)
    plt.grid(color='#737373', linestyle='--',linewidth=2, alpha=0.25)
    plt.ylabel(ylab)
    plt.xlabel('Delta -->')

def plotting(hclib, design, y_axis, l=0, r=len(delta)):
    if(hclib):
        y_axis= [y_axis[0]]*len(delta)
    print(delta[l:r])
    print(y_axis[l:r])
    print("--------------------------------------------\n")
    plt.plot(delta[l:r],y_axis[l:r], design, alpha=1/3)
    
""" def set_plot_save(title, ylab, FF_yaxis, hclib_yaxis):
    set_plot(title,ylab)
    plotting(0, 'm^--', FF_yaxis)
    plotting(1, 'bo--', hclib_yaxis)
    plot_name='Graphs/{0}_{1}_Time'.format()
    plt.savefig(plot_name)

    set_plot(title,ylab)
    plotting(0, 'm^--', FF_yaxis,0,10)
    plotting(1, 'bo--', hclib_yaxis,0,10)
    plt.savefig('Graphs/{0}_{1}_Time_Zoom') """

FF_filename= "results/FF_"+benchmark_name+"_"+benchmark_param+"_"  #FF_fib_40_delta_1
open_file(FF_filename)
FF_time= F_Kernel_time
FF_ratio= F_ratio

reset_vals_final()
reset_vals_temp()

hclib_filename= "results/hclib_"+benchmark_name+"_"+benchmark_param+"_"  #hclib_fib_40_1
Hclib_openFile(hclib_filename)

title='FF v/s HClib Time [{0}({1})]'.format(benchmark_name,benchmark_param)
ylab='Kernel_Time (msec)'

fig=plt.figure()
set_plot(title,ylab)
plotting(0, 'm^--', FF_time)
plotting(1, 'bo--', F_Kernel_time)
plt.legend(['Fence Free Deq','HCLib Deque'])
plot_name='Graphs/{0}_{1}_Time.png'.format(benchmark_name,benchmark_param)
plot_name='Graphs/{0}_{1}_Time'.format(benchmark_name,benchmark_param)
plt.savefig(plot_name)
plt.close(fig)

fig=plt.figure()
set_plot(title,ylab)
plotting(0, 'm^--', FF_time,0,10)
plotting(1, 'bo--', F_Kernel_time,0,10)
plt.legend(['Fence Free Deq','HCLib Deque'])
plot_name='Graphs/{0}_{1}_Time_Zoom.png'.format(benchmark_name,benchmark_param)
plt.savefig(plot_name)
plt.close(fig)

title='FF v/s HClib Steal Ratio [{0}({1})]'.format(benchmark_name,benchmark_param)
ylab='Steal Percent (msec)'

fig=plt.figure()
set_plot(title,ylab)
plotting(0, 'm^--', FF_ratio)
plotting(1, 'bo--', F_ratio)
plt.legend(['Fence Free Deq','HCLib Deque'])
plot_name='Graphs/{0}_{1}_Steal.png'.format(benchmark_name,benchmark_param)
plt.savefig(plot_name)
plt.close(fig)

fig=plt.figure()
set_plot(title,ylab)
plotting(0, 'm^--', FF_ratio,0,10)
plotting(1, 'bo--', F_ratio, 0, 10)
plt.legend(['Fence Free Deq','HCLib Deque'])
plot_name='Graphs/{0}_{1}_Steal_Zoom.png'.format(benchmark_name,benchmark_param)
plt.savefig(plot_name)
plt.close(fig)