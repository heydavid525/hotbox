#mlr.cass Quick Start

###Make sure you have cassandra cpp driver installed.

see https://datastax.github.io/cpp-driver/

###Load existing lisvm file to cassandra

assume you are in app folder

    cd datasets
    
    python loadSvmToCassandra.py /path/to/svmfile num_of_partitions keyspace.tbname
    
loadSvmToCassandra.py will first create a table named tbname under keysapce and load svmfile to that table.
Then it will generate metadata files.

Note: 

1. `num_of_partitions` must be equal to number of clients of the mlr application to run.
2. `keyspace` must exist.

### Modify `script/launch.py`
    
    ...
    params = {
        "train_file": join(app_dir, "datasets/keyspace.tbname")
    ...
    
### Launch

    python script/launch.py
    
    
**It will output connection time, load time, parse time and trainning time of each clients**.

Note: It does **Not** support load test data from cassandra for now.