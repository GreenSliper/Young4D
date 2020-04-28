#include <stdio.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <atomic>
#include "stdlib.h"
#include "InfInt.h"
#include <algorithm>

using namespace std;
typedef unsigned long long ull;
typedef vector<vector<vector<unsigned int>>> v3i;
typedef vector<vector<vector<bool>>> v3b;
typedef vector<vector<bool>> v2b;
typedef vector<bool> vb;
//string constants
const string COLUMN_DELIM = " ";
const string LAYER2D_DELIM = ",";
const string LAYER3D_DELIM = ";";
const string LEX_DELIM = "|";
const string CONNECTED_DELIM = " ";
//lex names
const string lexDimension = "dim";
const string lexGeneration = "level";
const string lexId = "id";
const string lexConnected = "connected";

class Young2D {
public:
	vector<short> columns = { 1 };
	Young2D(int startCubesInZeroPoint) {
		columns.at(0) = startCubesInZeroPoint;
	}
	Young2D() {}
	//returns 1 if added 0 if not. If you use checkOnly it will only return the possibility of adding a cube.
	int AddCube(int x, bool checkOnly = false)
	{
		if (x < 0)
			return 0;
		if (columns.size() == x) //you can append if previous not zero
		{
			if (columns.at(x - 1) > 0) {
				if(!checkOnly)
					columns.push_back(1);	//new column
				return 1;
			}
		}
		else if ((int)columns.size() > x)
		{
			if (x==0 || columns.at(x - 1) > columns.at(x)) {
				if (!checkOnly)
					columns.at(x)++;
				return 1;
			}
		}
		//previous column does not exist
		return 0;
	}

	int GetColumn(int x)
	{
		if ((int)columns.size() > x)
			return columns.at(x);
		else
			return 0;
	}

	void Print()
	{
		for (int i = 0; i < (int)columns.size(); i++)
			cout << columns[i] << ' ';
		cout << endl;
	}

	string ToString()
	{
		string result = "";
		int i;
		for (i = 0; i < (int)columns.size() - 1; i++)
			result += to_string(columns[i]) + COLUMN_DELIM;
		result += to_string(columns[i]);
		return result;
	}

	void Clear()
	{
		columns.clear();
	}

	Young2D* Clone()
	{
		Young2D* newObj = new Young2D();
		newObj->columns = columns;
		return newObj;
	}

	bool operator==(Young2D& other) const
	{
		bool result = columns.size() == other.columns.size();
		for (int x = 0; result && x < columns.size(); x++)
			result = columns[x] == other.columns[x];
		return result;
	}

};

class Young3D {
public:
	vector<Young2D> layers;
	Young3D(int startCubesInZeroPoint) {
		layers = { Young2D(1) };
		layers.at(0).columns.at(0) = startCubesInZeroPoint;
	}
	Young3D() {}
	//returns 1 if added 0 if not. If you use checkOnly it will only return the possibility of adding a cube
	int AddCube(int x, int y, bool checkOnly = false)
	{
		if (y < 0)
			return 0;
		if (y == (int)layers.size()) // new layer needed
		{
			if (x==0 && layers.at(y - 1).columns.at(0) > 0) {
				if (!checkOnly)
					layers.push_back(Young2D(1));
				return 1;
			}
		}
		else if (y < (int)layers.size()) //add in existing layer
			if(y==0 || layers.at(y - 1).GetColumn(x) > layers.at(y).GetColumn(x)) //y-condition check
				return layers.at(y).AddCube(x, checkOnly);
		//err, cannot be added
		return 0;
	}

	Young2D* GetLayer(int index)
	{
		if (index < (int)layers.size())
			return &layers[index];
		return NULL;
	}

	void Print()
	{
		for (int i = 0; i < (int)layers.size(); i++)
			layers[i].Print();
		cout << endl;
	}

	string ToString()
	{
		string result = "";
		int i;
		for (i = 0; i < (int)layers.size() - 1; i++)
			result += layers[i].ToString() + LAYER2D_DELIM;
		result += layers[i].ToString();
		return result;
	}

	Young3D* Clone()
	{
		Young3D* newObj = new Young3D();
		newObj->layers = layers;
		for (int x = 0; x < (int)layers.size(); x++)
			newObj->layers[x] = *layers[x].Clone();
		return newObj;
	}

