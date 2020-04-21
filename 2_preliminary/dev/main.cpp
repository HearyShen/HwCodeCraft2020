#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include <stack>
#include <set>
#include <map>
#include <thread>
#include <mutex>
#include <chrono>

#define MIN_LEN 3
#define MAX_LEN 7
#define TOTAL_THREADS 4

using namespace std;

typedef vector<int> List;						  // [id1, id2, ..., idn]
typedef stack<List> DfsStack;					  // Stack{path1, path2, ...}
typedef map<int, set<int>> Matrix;				  // {from: toes}
typedef map<int, map<int, vector<string>>> Slots; // {len: {start_id: paths}}}

int fileToMatrix(const string filename, Matrix &matrix);
void displayMatrix(Matrix &matrix);
bool listHas(const List &list, const int num);
string listToString(const List &list);
int dfs(Matrix &matrix, const int start, const int minLen, const int maxLen, Slots &results);
void dfsThread(const int threadID, Matrix &matrix, Matrix::iterator &mit, Slots &results, int &cycleCount);
void resultToFile(Slots &results, const string filename, const int minLen, const int maxLen, const int count);

mutex mit_lock, count_lock, results_lock;

int main(int argc, char *argv[])
{
	string filename;
	string outFilename = "result.txt";
	Matrix matrix;
	int retCode, start, cycleCount;
	clock_t tic, toc;
	time_t ttic, ttoc;
	Slots results;
	thread threads[TOTAL_THREADS];

	ttic = time(NULL);
	tic = clock();
	filename = argv[1];
	retCode = fileToMatrix(filename, matrix);
	toc = clock();
	cout << "Readfile to matrix: " << retCode << " lines in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;

	tic = clock();
	cycleCount = 0;
	Matrix::iterator mit = matrix.begin();
	for (int i = 0; i < TOTAL_THREADS; i++)
	{	
		threads[i] = thread(dfsThread, i, ref(matrix), ref(mit), ref(results), ref(cycleCount));
		cout << "> dfsThread: " << i << " created" << endl;
	}
	for (int i = 0; i < TOTAL_THREADS; i++)
	{
		threads[i].join();
		cout << "> dfsThread: " << i << " joined" << endl;
	}
	toc = clock();
	cout << "Detected " << cycleCount << " cycles in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;

	tic = clock();
	resultToFile(results, outFilename, MIN_LEN, MAX_LEN, cycleCount);
	toc = clock();
	ttoc = time(NULL);
	cout << "Written to file in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;
	cout << "Total finished in " << difftime(ttoc, ttic) << "s" << endl;

	return 0;
}

int fileToMatrix(const string filename, Matrix &matrix)
{
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
		matrix[from].insert(to);
		count++;
	}
	inText.close();

	return count;
}

void displayMatrix(Matrix &matrix)
{
	for (Matrix::iterator mit = matrix.begin(); mit != matrix.end(); mit++)
	{
		cout << (*mit).first << ": ";
		for (set<int>::iterator sit = (*mit).second.begin(); sit != (*mit).second.end(); sit++)
		{
			cout << *sit << " ";
		}
		cout << endl;
	}
}

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
	string s = to_string(list[0]);
	for (int i = 1; i < list.size(); i++)
	{
		s += "," + to_string(list[i]);
	}
	return s;
}

int dfs(Matrix &matrix, const int start, const int minLen, const int maxLen, Slots &results)
{
	DfsStack dfsStack;
	List curPath, nextPath;
	set<int> nextNodes;
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

		nextNodes = matrix[curNode];
		for (set<int>::reverse_iterator sit = nextNodes.rbegin(); sit != nextNodes.rend(); sit++)
		{
			nextNode = *sit;

			if (nextNode == curPath[0])
			{ // if target cycle detected, then record it
				if (minLen <= curLen && curLen <= maxLen)
				{
					// valid length
					results_lock.lock();
					results[curLen][curPath[0]].push_back((listToString(curPath)));
					results_lock.unlock();
					cycleCount++;
				}
			}
			else
			{ // if not target cycle, then search deeper
				if (curLen < maxLen && nextNode > curPath[0] && !listHas(curPath, nextNode))
				{
					nextPath.assign(curPath.begin(), curPath.end());
					nextPath.push_back(nextNode);
					dfsStack.push(nextPath);
				}
			}
		}
	}
	return cycleCount;
}

void dfsThread(const int threadID, Matrix &matrix, Matrix::iterator &mit, Slots &results, int &cycleCount)
{
	int count;
	int start;

	// cout << "> dfsThread: " << threadID << " running" << endl;
	this_thread::sleep_for(chrono::seconds(threadID*2));	// NOTE: this avoids segment fault.
	
	while (true)
	{
		mit_lock.lock();
		if (mit != matrix.end())
		{
			start = (*mit).first;
			mit++;
			mit_lock.unlock();
		}
		else
		{
			mit_lock.unlock();
			break;
		}
		
		
		// cout << "Scaning from " << start << " in thread " << threadID << endl;
		count = dfs(matrix, start, MIN_LEN, MAX_LEN, results);

		count_lock.lock();
		cycleCount += count;
		count_lock.unlock();
	}
	// cout << "> dfsThread: " << threadID << " exiting" << endl;
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