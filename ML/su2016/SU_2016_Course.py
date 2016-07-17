
# coding: utf-8

# In[47]:

import csv
import random
import sys
from collections import Counter
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
import pandas as pd
import matplotlib
import matplotlib.pyplot as plt

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
header.extend(["car" + str(i) for i in range(0,7)])
# Load the dataset contained in the file
dataset=[]
#c = Counter()
for row in csv_reader:
    dataset.append(row)
#    c.update([row[3]])
#print c




# Using average for missing value, but didn't work better
'''
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
'''

# Replace the missing values
# Insert one-hot values
for index,row in enumerate(dataset):
    dataset[index]=[value if value not in ["NR",""] else -1 for col, value in enumerate(row)]
    if row[3] == 'RENAULT':
        dataset[index].extend([1,0,0,0,0,0,0])
    elif row[3] == 'PEUGEOT':
        dataset[index].extend([0,1,0,0,0,0,0])
    elif row[3] == 'CITROEN':
        dataset[index].extend([0,0,1,0,0,0,0])
    elif row[3] == 'VOLKSWAGEN':
        dataset[index].extend([0,0,0,1,0,0,0])
    elif row[3] == 'MERCEDES':
        dataset[index].extend([0,0,0,0,1,0,0])
    elif row[3] == 'BMW':
        dataset[index].extend([0,0,0,0,0,1,0])
    else:
        dataset[index].extend([0,0,0,0,0,0,1])

# In[39]:

# Filter the dataset based on the column name
feature_to_filter=["crm","annee_naissance","kmage_annuel", "annee_permis", "puis_fiscale", "anc_veh", "var1", "var2", "var4", "var5", "var7", "var9", "var10", "var11", "var12", "var13", "var15", "var16", "var17", "var18", "var19", "var20", "var21", "var22"]
feature_to_filter.extend(["car" + str(i) for i in range(0,7)])

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
    if random.random() < 0.50:
        train_dataset.append(row)
        train_target.append(target)
    else:
        test_dataset.append(row)
        test_target.append(target)


# In[41]:

#Build the model
#model=ExtraTreesRegressor()
#model=RandomForestRegressor()
model=GradientBoostingRegressor()
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
    if row[3] == 'RENAULT':
        data_test[i].extend([1,0,0,0,0,0,0])
    elif row[3] == 'PEUGEOT':
        data_test[i].extend([0,1,0,0,0,0,0])
    elif row[3] == 'CITROEN':
        data_test[i].extend([0,0,1,0,0,0,0])
    elif row[3] == 'VOLKSWAGEN':
        data_test[i].extend([0,0,0,1,0,0,0])
    elif row[3] == 'MERCEDES':
        data_test[i].extend([0,0,0,0,1,0,0])
    elif row[3] == 'BMW':
        data_test[i].extend([0,0,0,0,0,1,0])
    else:
        data_test[i].extend([0,0,0,0,0,0,1])


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