	void Clear()
	{
		for (int i = 0; i < (int)layers.size(); i++)
			layers[i].Clear();
	}

	bool operator==(Young3D& other) const
	{
		bool result = layers.size() == other.layers.size();
		for (int x = 0; result && x < layers.size(); x++)
			result = layers[x] == other.layers[x];
		return result;
	}

};

class Young4D {
public:
	vector<Young3D> layers;
	//dimension of current 4d graph
	InfInt dim = "1";
	InfInt id = "0";
	vector<InfInt> connectedGraphs;
	v3b cornerMatrix; 
	
	Young4D(int startCubesInZeroPoint) {
		layers = { Young3D(1) };
		layers.at(0).GetLayer(0)->columns.at(0) = startCubesInZeroPoint;
		cornerMatrix = { { {true, /*1, 0, 0*/ true}, { /*0, 1, 0*/true} }, {{/*1, 0, 0*/true}} }; //default matrix
	}

	Young4D() {}
	
	//returns 1 if added 0 if not
	int AddCube(int x, int y, int z, bool checkOnly = false, bool RecalcCorner = true)
	{
		if (z < 0)
			return 0;
		if (z == (int)layers.size()) // new 3d layer needed
		{
			Young2D* lyr;
			if ((lyr = layers.at(z - 1).GetLayer(y)) != NULL) {
				if (y == 0 && x == 0 && lyr->GetColumn(x) != 0)
				{
					if (!checkOnly) {
						layers.push_back(Young3D(1));
						if (RecalcCorner)
							RecalcCornerMatrix(x, y, z);
					}
					return 1;
				}
			}
		}
		else if (z < (int)layers.size())	//add in existing layer
		{
			Young2D* lyr, *lyr1 = NULL;
			if (z == 0 || ((lyr = layers.at(z - 1).GetLayer(y))!=NULL && (lyr1 = layers.at(z).GetLayer(y)) == NULL || (lyr1!=NULL
				&& lyr->GetColumn(x) > lyr1->GetColumn(x)))) //z - condition check
				if (layers.at(z).AddCube(x, y, checkOnly))
				{
					if (RecalcCorner && !checkOnly)
						RecalcCornerMatrix(x, y, z);
					return 1;
				}
		}
		//err, cannot be added
		return 0;
	}

	void RecalcCornerPoint(int x, int y, int z)
	{
		//increase the size of container if needed
		while (cornerMatrix.size() <= z)
			cornerMatrix.push_back({ {false} });
		while (cornerMatrix.at(z).size() <= y)
			cornerMatrix.at(z).push_back({ false });
		while (cornerMatrix.at(z).at(y).size() <= x)
			cornerMatrix.at(z).at(y).push_back(false);
		//just change the value
		cornerMatrix.at(z).at(y).at(x) = AddCube(x, y, z, true, false);
	}

	//pass coords of the added cube
	void RecalcCornerMatrix(int x, int y, int z)
	{
		RecalcCornerPoint(x, y, z);
		RecalcCornerPoint(x + 1, y, z);
		RecalcCornerPoint(x, y + 1, z);
		RecalcCornerPoint(x, y, z + 1);
	}

	vector<Young4D> CreateNextGen(bool recalcChildren = true)
	{
		vector<Young4D> nextGen = {};
		for (int z = 0; z < (int)cornerMatrix.size(); z++)
			for (int y = 0; y < (int)cornerMatrix[z].size(); y++)
				for (int x = 0; x < (int)cornerMatrix[z][y].size(); x++)
					if (cornerMatrix[z][y][x]) {
						Young4D *cln = Clone();
						cln->AddCube(x, y, z, false, recalcChildren);
						cln->dim = InfInt(dim);
						cln->connectedGraphs.push_back(id);
						nextGen.push_back(*cln);
					}
		return nextGen;
	}

	void Print()
	{
		for (int i = 0; i < (int)layers.size(); i++)
			layers[i].Print();
		cout << endl;
	}

