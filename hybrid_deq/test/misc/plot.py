# import matplotlib.pyplot as plt
# import numpy as np

# read data from txt files

ff_value = 0
org_value = 0

ff_list = []
org_list = []


for i in range(1, 7):
    file = open(f'./results/ff_same_both_{i}.txt', 'r')
    data = file.read()
    # fetch last line
    last_line = data.splitlines()[-1]
    # print(last_line)
    value = last_line.split(' ')[4]
    ff_value += float(value)
    ff_list.append(float(value))


for i in range(1, 7):
    file = open(f'./results/org_same_both_{i}.txt', 'r')
    data = file.read()
    # fetch last line
    last_line = data.splitlines()[-1]
    # print(last_line)
    value = last_line.split(' ')[4]
    org_value += float(value)
    org_list.append(float(value))

print(f'ff_value: {ff_value/6}')
print(f'org_value: {org_value/6}')
print()

# print median

ff_list.sort()
org_list.sort()

print(f'ff_median: {ff_list[3]}')
print(f'org_median: {org_list[3]}')
print()

# minest value

print(f'ff_min: {ff_list[0]}')
print(f'org_min: {org_list[0]}')
print()
# max value

print(f'ff_max: {ff_list[-1]}')
print(f'org_max: {org_list[-1]}')


