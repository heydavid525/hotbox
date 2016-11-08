#!/usr/bin/python
import csv
import math
import pandas
import sklearn
from sklearn import cluster
import nltk
import logging
import numpy
from scipy import sparse


def distinct(token_list):
    seen = set()
    seen_add = seen.add
    return [x for x in token_list if not (x in seen or seen_add(x))]


def myHash(feature_set, feature):
    if pandas.isnull(feature):
        return feature
    for ele in feature_set:
        if ele == feature:
            return feature_set.index(feature)
    feature_set.append(feature)
    return feature_set.index(feature)


def hash_string_feature(column, index):
    feature_set = []
    numerical_column = []
    for cell in column:
        numerical_column.append(myHash(feature_set, cell))
    return pandas.Series(numerical_column, index=index)


def parse_to_tokens(token_list, string):
    if pandas.isnull(string):
        return
    token_list_append = token_list.append
    for token in nltk.word_tokenize(string.replace("~", " ").lower()):
        if token not in token_list:
            token_list_append(token)
    pass


def tokens_to_binary_features(token_list, string):
    feature = [0] * len(token_list)
    if pandas.isnull(string):
        return feature
    else:
        tokens = nltk.word_tokenize(string.replace("~", " ").lower())
        for token in tokens:
            try:
                feature[token_list.index(token)] = 1
            except Exception, e:
                print "impossible! tokenList has no token #", token
        return feature


def binarize_string_feature(column):
    token_list = list()
    for cell in column:
        parse_to_tokens(token_list, cell)
    logging.info("tokens size: {}".format(len(token_list)))
    mat = sparse.lil_matrix((column.size, len(token_list)), dtype=numpy.int32)
    for row_id, cell in enumerate(column):
        # binary_features.append(tokens_to_binary_features(token_list, cell))
        tokens = nltk.word_tokenize(cell.replace("~", " ").lower())
        for token in tokens:
            mat[row_id, token_list.index(token)] = 1
    return mat


def binarize_string_feature_to_dense_string(column):
    binary_features = []
    token_list = list()
    for cell in column:
        parse_to_tokens(token_list, cell)
    token_list = list(token_list)
    for cell in column:
        binary_features.append(', '.join(str(x) for x in tokens_to_binary_features(token_list, cell)))
    return pandas.Series(binary_features)


def cluster_string_feature(column, n_clusters):
    logging.info("binarizing")
    mat = binarize_string_feature(column)
    logging.info("binarize done")
    km = sklearn.cluster.KMeans(n_clusters=n_clusters, n_jobs=-1)
    logging.info("fitting")
    km.fit(mat)
    logging.info("fit done")
    logging.info("saving model")
    pandas.Series(km.labels_).to_csv("kmeans-model-labels-" + column.name)
    logging.info("saving model done")
    logging.info("predicting")
    list_ = []
    for index, cell in enumerate(column):
        if pandas.isnull(cell):
            print cell, 'is null'
            list_.append(cell)
        else:
            list_.append(km.labels_[index])
    logging.info("predicting done")
    return pandas.Series(list_)


def split_string_feature(column):
    unit_features = []
    section_features = []
    for cell in column:
        features = cell.split(',')
        unit_features.append(features[0])
        section_features.append(features[1])
    return pandas.DataFrame(
        {'Unit Name': pandas.Series(unit_features), 'Section Name': pandas.Series(section_features)})


def save_as_libsvm(df, path):
    output_file = open(path, 'w')
    label_column = df['Correct First Attempt']
    df = df.drop('Correct First Attempt', 1).drop('Row', 1)
    for i, row in df.iterrows():
        output_file.write(str(label_column[i]))
        for index in range(len(row)):
            if not pandas.isnull(row[index]):
                output_file.write(" {}:{}".format(index, row[index]))
        output_file.write("\n")
    output_file.close()
    pass


if __name__ == '__main__':
    logging.basicConfig(level=logging.INFO,
                        format='%(asctime)s %(message)s')
    data_file = 'part-00000'
    logging.info('reading data from ' + data_file)
    df = pandas.read_table(data_file, None, header=0, delimiter="\t")
    logging.info('reading done')
    out_df = pandas.DataFrame()
    out_df['Row'] = df['Row']
    logging.info('clustering #Step Name')
    out_df['Step Name'] = cluster_string_feature(df['Step Name'], 1000)
    logging.info('clustering #Step Name Done!')
    out_df.to_csv("out2.csv", sep='\t', header=True, index=False)
    # save_as_libsvm(df, 'kddb.libsvm')
