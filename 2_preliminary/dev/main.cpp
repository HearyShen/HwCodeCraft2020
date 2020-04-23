#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <ctime>
#include <vector>
#include <stack>
#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <algorithm>
#include <queue>

#define MIN_LEN 3
#define MAX_LEN 7
#define MAX_STEP MAX_LEN/2
#define TOTAL_THREADS 4

using namespace std;

typedef set<int> Set;
typedef vector<int> List; // [id1, id2, ..., idn]
typedef queue<int> Queue;
typedef stack<List> DfsStack;					  // Stack{path1, path2, ...}
typedef map<int, vector<int>> Matrix;			  // {from: toes}
typedef map<int, map<int, vector<string>>> Slots; // {len: {start_id: paths}}}

int fileToMatrix(const string filename, Matrix &matrix, Matrix &rMatrix);
// void displayMatrix(Matrix &matrix);
bool listHas(const List &list, const int num);
string listToString(const List &list);
int multiStepNeighbors(Matrix &matrix, const int start, const int maxLen, Set &neighbors);
int dfs(Matrix &matrix, const int start, const int minLen, const int maxLen, Set &neighbors, Slots &results);
void dfsThread(const int threadID, Matrix &matrix, Matrix::iterator &mit, Matrix &rMatrix, Slots &results, int &cycleCount);
void mergeResults(Slots threadResults[TOTAL_THREADS], Slots &results, const int minLen, const int maxLen);
void resultToFile(Slots &results, const string filename, const int minLen, const int maxLen, const int count);

mutex mit_lock;
time_t g_tic = time(NULL);

int main(int argc, char *argv[])
{
	string filename;
	string outFilename = "result.txt";
	Matrix matrix, rMatrix;
	int retCode, start, cycleCount;
	int cycleCounts[TOTAL_THREADS] = {0};
	clock_t tic, toc;
	time_t ttic, ttoc;
	Slots results, threadResults[TOTAL_THREADS];
	thread threads[TOTAL_THREADS];

	ttic = time(NULL);
	tic = clock();
	filename = argv[1];
	retCode = fileToMatrix(filename, matrix, rMatrix);
	toc = clock();
	cout << "Readfile to matrix: " << retCode << " lines in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;

	tic = clock();
	Matrix::iterator mit = matrix.begin();
	for (int i = 0; i < TOTAL_THREADS; i++)
	{
		threads[i] = thread(dfsThread, i, ref(matrix), ref(mit), ref(rMatrix), ref(threadResults[i]), ref(cycleCounts[i]));
		cout << "> dfsThread: " << i << " created" << endl;
	}
	for (int i = 0; i < TOTAL_THREADS; i++)
	{
		threads[i].join();
	}
	cycleCount = 0;
	for (int i = 0; i < TOTAL_THREADS; i++)
	{
		cycleCount += cycleCounts[i];
	}
	toc = clock();
	cout << "Detected " << cycleCount << " cycles in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;

	tic = clock();
	mergeResults(threadResults, results, MIN_LEN, MAX_LEN);
	toc = clock();
	cout << "Threads' results merged in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;

	tic = clock();
	resultToFile(results, outFilename, MIN_LEN, MAX_LEN, cycleCount);
	toc = clock();
	ttoc = time(NULL);
	cout << "Written to file in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;
	cout << "Total finished in " << difftime(ttoc, ttic) << "s" << endl;

	return 0;
}

int fileToMatrix(const string filename, Matrix &matrix, Matrix &rMatrix)
{
	int userID;

	ifstream inText(filename.c_str());
	if (!inText)
	{
		cout << "Unable to open: " << filename << endl;
		return -1;
	}

	int count = 0;
	char comma;
	int from;
	int to;
	int amount;

	while (inText >> from >> comma >> to >> comma >> amount)
	{
		matrix[from].push_back(to);
		rMatrix[to].push_back(from);
		count++;
	}
	inText.close();

	// sort it ordered
	for (Matrix::iterator mit = matrix.begin(); mit != matrix.end(); mit++)
	{
		sort((*mit).second.begin(), (*mit).second.end());
	}
	for (Matrix::iterator mit = rMatrix.begin(); mit != rMatrix.end(); mit++)
	{
		sort((*mit).second.begin(), (*mit).second.end());
	}

	return count;
}

// void displayMatrix(Matrix &matrix)
// {
// 	for (Matrix::iterator mit = matrix.begin(); mit != matrix.end(); mit++)
// 	{
// 		cout << (*mit).first << ": ";
// 		for (set<int>::iterator sit = (*mit).second.begin(); sit != (*mit).second.end(); sit++)
// 		{
// 			cout << *sit << " ";
// 		}
// 		cout << endl;
// 	}
// }

bool listHas(const List &list, int num)
{
	for (int i = 0; i < list.size(); i++)
	{
		if (list[i] == num)
		{
			return true;
		}
	}
	return false;
}

string listToString(const List &list)
{
	stringstream ss;
	for(int i = 0; i < list.size(); ++i)
	{
		if(i != 0)
			ss << ",";
		ss << list[i];
	}
	return ss.str();
}

