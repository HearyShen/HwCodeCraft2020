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

#define MIN_LEN 3
#define MAX_LEN 7
#define TOTAL_THREADS 3

using namespace std;

typedef vector<int> List;						  // [id1, id2, ..., idn]
typedef stack<List> DfsStack;					  // Stack{path1, path2, ...}
typedef map<int, vector<int>> Matrix;				  // {from: toes}
typedef map<int, map<int, vector<string>>> Slots; // {len: {start_id: paths}}}

int fileToMatrix(const string filename, Matrix &matrix);
// void displayMatrix(Matrix &matrix);
bool listHas(const List &list, const int num);
string listToString(const List &list);
int dfs(Matrix &matrix, const int start, const int minLen, const int maxLen, Slots &results);
void dfsThread(const int threadID, Matrix &matrix, Matrix::iterator &mit, Slots &results, int &cycleCount);
void mergeResults(Slots threadResults[TOTAL_THREADS], Slots &results, const int minLen, const int maxLen);
void resultToFile(Slots &results, const string filename, const int minLen, const int maxLen, const int count);

mutex mit_lock, matrix_lock;
time_t g_tic = time(NULL);

int main(int argc, char *argv[])
{
	string filename;
	string outFilename = "result.txt";
	Matrix matrix;
	int retCode, start, cycleCount;
	int cycleCounts[TOTAL_THREADS] = {0};
	clock_t tic, toc;
	time_t ttic, ttoc;
	Slots results, threadResults[TOTAL_THREADS];
	thread threads[TOTAL_THREADS];

	ttic = time(NULL);
	tic = clock();
	filename = argv[1];
	retCode = fileToMatrix(filename, matrix);
	toc = clock();
	cout << "Readfile to matrix: " << retCode << " lines in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;

	tic = clock();
	Matrix::iterator mit = matrix.begin();
	for (int i = 0; i < TOTAL_THREADS; i++)
	{	
		threads[i] = thread(dfsThread, i, ref(matrix), ref(mit), ref(threadResults[i]), ref(cycleCounts[i]));
		cout << "> dfsThread: " << i << " created" << endl;
	}
	for (int i = 0; i < TOTAL_THREADS; i++)
	{
		threads[i].join();
	}
	cycleCount = 0;
	for (int i=0; i < TOTAL_THREADS; i++)
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

int fileToMatrix(const string filename, Matrix &matrix)
{
	map<int, set<int>> orderedMatrix;	// use set for ordered insert
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
		orderedMatrix[from].insert(to);
		count++;
	}
	inText.close();

	// write the ordered nextNodes in to matrix
	for (map<int, set<int>>::iterator mit=orderedMatrix.begin(); mit != orderedMatrix.end(); mit++)
	{
		userID = (*mit).first;
		matrix[userID].assign((*mit).second.begin(), (*mit).second.end());
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
	vector<int> nextNodes;
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

		matrix_lock.lock();
		nextNodes = matrix[curNode];
		matrix_lock.unlock();
		for (int i=nextNodes.size()-1; i>=0; i--)
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
		
		count = dfs(matrix, start, MIN_LEN, MAX_LEN, results);

		cycleCount += count;
	}
}

void mergeResults(Slots threadResults[TOTAL_THREADS], Slots &results, const int minLen, const int maxLen)
{
	int userID;

	for (int tID=0; tID < TOTAL_THREADS; tID++)
	{	// for each thread's result slots
		for (int pathLen = minLen; pathLen <= maxLen; pathLen++)
		{ // for each pathLen
			for (map<int, vector<string>>::iterator mit=threadResults[tID][pathLen].begin(); mit != threadResults[tID][pathLen].end(); mit++)
			{	// for each user's cycle paths
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