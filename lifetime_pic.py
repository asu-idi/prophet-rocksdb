from curses import keyname
from pickle import BINSTRING
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import MaxNLocator
from matplotlib.pyplot import MultipleLocator
x_list = []
type_list = []
lifetime_list = []
predict_list = []
real_type = []

def addtwodimdict(thedict, key_a, key_b, val): 
    if key_a in adic:
        thedict[key_a].update({key_b: val})
    else:
        thedict.update({key_a:{key_b: val}})

for line in open("lifetime.out"):
    #print(line.split(' '))
    x_list.append(int(line.split(' ')[0]))
    predict_list.append(int(line.split(' ')[1]))
    type_list.append(int(line.split(' ')[2]))
    lifetime_list.append(int(line.split(' ')[3]))
    real_type.append(int(line.split(' ')[4]))


ans_list = []
real_distribution = []
right = 0
for i in range(0, len(x_list)):
    ans_list.append(predict_list[i] - lifetime_list[i])
    if(predict_list[i] - lifetime_list[i] >= -25 and predict_list[i] - lifetime_list[i] <= 25):
        right = right + 1
    real_distribution.append(lifetime_list[i])


print("total right rate=%.2lf"%(right / len(x_list)))
font_size=24
ticks_size=18

plt.rcParams["font.family"] = "serif"
plt.rcParams["font.serif"] = ["Times New Roman"]
plt.figure(figsize=(6.5, 3.2))
plt.xticks(fontsize=ticks_size, weight="bold")
plt.yticks(fontsize=ticks_size, weight="bold")
plt.xlim([-200, 200])
plt.ylim([0, 25000])
plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
plt.ylabel('Number', fontsize=font_size, weight="bold")
plt.subplots_adjust(top=0.94, right=0.965, left=0.14, bottom=0.26)
plt.gca().set_yticklabels([str(int(y / 1000)) + "k" for y in plt.gca().get_yticks()])
plt.hist(ans_list, bins=1000, color="gold")
plt.show()


plt.rcParams["font.family"] = "serif"
plt.rcParams["font.serif"] = ["Times New Roman"]
plt.figure(figsize=(6.5, 3.2))
plt.xticks(fontsize=ticks_size, weight="bold")
plt.yticks(fontsize=ticks_size, weight="bold")
plt.xlim([-100, 1200])
plt.ylim([0, 15000])
plt.gca().set_yticklabels([str(int(y / 1000)) + "k" for y in plt.gca().get_yticks()])
plt.xlabel('Real lifetime', fontsize=font_size, weight="bold")
plt.ylabel('Number', fontsize=font_size, weight="bold")
plt.subplots_adjust(top=0.94, right=0.96, left=0.14, bottom=0.26)
plt.hist(real_distribution, bins=1000, color="purple")
plt.show()