	string GetIncidenceLine()
	{
		string line = "";
		InfInt ind = 0;
		for (int x = 0; x < connectedGraphs.size() && connectedGraphs[x] < id; x++)
		{
			while (ind++ < connectedGraphs[x])
				line += "0" + CONNECTED_DELIM;
			line += "1" + CONNECTED_DELIM;
		}
		while (ind++ < id)
			line += "0" + CONNECTED_DELIM;
		line += '-';
		return line;
	}

	char* GetIncidenceLineFast()
	{
		char* line = new char[id.toLongLong()+1];// (char*)malloc(id.toLongLong() * sizeof(char));
		ull index = 0;
		for (int x = 0; x < connectedGraphs.size() && connectedGraphs[x] < id; x++)
		{
			ull con = connectedGraphs[x].toUnsignedLong();
			while (index++ < con)
				line[index - 1] = '0';
			line[index - 1] = '1';
		}
		ull idUll = id.toUnsignedLong();
		while (index++ < idUll)
			line[index - 1] = '0';
		line[index - 1] = '-';
		return line;
	}

	string GetConnected()
	{
		string connected = "";
		int x;
		if (connectedGraphs.size() != 0)
		{
			for (x = 0; x < connectedGraphs.size() - 1; x++)
				connected += connectedGraphs[x].toString() + CONNECTED_DELIM;
			connected += connectedGraphs[x].toString();
		}
		return connected;
	}

	//exportFormat - add id, dimension and connected graphs' id-s. Add generation for full format
	string ToString(bool useExportFormat = true, int generation = -1)
	{
		string result = "";
		int i;
		for (i = 0; i < (int)layers.size()-1; i++)	//form graph base
			result += layers[i].ToString() + LAYER3D_DELIM;
		result += layers[i].ToString();
		//add format
		if (useExportFormat)
		{
			if (generation >= 0)
				result += LEX_DELIM + lexGeneration + "=" + to_string(generation);
			result += LEX_DELIM + lexId + "=" + id.toString()
				+ LEX_DELIM + lexDimension +"=" + dim.toString()
				+ LEX_DELIM + lexConnected + "=" + GetConnected();
		}
		return result;
	}

	void PrintCorners()
	{
		for (int z = 0; z < (int)cornerMatrix.size(); z++)
		{
			for (int y = 0; y < (int)cornerMatrix[z].size(); y++)
			{
				for (int x = 0; x < (int)cornerMatrix[z][y].size(); x++)
				{
					if (cornerMatrix[z][y][x])
						cout << "1 ";
					else
						cout << "0 ";
				}
				cout << endl;
			}
			cout << endl;
		}
	}

	Young4D* Clone(bool cloneDim = false)
	{
		Young4D* newObj = new Young4D();
		newObj->layers = layers;
		newObj->cornerMatrix = cornerMatrix;
		for (int x = 0; x < (int)layers.size(); x++)
			newObj->layers[x] = *layers[x].Clone();
		if (cloneDim)
			newObj->dim = dim;
		return newObj;
	}

	void Clear()
	{
		for (int i = 0; i < (int)layers.size(); i++)
			layers[i].Clear();
		connectedGraphs.clear();
	}

	bool operator==(Young4D& other) const
	{
		bool result = layers.size() == other.layers.size();
		for (int x = 0; result && x < layers.size(); x++)
			result = layers[x] == other.layers[x];
		return result;
	}
};


vector<string> split(const string& str, const string& delim)
{
	vector<string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == string::npos) pos = str.length();
		string token = str.substr(prev, pos - prev);
		if (!token.empty()) tokens.push_back(token);
		prev = pos + delim.length();
	} while (pos < str.length() && prev < str.length());
	return tokens;
}


void PrintYoung4DTree(vector<vector<Young4D>> generations, bool drawCorners = false)
{
	for (int x = 0; x < generations.size(); x++)
	{
		cout << "LEVEL " << x + 1 << endl;
		for (int y = 0; y < generations[x].size(); y++) {
			cout << "ID " << generations[x][y].id.toString() << "; VARIANT " << y + 1 << "; DIMENSION " << generations[x][y].dim.toString() << endl;
			cout << "CONNECTED: " << generations[x][y].GetConnected() << endl;
			generations[x][y].Print();
			if (drawCorners) {
				cout << "CORNERS: " << endl;
				generations[x][y].PrintCorners();
			}
		}
	}
}

