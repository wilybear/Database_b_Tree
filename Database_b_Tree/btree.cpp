#include <iostream>
#include<fstream>
#include<string>
#include <vector>
#include<stack>
#include <algorithm>
#include <queue>
#include<sstream>
using namespace std;

const int ZERO = 0;

/*created by 12161553 김현식*/

vector<string> split(const char* str, char c = ' ')
{
	vector<string> result;
	do
	{
		const char* begin = str;
		while (*str != c && *str)
			str++;
		result.push_back(string(begin, str));
	} while (0 != *str++);

	return result;
}

bool compare(pair<int, int> a, pair<int, int> b) {
	if (a.first == 0 && b.first != 0) {
		return false;
	}
	else if (a.first != 0 && b.first == 0) {
		return true;
	}
	return a.first < b.first;
}

class Btree {
public:
	int rootId;
	int block_size;
	int level;
	int EntryCnt;	//한 노드 안에 몇개의 엔트리가 있는지
	const int HEADER = 12;
	const char* fileName;
	fstream fout;
	Btree(const char* fileName) {
		this->fileName = fileName;
		readHeader();
		EntryCnt = (block_size - 4) / 8;
	}
	Btree(const char* fileName, int blockSize) {
		EntryCnt = (blockSize - 4) / 8;
		this->fileName = fileName;
		level = 0;
		rootId = 0;
		block_size = blockSize;

		fout.open(fileName, ios::binary | ios::out);
		fout.close();
		//헤더 파일 생성
		fout.open(fileName, ios::in|ios::binary | ios::out);

		fout.write(reinterpret_cast<const char*>(&block_size), 4);
		fout.write(reinterpret_cast<const char*>(&rootId), 4);
		fout.write(reinterpret_cast<const char*>(&level), 4);
		fout.close();
	}
	/*
	Header 관련 함수들
	*/
	void readHeader() {
		fout.open(fileName, ios::in | ios::binary | ios::out);
		fout.read(reinterpret_cast<char*>(&block_size), 4);
		fout.seekg(4);
		fout.read(reinterpret_cast<char*>(&rootId), 4);
		fout.seekg(8);
		fout.read(reinterpret_cast<char*>(&level), 4);
		EntryCnt = (block_size - 4) / 8;
		fout.close();
	}
	void updateHeader(int _rootId, int _level) {
		fout.open(fileName, ios::in | ios::binary | ios::out);
		fout.seekp(4);
		fout.write(reinterpret_cast<const char*>(&_rootId), 4);
		fout.write(reinterpret_cast<const char*>(&_level), 4);
		fout.close();
		rootId = _rootId;
		level = _level;
	}

	/*
	insert함수
	*/
	void insert(int key, int value) {
		readHeader();
		stack<int> history;
		if (rootId == 0) {
			int newBID = createBlock();
			updateHeader(newBID, level);
			writeOnLeafNode(newBID, key, value,history);
		}
		else {
			history = find_node(key);
			int currentBID = history.top();
			history.pop();
			writeOnLeafNode(currentBID, key, value,history);
		}

	}

	void writeOnLeafNode(int bID, int key, int value,stack<int> history) {
		vector<int> leaf = readBlock(bID);
		vector<pair<int, int>> key_value;
		int nullCnt = 0;
		int nextLeaf = leaf[leaf.size() - 1];
		for (int i = 0; i < leaf.size()-1; i+=2) {
			key_value.push_back(make_pair(leaf[i], leaf[i + 1]));
			if (leaf[i] == 0) {
				nullCnt++;
			}
		}
		key_value.push_back(make_pair(key, value));
		clearBlock(bID);
		sort(key_value.begin(), key_value.end(),compare);
		//leaf node가 꽉차면
		if (nullCnt == 0) {
			//절반으로 나누어 반반씩 할당, 새로운 non leaf만들거나 기존 non leaf에 추가
			int mid = key_value.size() / 2;
			int secondID = createBlock();
			int first_key, second_key;
			first_key = key_value[0].first;
			second_key = key_value[mid + 1].first;
			fout.open(fileName, ios::in | ios::binary | ios::out);
			fout.seekp(getOffset(bID), ios::beg);
			for (int i = 0; i <mid; i++) {
				fout.write(reinterpret_cast<const char*>(&key_value[i].first), 4);
				fout.write(reinterpret_cast<const char*>(&key_value[i].second), 4);	
			}	
			fout.seekp((EntryCnt - mid)*8,ios::cur);
			fout.write(reinterpret_cast<const char*>(&secondID), 4);

			fout.seekp(getOffset(secondID), ios::beg);
			for (int i = mid; i <key_value.size(); i++) {
				fout.write(reinterpret_cast<const char*>(&key_value[i].first), 4);
				fout.write(reinterpret_cast<const char*>(&key_value[i].second), 4);
			}
			fout.seekp((mid-1)*8, ios::cur);

			fout.write(reinterpret_cast<const char*>(&nextLeaf), 4);
			fout.close();
			
			writeOnNonLeafNode(bID,secondID,key_value[mid].first,history);

		}
		else {
			//leaf node에 자리가 있다면
			fout.open(fileName, ios::in | ios::binary | ios::out);
			fout.seekp(getOffset(bID),ios::beg);
			for (int i = 0; i < key_value.size() - 1;i++) {
				fout.write(reinterpret_cast<const char*>(&key_value[i].first), 4);
				fout.write(reinterpret_cast<const char*>(&key_value[i].second), 4);
			}
			fout.write(reinterpret_cast<const char*>(&nextLeaf), 4);
			fout.close();
		}
	}

