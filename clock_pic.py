import matplotlib.pyplot as plt
import numpy as np

prev_list = []
tmp_prev_flush_list = []
prev_flush_list = []
type_list = []
tot = 0
for line in open("clock.out"): 
    tot = tot + 1
    if(tot != 1):
        prev_list.append(int(line.split(' ')[0]))
        tmp_prev_flush_list.append(int(line.split(' ')[1])) 
        type_list.append(int(line.split(' ')[2]))


y = np.array(prev_list)
plt.hist(prev_list, bins=100, color="brown")
plt.show()


tot = 0
for i in range(0, len(type_list)):
    tot = tot + 1
    if i + 1 < len(type_list) and type_list[i] == 2 and type_list[i + 1] == 1:
        prev_flush_list.append(tmp_prev_flush_list[i])
        

# y = np.array(prev_flush_list)
plt.hist(prev_flush_list, bins=100, color="brown")
plt.show()
