//////////////////////////////////////////////////////////////////////////////
/// Copyright (C) 2014 Gefu Tang <tanggefu@gmail.com>. All Rights Reserved.
///
/// This file is part of LSHBOX.
///
/// LSHBOX is free software: you can redistribute it and/or modify it under
/// the terms of the GNU General Public License as published by the Free
/// Software Foundation, either version 3 of the License, or(at your option)
/// any later version.
///
/// LSHBOX is distributed in the hope that it will be useful, but WITHOUT
/// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
/// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
/// more details.
///
/// You should have received a copy of the GNU General Public License along
/// with LSHBOX. If not, see <http://www.gnu.org/licenses/>.
///
/// @version 0.1
/// @author Gefu Tang & Zhifeng Xiao
/// @date 2014.6.30
//////////////////////////////////////////////////////////////////////////////

/**
 * @file newitqlsh.h
 *
 * @brief Locality-Sensitive Hashing Scheme Based on Iterative Quantization.
 */
#pragma once
#include <map>
#include <vector>
#include <random>
#include <iostream>
#include <functional>
#include <unordered_map>
#include <eigen/Eigen/Dense>
#include <cmath>
#include "probing.h"
#include <lshbox/utils.h>

namespace lshbox
{
/**
 * Locality-Sensitive Hashing Scheme Based on Iterative Quantization.
 *
 *
 * For more information on iterative quantization based LSH, see the following reference.
 *
 *     Gong Y, Lazebnik S, Gordo A, et al. Iterative quantization: A procrustean
 *     approach to learning binary codes for large-scale image retrieval[J].
 *     Pattern Analysis and Machine Intelligence, IEEE Transactions on, 2013,
 *     35(12): 2916-2929.
 */
template<typename DATATYPE = float>
class laPcaLsh 
{
public:
    typedef unsigned long long BIDTYPE;
    struct Parameter
    {
        /// Hash table size
        unsigned M = 0;
        /// Number of hash tables
        unsigned L = 0;
        /// Dimension of the vector, it can be obtained from the instance of Matrix
        unsigned D = 0;
        /// Binary code bytes
        unsigned N = 0;
        /// Size of vectors in train
        unsigned S = 0;
        /// Training iterations
        unsigned I = 0;
    };
    laPcaLsh() {}
    laPcaLsh(const Parameter &param_)
    {
        reset(param_);
    }
    ~laPcaLsh() {}
    /**
     * Reset the parameter setting
     *
     * @param param_ A instance of laPcaLsh<DATATYPE>::Parametor, which contains
     * the necessary parameters
     */
    void reset(const Parameter &param_);

    static bool trainSingleTable( 
        const Matrix<DATATYPE> &data,
        std::vector<std::vector<float> >* pcsPointer,
        Parameter param);
    void trainAll(const Matrix<DATATYPE> &data, unsigned batchSize);
    /**
     * Hash the dataset.
     *
     * @param data A instance of Matrix<DATATYPE>, it is the search dataset.
     */
    void hash(Matrix<DATATYPE> &data);
    /**
     * calculate mean of each dimension.
     *
     * @param data A instance of Matrix<DATATYPE>, it is the search dataset.
     */
    std::vector<std::vector<float>> getMeanAndSTD(Matrix<DATATYPE> &data);
    void setMeanAndSTD(Matrix<DATATYPE> &data);
    /**
     * Insert a vector to the index.
     *
     * @param key   The sequence number of vector
     * @param domin The pointer to the vector
     */
    void insert(unsigned key, const DATATYPE *domin);
    /**
     * probe bucket
     * @param t table to probe
     * @param bucketId bucket to probe
     * @param scanner Top-K scanner, use for scan the approximate nearest neighborholds
     * @return number of item probed
     */
    template<typename PROBER>
    int probe(unsigned t, BIDTYPE bucketId, PROBER &prober);
    /**
     * get the hash value of a vector.
     *
     * @param k     The idx of the table
     * @param domin The pointer to the vector
     * @return      The hash value
     */
    BIDTYPE getHashVal(unsigned k, const DATATYPE *domin);
    /**
     * Load the index from binary file.
     *
     * @param file The path of binary file.
     */
    void load(const std::string &file);
    /**
     * Save the index as binary file.
     *
     * @param file The path of binary file.
     */
    void save(const std::string &file);
    /**
     * Added by Jinfeng
     * getHashFloats for of a vector, i.e. projection
     *
     * @param k     The idx of the table
     * @param domin The pointer to the vector
     * @return      The hash value
     */
    std::vector<float> getHashFloats(unsigned k, const DATATYPE *domin);
    /**
     * Added by Jinfeng
     * get quantization of a vector, i.e. quantization
     *
     * @param k     The idx of the table
     * @param domin The pointer to the vector
     * @return      The hash value
     */
    std::vector<bool> quantization(const std::vector<float>& hashFloats);
    /**
     * Added by Jinfeng
     * getHashBits for of a vector, i.e. projection + quantization
     *
     * @param k     The idx of the table
     * @param domin The pointer to the vector
     * @return      The hash value
     */
    std::vector<bool> getHashBits(unsigned k, const DATATYPE *domin);
    /**
     * get all the buckets.
     */
    int getTableSize();
    int getMaxBucketSize();

