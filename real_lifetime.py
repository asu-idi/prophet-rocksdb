from curses import keyname
from pickle import BINSTRING
import matplotlib.pyplot as plt
import numpy as np
x_list = []
type_list = []
lifetime_list = []
predict_list = []
real_type = []
level_number = []

for line in open("lifetime.out"):
    #print(line.split(' '))
    x_list.append(int(line.split(' ')[0]))
    predict_list.append(int(line.split(' ')[1]))
    type_list.append(int(line.split(' ')[2]))
    lifetime_list.append(int(line.split(' ')[3]))
    real_type.append(int(line.split(' ')[4]))
    level_number.append(int(line.split(' ')[7]))

tot_cnt = 0
for l in range(0, 6): #each level
    real_lifetime_list_0 = []
    real_lifetime_list_n1 = []
    sum = 0
    sum0 = 0
    sum1 = 0
    tot = 0
    cnt0 = 0
    cntn1 = 0
    num = 0
    for i in range(0, len(x_list)):
        if x_list[i] == l:
            if real_type[i] == 0:
                real_lifetime_list_0.append(lifetime_list[i])
                cnt0 += 1
                sum0 += lifetime_list[i]
            else:
                real_lifetime_list_n1.append(lifetime_list[i])
                cntn1 += 1
                sum1 += lifetime_list[i]
            sum += lifetime_list[i]
            tot += 1
            if level_number[i] > num:
                num = level_number[i]
    d =  0 if tot == 0 else sum / tot
    d1 = 0 if cnt0 == 0 else sum0 / cnt0
    d2 = 0 if cntn1 == 0 else sum1 / cntn1
    print("level=%d num=%d all_ave=%d ave_0=%d ave_n1=%d cnt0=%d cntn1=%d" % (l, num, d, d1, d2, cnt0, cntn1))
    tot_cnt += num
    
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.xlabel('Real lifetime', fontsize=24, weight="bold")
    plt.ylabel('Number', fontsize=24, weight="bold")
    plt.xticks(fontsize=18, weight="bold")
    plt.yticks(fontsize=18, weight="bold")
    plt.subplots_adjust(top=0.94, right=0.96, left=0.16, bottom=0.15)
    if(len(real_lifetime_list_0) != 0):
        plt.hist(real_lifetime_list_0, bins=50, color="gold")
        plt.show()
    
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]   
    plt.xlabel('Real lifetime', fontsize=24, weight="bold")
    plt.ylabel('Number', fontsize=24, weight="bold")
    plt.xticks(fontsize=18, weight="bold")
    plt.yticks(fontsize=18, weight="bold")
    plt.subplots_adjust(top=0.94, right=0.96, left=0.16, bottom=0.15)    
    if(len(real_lifetime_list_n1) != 0):
        plt.hist(real_lifetime_list_n1, bins=50, color="purple")
        plt.show()

print(tot_cnt)