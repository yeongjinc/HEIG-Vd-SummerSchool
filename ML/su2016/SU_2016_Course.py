
# coding: utf-8

# In[47]:

import csv
import random
import sys
import re
from collections import Counter
from collections import OrderedDict
from sklearn import naive_bayes
from sklearn.ensemble.forest import RandomForestRegressor, ExtraTreesRegressor
from sklearn.ensemble.gradient_boosting import GradientBoostingRegressor
from sklearn.tree.tree import DecisionTreeRegressor
from sklearn.linear_model.ridge import Ridge
import sklearn.metrics
from sklearn.naive_bayes import GaussianNB
from sklearn.neighbors.regression import KNeighborsRegressor
from sklearn import cross_validation
from sklearn import datasets
from sklearn.cross_validation import StratifiedKFold
from xgboost import XGBRegressor
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt

#one hot encoding
def oneHot(header, prefix, ds, odic, col):
    n = len(odic) + 1
    ret = [prefix + str(i) for i in range(0,n)]
    header.extend(ret)

    for index,row in enumerate(ds):
        l = [0 for i in range(0, n)]
        kl = str(row[col])
        kl = kl.strip()
        for k in re.split(',', kl):
            try:
                l[odic.keys().index(k)] = 1
            except:
                pass
        ds[index].extend(l)

    return ret


#Set the seed
random.seed(42)
# In[28]:

# Open the file
filepath_train="./data/ech_apprentissage.csv"
file_open=open(filepath_train,"r")
# Open the csv reader over the file
csv_reader=csv.reader(file_open,delimiter=";")
# Read the first line which is the header
header=csv_reader.next()
# Load the dataset contained in the file
_dataset=[]
_ins=[]
for row in csv_reader:
    _dataset.append(row)
    _ins.append(float(row[len(row)-1]))

_ins.sort()
outlierPercentage = 0.0
outlierIndexL = int(len(_ins) * outlierPercentage)
outlierIndexH = len(_ins) - outlierIndexL - 1

dataset=[]
for row in _dataset:
    ins = float(row[len(row)-1])
    if ins >= _ins[outlierIndexL] and ins <= _ins[outlierIndexH]:
        dataset.append(row)

# Using average for missing value, but didn't work better
"""
def isNumber(s):
    try:
        float(s)
        return True
    except:
        return False

cnt = [0] * 40
S = [0.0] * 40
for row in dataset:
    for index, value in enumerate(row):
        if isNumber(value):
            cnt[index] += 1
            S[index] += float(value)

avg = []
for i, s in enumerate(S):
    if s == 0.0:
        avg.append(-1)
    else:
        avg.append(s / cnt[i])
"""

# Replace the missing values
# Count string values for one hot
brandCol = 3
brandPrefix = 'brand'
brandCounter = Counter()
energyCol = 7
energyPrefix = 'energy'
energyCounter = Counter()
var6Col = 16
var6Prefix = 'cval6'
var6Counter = Counter()
var8Col = 18
var8Prefix = 'cval8'
var8Counter = Counter()
var14Col = 24
var14Prefix = 'cval14'
var14Counter = Counter()
profCol = 10
profPrefix = 'prof'
profCounter = Counter()

for index,row in enumerate(dataset):
    dataset[index]=[value if value not in ["NR",""] else -1 for col, value in enumerate(row)]
    brandCounter.update([row[brandCol]])
    energyCounter.update([row[energyCol]])
    var6Counter.update([row[var6Col]]);
    var8Counter.update([row[var8Col]]);
    var14Counter.update([row[var14Col]]);
    profCounter.update(re.split(',', row[profCol]))

brandODic = OrderedDict(brandCounter)
energyODic = OrderedDict(energyCounter)
var6ODic = OrderedDict(var6Counter)
var8ODic = OrderedDict(var8Counter)
var14ODic = OrderedDict(var14Counter)
profODic = OrderedDict(profCounter)

brandRes = oneHot(header, brandPrefix, dataset, brandODic, brandCol)
energyRes = oneHot(header, energyPrefix, dataset, energyODic, energyCol)
var6Res = oneHot(header, var6Prefix, dataset, var6ODic, var6Col)
var8Res = oneHot(header, var8Prefix, dataset, var8ODic, var8Col)
var14Res = oneHot(header, var14Prefix, dataset, var14ODic, var14Col)
profRes = oneHot(header, profPrefix, dataset, profODic, profCol)
# In[39]:


