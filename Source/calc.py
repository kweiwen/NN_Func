import numpy as np

def add(i, j):
    arr1 = np.array([i,i,i])
    arr2 = np.array([j,j,j])
    return np.sum(arr1*arr2)