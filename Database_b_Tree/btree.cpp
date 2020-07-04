#include <iostream>
#include<fstream>
#include<string>
#include <vector>
#include<stack>
#include <algorithm>
using namespace std;

const int ZERO = 0;

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
	Btree(const char* fileName, int blockSize) {
		EntryCnt = (blockSize - 4) / 8;
		cout << "EntryCnt = " << EntryCnt << endl;
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
	void print();
	int search(int key) { // point search
		readBlock(rootId);

		return 1;
	}

	int* search(int startRange, int endRange); // range search

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



vector<pair<int,int>> read_initial_data() {
	ifstream readFile;
	readFile.open("sample_insertion_input.txt");
	vector<pair<int, int>> datas;
	if (readFile.is_open()) {
		while (readFile.peek() != EOF) {
			string temp;
			getline(readFile, temp);
			vector<string> tokens = split(temp.c_str(), ',');
			int key, value;
			for (int i = 0; i < tokens.size(); i = i + 2) {
				datas.push_back(make_pair(stoi(tokens[i]), stoi(tokens[i + 1])));
			}
			cout << endl;
		}
	}
	return datas;
}




//int argc, char* argv[]
int main()
{
	//char command = argv[1][0];
	
	Btree myBtree = Btree("btree.bin", 36);
	vector<pair<int,int>> datas = read_initial_data();
	for (auto p: datas) {
		myBtree.insert(p.first, p.second);
	}


	/*
	switch (command)
	{
	case 'c':
		Btree myBtree = Btree("btree.bin", 4);
		// create index file
		break;
	case 'i':
		// insert records from [records data file], ex) records.txt
		break;
	case 's':
		// search keys in [input file] and print results to [output file]
		break;
	case 'r':
		// search keys in [input file] and print results to [output file]
		break;
	case 'p':
		// print B+-Tree structure to [output file]
		break;
	}
	*/
}