# Filter the dataset based on the column name
feature_to_filter=["crm","annee_naissance","kmage_annuel", "puis_fiscale", "anc_veh", "var1", "var2", "var4", "var5", "var7", "var9", "var10", "var12", "var13", "var15", "var16", "var17", "var18", "var19", "var20", "var21", "var22"]
feature_to_filter.extend(brandRes)
feature_to_filter.extend(energyRes)
feature_to_filter.extend(var6Res)
feature_to_filter.extend(var8Res)
feature_to_filter.extend(var14Res)
feature_to_filter.extend(profRes)

indexes_to_filter=[]
for feature in feature_to_filter:
    indexes_to_filter.append(header.index(feature))

dataset_filtered=[]
for row in dataset:
    dataset_filtered.append([float(row[index]) for index in indexes_to_filter])
# Build the structure containing the target
targets=[]
for row in dataset:
    targets.append(float(row[header.index("prime_tot_ttc")]))


# In[40]:

#Split the datasets to have one for learning and the other for the test
train_dataset=[]
test_dataset=[]
train_target=[]
test_target=[]

for row,target in zip(dataset_filtered,targets):
    if random.random() < 0.70:
        train_dataset.append(row)
        train_target.append(target)
    else:
        test_dataset.append(row)
        test_target.append(target)


# In[41]:

#Build the model
#model=ExtraTreesRegressor()
#model=RandomForestRegressor()
#params = {'n_estimators': 500, 'max_depth': 4, 'min_samples_split': 2,
#                  'learning_rate': 0.01, 'loss': 'ls'}
params = {'n_estimators': 400, 'max_depth': 7}
#model=GradientBoostingRegressor(**params)
model=XGBRegressor(**params)
#model=GaussianNB()
#model=Ridge()
#model=KNeighborsRegressor()
#model=DecisionTreeRegressor()
model.fit(train_dataset,train_target)

#Predict with the model
predictions=model.predict(test_dataset)


# In[51]:

### Cross Validation ###

#cv = StratifiedKFold(train_dataset, n_folds=5)

###scoring
scores = cross_validation.cross_val_score(model, train_dataset, train_target, cv=5)
print "Accuracy: %0.2f (+/- %0.2f)" % (scores.mean(), scores.std() * 2)

### getting the predictions ###
#predicted = cross_validation.cross_val_predict(clf, train_dataset, train_target, cv=10)
#print metrics.accuracy_score(train_target, predicted)
model.fit(train_dataset,train_target)
predictions=model.predict(test_dataset)


# In[50]:

#Evaluate the quality of the prediction
print sklearn.metrics.mean_absolute_error(predictions,test_target)

#Alternative -- Compute the mean absolute percentage error


# In[ ]:

# Now load the test file and use the model built to score the dataset and create the submission file.


filepath_test="./data/ech_test.csv"
file_test=open(filepath_test, "r")
csvr=csv.reader(file_test, delimiter=";")
head=csvr.next()
data_test=[]
for row in csvr:
    row.append('0') # for answer field
    data_test.append(row)

for i,row in enumerate(data_test):
    data_test[i] = [value if value not in ["NR", ""] else -1 for value in row]

oneHot(header, brandPrefix, data_test, brandODic, brandCol)
oneHot(header, energyPrefix, data_test, energyODic, energyCol)
oneHot(header, var6Prefix, data_test, var6ODic, var6Col)
oneHot(header, var8Prefix, data_test, var8ODic, var8Col)
oneHot(header, var14Prefix, data_test, var14ODic, var14Col)
oneHot(header, profPrefix, data_test, profODic, profCol)

data_filtered = []
for row in data_test:
    data_filtered.append([float(row[i]) for i in indexes_to_filter])

p = model.predict(data_filtered)

filepath_res="res.csv"
file_res=open(filepath_res, "w")
csvw = csv.writer(file_res, delimiter=";")
csvw.writerow(["ID", "COTIS"])

for i,r in enumerate(p):
    #print i+300000+1, ',', r
    csvw.writerow([i+300000+1, r])