    /**
     * ranking hash code to query the approximate nearest neighborholds.
     *
     * @param domin   The pointer to the vector
     * @param scanner Top-K scanner, use for scan the approximate nearest neighborholds
     * @param numItem number of buckets to return
     * */
    template<typename PROBER>
    void KItemByProber(const DATATYPE *domin, PROBER &prober, int numItems);

    Parameter param;
    std::vector<std::unordered_map<BIDTYPE, std::vector<unsigned> > > tables;

private:
    std::vector<std::vector<std::vector<float> > > pcsAll;
    std::vector<std::vector<unsigned> > rndArray;
    std::vector<std::vector<float>> meanAndSTD;
};
}

// ------------------------- implementation -------------------------
template<typename DATATYPE>
void lshbox::laPcaLsh<DATATYPE>::reset(const Parameter &param_)
{
    param = param_;
    tables.resize(param.L);
    rndArray.resize(param.L);
    pcsAll.resize(param.L);
    std::mt19937 rng(unsigned(std::time(0)));
    std::uniform_int_distribution<unsigned> usArray(0, param.M - 1);
    for (std::vector<std::vector<unsigned> >::iterator iter = rndArray.begin(); iter != rndArray.end(); ++iter)
    {
        for (unsigned i = 0; i != param.N; ++i)
        {
            iter->push_back(usArray(rng));
        }
    }
}

// trainSingleTable, data, pcsAll[k]
template<typename DATATYPE>
bool lshbox::laPcaLsh<DATATYPE>::trainSingleTable(
    const Matrix<DATATYPE> &data,
    std::vector<std::vector<float> >* pcsPointer,
    Parameter param)
{
        // std::cout << "table " << k << " starts PCA " << std::endl;
        std::vector<bool> selected = selection(data.getSize(), param.S);

        std::vector<unsigned> seqs;
        seqs.reserve(param.S);
        for (unsigned idxToSelected = 0; idxToSelected < selected.size(); ++idxToSelected) {
            if (selected[idxToSelected]) {
                seqs.push_back(idxToSelected);
            }
        }
        assert(seqs.size() == param.S);


        // pca
        // Eigen::MatrixXf tmp(param.S, data.getDim());
        Eigen::MatrixXf centered(param.S, data.getDim());
        std::vector<float> vals(0);
        vals.resize(data.getDim());
        for (unsigned i = 0; i != centered.rows(); ++i)
        {
            for (int j = 0; j != data.getDim(); ++j)
            {
                vals[j] = data[seqs[i]][j];
            }
            centered.row(i) = Eigen::Map<Eigen::VectorXf>(&vals[0], data.getDim());
        }

        // delete seqs to save memory
        seqs.clear();
        seqs.shrink_to_fit();

        std::cout << "start pca " << std::endl;
        centered = centered.rowwise() - centered.colwise().mean();

        // implement efficient cov computation, using eigen lead to multiple copies of the data
        Eigen::MatrixXf cov = (centered.transpose() * centered) / float(centered.rows() - 1);

        Eigen::MatrixXf mat_pca =
            Eigen::SelfAdjointEigenSolver<Eigen::MatrixXf>(cov).eigenvectors().rightCols(param.N);

        (*pcsPointer).resize(param.N);
        for (unsigned i = 0; i != (*pcsPointer).size(); ++i)
        {
            (*pcsPointer)[i].resize(param.D);
            for (unsigned j = 0; j != (*pcsPointer)[i].size(); ++j)
            {
                (*pcsPointer)[i][j] = mat_pca(j, i);
            }
        }

        // itq rotation

        // Eigen::MatrixXf mat_c = tmp * mat_pca; original implementation, same result as using centered
        Eigen::MatrixXf mat_c = centered * mat_pca;

        // delete centered
        centered = Eigen::MatrixXf();

        std::hash<std::thread::id> hasher;
        std::thread::id this_id = std::this_thread::get_id();
        std::mt19937 rng((unsigned) hasher(this_id) + std::time(0));
        std::normal_distribution<float> nd;
        Eigen::MatrixXf R(param.N, param.N);
        for (unsigned i = 0; i != R.rows(); ++i)
        {
            for (unsigned j = 0; j != R.cols(); ++j)
            {
                R(i, j) = nd(rng);
            }
        }
        Eigen::JacobiSVD<Eigen::MatrixXf> svd(R, Eigen::ComputeThinU | Eigen::ComputeThinV);
        R = svd.matrixU();
        std::cout << "finish PCA" << std::endl;
        return true;
}

