import matplotlib.pyplot as plt
import numpy as np


num_list = []
x_list = []
for line in open("level.out"): 
    x_list.append(int(line.split(' ')[0]))
    num_list.append(int(line.split(' ')[1])) 
print(len(num_list))
print(len(x_list))
x = np.array(x_list)
y = np.array(num_list)



font_size=18
plt.xticks(fontsize=18)
plt.yticks(fontsize=18)
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.serif"] = ["Times New Roman"]
plt.ylim([-0.5, 5.5])
#plt.xlim([9800, 9850])
plt.xlabel('FC-Ticks', fontsize=font_size)  
plt.ylabel('Level', fontsize=font_size)
plt.scatter(x, y, s=40)
plt.show()