for l in range(0, 7): #each level
    kind_correct = {
       "0": {"0": 0, "-1": 0},
        "-1": {"0": 0, "-1": 0},
        "1": {"0": 0, "-1": 0},
        "2": {"0": 0, "-1": 0},
        "3": {"0": 0, "-1": 0},
        "4": {"0": 0, "-1": 0},
        "5": {"0": 0, "-1": 0},
        "-2": {"0": 0, "-1": 0},
        "-3": {"0": 0, "-1": 0},
        "-4": {"0": 0, "-1": 0},
        "-5": {"0": 0, "-1": 0},
        "-6": {"0": 0, "-1": 0}
    }
    kind_num = {
        "0": {"0": 0, "-1": 0},
        "-1": {"0": 0, "-1": 0},
        "1": {"0": 0, "-1": 0},
        "2": {"0": 0, "-1": 0},
        "3": {"0": 0, "-1": 0},
        "4": {"0": 0, "-1": 0},
        "5": {"0": 0, "-1": 0},
        "-2": {"0": 0, "-1": 0},
        "-3": {"0": 0, "-1": 0},
        "-4": {"0": 0, "-1": 0},
        "-5": {"0": 0, "-1": 0},
        "-6": {"0": 0, "-1": 0}
    }
    kind_ave = {
       "0": {"0": 0, "-1": 0},
        "-1": {"0": 0, "-1": 0},
        "1": {"0": 0, "-1": 0},
        "2": {"0": 0, "-1": 0},
        "3": {"0": 0, "-1": 0},
        "4": {"0": 0, "-1": 0},
        "5": {"0": 0, "-1": 0},
        "-2": {"0": 0, "-1": 0},
        "-3": {"0": 0, "-1": 0},
        "-4": {"0": 0, "-1": 0},
        "-5": {"0": 0, "-1": 0},
        "-6": {"0": 0, "-1": 0}
    }
    tot = 0 
    correct = 0
    sum = 0
    data1_list = [] #short-lived
    data1_list_miss = []
    data2_list = [] #current level compaction
    data2_list_miss = []
    data3_list = [] #upper level compaction
    data3_list_miss = [] 
    data4_list = [] #trivial compaction
    data4_list_miss = [] #trivial compaction
    data5_list = []
    data5_list_miss = []
    THRESHOLD = 5 * (l + 1)

    level0_num = 0 #compacted by current level num
    leveln1_num = 0 #compacted by top level num
    level1_num = 0 #compacted by unknow file
    for i in range(0, len(x_list)):
        
        if x_list[i] == l:
            key1=str(type_list[i])
            key2=str(real_type[i])
            sum = sum + lifetime_list[i]
            kind_ave[key1][key2] += lifetime_list[i]
            diff = predict_list[i] - lifetime_list[i]
            
            if type_list[i] == 1:
                if real_type[i] == -1:
                    data1_list.append(diff)
                else:
                    data1_list_miss.append(diff)
            elif type_list[i] == 2:
                if real_type[i] == -1:
                    data2_list.append(diff)
                else:
                    data2_list_miss.append(diff)
            elif type_list[i] == 3:
                if real_type[i] == -1: #compacted
                    data3_list.append(diff)
                else:
                    data3_list_miss.append(diff)
            elif type_list[i] == 4:
                if real_type[i] == -1:  
                    data4_list.append(diff)
                else:
                    data4_list_miss.append(diff) #type_list[i] == 0 but real_type[i] == 1
            elif type_list[i] == 5:
                if real_type[i] == -1:
                    data5_list.append(diff)
                else:
                    data5_list_miss.append(diff)
            tot = tot + 1
            kind_num[key1][key2] += 1
            if real_type[i] == -1:
                leveln1_num += 1
            elif real_type[i] == 1:
                level1_num += 1
            else:
                level0_num += 1
            if diff >= -THRESHOLD and diff <= THRESHOLD:
                correct = correct + 1
                kind_correct[key1][key2] += 1

    if tot == 0:
        continue
    print("level %d correct_rate=%.3lf average_lifetime=%.3lf type-1_num=%d type0_num=%d type1_num=%d" % (l, correct / tot, sum / tot, leveln1_num, level0_num, level1_num)) #right number 
    for key1 in kind_num:
        for key2 in kind_num[key1]:
            if(kind_num[key1][key2] != 0):
                key11 = ""
                key22 = ""
                if key1 == "1":
                    key11 = "2B"
                elif key1 == "2":
                    key11 = "1"
                elif key1 == "3":
                    key11 = "2A"
                else:
                    key11 = "3"
                if key2 == "0":
                    key22 = "1"
                else:
                    key22 = "2"
                print("P%s D%s Accuracy=%.3lf Predict_Ave=%.3lf num=%d" % (key11, key22, kind_correct[key1][key2] / kind_num[key1][key2], kind_ave[key1][key2] / kind_num[key1][key2], kind_num[key1][key2]))
    if(l != 4): 
        continue
    bins_num = 50

    #P1D1
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.figure(figsize=(6.5, 3.2))
    plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
    plt.ylabel('Number', fontsize=font_size, weight="bold")
    plt.xticks(fontsize=ticks_size, weight="bold")
    plt.yticks(fontsize=ticks_size, weight="bold")
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
    plt.subplots_adjust(top=0.94, right=0.99, left=0.14, bottom=0.26)
    if((len(data2_list_miss) != 0)):
        plt.hist(data2_list_miss, bins=bins_num, color="green") #current
        plt.show()        

    #P1D2    
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.figure(figsize=(6.5, 3.2))
    plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
    plt.ylabel('Number', fontsize=font_size, weight="bold")
    plt.xticks(fontsize=ticks_size, weight="bold")
    plt.yticks(fontsize=ticks_size, weight="bold")
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
    plt.subplots_adjust(top=0.94, right=0.99, left=0.125, bottom=0.26)
    if(len(data2_list) != 0): #short-lived
        plt.hist(data2_list, bins=bins_num, color="yellow") #upper
        plt.show()
    

    #P2AD1
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.figure(figsize=(6.5, 3.2))
    plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
    plt.ylabel('Number', fontsize=font_size, weight="bold")
    plt.xticks(fontsize=ticks_size, weight="bold")
    plt.yticks(fontsize=ticks_size, weight="bold")
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.gca().yaxis.set_major_locator(MultipleLocator(10))
    plt.subplots_adjust(top=0.94, right=0.99, left=0.125, bottom=0.26)
    if(len(data3_list_miss) != 0):
        plt.hist(data3_list_miss, bins=bins_num, color="purple") #current
        plt.show()

    #P2AD2
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.figure(figsize=(6.5, 3.2))
    plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
    plt.ylabel('Number', fontsize=font_size, weight="bold")
    plt.xticks(fontsize=ticks_size, weight="bold")
    plt.yticks(fontsize=ticks_size, weight="bold")
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.gca().yaxis.set_major_locator(MultipleLocator(300))
    plt.subplots_adjust(top=0.94, right=0.99, left=0.16, bottom=0.26)
    if(len(data3_list) != 0):
        plt.hist(data3_list, bins=bins_num, color="blue") #upper
        plt.show()


    #P2BD1
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.figure(figsize=(6.5, 3.2))
    plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
    plt.ylabel('Number', fontsize=font_size, weight="bold")
    plt.xticks(fontsize=ticks_size, weight="bold")
    plt.yticks(fontsize=ticks_size, weight="bold")
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
    plt.subplots_adjust(top=0.94, right=0.99, left=0.11, bottom=0.26)
    if((len(data1_list_miss) != 0)):
        plt.hist(data1_list_miss, bins=bins_num, color="orange") #current
        plt.show()

    #P2BD2
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.figure(figsize=(6.5, 3.2))
    plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
    plt.ylabel('Number', fontsize=font_size, weight="bold")
    plt.xticks(fontsize=ticks_size, weight="bold")
    plt.yticks(fontsize=ticks_size, weight="bold")
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
    plt.subplots_adjust(top=0.94, right=0.99, left=0.14, bottom=0.26)
    if(len(data1_list) != 0): #short-lived
        plt.hist(data1_list, bins=bins_num, color="red") #upper
        plt.show()

    #P3AD1
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.figure(figsize=(6.5, 3.2))
    plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
    plt.ylabel('Number', fontsize=font_size, weight="bold")
    plt.xticks(fontsize=ticks_size, weight="bold")
    plt.yticks(fontsize=ticks_size, weight="bold")
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
    plt.subplots_adjust(top=0.94, right=0.97, left=0.1, bottom=0.265)
    if(len(data4_list_miss) != 0):
        plt.hist(data4_list_miss, bins=bins_num, color="black") #current
        plt.show()

    #P3AD2
    plt.rcParams["font.family"] = "serif"
    plt.rcParams["font.serif"] = ["Times New Roman"]
    plt.figure(figsize=(6.5, 3.2))
    plt.xlabel('Lifetime prediction error (FC-ticks)', fontsize=font_size, weight="bold")
    plt.ylabel('Number', fontsize=font_size, weight="bold")
    plt.xticks(fontsize=ticks_size, weight="bold")
    plt.yticks(fontsize=ticks_size, weight="bold")
    plt.gca().xaxis.set_major_locator(MaxNLocator(integer=True))
    plt.gca().yaxis.set_major_locator(MaxNLocator(integer=True))
    plt.subplots_adjust(top=0.94, right=0.99, left=0.125, bottom=0.26)
    if(len(data4_list) != 0):
        plt.hist(data4_list, bins=bins_num, color="pink") # upper
        plt.show()





#4 [0-9]* 0 [0-9]* -1
