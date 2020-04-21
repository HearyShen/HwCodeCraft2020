#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include <stack>
#include <set>
#include <map>

#define MIN_LEN 3
#define MAX_LEN 7

using namespace std;

typedef vector<int> List;			  // [id1, id2, ..., idn]
typedef stack<List> DfsStack;		  // Stack{path1, path2, ...}
typedef map<int, set<int>> Matrix;		  // {from: toes}
typedef map<int, map<int, vector<string>>> Slots; // {len: {start_id: paths}}}

int fileToMatrix(string filename, Matrix &matrix);
void displayMatrix(Matrix &matrix);
bool listHas(List &list, int num);
string listToString(List &list);
int dfs(Matrix &matrix, int start, int minLen, int maxLen, Slots &results);
void resultToFile(Slots &results, string filename, int minLen, int maxLen, int count);

int main(int argc, char *argv[])
{
	string filename;
	string outFilename;
	Matrix matrix;
	int retCode, start, cycleCount;
	clock_t tic, toc, ttic, ttoc;
	Slots results;

	ttic = clock();
	tic = clock();
	filename = "/data/test_data.txt";
	outFilename = "/projects/student/result.txt";
	retCode = fileToMatrix(filename, matrix);
	toc = clock();
	cout << "Readfile to matrix: " << retCode << " lines in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;

	tic = clock();
	cycleCount = 0;
	for (Matrix::iterator mit = matrix.begin(); mit != matrix.end(); mit++)
	{
		start = (*mit).first;
		cycleCount += dfs(matrix, start, MIN_LEN, MAX_LEN, results);
	}
	toc = clock();
	cout << "Detected " << cycleCount << " cycles in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;

	tic = clock();
	resultToFile(results, outFilename, MIN_LEN, MAX_LEN, cycleCount);
	toc = clock();
	ttoc = clock();
	cout << "Written to file in " << (double)(toc - tic) / CLOCKS_PER_SEC << "s" << endl;
	cout << "Total finished in " << (double)(ttoc - ttic) / CLOCKS_PER_SEC << "s" << endl;

	return 0;
}

int fileToMatrix(string filename, Matrix &matrix)
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

bool listHas(List &list, int num)
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

string listToString(List &list)
{
	string s = to_string(list[0]);
	for (int i=1; i< list.size(); i++)
	{
		s += "," + to_string(list[i]);
	}
	return s;
}

int dfs(Matrix &matrix, int start, int minLen, int maxLen, Slots &results)
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
			{ 	// if target cycle detected, then record it
				if (minLen <= curLen && curLen <= maxLen)
				{
					// valid length
					results[curLen][curPath[0]].push_back((listToString(curPath)));
					cycleCount++;
				}
			}
			else
			{	// if not target cycle, then search deeper
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

void resultToFile(Slots &results, string filename, int minLen, int maxLen, int count)
{
	int outCount, pathLen;
	ofstream outText(filename.c_str());

	outCount = 0;
	outText << count << endl;
	for (int i = minLen; i <= maxLen; i++)
	{ // i-len path
		for (map<int, vector<string>>::iterator mit = results[i].begin(); mit != results[i].end(); mit++)
		{	// i-len path -> start from id j
			for (int j=0; j<(*mit).second.size(); j++)
			{	// i-len path -> paths start from id j -> each path
				outText << (*mit).second[j] << endl;
			}
		}
	}
	outText.close();
}