//simpleFormat = write only diagram & dim
void ExportAppendYoung4DLevel(vector<Young4D> generations, int levelNum, string fileName = "Young4D_Graph.txt", bool writeGenerationForEach = false, bool addLineBetweenLevels = true, bool simpleExport = false, bool rewrite = false)
{
	ofstream out;
	if(rewrite)
		out.open(fileName);
	else
		out.open(fileName, ios_base::app);
	if (simpleExport)
	{
		for (ull y = 0; y < generations.size(); y++)
			out << generations[y].ToString(false) << '|' << generations[y].dim.toString() << endl;
	}
	else if (writeGenerationForEach)
		for (ull y = 0; y < generations.size(); y++)
			out << generations[y].ToString(true, levelNum + 1) << endl;		//adds generation automatically
	else
	{
		out << lexGeneration + "=" << levelNum + 1 << endl;					//add generation line manually
		for (ull y = 0; y < generations.size(); y++)
			out << generations[y].ToString(true) << endl;
		if (addLineBetweenLevels)
			out << endl;
	}
	out.close();
}

//simpleFormat = write only diagram & dim
void ExportYoung4DTree(vector<vector<Young4D>> generations, string fileName = "Young4D_Graph.txt", bool writeGenerationForEach = false, bool addLineBetweenLevels = true, bool simpleExport = false)
{
	ofstream out;
	out.open(fileName);
	for (int x = 0; x < generations.size(); x++)
	{
		if (simpleExport)
		{
			for (ull y = 0; y < generations[x].size(); y++)
				out << generations[x][y].ToString(false) << '|' << generations[x][y].dim.toString() << endl;
		}
		else if (writeGenerationForEach)
			for (ull y = 0; y < generations[x].size(); y++)
				out << generations[x][y].ToString(true, x + 1) << endl;		//adds generation automatically
		else
		{
			out << lexGeneration + "=" << x + 1 << endl;						//add generation line manually
			for (ull y = 0; y < generations[x].size(); y++)
				out << generations[x][y].ToString(true) << endl;
			if (addLineBetweenLevels)
				out << endl;
		}
	}
	out.close();
}

void ExportAppendIncidenceMatrix(vector<Young4D> generations, string fileName = "Young4D_Incidence.txt")
{
	ofstream out;
	out.open(fileName, ios_base::app);
	for (ull y = 0; y < generations.size(); y++)
	{
		if (generations[y].connectedGraphs.size() > 0)
		{
			char *incLine = generations[y].GetIncidenceLineFast();
			out.write(incLine, generations[y].id.toLongLong()+1);
			out << endl;
			delete[] incLine;
		}
		else
			out << '-' << endl;
	}
	out.close();
}

void ExportIncidenceMatrix(vector<vector<Young4D>> generations, string fileName = "Young4D_Incidence.txt")
{
	ofstream out;
	out.open(fileName);
	for (int x = 0; x < generations.size(); x++)
	{
		for (ull y = 0; y < generations[x].size(); y++)
		{
			if (generations[x][y].connectedGraphs.size() > 0)
			{
				out << generations[x][y].GetIncidenceLine() << endl;
			}
			else
				out << '-' << endl;
		}
	}
	out.close();
}

string dimSumSeq1 = "", dimSumSeq2 = "";
void ExportAppendDimensionSumSequences(vector<Young4D> generations, string fileName = "Young4D_DimSumSequences.txt")
{
	ofstream out;
	out.open(fileName);
	InfInt dimSum = 0, dimSqrSum = 0;
	for (int y = 0; y < generations.size(); y++) {
		dimSum += generations[y].dim;
		dimSqrSum += generations[y].dim * generations[y].dim;
	}
	dimSumSeq1 += dimSum.toString() + " ";
	dimSumSeq2 += dimSqrSum.toString() + " ";
	out << dimSumSeq1 << endl;
	out << dimSumSeq2 << endl;
	out.close();
}