int multiStepNeighbors(Matrix &matrix, const int start, const int maxLen, Set &neighbors)
{
	Queue bfsQueue;
	int curNode, len, queueSize;
	Matrix::iterator matrix_iter;
	List nextNodes;

	len = 0;
	bfsQueue.push(start);
	while (!bfsQueue.empty())
	{
		len++;
		if (len > maxLen)
		{
			break;
		}

		queueSize = bfsQueue.size();
		for (int i = 0; i < queueSize; i++)
		{
			curNode = bfsQueue.front();
			bfsQueue.pop();

			// CAUTION: find next nodes, read-only to matrix
			matrix_iter = matrix.find(curNode);
			if (matrix_iter != matrix.end())
			{
				nextNodes = matrix_iter->second;
			}
			else
			{
				continue;
			}

			for (int j = 0; j < nextNodes.size(); j++)
			{
				neighbors.insert(nextNodes[j]);
				if (len < maxLen)
				{
					bfsQueue.push(nextNodes[j]);
				}
			}
		}
	}
	return neighbors.size();
}

int dfs(Matrix &matrix, const int start, const int minLen, const int maxLen, Set &neighbors, Slots &results)
{
	DfsStack dfsStack;
	List curPath, nextPath;
	Matrix::iterator matrix_iter;
	List nextNodes;
	int curNode, nextNode, curLen, cycleCount;

	cycleCount = 0;
	curPath.push_back(start);
	dfsStack.push(curPath);
	while (!dfsStack.empty())
	{
		curPath = dfsStack.top();
		dfsStack.pop();
		curLen = curPath.size();
		curNode = curPath.back();

		// CAUTION: find next nodes, read-only to matrix
		matrix_iter = matrix.find(curNode);
		if (matrix_iter != matrix.end())
		{
			nextNodes = matrix_iter->second;
		}
		else
		{
			continue;
		}

		for (int i = nextNodes.size() - 1; i >= 0; i--)
		{
			nextNode = nextNodes[i];

			if (nextNode == curPath[0])
			{ // if target cycle detected, then record it
				if (minLen <= curLen && curLen <= maxLen)
				{
					// valid length
					results[curLen][curPath[0]].push_back((listToString(curPath)));
					cycleCount++;
				}
			}
			else
			{ // if not target cycle, then search deeper
				if (curLen < maxLen && nextNode > curPath[0] && !listHas(curPath, nextNode))
				{
					if (curLen > MAX_STEP && neighbors.find(nextNode) == neighbors.end())
					{	// if MAX_STEP steps away from start, nextNode should be in MAX_STEP neighbors of reverse direction
						continue;
					}
					nextPath.assign(curPath.begin(), curPath.end());
					nextPath.push_back(nextNode);
					dfsStack.push(nextPath);
				}
			}
		}
	}
	return cycleCount;
}

void dfsThread(const int threadID, Matrix &matrix, Matrix::iterator &mit, Matrix &rMatrix, Slots &results, int &cycleCount)
{
	int count, start;
	Set mSNeighbors;

	while (true)
	{
		mit_lock.lock();
		if (mit != matrix.end())
		{
			start = (*mit).first;
			++mit;
			mit_lock.unlock();
		}
		else
		{
			mit_lock.unlock();
			break;
		}

		// multiStepNeighbors(matrix, start, maxStep, mSNeighbors);		// not necessary
		multiStepNeighbors(rMatrix, start, MAX_STEP, mSNeighbors);
		count = dfs(matrix, start, MIN_LEN, MAX_LEN, mSNeighbors, results);
		mSNeighbors.clear();

		cycleCount += count;
	}
}

void mergeResults(Slots threadResults[TOTAL_THREADS], Slots &results, const int minLen, const int maxLen)
{
	int userID;

	for (int tID = 0; tID < TOTAL_THREADS; tID++)
	{ // for each thread's result slots
		for (int pathLen = minLen; pathLen <= maxLen; pathLen++)
		{ // for each pathLen
			for (map<int, vector<string>>::iterator mit = threadResults[tID][pathLen].begin(); mit != threadResults[tID][pathLen].end(); mit++)
			{ // for each user's cycle paths
				userID = (*mit).first;
				results[pathLen][userID].assign((*mit).second.begin(), (*mit).second.end());
			}
		}
	}
}

void resultToFile(Slots &results, const string filename, const int minLen, const int maxLen, const int count)
{
	int outCount;
	ofstream outText(filename.c_str());

	outCount = 0;
	outText << count << endl;
	for (int i = minLen; i <= maxLen; i++)
	{ // i-len path
		for (map<int, vector<string>>::iterator mit = results[i].begin(); mit != results[i].end(); mit++)
		{ // i-len path -> start from id j
			for (int j = 0; j < (*mit).second.size(); j++)
			{ // i-len path -> paths start from id j -> each path
				outText << (*mit).second[j] << endl;
			}
		}
	}
	outText.close();
}