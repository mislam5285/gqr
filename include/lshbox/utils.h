#include <iostream>
#include <string>
#include <fstream>
#include <utility>
#include <lshbox.h>
#include <random>
#include <thread>
#include <functional>
#include <lshbox/matrix.h>
#pragma once
using std::vector;
using std::string;

/* select k elements of n elements
 * @n: total number of elements 
 * @k: number of selected elements
 * @return vector of bool, consisting of n elements and k of them are true. 
 * */

vector<bool> selection(unsigned n, unsigned k) {
    std::hash<std::thread::id> hasher;
    std::thread::id this_id = std::this_thread::get_id();
    std::mt19937 rng((unsigned) hasher(this_id) + std::time(0));
    std::uniform_int_distribution<unsigned> usBits(0, n - 1);
    usBits(rng);
    vector<bool> selected(n, 0);
    unsigned numSelected = 0;

    for (unsigned idxToSelected = 0; idxToSelected < selected.size(); ++idxToSelected) {
        if (usBits(rng) < k) {
            selected[idxToSelected] = true;
            numSelected++;
        }

        if (numSelected == k) {
            break;
        }
    }

    // if have not selected enough items
    while (numSelected < k) {
        unsigned target = usBits(rng);
        while (selected[target] == true) {
            target = usBits(rng);
        }
        selected[target] = true;
        numSelected++;
    }

    return selected;
}

template<typename ScannerT, typename AnswerT>
bool setStat(
        // lshbox::Scanner<lshbox::Matrix<DATATYPE>::Accessor> scanner, 
        ScannerT scanner, 
        const AnswerT& ans, 
        lshbox::Stat& recall,
        lshbox::Stat& precision) {

    scanner.topk().genTopk(); // must getTopk for scanner, other wise will wrong
    float thisRecall = scanner.topk().recall(ans);

    float matched = thisRecall * (scanner.getK() - 1); 
    float thisPrecision;
    if(scanner.cnt() == 0)
        thisPrecision = 0;
    else
        thisPrecision = matched / scanner.cnt();

    recall << thisRecall;
    precision << thisPrecision;

    if (thisRecall > 0.99) return true;
    else return false;
}

namespace lshbox {

vector<bool> to_bits (unsigned long long num)
{
    vector<bool> bits;
    while(num > 0 ){
        bits.push_back(num % 2);
        num /= 2;
    }
    return bits;
}

template<typename DATATYPE>
void loadFvecs(Matrix<DATATYPE>& data, const char* dataFile, int dimension, int cardinality) {
    data.reset(dimension, cardinality);
    std::ifstream fin(dataFile, std::ios::binary);
    int dim;
    for (int i = 0; i < cardinality; ++i) {
        fin.read((char*)&dim, sizeof(int));
        assert(dim == dimension);
        fin.read((char *)(data.getData() + i * dimension), sizeof(float) * dimension);
    }
    fin.close();
}

/*
 * padding meaningless euclidean distance
 * */
string genBenchFromIvecs(const char* ivecBenchFile, int numQueries, int topK) {
    std::ifstream fin(ivecBenchFile, std::ios::binary);
    if (!fin) {
        std::cout << "cannot open file " << ivecBenchFile << std::endl;
        assert(false);
    }
    std::vector<vector<int>> bench;
    bench.resize(numQueries);

    for (int i = 0; i < numQueries; ++i) {
        int length;
        fin.read((char*)&length, sizeof(int));
        assert(length >= topK);
        int nnIdx;
        for (int j = 0; j < length; ++j ) {
            fin.read((char*)&nnIdx, sizeof(int));
            if (j < topK) 
                bench[i].push_back(nnIdx);
        }
    }
    fin.close();

    string lshBenchFile = string(ivecBenchFile) + ".lshbox";
    std::ofstream fout(lshBenchFile.c_str());
    if (!fout) {
        std::cout << "cannot create file " << ivecBenchFile << std::endl;
        assert(false);
    }
    fout << bench.size() << " " << bench[0].size() << std::endl;
    for (int i = 0; i < bench.size(); ++i) {
        fout << i ;
        for (int j = 0; j < bench[i].size(); ++j) {
            fout << "\t" << bench[i][j] << " " << j;
        }
        fout << std::endl;
    }
    fout.close();
    return lshBenchFile;
}
};

namespace std {
    template<typename FIRST, typename SECOND>
        std::string to_string(const vector<std::pair<FIRST, SECOND>>& vec){
            std::string str = "";
            for(int i = 0; i < vec.size(); ++i){
                str += "<" + std::to_string(vec[i].first)
                    + "\t" + std::to_string(vec[i].second) + ">, ";
            }
            return str;
        }
    template<typename T>
        std::string to_string(const vector<T>& vec){
            std::string str = "";
            for(int i = 0; i < vec.size(); ++i){
                str += std::to_string(vec[i]) + ", ";
            }
            return str;
        }

};
