#!/usr/bin/python
'author: songts'
'mail: songts@act.buaa.edu.cn'
'usage: python insert.py libsvm_file_name partitions spacename.dbname'
import sys
argv = sys.argv
if (len(argv) < 4):
    print 'usage: python insert.py libsvm_file_name partitions spacename.dbname'
    exit()
file_name = argv[1]
partitions = int(argv[2])
space_db_name = argv[3]
out_file_base = space_db_name
spacename = space_db_name.split('.')[0]
dbname = space_db_name.split('.')[1]

from cassandra.cluster import Cluster
cluster = Cluster(['127.0.0.1'])
session=cluster.connect(spacename)

session.execute('create table %s (partition_key int, id int, content varchar, primary key (partition_key, id))' %dbname)

i=0
max = 0
s = set()
num_train_total = {}

with open(file_name, 'r') as f:
    for line in f.readlines():
        partition_key = i % partitions
        sstr = line[:-2]
        try:
            cur = int(sstr[sstr.rfind(' ')+1:sstr.rfind(':')])
        except ValueError, e:
            cur = 1
        if cur > max:
            max = cur
        label = line[:sstr.find(' ')]
        s.add(label)
        session.execute("INSERT INTO " + dbname + " (partition_key, id, content) values (%s, %s, %s)", (partition_key, i, line) )
        print('record %d inserted as partition_key %d' %(i, partition_key))
        i = i + 1
num = 0
with open(file_name, 'r') as f:
    num = len(f.readlines())
    for index in range(partitions):
        num_train_total[index] = num / partitions
    for index in range(num % partitions):
        num_train_total[index] += 1

if (partitions == 1):
    with open(out_file_base+'.meta', 'w') as f:
        f.write('num_train_total: %d\n' %num)
        f.write('num_train_this_partition: %d\n' %num_train_total[0])
        f.write('feature_dim: %d\n' %max)
        f.write('num_labels: %d\n' %len(s))
        f.write('format: libsvm\n')
        f.write('feature_one_based: 1\n')
        f.write('label_one_based: 1\n')
        f.write('snappy_compressed: 0')
else:
    for index in range(partitions):
        with open(out_file_base+'.'+str(index)+'.meta', 'w') as f:
            f.write('num_train_total: %d\n' %num)
            f.write('num_train_this_partition: %d\n' %num_train_total[index])
            f.write('feature_dim: %d\n' %max)
            f.write('num_labels: %d\n' %len(s))
            f.write('format: libsvm\n')
            f.write('feature_one_based: 1\n')
            f.write('label_one_based: 1\n')
            f.write('snappy_compressed: 0')