template<typename DATATYPE>
void lshbox::laPcaLsh<DATATYPE>::trainAll(const Matrix<DATATYPE> &data, unsigned batchSize){

    // use loop
    unsigned numBatches = param.L / batchSize;
    for (unsigned batch = 0; batch < numBatches; ++batch) {
        std::vector<std::thread> threads;
        threads.resize(batchSize);

        unsigned startk = batch * batchSize;
        for (unsigned i = 0; i < threads.size(); ++i) {
            unsigned tableId = startk + i;
            threads[i] = std::thread(
                    trainSingleTable, 
                    data, &pcsAll[tableId], param);
            std::cout << "table " + std::to_string(tableId) + " added!\n" << std::endl;
        }

        for (unsigned i = 0; i < threads.size(); ++i) {
            threads[i].join();
        }
        std::cout << "batch " + std::to_string(batch) + " finished!\n" << std::endl;
    }

    unsigned remaining = param.L % batchSize;
    std::vector<std::thread> threads;
    threads.resize(remaining);

    unsigned startk = numBatches * batchSize;
    for (unsigned i = 0; i < threads.size(); ++i) {
        unsigned tableId = startk + i;
        threads[i] = std::thread(
                trainSingleTable, 
                data, &pcsAll[tableId], param);
        std::cout << "table " + std::to_string(tableId) + " added!\n" << std::endl;
    }

    for (unsigned i = 0; i < threads.size(); ++i) {
        threads[i].join();
    }
    std::cout << "last batch finished\n " << std::endl;

}

template<typename DATATYPE>
std::vector<std::vector<float>> lshbox::laPcaLsh<DATATYPE>::getMeanAndSTD(Matrix<DATATYPE> &data)
{
    // calculate mean
    std::vector<float> sumPositive;
    std::vector<float> sumNegative;
    sumPositive.resize(param.N);
    sumNegative.resize(param.N);

    std::vector<int> countPositive;
    std::vector<int> countNegative;
    countPositive.resize(param.N);
    countNegative.resize(param.N);
    for (int i = 0; i < param.N; ++i) {
        sumPositive[i] = 0;
        sumNegative[i] = 0;
        countPositive[i] = 0;
        countNegative[i] = 0;
    }

    for (unsigned dIdx = 0; dIdx != data.getSize(); ++dIdx)
    {
        std::vector<float> hashFloats = getHashFloats(0, data[dIdx]);
        for (int hfIdx = 0; hfIdx < hashFloats.size(); ++hfIdx) {
            if (hashFloats[hfIdx] >= 0) {
                sumPositive[hfIdx] += hashFloats[hfIdx];
                countPositive[hfIdx]++;
            } else {
                sumNegative[hfIdx] += hashFloats[hfIdx];
                countNegative[hfIdx]++;
            }
        }
    }
    for (int i = 0; i < param.N; ++i) {
        if(countPositive[i] != 0) sumPositive[i] /= countPositive[i];
        if(countNegative[i] != 0) sumNegative[i] /= countNegative[i];
    }

    // calculate STD
    std::vector<float> stdPositive;
    std::vector<float> stdNegative;
    stdPositive.resize(param.N);
    stdNegative.resize(param.N);
    for (int i = 0; i < param.N; ++i) {
        stdPositive[i] = 0;
        stdNegative[i] = 0;
    }
    for (unsigned dIdx = 0; dIdx < data.getSize(); ++dIdx) {
        std::vector<float> hashFloats = getHashFloats(0, data[dIdx]);
        for (int hfIdx = 0; hfIdx < hashFloats.size(); ++hfIdx) {
            if ( hashFloats[hfIdx] >= 0 ) {
                stdPositive[hfIdx] += (hashFloats[hfIdx] - sumPositive[hfIdx]) * (hashFloats[hfIdx] - sumPositive[hfIdx]);
            } else {
                stdNegative[hfIdx] += (hashFloats[hfIdx] - sumNegative[hfIdx]) * (hashFloats[hfIdx] - sumNegative[hfIdx]);
            }
        }
    } 
    for (int i = 0; i < param.N; ++i) {
        if(countPositive[i] != 0) stdPositive[i] /= countPositive[i];
        stdPositive[i] = sqrt(stdPositive[i]);
        assert(stdPositive[i] > 0);

        if(countNegative[i] != 0) stdNegative[i] /= countNegative[i];
        stdNegative[i] = sqrt(stdNegative[i]);
        assert(stdNegative[i] > 0);
    }

    std::vector<std::vector<float>> result;
    result.push_back(sumPositive);
    result.push_back(sumNegative);
    result.push_back(stdPositive);
    result.push_back(stdNegative);
    return result;
}
template<typename DATATYPE>
void lshbox::laPcaLsh<DATATYPE>::setMeanAndSTD(Matrix<DATATYPE> &data){
    meanAndSTD = getMeanAndSTD(data);
}

