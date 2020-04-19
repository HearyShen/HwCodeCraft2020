import numpy as np
from collections import OrderedDict, deque


def readData(path='test_data.txt'):
    cache = []
    maxID = 0
    with open(path) as f:
        for line in f:
            nums = [int(num) for num in line.strip().split(',')]
            sender, receiver, amount = nums
            cache.append(nums)
            maxID = max(sender, receiver, maxID)

    mSize = maxID + 1
    transMatrix = np.zeros((mSize, mSize), dtype=int)

    for nums in cache:
        sender, receiver, amount = nums
        transMatrix[sender][receiver] = amount
    return transMatrix


def bfs(transMatrix, start, cyclePathsSlots):
    bfsQueue = deque()
    bfsQueue.append([start])
    while bfsQueue:
        curPath = bfsQueue.popleft()
        curUser = curPath[-1]
        nextUsers = transMatrix[curUser].nonzero()[0]
        for nextUser in nextUsers:
            if nextUser in curPath:
                if nextUser == curPath[0] and 3<=len(curPath)<=7:
                    cyclePathsSlots[len(curPath)].append(curPath)
            else:
                if nextUser > start:
                    bfsQueue.append(curPath + [nextUser])


def writeResult(cyclePathsSlots, path='result.txt'):
    count = sum([len(slot) for slot in cyclePathsSlots])
    with open(path, 'w') as f:
        f.write(str(count) + '\n')
        for i in range(len(cyclePathsSlots)):
            for cycPath in cyclePathsSlots[i]:
                line = ','.join([str(u) for u in cycPath])
                f.write(line + '\n')


if __name__ == "__main__":
    transMatrix = readData(path=r'/data/test_data.txt')
    cyclePathsSlots = [[] for i in range(8)]

    for start in range(len(transMatrix)):
        bfs(transMatrix, start, cyclePathsSlots)

    writeResult(cyclePathsSlots, r'/projects/student/result.txt')