void ExportDimensionSumSequences(vector<vector<Young4D>> generations, string fileName = "Young4D_DimSumSequences.txt")
{
	ofstream out;
	out.open(fileName);
	string seq1 = "", seq2 = "";
	for (int x = 0; x < generations.size(); x++)
	{
		InfInt dimSum = 0, dimSqrSum = 0;
		for (int y = 0; y < generations[x].size(); y++) {
			dimSum += generations[x][y].dim;
			dimSqrSum += generations[x][y].dim * generations[x][y].dim;
		}
		seq1 += dimSum.toString() + " ";
		seq2 += dimSqrSum.toString() + " ";
	}
	out << seq1 << endl;
	out << seq2 << endl;
	out.close();
}

void FreeYoung4DTree(vector<vector<Young4D>> &generations)
{
	for (int x = 0; x < generations.size(); x++)
	{
		for (int y = 0; y < generations[x].size(); y++)
			generations[x][y].Clear();
		generations[x].clear();
	}
	generations.clear();
}

vector<vector<Young4D>> GenerateYoung4DTree(Young4D zeroGraph = Young4D(1), bool appendEverything = false, bool countInfinite = false)
{
	int genCount = 0;
	if (!countInfinite) {
		cout << "Generation count:" << endl;
		cin >> genCount;
	}
	else
		genCount = INT32_MAX;
	vector<vector<Young4D>> generations(genCount);
	zeroGraph.id = 1;
	generations[0] = { zeroGraph };
	if (appendEverything)
	{
		ExportAppendYoung4DLevel((vector<Young4D>)generations[0], 0, "Young4D_Graph.txt", false, true, false, true);
		ExportYoung4DTree(generations, "Young4D_BestInGenerations.txt", false, false, true);
		ExportIncidenceMatrix(generations);
		ExportAppendDimensionSumSequences(generations[0]);
	}
	InfInt lastID = 2; //1st id + 1
	for (int x = 0; x < generations.size() - 1; x++)
	{
		cout << "Calculating generation " << x + 2 << "..." << endl;
		vector<Young4D> ngen; int lastCheckedIndex = 0;
		for (ull y = 0; y < generations[x].size(); y++) {
			ngen = generations[x][y].CreateNextGen();
			generations[x + 1].insert(generations[x + 1].end(), ngen.begin(), ngen.end());
			//search for same
			for (int y = 0; y < generations[x + 1].size(); y++) {
				for (int z = max(y + 1, lastCheckedIndex); z < generations[x + 1].size(); z++) {
					if (generations[x + 1][y] == generations[x + 1][z])
					{
						generations[x + 1][y].dim += generations[x + 1][z].dim;
						//2 different parents, same children
						generations[x + 1][y].connectedGraphs.insert(generations[x + 1][y].connectedGraphs.end(), generations[x + 1][z].connectedGraphs.begin(), generations[x + 1][z].connectedGraphs.end());
						generations[x + 1][z].Clear();
						generations[x + 1].erase(generations[x + 1].begin() + z);
						z--;
					}
				}
			}
			lastCheckedIndex = generations[x + 1].size() - 1;
		}
		//add id-s to new objects, add children IDs to parents' 'connected' lists
		for (int y = 0; y < generations[x+1].size(); y++) {
			generations[x + 1][y].id = lastID++;
			InfInt firstIdLastGen = generations[x][0].id;
			for (int i = 0; i < generations[x + 1][y].connectedGraphs.size(); i++)
				generations[x][(generations[x + 1][y].connectedGraphs[i] - firstIdLastGen).toLongLong()].connectedGraphs.push_back(lastID - 1);
		}

		if (appendEverything)
		{
			vector<Young4D> best(1); InfInt bestDim = -1;
			for (int y = 0; y < generations[x + 1].size(); y++) {
				if (generations[x + 1][y].dim > bestDim)
				{
					best[0] = generations[x + 1][y];
					bestDim = generations[x + 1][y].dim;
				}
			}
			if(x==0)
				ExportAppendYoung4DLevel((vector<Young4D>)generations[x], x, "Young4D_GraphWithChildren.txt", false, true, false, true);
			else
				ExportAppendYoung4DLevel((vector<Young4D>)generations[x], x, "Young4D_GraphWithChildren.txt");
			ExportAppendYoung4DLevel((vector<Young4D>)generations[x + 1], x + 1);
			ExportAppendYoung4DLevel(best, 0, "Young4D_BestInGenerations.txt", false, false, true);
			ExportAppendIncidenceMatrix(generations[x + 1]);
			ExportAppendDimensionSumSequences(generations[x + 1]);
			for (int i = 0; i < generations[x].size(); i++)
				generations[x][i].Clear();
			generations[x].clear();
		}

	}
	return generations;
}