	void writeOnNonLeafNode(int firstBID, int secondBID,int secondKey, stack<int> history) {
		int curBID;
		if (history.empty()) {
			//root id
			curBID = createBlock();
			updateHeader(curBID, level+1);
			fout.open(fileName, ios::in | ios::binary | ios::out);
			fout.seekp(getOffset(curBID), ios::beg);
			fout.write(reinterpret_cast<const char*>(&firstBID), 4);
			fout.write(reinterpret_cast<const char*>(&secondKey), 4);
			fout.write(reinterpret_cast<const char*>(&secondBID), 4);
			fout.close();
		}
		else {
			//rootNode가 아닐때
			curBID = history.top();
			history.pop();
			vector<int> Nonleaf = readBlock(curBID);
			vector<pair<int, int>> key_pointer;
			int nullCnt = 0;
			for (int i = 1; i < Nonleaf.size() - 1; i += 2) {
				key_pointer.push_back(make_pair(Nonleaf[i], Nonleaf[i + 1]));
				if (Nonleaf[i] == 0) {
					nullCnt++;
				}
			}

			if (nullCnt == 0) {	//자리 없을때
				key_pointer.push_back(make_pair(secondKey, secondBID));
				clearBlock(curBID);
				sort(key_pointer.begin(), key_pointer.end(), compare);
			
				int mid = key_pointer.size() / 2;
				int newBID = createBlock();
				int newKey = key_pointer[mid].first;
				int midPointer = key_pointer[mid].second;

				fout.open(fileName, ios::in | ios::binary | ios::out);
				fout.seekp(getOffset(curBID), ios::beg);
				fout.write(reinterpret_cast<const char*>(&Nonleaf[0]), 4);
				for (int i = 0; i < mid; i++) {
					fout.write(reinterpret_cast<const char*>(&key_pointer[i].first), 4);
					fout.write(reinterpret_cast<const char*>(&key_pointer[i].second), 4);
				}

				fout.seekp(getOffset(newBID), ios::beg);
				fout.write(reinterpret_cast<const char*>(&midPointer), 4);
				for (int i = mid+1; i < key_pointer.size(); i++) {
					fout.write(reinterpret_cast<const char*>(&key_pointer[i].first), 4);
					fout.write(reinterpret_cast<const char*>(&key_pointer[i].second), 4);
				}
				fout.close();

				writeOnNonLeafNode(curBID,newBID,newKey,history);

			}
			else {	//non leaf안에 자리가 남을때

				fout.open(fileName, ios::in | ios::binary | ios::out);
				fout.seekp(getOffset(curBID)+4, ios::beg);
				if (Nonleaf[0] == firstBID) {
					fout.write(reinterpret_cast<const char*>(&secondKey), 4);
					fout.write(reinterpret_cast<const char*>(&secondBID), 4);
				}
				for (int i = 0; i < key_pointer.size()-1; i++) {
					fout.write(reinterpret_cast<const char*>(&key_pointer[i].first), 4);
					fout.write(reinterpret_cast<const char*>(&key_pointer[i].second), 4);
					if (key_pointer[i].second == firstBID) {
						fout.write(reinterpret_cast<const char*>(&secondKey), 4);
						fout.write(reinterpret_cast<const char*>(&secondBID), 4);
					}
				}
				fout.close();
			}

		}
	}

