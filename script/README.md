***************************************************************************************

## Usages

### Introduction

```
sample_queries.sh
    input: $base_file, $num_queries
    output: $query_file

cal_groundtruth.sh
    input: $base_file, $query_file, $topk, $numThreads
    output: $benchmark_file
```

### Procedure to run an application

- Prepare base_file (fvecs as defined in TEXMEX(http://corpus-texmex.irisa.fr/))
- Run sample_queries.sh on base_file to produce query_file
- Run cal_groundtruth.sh (using base_file & query_file as parameters) to produce benchmark_file
- Train model using Matlab code. Do not forget to "mkdir hashingCodeTXT" first, otherwise error would occur.  Make sure that the parameters in Matlab code is consistent with configuration in search.sh. Otherwise you may get "Invalid file identifier" error in MATLAB
- Run search.sh

***************************************************************************************

## Configurations in search.sh

### hash_method
    - PCAH - PCA hashing
    - ITQ - Iterative Quantization

### query_method
    - GQR - Generate-to-probe Quantization Ranking
    - GHR or HL - Generate-to-probe Hamming Ranking
    - QR - Quantization Ranking
    - HR - Hamming Ranking
    - MIH - Multi-index Hashing
    
### codelength
    - Default code length is 12, 16, 18 and 20 for CIFAR60K, GIST1M, TINY5M and SIFT10M, respectively. We experimentally verify that the above settings is almost optimal.

### base_format
    - For now, fvecs is the only format we supported. See TEXMEX(http://corpus-texmex.irisa.fr/) for details.

### num_queries
    - For each dataset and querying method, we usually randomly sample 1000 items as queries.

### topk
    - By default, we do 20-nearest neighbors search.

### model_file & base_bits_file
    - model learned from dataset using hash_method mentioned above.
    

***************************************************************************************

## More scripts

### Data transformation

GQR only takes fvecs as input formats. However, many datasets such as Glove and Tiny have their own format. We provide applications for dataset transformation under folder data_to_fvecs.

### K-Means hashing

K-Means Hashing requires other scirpts to run, please refer to folder ../learn/KMH/readme.txt for details.