vector<vector<Young4D>> GenerateYoung4DTreeBestOnly(Young4D* zeroGraph = new Young4D(1), bool findOneOnly = true)
{
	int genCount = 0;
	cout << "Generation count:" << endl;
	cin >> genCount;
	vector<vector<Young4D>> generations(genCount);
	vector<vector<Young4D>> bestInGenerations(genCount);
	generations[0] = bestInGenerations[0] = { *zeroGraph };
	for (int x = 0; x < generations.size() - 1; x++)
	{
		vector<Young4D> ngen; vector<Young4D*> best; InfInt bestDim = -1; int lastCheckedIndex = 0;
		for (int y = 0; y < generations[x].size(); y++) {
			ngen = generations[x][y].CreateNextGen();
			//clear parent
			generations[x][y].Clear();
			generations[x + 1].insert(generations[x + 1].end(), ngen.begin(), ngen.end());
			//search for same
			for (int y = 0; y < generations[x + 1].size(); y++) {
				for (int z = max(y + 1, lastCheckedIndex); z < generations[x + 1].size(); z++) {
					if (generations[x + 1][y] == generations[x + 1][z])
					{
						generations[x + 1][y].dim += generations[x + 1][z].dim;
						generations[x + 1][z].Clear();
						generations[x + 1].erase(generations[x + 1].begin() + z);
						z--;
					}
				}
			}
			lastCheckedIndex = generations[x + 1].size() - 1;
		}
		//quickly find best partitions
		for (int y = 0; y < generations[x + 1].size(); y++) {
			if (generations[x + 1][y].dim > bestDim)
			{
				best.clear();
				bestDim = generations[x + 1][y].dim;
				best.push_back(&generations[x + 1][y]);
			}
			else if (!findOneOnly && generations[x + 1][y].dim == bestDim)
				best.push_back(&generations[x + 1][y]);
		}
		//save them to vector
		for (int y = 0; y < best.size(); y++) {
			bestInGenerations[x + 1].push_back(*(best[y]->Clone(true)));
		}
		//clear prev generation
		generations[x].clear();
	}
	generations.clear();
	return bestInGenerations;
}

int main()
{
	vector<vector<Young4D>> generations, fastGen;
	int dialogueResult = 0;
	char yn = ' ';
	do {
		cout << "Menu" << endl << "1. Generate enumeration of all solid partitions with sizes <= N" << endl << "2. Build incidence matrix"
			<< endl << "3. Build dimension sum sequence" << endl << "4. Find partitions with max dimension on each graph level"
			<< endl << "5. Calculate everything" << endl << "6. Exit" << endl;
		cin >> dialogueResult;
		switch (dialogueResult)
		{
		case 1: 
			FreeYoung4DTree(generations);
			generations = GenerateYoung4DTree();
			ExportYoung4DTree(generations);
			break;
		case 2: 
			if (generations.size() == 0) {
				cout << "Generations tree not built. Do it now? (y/n)" << endl;
				cin >> yn;
				if (yn == 'y' || yn == 'Y')
					generations = GenerateYoung4DTree();
				else
					break;
			}
			ExportIncidenceMatrix(generations);
			break;
		case 3: 
			if (generations.size() == 0) {
				cout << "Generations tree not built. Do it now? (y/n)" << endl;
				cin >> yn;
				if (yn == 'y' || yn == 'Y')
					generations = GenerateYoung4DTree();
				else
					break;
			}
			ExportDimensionSumSequences(generations);
			break;
		case 4:
			FreeYoung4DTree(fastGen);
			fastGen = GenerateYoung4DTreeBestOnly();
			ExportYoung4DTree(fastGen, "Young4D_BestInGenerations.txt", false, false, true);
			break;
		case 5: GenerateYoung4DTree(Young4D(1), true);
			break; 
		case 6: break;
		default: cout << "Operation not found, retry" << endl;
			break;
		}
	} while (dialogueResult!=6);
	FreeYoung4DTree(generations);
	FreeYoung4DTree(fastGen);
	system("pause");
	return 0;
}