	stack<int> find_node(int key) {
		readHeader();
		vector<int> blockVector = readBlock(rootId);
		stack<int> history;
		int currentlevel = 0;
		int nextBID = rootId;
		history.push(rootId);
		while (currentlevel != level) {
			//non-leaf node 일때
			nextBID = 0;
			currentlevel++;
			for (int i = 1; i < blockVector.size(); i = i + 2) {
				if (key < blockVector[i]||blockVector[i]==0) {
					nextBID = blockVector[i - 1];
					break;
				}
			}
			if (nextBID == 0) {
				nextBID = blockVector[blockVector.size() - 1];
			}
			blockVector = readBlock(nextBID);
			history.push(nextBID);
		}
		//currentlevel == level => leaf node 일떄
		return history;
	}
	
	
	/*
	print 함수 (bfs 이용)
	*/
	void print(const char* resultFile) {
		readHeader();
		stringstream buffer;
		int curlevel = 0;
		buffer << "<" << curlevel << ">" << endl;
		pair<vector<int>,int> nextID = printNonLeafBlock(rootId,curlevel, buffer);
		queue<pair<vector<int>, int>> idQ;
		idQ.push(nextID);
		bool first = true;
		while (!idQ.empty())
		{
			vector<int> qID = idQ.front().first;
			int qlevel = idQ.front().second;
			idQ.pop();
			if (curlevel != qlevel) {
				if (qlevel == level) {
					//leafnode
					if (first) {
						buffer.seekp(-1, ios::cur);
						buffer << endl;
						buffer << "<" << qlevel << ">" << endl;
						first = false;
					}
					for (auto id : qID) {
						pintLeafBlock(id, buffer);
					}
					continue;
				}
				else {
					curlevel++;
					buffer.seekp(-1, ios::cur);
					buffer << endl;
					buffer << "<" << curlevel << ">" << endl;
				}
			}
			for (auto id : qID) {
				idQ.push(printNonLeafBlock(id, curlevel, buffer));
			}
		}
		buffer.seekp(-1, ios::cur);
		buffer << endl;
		fout.open(resultFile, fstream::out);
		fout << buffer.str();
		fout.close();
	}
	//Leaf Node의 키값 출력
	void pintLeafBlock(int BID,stringstream& buffer) {
		vector<int> block_v = readBlockWithoutZero(BID);
		for (int i = 0; i < block_v.size()-1; i+=2) {
			buffer << block_v[i] << ",";
		}
	}
	//해당 nonLeafNode의 키값 출력, level 과 pointer들 return
	pair<vector<int>,int> printNonLeafBlock(int BID,int curlevel, stringstream& buffer) {
		vector<int> block_v = readBlockWithoutZero(BID);
		vector<int> next_blockid;
		for (int i = 0; i < block_v .size(); i++) {
			if (i % 2 == 0) {
				next_blockid.push_back(block_v[i]);
			}
			else {
				buffer <<block_v[i] <<",";
			}
		}
		return make_pair(next_blockid,curlevel+1);
	}

	void pointSearch(vector<int> datas,const char* resultFile) {
		stringstream s;
		for (auto k : datas) {
			search(k,s);
		}
	
		fout.open(resultFile, fstream::out);
		fout << s.str();
		fout.close();
	}

	int search(int key,stringstream& ss) { // point search
		stack<int> history = find_node(key);
		int leafNode = history.top();
		vector<int> block_v= readBlock(leafNode);
		vector<pair<int, int>> key_value;
		for (int i = 0; i < block_v.size() - 1; i += 2) {
			if (block_v[i] == 0) {
				break;
			}
			key_value.push_back(make_pair(block_v[i], block_v[i + 1]));
		
		}
		for (auto p : key_value) {
			if (p.first == key) {
				ss << p.first << "," << p.second << endl;
			}
		}
		return 1;
	}

	//range search
	void rangeSearch(vector<pair<int,int>> datas, const char* resultFile) {
		stringstream s;
		for (auto k : datas) {
			stack<int> history = find_node(k.first);
			int curBID = history.top();
			while (curBID!=0) {
				vector<int> curBlock_v = readBlock(curBID);
				for (int i = 0; i < curBlock_v.size() - 1; i += 2) {
					if (curBlock_v[i] == 0) {
						break;
					}
					if (curBlock_v[i] >= k.first&& curBlock_v[i]<= k.second) {
						s << curBlock_v[i] << "," << curBlock_v[i + 1] << "\t";
					}
					if (curBlock_v[i] > k.second) {
						curBID = 0;
						break;
					}
				}
				if (curBID != 0) {
					curBID = curBlock_v[curBlock_v.size() - 1];
				}
			}
			s << endl;
		}

		fout.open(resultFile, fstream::out);
		fout << s.str();
		fout.close();

	}