template<typename DATATYPE>
void lshbox::laPcaLsh<DATATYPE>::hash(Matrix<DATATYPE> &data)
{
    progress_display pd(data.getSize());
    for (unsigned i = 0; i != data.getSize(); ++i)
    {
        insert(i, data[i]);
        ++pd;
    }
}
template<typename DATATYPE>
void lshbox::laPcaLsh<DATATYPE>::insert(unsigned key, const DATATYPE *domin)
{
    for (unsigned k = 0; k != param.L; ++k)
    {
        BIDTYPE hashVal = getHashVal(k, domin);
        tables[k][hashVal].push_back(key);
    }
}
template<typename DATATYPE>
template<typename PROBER>
int lshbox::laPcaLsh<DATATYPE>::probe(unsigned t, BIDTYPE bucketId, PROBER& prober)
{
    int numProbed = 0;
    if (tables[t].find(bucketId) != tables[t].end())
    {
        numProbed = tables[t][bucketId].size();
        for (std::vector<unsigned>::iterator iter = tables[t][bucketId].begin(); iter != tables[t][bucketId].end(); ++iter)
        {
            prober(*iter);
        }
    }
    
    return numProbed;
}
template<typename DATATYPE>
typename lshbox::laPcaLsh<DATATYPE>::BIDTYPE lshbox::laPcaLsh<DATATYPE>::getHashVal(unsigned k, const DATATYPE *domin)
{
    std::vector<float> domin_pc(pcsAll[k].size());
    for (unsigned i = 0; i != domin_pc.size(); ++i)
    {
        for (unsigned j = 0; j != pcsAll[k][i].size(); ++j)
        {
            domin_pc[i] += domin[j] * pcsAll[k][i][j];
        }
    }

    BIDTYPE hashVal = 0;
    for (unsigned i = 0; i != domin_pc.size(); ++i)
    {
        float product = domin_pc[i];
        hashVal <<= 1; // hashVal *= 2
        if (product > 0)
        {
            hashVal += 1;
        }
    }
    return hashVal;
}
template<typename DATATYPE>
void lshbox::laPcaLsh<DATATYPE>::load(const std::string &file)
{
    std::ifstream in(file, std::ios::binary);
    if (!in) {
        std::cout << "cannot open file" << file << std::endl;
        exit(0);
    }
    in.read((char *)&param.M, sizeof(unsigned));
    in.read((char *)&param.L, sizeof(unsigned));
    in.read((char *)&param.D, sizeof(unsigned));
    in.read((char *)&param.N, sizeof(unsigned));
    in.read((char *)&param.S, sizeof(unsigned));
    tables.resize(param.L);
    rndArray.resize(param.L);
    pcsAll.resize(param.L);
    for (unsigned i = 0; i != param.L; ++i)
    {
        rndArray[i].resize(param.N);
        in.read((char *)&rndArray[i][0], sizeof(unsigned) * param.N);
        unsigned count;
        in.read((char *)&count, sizeof(unsigned));
        for (unsigned j = 0; j != count; ++j)
        {
            BIDTYPE target;
            in.read((char *)&target, sizeof(BIDTYPE));

            // unsigned fourByteTmp;
            // in.read((char *)&fourByteTmp, sizeof(fourByteTmp));
            // BIDTYPE target;
            // target = fourByteTmp;

            unsigned length;
            in.read((char *)&length, sizeof(unsigned));
            tables[i][target].resize(length);
            in.read((char *) & (tables[i][target][0]), sizeof(unsigned) * length);
        }
        pcsAll[i].resize(param.N);
        for (unsigned j = 0; j != param.N; ++j)
        {
            pcsAll[i][j].resize(param.D);
            in.read((char *)&pcsAll[i][j][0], sizeof(float) * param.D);
        }
    }
    in.close();
}
template<typename DATATYPE>
void lshbox::laPcaLsh<DATATYPE>::save(const std::string &file)
{
    std::ofstream out(file, std::ios::binary);
    out.write((char *)&param.M, sizeof(unsigned)); // 4 bytes
    out.write((char *)&param.L, sizeof(unsigned)); // 4 bytes
    out.write((char *)&param.D, sizeof(unsigned)); // 4 bytes
    out.write((char *)&param.N, sizeof(unsigned)); // 4 bytes
    out.write((char *)&param.S, sizeof(unsigned)); // 4 bytes
    for (int i = 0; i != param.L; ++i)
    {
        out.write((char *)&rndArray[i][0], sizeof(unsigned) * param.N);  // 4 * N bytes
        unsigned count = unsigned(tables[i].size());
        out.write((char *)&count, sizeof(unsigned));
        for (std::unordered_map<BIDTYPE, std::vector<unsigned> >::iterator iter = tables[i].begin(); iter != tables[i].end(); ++iter)
        {
            BIDTYPE target = iter->first;
            out.write((char *)&target, sizeof(BIDTYPE));
            unsigned length = unsigned(iter->second.size());
            out.write((char *)&length, sizeof(unsigned));
            out.write((char *) & ((iter->second)[0]), sizeof(unsigned) * length);
        }
        for (unsigned j = 0; j != param.N; ++j)
        {
            out.write((char *)&pcsAll[i][j][0], sizeof(float) * param.D);
        }
    }
    out.close();
}
template<typename DATATYPE>
std::vector<float> lshbox::laPcaLsh<DATATYPE>::getHashFloats(unsigned k, const DATATYPE *domin)
{
    std::vector<float> domin_pc(pcsAll[k].size());
    for (unsigned i = 0; i != domin_pc.size(); ++i)
    {
        for (unsigned j = 0; j != pcsAll[k][i].size(); ++j)
        {
            domin_pc[i] += domin[j] * pcsAll[k][i][j];
        }
    }

    return domin_pc;
}
template<typename DATATYPE>
std::vector<bool> lshbox::laPcaLsh<DATATYPE>::quantization(const std::vector<float>& hashFloats)
{
    std::vector<bool> hashBits;
    hashBits.resize(hashFloats.size());
    for (int i = 0; i < hashFloats.size(); ++i) {
        if (hashFloats[i] >= 0) {
            hashBits[i] = 1;
        } else {
            hashBits[i] = 0;
        }
    }
    return  hashBits;
}
template<typename DATATYPE>
std::vector<bool> lshbox::laPcaLsh<DATATYPE>::getHashBits(unsigned k, const DATATYPE *domin)
{
    std::vector<float> hashFloats = getHashFloats(k, domin);
    std::vector<bool> hashBits = quantization(hashFloats);
    return hashBits;
}
template<typename DATATYPE>
int lshbox::laPcaLsh<DATATYPE>::getTableSize()
{
    // assert(param.L == 1);
    return tables[0].size();
}
template<typename DATATYPE>
int lshbox::laPcaLsh<DATATYPE>::getMaxBucketSize()
{
    // assert(param.L == 1);
    int max = 0;
    std::unordered_map<BIDTYPE, std::vector<unsigned> >::const_iterator it;
    for (it = tables[0].begin(); it != tables[0].end(); ++it) {
        if (it->second.size() > max) {
            max = it->second.size();
        }
    }
    return max;
}

template<typename DATATYPE>
template<typename PROBER>
void lshbox::laPcaLsh<DATATYPE>::KItemByProber(const DATATYPE *domin, PROBER &prober, int numItems) {
    // assert(param.L == 1);

    while(prober.getNumItemsProbed() < numItems && prober.nextBucketExisted()) {
        // <table, bucketId>
        const std::pair<unsigned, BIDTYPE>& probePair = prober.getNextBID();
        probe(probePair.first, probePair.second, prober); 
    }
}
