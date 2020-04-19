#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <vector>
#include <queue>
#include <map>

#define MIN_LEN 3
#define MAX_LEN 7

using namespace std;

typedef vector<int> List;			  // [id1, id2, ..., idn]
typedef queue<List> Queue;			  // Queue([id1, id2, ...], [id2, id3, ...])
typedef map<int, List> Matrix;		  // {from: toes}
typedef map<int, vector<List> > Slots; // {len: paths}

int fileToMatrix(string filename, Matrix &matrix);
void displayMatrix(Matrix &matrix);
bool listHas(List &list, int num);
int bfs(Matrix &matrix, int start, int minLen, int maxLen, Slots &results);
void resultToFile(Slots &results, string filename, int minLen, int maxLen, int count);

int main(int argc, char* argv[])
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
		cycleCount += bfs(matrix, start, MIN_LEN, MAX_LEN, results);
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
		if (amount > 0)
		{
			matrix[from].push_back(to);
		}
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
		for (int i = 0; i < (*mit).second.size(); i++)
		{
			cout << (*mit).second[i] << " ";
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

int bfs(Matrix &matrix, int start, int minLen, int maxLen, Slots &results)
{
	List startPath, curPath;
	Queue bfsQueue;
	int curNode, nextNode;
	List nextNodes;
	int len, bfsQueueLen, cycleCount;

	len = 0;
	cycleCount = 0;
	startPath.push_back(start);
	bfsQueue.push(startPath);
	while (!bfsQueue.empty())
	{
		curPath = bfsQueue.front();
		bfsQueue.pop();

		curNode = curPath.back();
		nextNodes = matrix[curNode];
		for (int j = 0; j < nextNodes.size(); j++)
		{
			nextNode = nextNodes[j];
			if (listHas(curPath, nextNode))
			{ // cycle detected
				len = curPath.size();
				if (minLen <= len && len <= maxLen && nextNodes[j] == start)
				{
					results[len].push_back(curPath);
					cycleCount++;
				}
			}
			else
			{ // no cycle
				if (nextNode > start && curPath.size() < maxLen)
				{
					List nextPath(curPath);
					nextPath.push_back(nextNode);
					bfsQueue.push(nextPath);
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
	{ // i-th slot
		for (int j = 0; j < results[i].size(); j++)
		{ // j-th list in i-th slot
			// outText << listToString(results[i][j]) << endl;
			pathLen = results[i][j].size();
			for (int k = 0; k < pathLen; k++)
			{ // k-th id in j-th list in i-th slot
				outText << results[i][j][k];
				if (k < pathLen - 1)
				{
					outText << ",";
				}
			}
			outText << endl;
			outCount++;
		}
	}
	outText.close();
}