	/*
	Block 관련 함수들
	*/

	//block을 4byte 단위로 읽어서 vector return
	vector<int> readBlock(int id) {
		fstream fin;
		fin.open(fileName, ios::binary | ios::in);
		fin.seekg(getOffset(id), ios::beg);
		vector<int> node(1 + EntryCnt * 2, 0);
		for (int i = 0; i < block_size / 4; i++) {
			fin.read(reinterpret_cast<char*>(&node[i]), 4);
		}
		return node;
	}

	vector<int> readBlockWithoutZero(int id) {
		fstream fin;
		fin.open(fileName, ios::binary | ios::in);
		fin.seekg(getOffset(id), ios::beg);

		vector<int> node;
		for (int i = 0; i < block_size / 4; i++) {
			int temp;
			fin.read(reinterpret_cast<char*>(&temp), 4);

			if (temp == 0) {
				break;
			}
			node.push_back(temp);
		}
		return node;
	}
	//새로운 블럭을 파일 끝에 생성하고 할당, blockID return
	int createBlock() {
		int length;
		int newBlockId;
		fout.open(fileName, ios::in | ios::binary | ios::out);
		fout.seekp(0,ios::end);
		length =fout.tellp();
		for (int i = block_size; i > 0; i = i-4) {
			fout.write(reinterpret_cast<const char*>(&ZERO), 4);
		}
		fout.close();
		newBlockId = (length - 12) / block_size+1;
		return newBlockId;
	}

	//블럭의 내용을 0으로 초기화
	void clearBlock(int id) {
		fout.open(fileName, ios::in | ios::binary | ios::out);
		fout.seekp(getOffset(id), ios::beg);
		for (int i = block_size; i > 0; i = i - 4) {
			fout.write(reinterpret_cast<const char*>(&ZERO), 4);
		}
		fout.close();
	}
	
	int getOffset(int id) {
		return HEADER + (id - 1) * block_size;
	}

};



vector<pair<int,int>> read_initial_data(const char* fileName) {
	ifstream readFile;
	readFile.open(fileName);
	vector<pair<int, int>> datas;
	if (readFile.is_open()) {
		while (readFile.peek() != EOF) {
			string temp;
			getline(readFile, temp);
			vector<string> tokens = split(temp.c_str(), ',');
			int key, value;
			for (int i = 0; i < tokens.size(); i = i + 2) {
				if (tokens[i] != ""&& tokens[i+1] != "") {
					datas.push_back(make_pair(stoi(tokens[i]), stoi(tokens[i + 1])));
				}
			}
		}
	}
	return datas;
}

vector<int> read_search_data(const char* fileName) {
	ifstream readFile;
	readFile.open(fileName);
	vector< int> datas;
	if (readFile.is_open()) {
		while (readFile.peek() != EOF) {
			string temp;
			getline(readFile, temp);
			if (temp != "") {
				datas.push_back(stoi(temp));
			}
		}
	}
	return datas;
}


int main(int argc, char* argv[])
{
	char command = argv[1][0];

	Btree* btree;
	vector<int> searchData;
	vector<pair<int, int>> datas;

	switch (command)
	{
	case 'c':
		btree = new Btree(argv[2],atoi(argv[3]));
		// create index file
		//bptree.exe c [bptree binary file] [block_size]
		break;
	case 'i':
		btree = new Btree(argv[2]);
		datas = read_initial_data(argv[3]);
		for (auto p : datas) {
			cout << p.first << p.second << endl;
			btree->insert(p.first, p.second);
		}
		// insert records from [records data file], ex) records.txt
		//bptree.exe i [bptree binary file] [records data text file]
		break;
	case 's':

		btree = new Btree(argv[2]);
		searchData = read_search_data(argv[3]);
		btree->pointSearch(searchData, argv[4]);
		//bptree.exe s[bptree binary file][input text file][output text file]
		// search keys in [input file] and print results to [output file]
		break;
	case 'r':
		btree = new Btree(argv[2]);
		datas = read_initial_data(argv[3]);
		btree->rangeSearch(datas, argv[4]);
		//bptree.exe r [bptree binary file] [input text file] [output text file]
		// search keys in [input file] and print results to [output file]
		break;
	case 'p':
		btree = new Btree(argv[2]);
		btree->print(argv[3]);
		//bptree.exe p [bptree binary file] [output text file]
		// print B+-Tree structure to [output file]
		break;
	}

}