#pragma once
#include "pch.h"
#include<map>
#include<set>
#include<string>
#include<vector>
#include<regex>
#include<utility> 
#include<algorithm>
#include "GraphManaged.h"
using namespace Automata;
using namespace Platform;
using namespace std;
class Methods {
public:
	static string ToCppString(String^ s) {
		string result;
		for (auto i : s) {
			result += i == L'\r' ? '\n' : i;
		}
		return result;
	}
	static String^ FromCppString(std::string s) {
		;
		std::wstring ws;
		ws.assign(s.begin(), s.end());
		String^ result = ref new String(ws.c_str());
		return result;
	}
	static int StringToInt(String^ s) {
		string new_s = ToCppString(s).c_str();
		int n = 0;
		for (int i = 0; i < new_s.size(); i++) {
			n = n * 10 + new_s[i] - '0';
		}
		return n;
	}
};
class graph {
private:
	set<int> extract_closure(int i, map<int, set<int>>& closures) {
		if (closures.find(i) == closures.end() && this->node.find(i) != this->node.end()) {
			closures[i].insert(i);
			for (auto j : this->node[i]["0"]) {
				closures[i].insert(j);
				for (auto k : extract_closure(j, closures)) {
					if (this->end[k]) {
						this->end[i] = true;
					}
					closures[i].insert(k);
				}
			}
		}
		return closures[i];
	}
	void phase_one() {
		map<int, set<int>> closures;
		for (map<int, map<string, set<int>>>::iterator i = this->node.begin(); i != this->node.end(); i++) {
			extract_closure(i->first, closures);
		}
		for (map<int, set<int>>::iterator i = closures.begin(); i != closures.end(); i++) {
			for (auto j : i->second) {
				for (map<string, set<int>>::iterator k = this->node[j].begin(); k != this->node[j].end(); k++) {
					for (auto m : k->second) {
						this->node[i->first][k->first].insert(m);
					}
				}
			}
		}
	}
	void determine(vector<vector<set<int>>>& arr, vector<set<int>>& memo, set<int> s) {
		if (find(memo.begin(), memo.end(), s) == memo.end()) {
			vector<set<int>> v_temp;
			memo.push_back(s);
			for (int l = 0; l < this->language.size(); l++) {
				set<int> s_temp;
				for (auto i : s) {
					for (auto j : this->node[i][this->language[l]]) {
						s_temp.insert(j);
					}
				}
				v_temp.push_back(s_temp);
			}
			arr.push_back(v_temp);
		}
	}
	void phase_two() {
		vector<vector<set<int>>> arr;
		vector<set<int>> memo;
		determine(arr, memo, this->start);
		int j = 0;
		for (int i = 0; i < this->language.size(); i++) {
			if (!arr[j][i].empty())
				determine(arr, memo, arr[j][i]);
			if (i == this->language.size() - 1 && j < arr.size() - 1) {
				i = -1; j++;
			}
		}
		makeCopy(graph(arr, this->language, memo, this->end));
	}
	void phase_four() {
		map<int, map<int, string>> result;
		for (map<int, map<string, set<int>>>::iterator i = this->node.begin(); i != this->node.end(); i++) {
			for (map<string, set<int>>::iterator j = i->second.begin(); j != i->second.end(); j++) {
				for (auto k : j->second) {
					result[i->first][k] += result[i->first][k] == "" ? j->first : "," + j->first;
				}
			}
		}
		makeCopy(graph(result,this->start,this->end));
	}
	vector<set<int>> minimize(vector<set<int>> classes, vector<vector<int>>& r_signature) {
		vector<set<int>> new_classes;
		vector<vector<int>> signatures;
		for (int i = 0; i < classes.size(); i++) {
			map<vector<int>, set<int>> class_signature;
			for (auto j : classes[i]) {
				vector<int> signature(this->language.size(), -1);
				for (int k = 0; k < this->language.size(); k++) {
					for (int s = 0; s < classes.size(); s++) {
						for (auto node : classes[s]) {
							for (auto nodes : this->node[j][this->language[k]]) {
								if (node == nodes) signature[k] = s;
							}
						}
					}
				}
				class_signature[signature].insert(j);
			}
			for (map<vector<int>, set<int>>::iterator j = class_signature.begin(); j != class_signature.end(); j++) {
				new_classes.push_back(j->second);
				signatures.push_back(j->first);
			}
		}
		r_signature = signatures;
		return new_classes;
	}
	void phase_three() {
		vector<set<int>> classes(2);
		vector<vector<int>> signatures;
		for (map<int, map<string, set<int>>>::iterator i = this->node.begin(); i != this->node.end(); i++) {
			if (!this->end[i->first]) classes[0].insert(i->first);
		}
		for (map<int, bool>::iterator i = this->end.begin(); i != this->end.end(); i++) {
			if (i->second) classes[1].insert(i->first);
		}
		while (classes != minimize(classes, signatures))classes = minimize(classes, signatures);
		makeCopy(graph(classes, signatures, this->language, this->start));
	}
	vector<string> split(const string& str, string pattern){
		regex r{ pattern };
		sregex_token_iterator start{str.begin(), str.end(), r, -1 },end;
		return vector<string>(start, end);
	}
	bool getTrapStates(int n,set<int>& isTrap, set<int>& p) {
		if (p.find(n) == p.end()) {
			p.insert(n);
			set<int> dest;
			for (map<string, set<int>>::iterator it = this->node[n].begin(); it != this->node[n].end(); it++) {
				for (auto d : it->second) {
					dest.insert(d);
				}
			}
			bool isTrapNode = true;
			if (dest.size() == 0 && this->end[n]) return !isTrapNode;
			if (dest.size() == 0 && !this->end[n]) return isTrapNode;
			if(dest.size()>0) {
				for (auto d : dest) {
					if (d != n && isTrap.find(d)==isTrap.end()) {
						auto isDestTrap = this->getTrapStates(d, isTrap, p);
						isTrapNode &= isDestTrap;
					}
				}
			}
			if (isTrapNode) isTrap.insert(n);
			return isTrapNode;
		}
	}
	void write(int i, String^& accumulated) {
		if (!this->Processed[i]) {
			this->Processed[i] = true;
			for (map<string, set<int>>::iterator j = this->node[i].begin(); j != this->node[i].end(); j++) {
				for (auto d : j->second) {
					if (this->nodes.find(d) != this->nodes.end()) {
						accumulated += i.ToString() + "->" + d.ToString() + " [label=\"" + Methods::FromCppString(j->first) + "\"];\n";
						if (this->end[i]) {
							accumulated += i.ToString() + "->" + "e;\n";
							this->end[i] = false;
						}
						write(d, accumulated);
					}
				}
			}
		}
	}
	void makeCopy(graph& r) {
		this->end = r.end;
		this->start = r.start;
		this->start_in_file = r.start_in_file;
		this->node = r.node;
		this->nodes = r.nodes;
		this->language = r.language;
		this->name = r.name + "_copy";
		this->Processed = r.Processed;
		this->end_in_file = r.end_in_file;
	}
	graph(map<int, map<int, string>> map_in, set<int> start,map<int, bool> end) {
		for (map<int, map<int, string>>::iterator i = map_in.begin(); i != map_in.end(); i++) {
			for (map<int, string>::iterator j = i->second.begin(); j != i->second.end(); j++) {
				this->insert(i->first, j->first, j->second);
			}
		}
		this->start = start; this->end = end;
	}
	graph(vector<vector<set<int>>> arr, vector<string> alph, vector<set<int>> s, map<int, bool> end) {
		for (int i = 0; i < arr.size(); i++) {
			for (int j = 0; j < arr[0].size(); j++) {
				for (int f = 0; f < s.size(); f++) {
					if (s[f] == arr[i][j]) {
						this->insert(i, f, alph[j]);
						for (auto k : s[f]) {
							if (end[k]) this->end[f] = true;
						}
						for (auto k : s[i]) {
							if (end[k]) this->end[i] = true;
						}
						break;
					}
				}
			}
		}
		this->start.insert(0);
	}
	graph(vector<set<int>> classes, vector<vector<int>> signatures, vector<string> lang, set<int> old_s) {
		for (int i = 0; i < classes.size(); i++) {
			for (int j = 0; j < lang.size(); j++) {
				if (signatures[i][j] != -1) {
					for (auto k : classes[i]) {
						if (old_s.find(k) != old_s.end()) this->start.insert(i);
					}
					this->insert(i, signatures[i][j], lang[j]);
				}

			}
		}
	}
public:
	graph(){}
	graph(graph& r) {
		this->end = r.end;
		this->start = r.start;
		this->start_in_file = r.start_in_file;
		this->node = r.node;
		this->nodes = r.nodes;
		this->language = r.language;
		this->name = r.name + "_copy";
		this->Processed = r.Processed;
		this->end_in_file = r.end_in_file;
	}
	graph(string bufferIn) {
		bool firstLine = 1;
		this->start_in_file=false;
		this->end_in_file=false;
		vector<string> lines=split(bufferIn, "\n");
		smatch list;
		for (auto line : lines) {
			if (!firstLine)
			if (regex_search(line, list, regex("([0-9]+) *-> *([0-9]+).*\"(\\w+)\""))) {
					int n[] = { 0, 0 };
					for (int k = 0; k < 2; k++) {
						for (int i = 0; i < list[k + 1].str().length(); i++) {
							n[k] = n[k] * 10 + list[k + 1].str()[i] - '0';
						}
					}
					this->insert(n[0], n[1], list[3].str());
				}
			else if (regex_search(line, list, regex("s|S *-> *([0-9]+)"))) {
				this->start_in_file = true;
						int n = 0;
						for (int i = 0; i < list[1].str().length(); i++) {
							n = n * 10 + list[1].str()[i] - '0';
						}
						this->start.insert(n);
				}
			else if (regex_search(line, list, regex("([0-9]+) *-> *f|F"))) {
				this->end_in_file = true;
						int n = 0;
						for (int i = 0; i < list[1].str().length(); i++) {
							n = n * 10 + list[1].str()[i] - '0';
						}
						this->end[n] = true;
				}
			firstLine = 0;
		}
	}
	void insert(int s, int f, string a) {
		this->node[s][a].insert(f);
		this->Processed[s] = false;
		this->nodes.insert(s); this->nodes.insert(f);
		if (a != "0" && find(this->language.begin(), this->language.end(), a) == this->language.end()) this->language.push_back(a);
	}
	void optimize(int p1,int p2,int p3) {
		if(p1) this->phase_one();
		if(p1 && p2)this->phase_two();
		if (p1 && p2 && p3)this->phase_three();
		//this->phase_four();
	}
	void Clean() {
		set<int> processing;
		set<int> traps;
		for (auto i : this->start) {
			this->getTrapStates(i, traps, processing);
		}
		for (auto trap : traps) {
			this->nodes.erase(trap);
		}
	}
	String^ ToString() {
		String^ accumulated = "";
		this->Clean();
		accumulated += "digraph result{\n";
		for (auto i : this->start) {
			accumulated += "s" + "->" + i.ToString() + ";\n";
			this->write(i, accumulated);
		}
		accumulated += "}";
		return accumulated;
	}
	GraphManaged^ ConvertFromNative() {
		this->phase_four();
		Map<int, IMap<String^, IVector<int>^>^>^ result = ref new Map<int, IMap<String^, IVector<int>^>^>();
		for (auto Node : this->node) {
			if (!result->HasKey(Node.first)) {
				result->Insert(Node.first, ref new Map<String^, IVector<int>^>());
			}
			for (auto weight : Node.second) {
				auto Weight = Methods::FromCppString(weight.first);
				if (!result->Lookup(Node.first)->HasKey(Weight)) {
					result->Lookup(Node.first)->Insert(Weight,ref new Vector<int>());
				}
				for (auto dest : weight.second) {
					uint32 index;
					if (!result->Lookup(Node.first)->Lookup(Weight)->IndexOf(dest, &index)) {
						result->Lookup(Node.first)->Lookup(Weight)->Append(dest);
					}
				}
			}
		}
		Vector<int>^ nodes = ref new Vector<int>();
		for (auto Node : this->nodes) {
			nodes->Append(Node);
		}
		Map<int, int>^ boundaries = ref new Map<int, int>();
		for (auto Node : this->nodes) {
			boundaries->Insert(Node, (this->start.find(Node) != this->start.end()) + (this->end[Node] << 1));
		}
		return ref new GraphManaged(result,boundaries,nodes);
	}
	bool start_in_file;
	bool end_in_file;
	map<int, map<string, set<int>>> node;
	map<int, bool> Processed;
	vector<string> language;
	set<int> nodes;
	set<int> start;
	map<int, bool> end;
	string name;
};